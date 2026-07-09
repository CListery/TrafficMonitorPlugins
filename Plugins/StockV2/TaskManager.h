#pragma once

#include "pch.h"
#include <afxwin.h>
#include <vector>
#include <afxmt.h>
#include <map>
#include <memory>
#include <mutex>

#define g_task LTaskManager::Instance()

// 任务回调函数类型定义
typedef void (*LTaskCallback)(LPVOID pParam);

// 任务状态枚举
enum class LTaskStatus
{
    STATE_IDLE,    // 空闲待执行
    STATE_RUNNING, // 正在执行
    STATE_ERROR,   // 执行异常
};

// 任务信息
struct LTaskInfo
{
    std::string taskKey;      // 用户自定义唯一KEY
    UINT nIntervalMs;         // 执行间隔
    LTaskStatus status;       // 当前状态
    BOOL bIsEnable;           // 启用状态
    ULONGLONG ullLastRunTime; // 上次执行时间
    ULONGLONG ullCostMs;      // 上次执行耗时(ms)
    BOOL bHasError;           // 是否有错误
    UINT nRunCount;           // 执行次数
};

// 任务实体
class LTaskItem
{
    friend class LTaskManager;

private:
    UINT m_nSysID;                                 // 内部ID
    std::string m_taskKey;                         // 用户自定义唯一KEY
    UINT m_nIntervalMs{0};                         // 执行间隔(毫秒)
    LTaskCallback m_pFunc{nullptr};                // 任务回调函数
    LPVOID m_pParam{nullptr};                      // 自定义参数
    LTaskStatus m_status{LTaskStatus::STATE_IDLE}; // 当前状态
    BOOL m_isEnable{true};                         // 启用状态
    ULONGLONG m_ullLastRunTime{0};                 // 上次执行时间戳
    ULONGLONG m_ullCostMs{0};                      // 上次执行耗时
    BOOL m_bHasError{FALSE};                       // 是否有错误
    UINT m_nRunCount{0};                           // 执行次数
    BOOL m_isExecuting{false};                     // 任务线程执行中标记，防止重复执行
    BOOL m_threadRunning{false};                   // 线程已成功启动
    ULONGLONG m_threadLaunchTime{0};               // 调度器启动线程的时间戳
};

// 定时任务管理器
class LTaskManager
{
private:
    static LTaskManager *m_pInstance;          // 单例实例
    static std::recursive_mutex m_mtxInstance; // 单例线程锁

    std::recursive_mutex m_mtxTask;           // 任务列表线程锁
    std::map<UINT, LTaskItem> m_mapTask;      // 内部：系统ID->任务
    std::map<std::string, UINT> m_mapKeyToID; // 内部：KEY->系统ID
    std::vector<std::string> m_vecDelKeys;    // 待删除任务队列
    CWinThread *m_pScheduleThread;            // 调度线程
    BOOL m_bRunFlag;                          // 调度线程运行标志
    UINT m_nNextTaskID;                       // 自增任务ID

    // 任务自动重置器
    struct TaskAutoResetFlag
    {
        LTaskManager *m_pMgr;
        std::string m_taskKey;

        TaskAutoResetFlag(LTaskManager *pMgr, const std::string &key)
            : m_pMgr(pMgr), m_taskKey(key)
        {
            std::lock_guard<std::recursive_mutex> lock(pMgr->m_mtxTask);
            auto pTask = pMgr->FindTaskByKey(m_taskKey);
            if (pTask)
            {
                pTask->m_threadRunning = true;
                LLOG_INFO("Task Start: %s", m_taskKey);
            }
        }

        ~TaskAutoResetFlag()
        {
            std::lock_guard<std::recursive_mutex> lock(m_pMgr->m_mtxTask);
            auto pTask = m_pMgr->FindTaskByKey(m_taskKey);
            if (pTask)
            {
                pTask->m_isExecuting = false;
                pTask->m_threadRunning = false;
                pTask->m_threadLaunchTime = 0;
                LLOG_INFO("Task Stop: %s", m_taskKey);
            }
        }
    };

private:
    LTaskManager();
    ~LTaskManager();

    // 禁用拷贝构造和赋值
    LTaskManager(const LTaskManager &) = delete;
    LTaskManager &operator=(const LTaskManager &) = delete;

public:
    // 获取单例实例
    static LTaskManager *Instance();

    // 启动
    BOOL StartUp();

    // 关停
    void ShutDown();

    // 添加定时任务：返回任务ID，失败返回0
    BOOL AddTask(const std::string &szKey, UINT nIntervalMs, LTaskCallback pFunc, LPVOID pParam = nullptr);

    // 删除定时任务
    BOOL DeleteTask(const std::string &szKey);

    // 修改任务间隔
    BOOL ModifyTaskInterval(const std::string &szKey, UINT nNewIntervalMs);

    // 启用/禁用任务
    BOOL EnableTask(const std::string &szKey, BOOL bEnable);

    // 重置任务（清除错误、恢复执行）
    BOOL ResetTask(const std::string &szKey);

    std::unique_ptr<LTaskInfo> GetTaskByKey(const std::string &szKey);

    std::vector<LTaskInfo> GetTaskByCallback(LTaskCallback pFunc, LPVOID pParam = nullptr);

    std::vector<LTaskInfo> GetAllTasks();

    BOOL IsTaskExists(LTaskCallback pFunc, LPVOID pParam = nullptr);

    BOOL IsTaskExists(const std::string &szKey);

private:
    // 调度线程函数
    static UINT ScheduleThread(LPVOID pParam);

    // 任务执行线程函数
    static UINT TaskWorkThread(LPVOID pParam);

    // 获取当前时间戳
    ULONGLONG GetTickMs();
    LTaskItem *FindTaskByKey(const std::string &szKey);
    void FillTaskInfo(const LTaskItem &item, LTaskInfo &info);
};
