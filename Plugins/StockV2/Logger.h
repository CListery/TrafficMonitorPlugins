#pragma once

#include <string>
#include <mutex>
#include <fstream>
#include <iosfwd>

enum class LogLevel
{
    LEVEL_DEBUG,
    LEVEL_INFO,
    LEVEL_WARN,
    LEVEL_ERROR
};

// ========== 参数适配器：编译期自动转换字符串类型 ==========
namespace
{
    // ---------- 窄字符格式化适配器：适配 vsprintf_s ----------
    template<typename T>
    inline T NarrowLogArgAdapt(const T& arg) { return arg; }

    // std::string 自动转 const char*
    inline const char* NarrowLogArgAdapt(const std::string& arg) { return arg.c_str(); }

#ifdef __WXWINDOWS__
    // wxString 自动转 UTF-8 窄字符串（匹配窄格式串的 %s）
    inline const char* NarrowLogArgAdapt(const wxString& arg)
    {
        // 用 wxConvUTF8 显式指定 UTF-8，兼容性更好
        return arg.mb_str(wxConvUTF8).data();
    }
#endif

    // ---------- 宽字符格式化适配器：适配 vswprintf_s ----------
    template<typename T>
    inline T WideLogArgAdapt(const T& arg) { return arg; }

    // std::wstring 自动转 const wchar_t*
    inline const wchar_t* WideLogArgAdapt(const std::wstring& arg) { return arg.c_str(); }

#ifdef __WXWINDOWS__
    // wxString 自动转宽字符指针（匹配宽格式串的 %s）
    inline const wchar_t* WideLogArgAdapt(const wxString& arg)
    {
        // Unicode 构建下 wxChar 等价于 wchar_t，直接返回 c_str 即可
        return arg.c_str();
    }
#endif
}

// 日志单例类
class LLogger
{
private:
    // 成员变量
    bool m_bEnabled;         // 日志总开关
    bool m_bWriteToFile;     // 写入文件开关
    bool m_bTraceable;       // 输出TRACE信息
    bool m_bFilterDuplicate; // 过滤重复日志
    bool m_bAutoFlush;       // 自动刷新文件

    std::wstring m_logFilePath; // 日志文件完整路径
    std::wofstream m_logFile;   // 日志文件句柄
    std::wstring m_lastLog;     // 上一条日志（过滤重复用）
    std::mutex m_mutex;         // 线程安全锁

    // 私有构造：单例模式
    LLogger();
    // 禁用拷贝/赋值/移动
    LLogger(const LLogger &) = delete;
    LLogger &operator=(const LLogger &) = delete;
    LLogger(LLogger &&) = delete;
    LLogger &operator=(LLogger &&) = delete;

private:
    // 核心输出逻辑
    void Output(LogLevel level, const char *szFile, const wchar_t *func, int line, const std::wstring &msg, bool ignore_filter = FALSE);

    // 初始化日志文件（自动生成路径）
    bool InitLogFile();

    // ========== 内部 C 可变参数实现（不对外暴露，避免绕过适配） ==========
    _Printf_format_string_ void WriteLogImplV(LogLevel level, bool ignore_filter, const char* szFile, const wchar_t* func, int nLine, const char* fmt, ...);
    _Printf_format_string_ void WriteLogImplVW(LogLevel level, bool ignore_filter, const char* szFile, const wchar_t* func, int nLine, const wchar_t* fmt, ...);

public:
    // 单例获取
    static LLogger &Instance();

    // 日志开关
    void SetEnabled(bool enable) noexcept { m_bEnabled = enable; }
    bool IsEnabled() const noexcept { return m_bEnabled; }
    void SetAutoFlush(bool enable) noexcept { m_bAutoFlush = enable; }

    // 功能配置接口
    void SetWriteToFile(bool enable) noexcept;
    void SetTraceable(bool enable) noexcept { m_bTraceable = enable; }
    void SetFilterDuplicate(bool enable) noexcept { m_bFilterDuplicate = enable; }
    void SetLogPath(const wxString &customPath); // 自定义日志路径

    void WriteLog(LogLevel level, bool ignore_filter, const char *szFile, const wchar_t *func, int nLine, const std::string &fmt);
    void WriteLog(LogLevel level, bool ignore_filter, const char *szFile, const wchar_t *func, int nLine, const std::wstring &fmt);
#ifdef __WXWINDOWS__
    // ========== wxString 无参直接输出 ==========
    void WriteLog(LogLevel level, bool ignore_filter, const char* szFile, const wchar_t* func, int nLine, const wxString& fmt);
#endif

    // ========== 模板化对外接口：窄字符版本 ==========
    template<typename... Args>
    void WriteLog(LogLevel level, bool ignore_filter, const char* szFile, const wchar_t* func, int nLine, const char* fmt, Args... args)
    {
        // 所有参数自动适配后，传入底层格式化函数
        WriteLogImplV(level, ignore_filter, szFile, func, nLine, fmt, NarrowLogArgAdapt(args)...);
    }

    // ========== 模板化对外接口：宽字符版本 ==========
    template<typename... Args>
    void WriteLog(LogLevel level, bool ignore_filter, const char* szFile, const wchar_t* func, int nLine, const wchar_t* fmt, Args... args)
    {
        // 所有参数自动适配后，传入底层格式化函数
        WriteLogImplVW(level, ignore_filter, szFile, func, nLine, fmt, WideLogArgAdapt(args)...);
    }

};

#define LLOG_DEBUG(fmt, ...) LLogger::Instance().WriteLog(LogLevel::LEVEL_DEBUG, false, __FILE__, __FUNCTIONW__, __LINE__, fmt, __VA_ARGS__)
#define LLOG_INFO(fmt, ...) LLogger::Instance().WriteLog(LogLevel::LEVEL_INFO, false, __FILE__, __FUNCTIONW__, __LINE__, fmt, __VA_ARGS__)
#define LLOG_WARN(fmt, ...) LLogger::Instance().WriteLog(LogLevel::LEVEL_WARN, false, __FILE__, __FUNCTIONW__, __LINE__, fmt, __VA_ARGS__)
#define LLOG_ERROR(fmt, ...) LLogger::Instance().WriteLog(LogLevel::LEVEL_ERROR, false, __FILE__, __FUNCTIONW__, __LINE__, fmt, __VA_ARGS__)

#define LLOG_DEBUG_F(fmt, ...) LLogger::Instance().WriteLog(LogLevel::LEVEL_DEBUG, true, __FILE__, __FUNCTIONW__, __LINE__, fmt, __VA_ARGS__)
#define LLOG_INFO_F(fmt, ...) LLogger::Instance().WriteLog(LogLevel::LEVEL_INFO, true, __FILE__, __FUNCTIONW__, __LINE__, fmt, __VA_ARGS__)
#define LLOG_WARN_F(fmt, ...) LLogger::Instance().WriteLog(LogLevel::LEVEL_WARN, true, __FILE__, __FUNCTIONW__, __LINE__, fmt, __VA_ARGS__)
#define LLOG_ERROR_F(fmt, ...) LLogger::Instance().WriteLog(LogLevel::LEVEL_ERROR, true, __FILE__, __FUNCTIONW__, __LINE__, fmt, __VA_ARGS__)

#define LLOG_ENABLE() LLogger::Instance().SetEnabled(true)
#define LLOG_DISABLE() LLogger::Instance().SetEnabled(false)
