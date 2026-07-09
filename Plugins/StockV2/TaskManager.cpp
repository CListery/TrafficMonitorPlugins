#include "pch.h"
#include "TaskManager.h"
#include <mutex>

// 单例静态成员初始化
LTaskManager *LTaskManager::m_pInstance = nullptr;
std::recursive_mutex LTaskManager::m_mtxInstance;

LTaskManager::LTaskManager()
    : m_pScheduleThread(nullptr), m_bRunFlag(FALSE), m_nNextTaskID(1)
{
}

LTaskManager::~LTaskManager()
{
    ShutDown();
}

LTaskManager *LTaskManager::Instance()
{
    if (m_pInstance == nullptr)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mtxInstance);
        if (m_pInstance == nullptr)
            m_pInstance = new LTaskManager();
    }
    return m_pInstance;
}

BOOL LTaskManager::StartUp()
{
    if (m_bRunFlag)
        return TRUE;

    // 启动调度线程
    m_bRunFlag = TRUE;
    m_pScheduleThread = AfxBeginThread(ScheduleThread, this, THREAD_PRIORITY_HIGHEST);

    return TRUE;
}

void LTaskManager::ShutDown()
{
    if (!m_bRunFlag)
        return;
    // 停止调度线程
    m_bRunFlag = FALSE;
    if (m_pScheduleThread != nullptr)
    {
        WaitForSingleObject(m_pScheduleThread->m_hThread, INFINITE);
        delete m_pScheduleThread;
        m_pScheduleThread = nullptr;
    }
    {
        // 清空任务列表
        std::lock_guard<std::recursive_mutex> lock(m_mtxTask);
        m_mapTask.clear();
        m_mapKeyToID.clear();
    }
    {
        // 释放单例
        std::lock_guard<std::recursive_mutex> lock(m_mtxInstance);
        delete m_pInstance;
        m_pInstance = nullptr;
    }
}

ULONGLONG LTaskManager::GetTickMs()
{
    return GetTickCount64();
}

LTaskItem *LTaskManager::FindTaskByKey(const std::string &szKey)
{
    std::lock_guard<std::recursive_mutex> lock(m_mtxTask);
    auto it = m_mapKeyToID.find(szKey);
    if (it == m_mapKeyToID.end())
        return nullptr;
    auto taskPair = m_mapTask.find(it->second);
    if (taskPair == m_mapTask.end())
    {
        return nullptr;
    }
    return &taskPair->second;
}

void LTaskManager::FillTaskInfo(const LTaskItem &item, LTaskInfo &info)
{
    info.taskKey = item.m_taskKey;
    info.nIntervalMs = item.m_nIntervalMs;
    info.status = item.m_status;
    info.bIsEnable = item.m_isEnable;
    info.ullLastRunTime = item.m_ullLastRunTime;
    info.ullCostMs = item.m_ullCostMs;
    info.bHasError = item.m_bHasError;
    info.nRunCount = item.m_nRunCount;
}

UINT LTaskManager::ScheduleThread(LPVOID pParam)
{
    auto pMgr = (LTaskManager *)pParam;
    // 任务线程启动超时阈值
    const ULONGLONG THREAD_START_TIMEOUT = 500;
    const ULONGLONG POLLING_INTERVAL = 10;

    while (pMgr->m_bRunFlag)
    {
        Sleep(POLLING_INTERVAL);

        std::lock_guard<std::recursive_mutex> lock(pMgr->m_mtxTask);
        ULONGLONG now = pMgr->GetTickMs();

        // 巡检所有执行中的任务线程
        for (auto &pair : pMgr->m_mapTask)
        {
            auto &item = pair.second;
            if (item.m_isExecuting &&
                !item.m_threadRunning &&
                item.m_threadLaunchTime > 0 &&
                (now - item.m_threadLaunchTime) > THREAD_START_TIMEOUT)
            {
                // 任务线程启动失败，重置，下次重新调度
                LLOG_WARN("Task Thread Reset: %s", item.m_taskKey);
                item.m_isExecuting = false;
                item.m_threadLaunchTime = 0;
                item.m_threadRunning = false;
            }
        }

        // 调度任务
        for (auto &pair : pMgr->m_mapTask)
        {
            auto &item = pair.second;

            if (!item.m_isEnable)
                continue;
            // 状态过滤
            if (item.m_bHasError || item.m_status != LTaskStatus::STATE_IDLE)
                continue;
            // 未到设定时间
            if (item.m_nIntervalMs > 0 && now - item.m_ullLastRunTime < item.m_nIntervalMs)
                continue;
            // 任务线程执行过滤
            if (item.m_isExecuting)
                continue;
            item.m_isExecuting = true;
            item.m_threadLaunchTime = now; // 保存任务线程启动时间
            item.m_threadRunning = false;  // 重置任务线程启动标识
            item.m_ullLastRunTime = now;   // 记录最后一次执行时间

            // 创建线程并保存句柄/ID
            std::string *pKey = new std::string(item.m_taskKey);
            CWinThread *pThread = AfxBeginThread(TaskWorkThread, pKey);

            if (pThread)
                pThread->m_bAutoDelete = TRUE;

            if (pThread == nullptr)
            {
                // 线程创建失败，立即回滚
                item.m_isExecuting = false;
                delete pKey;
                continue;
            }
        }

        // 删除任务
        for (auto &key : pMgr->m_vecDelKeys)
        {
            auto keyIt = pMgr->m_mapKeyToID.find(key);
            if (keyIt == pMgr->m_mapKeyToID.end())
                continue;

            UINT id = keyIt->second;
            pMgr->m_mapTask.erase(id);
            pMgr->m_mapKeyToID.erase(keyIt);
            LLOG_INFO("Task Deleted: %s", key);
        }
        pMgr->m_vecDelKeys.clear();
    }

    return 0;
}

UINT LTaskManager::TaskWorkThread(LPVOID pParam)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    const std::string *pKey = (std::string *)pParam;
    if (pKey == nullptr)
    {
        return 0;
    }

    auto pMgr = LTaskManager::Instance();

    // 绑定自动重置器
    TaskAutoResetFlag autoReset(pMgr, *pKey);

    LTaskCallback pFunc = nullptr;
    LPVOID pUserParam = nullptr;
    ULONGLONG start = pMgr->GetTickMs();

    {
        // 更新状态并获取回调和参数
        std::lock_guard<std::recursive_mutex> lock(pMgr->m_mtxTask);
        auto pTask = pMgr->FindTaskByKey(*pKey);
        if (pTask)
        {
            pFunc = pTask->m_pFunc;
            pUserParam = pTask->m_pParam;
            pTask->m_status = LTaskStatus::STATE_RUNNING;
        }
    }

    // 执行任务
    BOOL bExecSucc = FALSE;
    if (pFunc)
    {
        try
        {
            pFunc(pUserParam);
            bExecSucc = TRUE;
        }
        catch (...)
        {
            LLOG_ERROR("Task Exec Error: %s", pKey);
        }
    }
    {
        std::lock_guard<std::recursive_mutex> lock(pMgr->m_mtxTask);
        auto pTask = pMgr->FindTaskByKey(*pKey);
        if (pTask)
        {
            if (bExecSucc)
            {
                // 执行成功：更新耗时+次数
                pTask->m_status = LTaskStatus::STATE_IDLE;
                pTask->m_ullCostMs = pMgr->GetTickMs() - start;
                pTask->m_nRunCount++;
            }
            else
            {
                // 执行异常：标记错误+自动停止
                pTask->m_status = LTaskStatus::STATE_ERROR;
                pTask->m_bHasError = TRUE;
            }
            if (pTask->m_nIntervalMs == 0)
            {
                pMgr->m_vecDelKeys.push_back(pTask->m_taskKey);
            }
        }
    }

    delete pKey;
    return 0;
}

// 添加任务
BOOL LTaskManager::AddTask(const std::string &szKey, UINT nIntervalMs, LTaskCallback pFunc, LPVOID pParam)
{
    if (szKey.empty() || pFunc == nullptr)
    {
        LLOG_WARN("INVALID TASK");
        return FALSE;
    }

    std::lock_guard<std::recursive_mutex> lock(m_mtxTask);

    if (m_mapKeyToID.find(szKey) != m_mapKeyToID.end())
    {
        return FALSE;
    }

    LTaskItem task;
    task.m_nSysID = m_nNextTaskID++;
    task.m_taskKey = std::string(szKey);
    task.m_nIntervalMs = nIntervalMs;
    task.m_pFunc = pFunc;
    task.m_pParam = pParam;
    task.m_status = LTaskStatus::STATE_IDLE;
    task.m_ullLastRunTime = GetTickMs();

    m_mapTask[task.m_nSysID] = task;
    m_mapKeyToID[szKey] = task.m_nSysID;

    LLOG_INFO("AddTask: %s", szKey);

    return TRUE;
}

// 删除任务
BOOL LTaskManager::DeleteTask(const std::string &szKey)
{
    std::lock_guard<std::recursive_mutex> lock(m_mtxTask);
    m_vecDelKeys.push_back(szKey);
    return TRUE;
}

// 修改任务间隔
BOOL LTaskManager::ModifyTaskInterval(const std::string &szKey, UINT nNewIntervalMs)
{
    std::lock_guard<std::recursive_mutex> lock(m_mtxTask);
    auto pTask = FindTaskByKey(szKey);
    if (!pTask)
        return FALSE;

    pTask->m_nIntervalMs = nNewIntervalMs;
    LLOG_INFO("Task Changed: %s  %d", szKey, nNewIntervalMs);
    return TRUE;
}

// 启用/禁用任务
BOOL LTaskManager::EnableTask(const std::string &szKey, BOOL bEnable)
{
    std::lock_guard<std::recursive_mutex> lock(m_mtxTask);
    auto pTask = FindTaskByKey(szKey);
    if (!pTask || pTask->m_bHasError)
    {
        return FALSE;
    }
    pTask->m_isEnable = bEnable;
    if (bEnable)
    {
        pTask->m_status = LTaskStatus::STATE_IDLE;
    }

    LLOG_INFO("Task Changed: %s  %s", szKey, bEnable ? "enabled" : "disabled");

    return TRUE;
}

// 重置任务：清除错误，恢复执行
BOOL LTaskManager::ResetTask(const std::string &szKey)
{
    std::lock_guard<std::recursive_mutex> lock(m_mtxTask);
    auto pTask = FindTaskByKey(szKey);
    if (!pTask)
    {
        return FALSE;
    }
    pTask->m_bHasError = FALSE;
    pTask->m_status = LTaskStatus::STATE_IDLE;
    pTask->m_isEnable = TRUE;
    pTask->m_ullCostMs = 0;
    pTask->m_ullLastRunTime = GetTickMs();

    LLOG_INFO("Task Reset: %s", szKey);

    return TRUE;
}

// ===================== 查询接口 =====================
std::unique_ptr<LTaskInfo> LTaskManager::GetTaskByKey(const std::string &szKey)
{
    std::unique_ptr<LTaskInfo> info;
    std::lock_guard<std::recursive_mutex> lock(m_mtxTask);
    auto pTask = FindTaskByKey(szKey);
    if (pTask != nullptr)
    {
        info = std::make_unique<LTaskInfo>();
        FillTaskInfo(*pTask, *info);
    }
    return info;
}

std::vector<LTaskInfo> LTaskManager::GetTaskByCallback(LTaskCallback pFunc, LPVOID pParam)
{
    std::vector<LTaskInfo> res;
    std::lock_guard<std::recursive_mutex> lock(m_mtxTask);
    for (const auto &pair : m_mapTask)
    {
        const auto &item = pair.second;
        if (item.m_pFunc == pFunc)
        {
            if (pParam == nullptr || item.m_pParam == pParam)
            {
                LTaskInfo info;
                FillTaskInfo(item, info);
                res.push_back(info);
            }
        }
    }
    return res;
}

std::vector<LTaskInfo> LTaskManager::GetAllTasks()
{
    std::vector<LTaskInfo> res;
    std::lock_guard<std::recursive_mutex> lock(m_mtxTask);
    for (const auto &pair : m_mapTask)
    {
        LTaskInfo info;
        FillTaskInfo(pair.second, info);
        res.push_back(info);
    }
    return res;
}

BOOL LTaskManager::IsTaskExists(LTaskCallback pFunc, LPVOID pParam)
{
    return !GetTaskByCallback(pFunc, pParam).empty();
}

BOOL LTaskManager::IsTaskExists(const std::string &szKey)
{
    std::lock_guard<std::recursive_mutex> lock(m_mtxTask);
    auto pTask = FindTaskByKey(szKey);
    return pTask != nullptr;
}