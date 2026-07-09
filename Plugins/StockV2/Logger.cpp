#include "pch.h"
#include "Logger.h"
#include <Windows.h>
#include <fstream>
#include <vector>
#include <locale>
#include <codecvt>

// 匿名命名空间：封装辅助函数，避免全局符号污染
namespace
{
    // 日志等级转宽字符串
    std::wstring GetLevelLabel(LogLevel level) noexcept
    {
        switch (level)
        {
        case LogLevel::LEVEL_DEBUG:
            return L"DEBUG";
        case LogLevel::LEVEL_INFO:
            return L"INFO";
        case LogLevel::LEVEL_WARN:
            return L"WARN";
        case LogLevel::LEVEL_ERROR:
            return L"ERROR";
        default:
            return L"UNKNOWN";
        }
    }

    // 获取格式化时间字符串（YYYY/MM/DD HH:MM:SS.ms）
    std::wstring GetTime()
    {
        SYSTEMTIME st{};
        GetLocalTime(&st);
        wchar_t buf[64]{};
        swprintf_s(buf, L"%d/%02d/%02d %02d:%02d:%02d.%03d",
                   st.wYear, st.wMonth, st.wDay,
                   st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        return buf;
    }

    // 获取文件名
    std::string GetFileName(const char *szFile)
    {
        if (!szFile || *szFile == '\0') return {};

        const char* posBack = strrchr(szFile, '\\');
        const char* posSlash = strrchr(szFile, '/');
        const char* pos = (posSlash && (!posBack || posSlash > posBack)) ? posSlash : posBack;

        return pos ? std::string(pos + 1) : std::string(szFile);

        //if (!szFile)
        //    return {};
        //std::string filePath(szFile);
        //const auto pos = filePath.find_last_of("\\/");
        //return (pos != std::string::npos) ? filePath.substr(pos + 1) : filePath;
    }

    std::wstring GetLocation(const char *szFile, const wchar_t *func, int line)
    {
        // 获取宽字符文件名
        std::string fileName = GetFileName(szFile);
        // 数字转宽字符串 std::to_wstring
        std::wstring lineStr = std::to_wstring(line);
        // 函数名兜底
        std::wstring funcName;
        if (func && *func != L'\0')
        {
            funcName = std::wstring(func) + L"()";
        }
        else
        {
            funcName = L"unknown";
        }

        // 全部使用宽字符串字面量 L"" 拼接
        return CommonUtils::StringHelper::ToWideStr(fileName) + L":" + lineStr + L" # " + funcName;
    }
}

LLogger::LLogger()
    : m_bEnabled(false),
      m_bWriteToFile(false),
      m_bTraceable(false),
      m_bFilterDuplicate(true),
      m_bAutoFlush(true),
      m_logFilePath(),
      m_logFile(),
      m_lastLog(),
      m_mutex()
{
// Debug模式默认开启日志
#ifdef _DEBUG
    m_bEnabled = true;
    // SetWriteToFile(true);
#endif
}

// 单例实例
LLogger &LLogger::Instance()
{
    static LLogger inst;
    return inst;
}

// 初始化日志文件：自动生成路径到程序目录
bool LLogger::InitLogFile()
{
    if (m_logFile.is_open())
        return true;

    if (m_logFilePath.empty())
    {        
        // 拼接日志路径：程序目录\模块名.log
        m_logFilePath = g_data.GetModuleParentPath() + wxFILE_SEP_PATH + g_data.GetModuleName() + wxFILE_SEP_EXT + L"log";
    }

    // 打开文件前注入 UTF-8 编码 locale
    m_logFile.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>));
    // 打开文件（追加模式 | 宽字符 | 无缓冲）
    m_logFile.open(m_logFilePath, std::ios::out | std::ios::app);

    return m_logFile.is_open();
}

// 设置文件写入开关
void LLogger::SetWriteToFile(bool enable) noexcept
{
    m_bWriteToFile = enable;
    if (enable)
    {
        InitLogFile();
    }
}

// 自定义日志路径
void LLogger::SetLogPath(const wxString &customPath)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logFile.is_open())
        m_logFile.close();
    m_logFilePath = customPath;
    InitLogFile();
}

void LLogger::WriteLog(LogLevel level, bool ignore_filter, const char *szFile, const wchar_t *func, int nLine, const std::string &fmt)
{
    if (!m_bEnabled)
        return;
    Output(level, szFile, func, nLine, CommonUtils::StringHelper::ToWideStr(fmt.c_str()), ignore_filter);
}

void LLogger::WriteLog(LogLevel level, bool ignore_filter, const char *szFile, const wchar_t *func, int nLine, const std::wstring &fmt)
{
    if (!m_bEnabled)
        return;
    Output(level, szFile, func, nLine, fmt, ignore_filter);
}

#ifdef __WXWINDOWS__
void LLogger::WriteLog(LogLevel level, bool ignore_filter, const char* szFile, const wchar_t* func, int nLine, const wxString& fmt)
{
    if (!m_bEnabled)
        return;
    // wxString Unicode 构建下可直接隐式转换为 std::wstring
    Output(level, szFile, func, nLine, fmt.wc_str(), ignore_filter);
}
#endif

// ANSI字符串日志
void LLogger::WriteLogImplV(LogLevel level, bool ignore_filter, const char *szFile, const wchar_t *func, int nLine, const char *fmt, ...)
{
    if (!m_bEnabled || !fmt)
        return;

    // 可变参数解析
    va_list args{};
    va_start(args, fmt);
    const int len = _vscprintf(fmt, args);
    if (len <= 0)
    {
        va_end(args);
        return;
    }
    std::vector<char> buf(len + 1);
    vsprintf_s(buf.data(), buf.size(), fmt, args);
    va_end(args);

    // 输出日志
    Output(level, szFile, func, nLine, CommonUtils::StringHelper::ToWideStr(buf.data()), ignore_filter);
}

// 宽字符字符串日志
void LLogger::WriteLogImplVW(LogLevel level, bool ignore_filter, const char *szFile, const wchar_t *func, int nLine, const wchar_t *fmt, ...)
{
    if (!m_bEnabled || !fmt)
        return;

    // 可变参数解析
    va_list args{};
    va_start(args, fmt);
    const int len = _vscwprintf(fmt, args);
    if (len <= 0)
    {
        va_end(args);
        return;
    }
    std::vector<wchar_t> buf(len + 1);
    vswprintf_s(buf.data(), buf.size(), fmt, args);
    va_end(args);

    // 输出日志
    Output(level, szFile, func, nLine, buf.data(), ignore_filter);
}

void LLogger::Output(LogLevel level, const char *szFile, const wchar_t *func, int line, const std::wstring &msg, bool ignore_filter)
{
    // 过滤重复日志
    if (m_bFilterDuplicate)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        // 非强制输出时，和上一条日志相同则过滤
        if (!ignore_filter && msg == m_lastLog)
        {
            return;
        }
        // 更新最后一条日志记录（强制输出也会更新基准）
        m_lastLog = msg;
    }

    // 拼接完整日志
    const std::wstring fullLog = L"[" + GetTime() + L"] " +
                                 L"[" + GetLevelLabel(level) + L"] " +
                                 L"[" + GetLocation(szFile, func, line) + L"] " +
                                 msg;

    // 输出到DebugView
    OutputDebugStringW((fullLog + L"\n").c_str());

    // 输出到ATL TRACE
    if (m_bTraceable)
    {
        ATLTRACE(L"%s\n", fullLog.c_str());
    }

    // 写入文件
    if (m_bWriteToFile)
    {
        std::lock_guard<std::mutex> lock(m_mutex); // 线程安全锁
        if (m_logFile.is_open() && m_bWriteToFile)
        {
            //m_logFile << fullLog << std::endl;
            // 用 L'\n' 替代 std::endl，避免不必要的强制刷新，由 m_bAutoFlush 统一控制
            m_logFile << fullLog << L'\n';
            if (m_bAutoFlush)
            {
                m_logFile.flush();
            }
        }
    }
}
