#pragma once
#include "pch.h"
#include <string>
#include <vector>
#include <cstdint>
#include <Windows.h>
#include <map>
#include <wx/hashmap.h>
#include <utilities/yyjson/yyjson.h>

#define UtilResHlp CommonUtils::ResourceHelper::Instance()
#define UtilScreenHlp CommonUtils::ScreenHelper::Instance()
#define UtilStringHlp CommonUtils::StringHelper
#define UtilNetHlp CommonUtils::NetHelper
#define UtilJsonHlp CommonUtils::JsonHelper

#define ResString UtilResHlp.StringRes
#define ResIcon UtilResHlp.IconRes
#define ResHicon UtilResHlp.HiconRes
#define ResHtml UtilResHlp.HtmlRes

namespace CommonUtils
{
    WX_DECLARE_HASH_MAP(UINT, wxString, wxIntegerHash, wxIntegerEqual, StrResMap);
    WX_DECLARE_HASH_MAP(UINT, wxIcon, wxIntegerHash, wxIntegerEqual, IconResMap);

    static constexpr int UTF8_BOM_LENGTH = 3;
    static const uint8_t UTF8_BOM[UTF8_BOM_LENGTH] = {0xEF, 0xBB, 0xBF};
    // U+200B 零宽空格 UTF-8 三字节序列
    static const uint8_t ZERO_WIDTH_SPACE[UTF8_BOM_LENGTH] = {0xE2, 0x80, 0x8B};

    // Base64编码字符集
    static const std::string s_base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    class ScreenHelper
    {
    public:
        ScreenHelper(const ScreenHelper &) = delete;
        ScreenHelper &operator=(const ScreenHelper &) = delete;

        static ScreenHelper &Instance();
        ~ScreenHelper();

    public:
        void DPIFromWindow(CWnd *pWnd);
        int DPI(int pixel);
        float DPIF(float pixel);
        int RDPI(int pixel);

    private:
        ScreenHelper();

    private:
        static ScreenHelper m_instance;

        int m_dpi{96};
    };

    class ResourceHelper
    {
    public:
        ResourceHelper(const ResourceHelper &) = delete;
        ResourceHelper &operator=(const ResourceHelper &) = delete;

        static ResourceHelper &Instance();
        ~ResourceHelper();

    public:
        // 读取程序资源
        bool ReadResource(const void **outData, size_t *outLen, const UINT resID, const LPCTSTR resType);
        // 读取文件资源
        wxString FileRes(const UINT resID);
        // 获取资源字符串
        const wxString &StringRes(const UINT resID);
        // 获取 HTML 资源
        const wxString &HtmlRes(const UINT resID);
        // 获取 ICON 资源
        const wxIcon &IconRes(const UINT id);
        // 获取 ICON 资源
        const HICON HiconRes(const UINT id);

    private:
        ResourceHelper();

    private:
        static ResourceHelper m_instance;

        IconResMap m_icons;
        StrResMap m_stringTable;
        StrResMap m_html;
    };

    class StringHelper
    {
    public:
        /// <summary>
        /// UTF8字符串 转 GBK字符串
        /// </summary>
        static std::string UTF8ToGBK(const std::string &utf8Str);

        /// <summary>
        /// 窄字符串 转 宽字符串
        /// </summary>
        static std::wstring ToWideStr(const char *str, bool isUtf8 = false);
        static std::wstring ToWideStr(const std::string &str, bool isUtf8 = false);

        /// <summary>
        /// 宽字符串 转 窄字符串
        /// </summary>
        static std::string ToMultiStr(const wchar_t *wstr, bool isUtf8 = false);
        static std::string ToMultiStr(const std::wstring &wstr, bool isUtf8 = false);

        /// <summary>
        /// unsigned short* 转 宽字符串
        /// </summary>
        static std::wstring WordToWideStr(const WORD *str);

        /// <summary>
        /// 字符串替换
        /// </summary>
        static void Replace(std::string &origin, const std::string &target, const std::string &content, bool replaceAll = false);

        /// <summary>
        /// Base64编码
        /// </summary>
        static std::string Base64Encode(const uint8_t *data, size_t length);

        /// <summary>
        /// 计算字符串SHA1哈希并转Base64
        /// </summary>
        static std::string Sha1ToBase64(const std::string &input);

        //将一个字符串转换成URL编码（以UTF8编码格式）
        // isQuery=false → 空格=%20
        wxString URLEncode(const wxString& raw, bool queryMode = false);

        static std::vector<std::string> split(const std::string &str, const char pattern);
        static std::vector<std::string> split(const std::string &str, const std::string &delimiter);
        static std::wstring vectorJoinString(const std::vector<std::wstring> data, const std::wstring &pattern);
        static std::string RemoveChar(const std::string &str, char ch);
        static std::string RemoveStr(const std::string str, const std::string del);
        static std::string ToUpperCase(std::string s);

        static wxArrayString split(const wxString &str, const wxString &pattern, bool keep_empty = true);
        static wxString remove(const wxString &str, const wxString &del);
        static wxString vectorJoinString(const wxArrayString &data, const wxString &pattern);

        // 转换时间 "09:29" "09:29:00"
        static int TimeToSecond(const wxString& strTime);
        // 递归终止：参数包为空，返回NaN
        static double selectValid()
        {
            return NAN;
        }
        // 重载：参数是浮点类型，先判断有限值
        // 递归展开参数包
        template <typename T, typename... Args>
        static typename std::enable_if<std::is_floating_point<T>::value, double>::type selectValid(T first, Args... rest)
        {
            // 当前参数是有效有限值，直接返回
            if (std::isfinite(static_cast<double>(first)))
            {
                return static_cast<double>(first);
            }
            // 当前参数无效，递归检查后面所有参数
            return selectValid(rest...);
        }
        // 重载：参数非浮点类型，直接跳过当前参数递归后续
        template <typename T, typename... Args>
        static typename std::enable_if<!std::is_floating_point<T>::value, double>::type selectValid(T /*first*/, Args... rest)
        {
            return selectValid(rest...);
        }
        static wxString toFixed(double num, int decimals = 2);
        static double toFixed(const wxString& str, int decimals = 2);
        static bool isValidNum(double v)
        {
            return !isnan(v) && v != 0.0;
        }
        static double parseDouble(const wxString& str);
        static int parseInt(const wxString& p1);

    };

    class NetHelper
    {
    public:
        static bool GetURL(const std::wstring& url, std::string& result, bool utf8, LPCTSTR user_agent, LPCTSTR headers, DWORD dwHeadersLength);
    };

    class JsonHelper {
    public:
        static wxString GetString(yyjson_val* obj, const wxString key, const wxString def_val = wxEmptyString);
        static float GetFloat(yyjson_val* obj, const wxString key, const float def_val = 0.0f);
        static long long GetLL(yyjson_val* obj, const wxString key, const long long def_val = 0L);
        static double GetDouble(yyjson_val* obj, const wxString key, const double def_val = 0.0F);
        static int GetInt(yyjson_val* obj, const wxString key, const int def_val = 0);
    };
}
