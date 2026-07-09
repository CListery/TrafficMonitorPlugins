#include "Common.h"
#include "pch.h"
#include "SHA1.h"
#include <vector>
#include <cctype>
#include <sstream>
#include <wx/tokenzr.h>
#include <afxinet.h>    //用于支持使用网络相关的类
#include <sstream>
#include <wx/url.h>

namespace CommonUtils
{
    // 过滤零宽空格
    wxString FilterZeroWidthSpace(const wxString &wxUtf8Str)
    {
        // wxString -> 原始UTF8 char缓冲区
        wxCharBuffer buf = wxUtf8Str.ToUTF8();
        const char *raw = buf.data();
        const size_t len = buf.length();

        wxMemoryBuffer outBuf;

        for (size_t i = 0; i < len;)
        {
            // 连续匹配3字节零宽空格字节序列
            if (i <= len - UTF8_BOM_LENGTH &&
                (uint8_t)raw[i] == ZERO_WIDTH_SPACE[0] &&
                (uint8_t)raw[i + 1] == ZERO_WIDTH_SPACE[1] &&
                (uint8_t)raw[i + 2] == ZERO_WIDTH_SPACE[2])
            {
                i += UTF8_BOM_LENGTH; // 跳过，不添加
            }
            else
            {
                outBuf.AppendByte(raw[i++]); // 正常字符，保留
            }
        }
        // 过滤后的UTF8二进制转回wxString
        return wxString::FromUTF8(
            reinterpret_cast<const char *>(outBuf.GetData()),
            outBuf.GetDataLen());
    }

    /// ===================================
    /// ScreenHelper
    /// ===================================

    ScreenHelper ScreenHelper::m_instance;

    ScreenHelper::ScreenHelper()
    {
        HDC hDC = ::GetDC(HWND_DESKTOP);
        m_dpi = GetDeviceCaps(hDC, LOGPIXELSY);
        ::ReleaseDC(HWND_DESKTOP, hDC);
    }

    ScreenHelper::~ScreenHelper()
    {
    }

    ScreenHelper &ScreenHelper::Instance()
    {
        return m_instance;
    }

    void ScreenHelper::DPIFromWindow(CWnd *pWnd)
    {
        CWindowDC dc(pWnd);
        HDC hDC = dc.GetSafeHdc();
        m_dpi = GetDeviceCaps(hDC, LOGPIXELSY);
    }

    int ScreenHelper::DPI(int pixel)
    {
        return m_dpi * pixel / 96;
    }

    float ScreenHelper::DPIF(float pixel)
    {
        return m_dpi * pixel / 96;
    }

    int ScreenHelper::RDPI(int pixel)
    {
        return pixel * 96 / m_dpi;
    }

    /// ===================================
    /// ResourceHelper
    /// ===================================

    ResourceHelper ResourceHelper::m_instance;

    ResourceHelper::ResourceHelper()
    {
    }

    ResourceHelper::~ResourceHelper()
    {
    }

    ResourceHelper &ResourceHelper::Instance()
    {
        return m_instance;
    }

    bool ResourceHelper::ReadResource(const void **outData, size_t *outLen, const UINT resID, const LPCTSTR resType)
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        LLOG_WARN("LOAD RES: #%u #%u", resType, resID);

        LPCWSTR resourceName = MAKEINTRESOURCE(resID);

        HINSTANCE hInstance = AfxFindResourceHandle(resourceName, resType);

        // 数字资源ID 标准写法 #数字
        wxString resName = wxString::Format(wxT("#%u"), resID);

        // 返回资源数据的指针。该指针为只读；
        //
        // 注意：资源类型既可以是真实字符串，也可以是通过 MAKEINTRESOURCE() 生成的整型值。
        // 具体来说，所有标准资源类型（即所有 RT_XXX 系列常量）均可传入此处。
        //
        // 执行成功返回 true，失败返回 false
        // 若未找到资源不会输出错误日志（该情况属于正常预期）
        // 但若发生其他类型错误，则会打印错误日志
        if (wxLoadUserResource(outData, outLen, resName, resType, hInstance))
        {
            return true;
        }
        else
        {
            LLOG_WARN("wxLoadUserResource Type:%s ID:%u ErrCode:%lu", resType, resID, GetLastError());

            HRSRC hResource = ::FindResource(hInstance, resourceName, resType);
            if (!hResource)
            {
                LLOG_WARN("LOAD RES FAIL! Type:%s ID:%u ErrCode:%lu", resType, resID, GetLastError());
                return false;
            }

            // 加载资源
            const HGLOBAL hData = ::LoadResource(hInstance, hResource);
            if (!hData)
            {
                LLOG_WARN("Failed to load resource, Type:%s ID:%u ErrCode:%lu", resType, resID, GetLastError());
                return false;
            }

            *outData = ::LockResource(hData);
            if (!*outData)
            {
                LLOG_WARN("Failed to lock resource, Type:%s ID:%u ErrCode:%lu", resType, resID, GetLastError());
                return false;
            }

            *outLen = ::SizeofResource(hInstance, hResource);
        }
        return true;
    }

    const wxString &ResourceHelper::StringRes(const UINT resID)
    {
        auto iter = m_stringTable.find(resID);
        if (iter != m_stringTable.end())
        {
            return iter->second;
        }
        else
        {
            AFX_MANAGE_STATE(AfxGetStaticModuleState());
            CString str;
            if (str.LoadString(resID))
            {
                m_stringTable[resID] = wxString(str);
            }
            return m_stringTable[resID];
        }
    }

    wxString ResourceHelper::FileRes(const UINT resID)
    {
        const void *pData = nullptr;
        size_t dataSize = 0;

        if (!ReadResource(&pData, &dataSize, resID, RT_HTML))
        {
            return "";
        }

        // 处理资源数据
        const uint8_t *pByte = static_cast<const uint8_t *>(pData);
        std::string utf8Str;
        // 跳过UTF-8 BOM (EF BB BF)
        if (dataSize >= UTF8_BOM_LENGTH &&
            (uint8_t)pByte[0] == UTF8_BOM[0] &&
            (uint8_t)pByte[1] == UTF8_BOM[1] &&
            (uint8_t)pByte[2] == UTF8_BOM[2])
        {
            // 跳过前3个BOM字节
            utf8Str = std::string(reinterpret_cast<LPCSTR>(pByte + UTF8_BOM_LENGTH), dataSize - UTF8_BOM_LENGTH);
        }
        else
        {
            // 无BOM，直接加载
            utf8Str = std::string(reinterpret_cast<LPCSTR>(pData), dataSize);
        }

        wxString wxRaw = wxString::FromUTF8(utf8Str);
        // 过滤零宽空格
        wxString cleanText = FilterZeroWidthSpace(wxRaw);

        return cleanText;
    }

    const wxString &ResourceHelper::HtmlRes(const UINT resID)
    {
        auto iter = m_html.find(resID);
        if (iter != m_html.end())
        {
            return iter->second;
        }

        m_html[resID] = FileRes(resID);

        return m_html[resID];
    }

    const wxIcon &ResourceHelper::IconRes(const UINT resID)
    {
        auto iter = m_icons.find(resID);
        if (iter != m_icons.end())
        {
            return iter->second;
        }
        else
        {
            AFX_MANAGE_STATE(AfxGetStaticModuleState());
            HICON hIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(resID), IMAGE_ICON, UtilScreenHlp.DPI(16), UtilScreenHlp.DPI(16), 0);
            wxIcon icon;
            icon.CreateFromHICON(hIcon);
            m_icons[resID] = icon;
            return m_icons[resID];
        }
    }

    const HICON ResourceHelper::HiconRes(const UINT id)
    {
        return IconRes(id).GetHICON();
    }

    /// ===================================
    /// StringHelper
    /// ===================================

    std::string StringHelper::UTF8ToGBK(const std::string &utf8Str)
    {
        if (utf8Str.empty())
            return {};

        // 复用现有转换逻辑：UTF8->宽字符->GBK
        return ToMultiStr(ToWideStr(utf8Str.c_str(), true).c_str(), false);
    }

    std::wstring StringHelper::ToWideStr(const std::string &str, bool isUtf8)
    {
        if (str.empty())
            return {};
        return ToWideStr(str.c_str(), isUtf8);
    }

    std::wstring StringHelper::ToWideStr(const char *str, bool isUtf8)
    {
        if (!str || *str == '\0')
            return {};

        const UINT codePage = isUtf8 ? CP_UTF8 : CP_ACP;
        // 获取宽字符长度
        const int bufferLen = MultiByteToWideChar(codePage, 0, str, -1, nullptr, 0);
        if (bufferLen <= 0)
            return {};

        // 分配内存并转换
        std::vector<wchar_t> wideBuffer(bufferLen, L'\0');
        MultiByteToWideChar(codePage, 0, str, -1, wideBuffer.data(), bufferLen);
        wideBuffer.resize(bufferLen - 1);

        return std::wstring(wideBuffer.data());
    }

    std::string StringHelper::ToMultiStr(const std::wstring &wstr, bool isUtf8)
    {
        if (wstr.empty())
            return {};
        return ToMultiStr(wstr.c_str(), isUtf8);
    }

    std::string StringHelper::ToMultiStr(const wchar_t *wstr, bool isUtf8)
    {
        if (!wstr)
            return {};

        const UINT codePage = isUtf8 ? CP_UTF8 : CP_ACP;
        // 获取窄字符长度
        const int bufferLen = WideCharToMultiByte(codePage, 0, wstr, -1, NULL, 0, NULL, NULL);
        if (bufferLen <= 0)
            return {};

        // 分配内存并转换
        std::vector<char> multiBuffer(bufferLen);
        WideCharToMultiByte(codePage, 0, wstr, -1, multiBuffer.data(), bufferLen, NULL, NULL);
        multiBuffer.resize(bufferLen - 1);

        return std::string(multiBuffer.data());
    }

    std::wstring StringHelper::WordToWideStr(const WORD *str)
    {
        if (!str)
            return {};
        // 类型强转：WORD与wchar_t内存布局一致
        return reinterpret_cast<const wchar_t *>(str);
    }

    void StringHelper::Replace(std::string &origin, const std::string &target, const std::string &content, bool replaceAll)
    {
        if (origin.empty() || target.empty())
            return;

        do
        {
            const size_t pos = origin.find(target);
            if (pos == std::string::npos)
                break;

            origin.replace(pos, target.length(), content);
        } while (replaceAll);
    }

    std::string StringHelper::Base64Encode(const uint8_t *data, size_t length)
    {
        std::string result;
        int i = 0, j = 0;
        uint8_t charArray3[3] = {0};
        uint8_t charArray4[4] = {0};

        while (length--)
        {
            charArray3[i++] = *(data++);
            if (i == 3)
            {
                charArray4[0] = (charArray3[0] & 0xfc) >> 2;
                charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
                charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
                charArray4[3] = charArray3[2] & 0x3f;

                for (i = 0; i < 4; i++)
                    result += s_base64Chars[charArray4[i]];
                i = 0;
            }
        }

        // 处理剩余字节
        if (i != 0)
        {
            for (j = i; j < 3; j++)
                charArray3[j] = '\0';

            charArray4[0] = (charArray3[0] & 0xfc) >> 2;
            charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
            charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
            charArray4[3] = charArray3[2] & 0x3f;

            for (j = 0; j < i + 1; j++)
                result += s_base64Chars[charArray4[j]];

            // 补全等号
            while (i++ < 3)
                result += '=';
        }
        return result;
    }

    // SHA1计算 + Base64编码
    std::string StringHelper::Sha1ToBase64(const std::string &input)
    {
        // 计算SHA-1哈希（输出20字节二进制）
        SHA1 sha;
        sha.Update(reinterpret_cast<const uint8_t *>(input.c_str()), input.size());
        uint8_t sha1Digest[20] = {0};
        sha.Final(sha1Digest);

        // Base64编码二进制哈希
        return Base64Encode(sha1Digest, 20);
    }

    wxString StringHelper::URLEncode(const wxString& raw, bool queryMode)
    {
        wxString res;
        wxCharBuffer buf = raw.ToUTF8();
        const char* p = buf.data();
        while (*p)
        {
            unsigned char c = static_cast<unsigned char>(*p);
            if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
                || c == '-' || c == '_' || c == '.' || c == '~' || c == '!' || c == '~' 
                || c == '*'/* || c == '\''*/ || c == '(' || c == ')')
            {
                res += wxChar(c);
            }
            else if (c == ' ' && queryMode)
            {
                res += '+';
            }
            else
            {
                res += wxString::Format("%%%02X", c);
            }
            p++;
        }
        return res;
    }
    
    std::vector<std::string> StringHelper::split(const std::string &str, const char pattern)
    {
        std::vector<std::string> res;
        if (str.size() <= 0)
        {
            return res;
        }
        if (str.find(pattern) == std::string::npos)
        {
            res.push_back(str);
            return res;
        }
        std::stringstream input(str); // 读取str到字符串流中
        std::string temp;
        // 使用getline函数从字符串流中读取,遇到分隔符时停止,和从cin中读取类似
        // 注意,getline默认是可以读取空格的
        int len = 0;
        while (getline(input, temp, pattern))
        {
            res.push_back(temp);
            len++;
        }
        res.resize(len);
        return res;
    }

    std::vector<std::string> StringHelper::split(const std::string &str, const std::string &delimiter)
    {
        std::vector<std::string> tokens;

        if (delimiter.empty())
        {
            tokens.push_back(str);
            return tokens;
        }

        size_t pos = 0;
        size_t prev = 0;

        while ((pos = str.find(delimiter, prev)) != std::string::npos)
        {
            tokens.push_back(str.substr(prev, pos - prev));
            prev = pos + delimiter.length();
        }

        // 添加最后一个片段
        tokens.push_back(str.substr(prev));

        return tokens;
    }

    std::wstring StringHelper::vectorJoinString(const std::vector<std::wstring> data, const std::wstring &pattern)
    {
        std::wstring str{};
        for (size_t index = 0; index < data.size(); index++)
        {
            if (index > 0)
                str.append(pattern);
            str.append(data[index]);
        }
        return str;
    }

    std::string StringHelper::RemoveChar(const std::string &str, char ch)
    {
        std::string result;
        for (char c : str)
        {
            if (c != ch)
            {
                result += c;
            }
        }
        return result;
    }

    std::string StringHelper::RemoveStr(const std::string str, const std::string del)
    {
        std::string result;

        if (del.empty())
        {
            return str;
        }

        size_t pos = 0;
        size_t prev = 0;

        while ((pos = str.find(del, prev)) != std::string::npos)
        {
            result += str.substr(prev, pos - prev);
            prev = pos + del.length();
        }

        result += str.substr(prev);

        return result;
    }

    std::string StringHelper::ToUpperCase(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c)
                       { return static_cast<char>(std::toupper(c)); });
        return s;
    }

    wxArrayString StringHelper::split(const wxString &str, const wxString &pattern, bool keep_empty)
    {
        wxArrayString arr;
        wxStringTokenizer token(str, pattern, keep_empty ? wxTOKEN_RET_EMPTY : wxTOKEN_DEFAULT);
        while (token.HasMoreTokens())
        {
            arr.Add(token.GetNextToken());
        }
        return arr;
    }

    wxString StringHelper::remove(const wxString &str, const wxString &del)
    {
        wxString copy = str;
        copy.Replace(del, "");
        return copy;
    }

    wxString StringHelper::vectorJoinString(const wxArrayString &data, const wxString &pattern)
    {
        wxString str{};
        for (size_t index = 0; index < data.size(); index++)
        {
            if (index > 0)
                str.append(pattern);
            str.append(data[index]);
        }
        return str;
    }

    // 转换时间 "09:29" "09:29:00"
    int StringHelper::TimeToSecond(const wxString& strTime)
    {
        wxArrayString times = UtilStringHlp::split(strTime, ":");
        int h, m, s = 0;
        switch (times.size()) {
        case 2:
            h = _ttoi(times[0]);
            m = _ttoi(times[1]);
            s = 0;
            break;
        case 3:
            h = _ttoi(times[0]);
            m = _ttoi(times[1]);
            s = _ttoi(times[2]);
            break;
        }
        return h * 60 * 60 + m * 60 + s;
    }

    wxString StringHelper::toFixed(double num, int decimals)
    {
        if (decimals < 0)
            decimals = 0;
        if (decimals > 15)
            decimals = 15;

        // std::ostringstream oss;
        // oss << std::fixed << std::setprecision(decimals) << num;
        // return oss.str();
        wxString fmt;
        fmt.Printf(wxT("%%.%df"), decimals);
        wxString result;
        result.Printf(fmt, num);
        return result;
    }

    double StringHelper::toFixed(const wxString& str, int decimals)
    {
        if (str.empty())
        {
            return NAN;
        }
        try
        {
            double num = std::stod(str.ToStdString());
            wxString str_num = toFixed(num, decimals);
            return std::stod(str_num.ToStdString());
        }
        catch (...)
        {
            return NAN;
        }
    }

    double StringHelper::parseDouble(const wxString& str)
    {
        if (str.empty())
        {
            return NAN;
        }
        try
        {
            return std::stod(str.ToStdString());
        }
        catch (...)
        {
            return NAN;
        }
    }

    int StringHelper::parseInt(const wxString& p1)
    {
        double d62 = parseDouble(p1);
        if (isValidNum(d62))
        {
            return static_cast<int>(std::round(d62));
        }
        return NAN;
    }

    /// ===================================
    /// NetHelper
    /// ===================================

    bool NetHelper::GetURL(const std::wstring &url, std::string &result, bool utf8, LPCTSTR user_agent, LPCTSTR headers, DWORD dwHeadersLength)
    {
        bool succeed{false};
        CInternetSession *pSession{};
        CHttpFile *pfile{};
        try
        {
            pSession = new CInternetSession(user_agent);
            pfile = (CHttpFile *)pSession->OpenURL(url.c_str(), 1, INTERNET_FLAG_TRANSFER_ASCII, headers, dwHeadersLength);
            DWORD dwStatusCode;
            pfile->QueryInfoStatusCode(dwStatusCode);
            if (dwStatusCode == HTTP_STATUS_OK)
            {
                CString content;
                CString data;
                while (pfile->ReadString(data))
                {
                    content += data;
                }
                result = (const char *)content.GetString();
                succeed = true;
            }
            pfile->Close();
            delete pfile;
            pSession->Close();
        }
        catch (CInternetException *e)
        {
            LLOG_ERROR_F(L"request fail!" + url);
            if (pfile != nullptr)
            {
                pfile->Close();
                delete pfile;
            }
            if (pSession != nullptr)
                pSession->Close();
            succeed = false;
            e->Delete();
            SAFE_DELETE(pSession);
        }
        SAFE_DELETE(pSession);
        return succeed;
    }

    /// ===================================
    /// JsonHelper
    /// ===================================

    wxString JsonHelper::GetString(yyjson_val* obj, const wxString key, const wxString def_val)
    {
        if (obj != nullptr)
        {
            yyjson_val* value = yyjson_obj_get(obj, key);
            if (value != nullptr)
            {
                const char* str = yyjson_get_str(value);
                if (str != nullptr)
                    return str;
            }
        }
        return def_val;
    }

    float JsonHelper::GetFloat(yyjson_val* obj, const wxString key, const float def_val)
    {
        if (obj != nullptr)
        {
            yyjson_val* value = yyjson_obj_get(obj, key);
            if (value != nullptr)
            {
                if (yyjson_is_real(value))
                {
                    return static_cast<float>(yyjson_get_real(value));
                }
            }
        }
        return def_val;
    }

    long long JsonHelper::GetLL(yyjson_val* obj, const wxString key, const long long def_val)
    {
        if (obj != nullptr)
        {
            yyjson_val* val = yyjson_obj_get(obj, key);
            try
            {

                if (val != nullptr)
                {
                    yyjson_is_num(val);
                    yyjson_get_type(val);
                    if (yyjson_is_sint(val))
                    {
                        return static_cast<long long>(yyjson_get_sint(val));
                    }
                    else if (yyjson_is_str(val))
                    {
                        return static_cast<long long>(std::stoll(yyjson_get_str(val)));
                    }
                }
            }
            catch (const std::exception& e)
            {
                return def_val;
            }
        }
        return def_val;
    }

    double JsonHelper::GetDouble(yyjson_val* obj, const wxString key, const double def_val)
    {
        if (obj != nullptr)
        {
            yyjson_val* val = yyjson_obj_get(obj, key);
            try
            {
                if (val != nullptr)
                {
                    if (yyjson_is_real(val))
                    {
                        return static_cast<double>(yyjson_get_real(val));
                    }
                    else if (yyjson_is_str(val))
                    {
                        return static_cast<double>(std::stod(yyjson_get_str(val)));
                    }
                }
            }
            catch (const std::invalid_argument&)
            {
                return def_val;
            }
            catch (const std::out_of_range&)
            {
                return def_val;
            }
        }
        return def_val;
    }

    int JsonHelper::GetInt(yyjson_val* obj, const wxString key, const int def_val)
    {
        if (obj != nullptr)
        {
            yyjson_val* val = yyjson_obj_get(obj, key);
            try
            {

                if (val != nullptr)
                {
                    if (yyjson_is_int(val))
                    {
                        return static_cast<int>(yyjson_get_int(val));
                    }
                    else if (yyjson_is_str(val))
                    {
                        return static_cast<int>(std::stoi(yyjson_get_str(val)));
                    }
                }
            }
            catch (const std::exception&)
            {
                return def_val;
            }
        }
        return def_val;
    }

}
