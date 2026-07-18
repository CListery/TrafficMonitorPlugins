#include "pch.h"
#include "DataManager.h"
#include <wx/filename.h>
#include <wx/config.h>
#include "TaskManager.h"
#include <iomanip>
#include <wx/webrequest.h>
#include <wx/wx.h>
#include <random>
#include <wx/hashmap.h>
#include "StockV2.h"

// 链接库（获取模块路径需要）
#pragma comment(lib, "shlwapi.lib")

LDataManager LDataManager::m_instance;

constexpr auto WEB_USERAGENT = _T("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/135.0.0.0 Safari/537.36 Edg/135.0.0.0");

namespace
{
    static const auto KEY_CFG_ALL_STOCK_DATA = "/stock/all_data";
    static const auto KEY_CFG_REALTIME_REFRESH_FREQ = "/stock/realtime_refresh_freq";
    static const auto KEY_CFG_DISPLAY_COLOR = "/stock/is_display_color";
    static const auto KEY_CFG_DISPLAY_NAME = "/stock/is_display_name";
    static const auto KEY_CFG_PRIORITY_DISPLAY_CHANGE = "/stock/is_priority_display_changed";
    static const auto KEY_CFG_KLINE_SIZE_W = "/stock/kline/size/w";
    static const auto KEY_CFG_KLINE_SIZE_H = "/stock/kline/size/h";

    long long generateRandomDouble()
    {
        // 静态随机引擎，只初始化一次，性能更好
        static std::mt19937_64 rng(std::random_device{}());

        // 第一个随机数范围 [1, 1234567890]
        std::uniform_int_distribution<long long> distA(1, 1234567890LL);
        long long a = distA(rng);

        // 第二个随机数范围 [1, 9876543210]
        std::uniform_int_distribution<long long> distB(1, 9876543210LL);
        long long b = distB(rng);

        return a + b;
    }

    void OnRefreshStockRealtimeData(LPVOID pParam)
    {
        wxArrayString codes = g_data.GetAllCodes();

        LLOG_INFO_F("OnRefreshStockRealtimeData: %d", codes.size());

        if (codes.empty())
        {
            return;
        }

        // https://w.sinajs.cn/rn=5769211975&list=sh601011,hf_BTC2606

        static const wxString url_prefix = "https://w.sinajs.cn/";

        wxArrayString params;
        params.push_back(L"rn=" + std::to_wstring(generateRandomDouble()));
        params.push_back(L"list=" + CommonUtils::StringHelper::vectorJoinString(codes, ","));

        wxString url = url_prefix + CommonUtils::StringHelper::vectorJoinString(params, "&");

        CString strHeaders = _T("Referer: https://finance.sina.com.cn");

        std::string jsonp;
        if (UtilNetHlp::GetURL(url.ToStdWstring(), jsonp, false, WEB_USERAGENT, strHeaders, strHeaders.GetLength()))
        {
            g_data.HandleStockRealtimeData(jsonp);
        }

        // m_currentRequest = wxWebSession::GetDefault().CreateRequest(wxApp::GetInstance(), url);
        //
        // if (!m_currentRequest.IsOk())
        //{
        //     return;
        // }

        // m_currentRequest.SetHeader("User-Agent", WEB_USERAGENT);
        // m_currentRequest.SetHeader("Referer", "https://finance.sina.com.cn");

        //// Start the request (events will be sent on success or failure)
        // m_currentRequest.Start();

        //// 工作线程轮询等待，不影响界面
        // while (m_currentRequest.GetState() == wxWebRequest::State_Active)
        //{
        //     wxMilliSleep(10); // 避免 CPU 空转
        // }

        // if (m_currentRequest.GetState() == wxWebRequest::State_Completed)
        //{
        //     wxString jsonp = m_currentRequest.GetResponse().AsString();
        //     LLOG_DEBUG_F(wxT("resp: %s"), jsonp);
        // }
    }
}

// LDataManager 构造
LDataManager::LDataManager()
{
    // 获取当前模块路径
    HMODULE hModule = reinterpret_cast<HMODULE>(&__ImageBase);
    wchar_t _path[MAX_PATH]{};
    GetModuleFileNameW(hModule, _path, MAX_PATH);

    // 提取目录路径
    wxFileName moduleDllPath(_path);
    m_moduleParentPath = moduleDllPath.GetPath();
    m_moduleName = moduleDllPath.GetName();
    m_isDisplayRightAlign = false;
}

LDataManager::~LDataManager()
{
    g_task->DeleteTask(TASK_KEY_INIT_LOAD);
    g_task->DeleteTask(TASK_KEY_STOCK_AUTO_REFRESH);
    ClearAllCodes();
    m_stockConfig.datas.clear();
}

LDataManager &LDataManager::Instance()
{
    return m_instance;
}

const wxString LDataManager::GetModuleParentPath()
{
    return m_moduleParentPath;
}
const wxString LDataManager::GetModuleName()
{
    return m_moduleName;
}

const wxArrayString LDataManager::GetAllCodes()
{
    wxArrayString all_codes;
    for (const auto &data : m_stockConfig.datas)
    {
        if (data && !data->code.empty())
        {
            all_codes.push_back(data->code);
        }
    }
    return all_codes;
}

void LDataManager::RealtimeRefreshFreq(long freq)
{
    m_stockConfig.realtimeRefreshFreq = freq;
}

long LDataManager::RealtimeRefreshFreq() const
{
    return m_stockConfig.realtimeRefreshFreq;
}

void LDataManager::IsDisplayName(bool flag)
{
    m_stockConfig.isDisplayName = flag;
}

bool LDataManager::IsDisplayName() const
{
    return m_stockConfig.isDisplayName;
}

void LDataManager::IsDisplayColor(bool flag)
{
    m_stockConfig.isDisplayColor = flag;
}

bool LDataManager::IsDisplayColor() const
{
    return m_stockConfig.isDisplayColor;
}

void LDataManager::IsDisplayRightAlign(bool flag)
{
    m_isDisplayRightAlign = flag;
}

bool LDataManager::IsDisplayRightAlign() const
{
    return m_isDisplayRightAlign;
}

void LDataManager::IsPriorityDisplayChanged(bool flag)
{
    m_stockConfig.isPriorityDisplayChanged = flag;
}

bool LDataManager::IsPriorityDisplayChanged() const
{
    return m_stockConfig.isPriorityDisplayChanged;
}

void LDataManager::KLineWH(int w, int h)
{
    m_stockConfig.klineW = w;
    m_stockConfig.klineH = h;
}

void LDataManager::KLineW(int w)
{
    m_stockConfig.klineW = w;
}

int LDataManager::KLineW() const
{
    return m_stockConfig.klineW;
}

void LDataManager::KLineH(int h)
{
    m_stockConfig.klineH = h;
}

int LDataManager::KLineH() const
{
    return m_stockConfig.klineH;
}

std::pair<int, int> LDataManager::KLineWH() const
{
    return std::make_pair(m_stockConfig.klineW, m_stockConfig.klineH);
}

int LDataManager::DecimalPlaces() const
{
    return m_stockConfig.decimalPlaces;
}

void LDataManager::DecimalPlaces(int num)
{
    m_stockConfig.decimalPlaces = num;
}

void LDataManager::InitConfig()
{
    wxConfigBase *pConfig = wxConfigBase::Get();

    wxString all_stock_datas = pConfig->Read(KEY_CFG_ALL_STOCK_DATA);
    LLOG_DEBUG("all_stock_datas: %s", all_stock_datas);
    m_stockConfig.datas.clear();
    m_stock_cache.clear();
    if (!all_stock_datas.empty())
    {
        wxArrayString all_stock_list = CommonUtils::StringHelper::split(all_stock_datas, ";");
        if (all_stock_list.empty())
        {
            LLOG_WARN_F("all_stock_datas may be error!");
            pConfig->Write(KEY_CFG_ALL_STOCK_DATA, wxEmptyString);
        }
        else
        {
            for (wxString cfg_data : all_stock_list)
            {
                if (cfg_data.empty())
                {
                    continue;
                }
                StockDataPtr data(new LStockData());
                if (!data->LoadByConfig(cfg_data))
                {
                    continue;
                }
                m_stockConfig.datas.push_back(data);
                m_stock_cache.insert(data);
            }
        }
    }

    m_stockConfig.realtimeRefreshFreq = pConfig->Read(KEY_CFG_REALTIME_REFRESH_FREQ, DEFAULT_STOCK_REFRESH_FREQ);

    m_stockConfig.isDisplayColor = pConfig->ReadBool(KEY_CFG_DISPLAY_COLOR, true);
    m_stockConfig.isDisplayName = pConfig->ReadBool(KEY_CFG_DISPLAY_NAME, true);
    m_stockConfig.isPriorityDisplayChanged = pConfig->ReadBool(KEY_CFG_PRIORITY_DISPLAY_CHANGE, false);

    m_stockConfig.klineW = pConfig->Read(KEY_CFG_KLINE_SIZE_W, DEFAULT_KLINE_VIEW_W);
    m_stockConfig.klineH = pConfig->Read(KEY_CFG_KLINE_SIZE_H, DEFAULT_KLINE_VIEW_H);

    LLOG_DEBUG("codes: %d", m_stock_cache.size());
}

void LDataManager::InitTasks()
{
    if (RealtimeRefreshFreq() >= DEFAULT_STOCK_REFRESH_FREQ)
    {
        g_task->AddTask(TASK_KEY_INIT_LOAD, 0, OnRefreshStockRealtimeData);
    }
    g_task->AddTask(TASK_KEY_STOCK_AUTO_REFRESH, RealtimeRefreshFreq() * 1000L, OnRefreshStockRealtimeData);
    // g_task->AddTask(TASK_KEY_STOCK_TIMELINE_AUTO_REFRESH, 1000L, OnRefreshStockTimelineData);
    // g_task->EnableTask(TASK_KEY_STOCK_TIMELINE_AUTO_REFRESH, false);
}

void LDataManager::ReLoadConfig()
{

    wxConfigBase *pConfig = wxConfigBase::Get();

    m_stockConfig.realtimeRefreshFreq = pConfig->Read(KEY_CFG_REALTIME_REFRESH_FREQ, DEFAULT_STOCK_REFRESH_FREQ);
    m_stockConfig.isDisplayColor = pConfig->ReadBool(KEY_CFG_DISPLAY_COLOR, true);
    m_stockConfig.isDisplayName = pConfig->ReadBool(KEY_CFG_DISPLAY_NAME, true);
    m_stockConfig.isPriorityDisplayChanged = pConfig->ReadBool(KEY_CFG_PRIORITY_DISPLAY_CHANGE, false);
    m_stockConfig.klineW = pConfig->Read(KEY_CFG_KLINE_SIZE_W, DEFAULT_KLINE_VIEW_W);
    m_stockConfig.klineH = pConfig->Read(KEY_CFG_KLINE_SIZE_H, DEFAULT_KLINE_VIEW_H);
}

void LDataManager::ReLoadAllStocks()
{
    m_stock_cache.clear();
    for (auto &&data : m_stockConfig.datas)
    {
        if (!data)
        {
            continue;
        }
        m_stock_cache.insert(data);
    }
}

bool LDataManager::IsChangeStockList()
{
    return AllStocks().size() != m_stockConfig.datas.size();
}

void LDataManager::SaveConfig()
{
    wxConfigBase *pConfig = wxConfigBase::Get();

    pConfig->Write(KEY_CFG_REALTIME_REFRESH_FREQ, m_stockConfig.realtimeRefreshFreq);
    pConfig->Write(KEY_CFG_DISPLAY_COLOR, m_stockConfig.isDisplayColor);
    pConfig->Write(KEY_CFG_PRIORITY_DISPLAY_CHANGE, m_stockConfig.isPriorityDisplayChanged);
    pConfig->Write(KEY_CFG_DISPLAY_NAME, m_stockConfig.isDisplayName);

    pConfig->Write(KEY_CFG_KLINE_SIZE_W, m_stockConfig.klineW);
    pConfig->Write(KEY_CFG_KLINE_SIZE_H, m_stockConfig.klineH);

    wxArrayString all_stock_list;
    m_stockConfig.datas.clear();

    m_stock_cache.ForEach([&all_stock_list, this](const wxString &code, StockDataPtr stock)
                          {
        if (stock && !stock->code.empty()) {
            all_stock_list.push_back(stock->ToConfig());
            m_stockConfig.datas.push_back(stock);
        } });

    wxString all_stock_datas = UtilStringHlp::vectorJoinString(all_stock_list, ";");
    LLOG_DEBUG("all_stock_datas: %s", all_stock_datas);
    pConfig->Write(KEY_CFG_ALL_STOCK_DATA, all_stock_datas);
    pConfig->Flush();

    g_task->ModifyTaskInterval(TASK_KEY_STOCK_AUTO_REFRESH, RealtimeRefreshFreq() * 1000L);
}

void LDataManager::AddStock(StockDataPtr data)
{
    if (data && !data->code.empty())
    {
        m_stock_cache.insert(data);
    }
}

void LDataManager::ClearAllCodes()
{
    m_stock_cache.clear();
}

wxVector<StockDataPtr> LDataManager::AllStocks()
{
    wxVector<StockDataPtr> datas;

    m_stock_cache.ForEach(
        [&datas](const wxString &code, StockDataPtr stock)
        {
            if (stock && !stock->code.empty())
            {
                datas.push_back(stock);
            }
        });

    return datas;
}

StockDataPtr LDataManager::GetStockByCode(const wxString &code)
{
    auto it = m_stock_cache.find(code);
    if (!it)
    {
        return StockDataPtr(nullptr);
    }
    return it;
}

StockDataPtr LDataManager::GetStockByIndex(WXUINT index)
{
    auto it = m_stock_cache.indexAt(index);
    if (!it)
    {
        return StockDataPtr(nullptr);
    }
    return it;
}

bool LDataManager::DeleteStockByCode(const wxString &code)
{
    return m_stock_cache.remove(code);
}

bool LDataManager::SetStockPosition(const StockDataPtr ptr, size_t pos)
{
    return m_stock_cache.move_to(ptr->code, pos);
}

bool LDataManager::HandleStockRealtimeData(wxString jsonp)
{
    if (jsonp.empty())
    {
        LLOG_WARN_F("HandleStockRealtimeData data EMPTY!");
        return false;
    }

    jsonp.Replace("\n", "");
    wxArrayString lines = UtilStringHlp::split(jsonp, ";");
    if (lines.empty())
    {
        LLOG_WARN_F("HandleStockRealtimeData data INVALID!");
        return false;
    }

    for (wxString line : lines)
    {
        if (line.empty())
        {
            continue;
        }
        line.Replace("var hq_str_", "");
        line.Replace("\"", "");

        wxArrayString item_arr = UtilStringHlp::split(line, '=');
        if (item_arr.size() != 2)
        {
            LLOG_WARN_F("HandleStockRealtimeData line data INVALID!");
            continue;
        }

        wxString stock_code = item_arr[0];
        wxString raw_data = item_arr[1];

        auto stock = m_stock_cache.find(stock_code);
        if (!stock)
        {
            LLOG_DEBUG("HandleStockRealtimeData ignore %s", stock_code);
            continue;
        }
        stock->LoadByRealtimeData(stock_code, raw_data);
    }
    return true;
}

void LDataManager::OnRefreshStockTimelineData(const std::string &callback_id, const std::string &data)
{
    wxLogDebug("OnRefreshStockTimelineData: %s", data.c_str());

    yyjson_doc *doc = yyjson_read(data.c_str(), data.size(), 0);
    if (doc == nullptr)
    {
        wxLogDebug("OnRefreshStockTimelineData: json parse error!");
        return;
    }
    yyjson_val *root = yyjson_doc_get_root(doc);
    if (root == nullptr)
    {
        wxLogDebug("OnRefreshStockTimelineData: root");
        yyjson_doc_free(doc);
        return;
    }

    LStockPeriodType type = IntToStockPeriodType(UtilJsonHlp::GetInt(root, "type", -1));
    switch (type)
    {
    case LStockPeriodType::TIMELINE:
        break;
    default:
        wxLogDebug("OnRefreshStockTimelineData: not support %d", type);
        yyjson_doc_free(doc);
        return;
    }

    std::string code = UtilJsonHlp::GetString(root, "code").ToStdString();

    auto stock = g_data.GetStockByCode(code);
    if (stock && !stock->code.empty())
    {
        if (stock->CanUpdateStockPeriodData(type))
        {
            wxArrayString params;
            wxString url_prefix = wxEmptyString;

            // h5t.js
            switch (market(stock->code))
            {
            case MarketType::MarketType_REPO:
                //     REPO: {
                //         T_Head_STR: "t1",
                //         T_EMI_URL: "http://finance.sina.com.cn/finance/eqlweight/$symbol.js",
                //         T_URL: "https://quotes.sina.cn/cn/api/openapi.php/CN_MinlineService.getMinlineData?symbol=$symbol&version=7.11.0&callback=$cb=&dpc=1",
                //         T5_URL: "http://finance.sina.com.cn/realstock/company/$symbol/hisdata/klc_cm_bond.js?day=$rn",
                //         TRADING_DATES_URL: c,
                //         HISTORY_DATA_URL: "http://finance.sina.com.cn/realstock/company/$symbol/hisdata/$y/$m.js?d=$date",
                //         LAST5_URL: "http://finance.sina.com.cn/realstock/lastfive/$symbol.js?_=$rn"
                //     },
                url_prefix = "https://quotes.sina.cn/cn/api/openapi.php/CN_MinlineService.getMinlineData?";

                // symbol=$symbol&version=7.11.0&callback=$cb=&dpc=1

                params.push_back(L"symbol=" + stock->code);
                params.push_back(L"version=7.11.0");
                params.push_back(L"dpc=1");

                // {
                //     "result": {
                //         "status": {
                //             "code": 0,
                //             "msg": "MySql:lv1:success"
                //         },
                //         "data": [
                //             {
                //                 "m": "09:30:00",
                //                 "v": "477000",
                //                 "p": "7.19",
                //                 "avg_p": "7.19",
                //                 "tot_v": "477000"
                //             },
                //         ]
                //     }
                // }

                break;
            case MarketType::MarketType_BJ:
            //     BJ: {
            //         T_URL: "https://cn.finance.sina.com.cn/minline/getMinlineData?symbol=$symbol&version=7.11.0&callback=$cb=&dpc=1"
            //     },
            case MarketType::MarketType_SI:
            //     SI: {
            //         T_Head_STR: "t1",
            //         T_URL: "https://cn.finance.sina.com.cn/minline/getMinlineData?symbol=$symbol&version=7.11.0&callback=$cb=&dpc=1",
            //         T5_URL: "http://finance.sina.com.cn/realstock/company/$symbol/hisdata/klc_cm.js?day=$rn",
            //         TRADING_DATES_URL: c,
            //         LAST5_URL: "http://finance.sina.com.cn/realstock/lastfive/$symbol.js?_=$rn"
            //     },
            case MarketType::MarketType_CNI:
            case MarketType::MarketType_DY:
            case MarketType::MarketType_GN:
            case MarketType::MarketType_HY:
            case MarketType::MarketType_SZ_SH:
                //     CN: {
                //         T_Head_STR: "t1",
                //         T_EMI_URL: "http://finance.sina.com.cn/finance/eqlweight/$symbol.js",
                //         T_URL: "https://cn.finance.sina.com.cn/minline/getMinlineData?symbol=$symbol&callback=$cb=&version=7.11.0&dpc=1",
                //         T5_URL: "http://finance.sina.com.cn/realstock/company/$symbol/hisdata/klc_cm.js?day=$rn",
                //         TRADING_DATES_URL: c,
                //         HISTORY_DATA_URL: "http://finance.sina.com.cn/realstock/company/$symbol/hisdata/$y/$m.js?d=$date",
                //         LAST5_URL: "http://finance.sina.com.cn/realstock/lastfive/$symbol.js?_=$rn"
                //     },
                url_prefix = "https://cn.finance.sina.com.cn/minline/getMinlineData?";

                params.push_back(L"symbol=" + stock->code);
                params.push_back(L"version=7.11.0");
                params.push_back(L"dpc=1");

                // {
                //     "result": {
                //         "status": {
                //             "code": 0,
                //             "msg": "MySql:lv1:success"
                //         },
                //         "data": [
                //             {
                //                 "m": "09:30:00",
                //                 "v": "5513800",
                //                 "p": "4.96",
                //                 "avg_p": "4.96",
                //                 "tot_v": "5513800"
                //             }
                //         ]
                //     }
                // }

                break;
            case MarketType::MarketType_OTC:
                //     OTC: {
                //         T_Head_STR: "t1",
                //         T_URL: "http://stock.finance.sina.com.cn/thirdmarket/api/openapi.php/NQHQService.minline?symbol=$symbol&callback=$cb=",
                //         TRADING_DATES_URL: c
                //     },

                // https://stock.finance.sina.com.cn/thirdmarket/quotes/832059.html

                url_prefix = "https://stock.finance.sina.com.cn/thirdmarket/api/openapi.php/NQHQService.minline?";

                params.push_back(L"symbol=" + stock->code);
                break;
            case MarketType::MarketType_BTC:
                break;
            case MarketType::MarketType_US:
            {
                //     US: {
                //         T_Head_STR: "t1",
                //         T_URL: "http://stock.finance.sina.com.cn/usstock/api/jsonp_v2.php/$cb=/US_MinlineNService.getMinline?symbol=$symbol&day=1&random=$rn",
                //         T5_URL: "http://stock.finance.sina.com.cn/usstock/api/jsonp_v2.php/$cb/US_MinlineNService.getMinline?symbol=$symbol&day=5&random=$rn",
                //         TRADING_DATES_URL: "http://stock.finance.sina.com.cn/usstock/api/openapi.php/US_MinKService.getTradeDays?&start_day=$start&end_day=$end&callback=$cb="
                //     },

                wxString code = UtilStringHlp::remove(stock->code, "gb_");
                url_prefix = wxT("https://stock.finance.sina.com.cn/usstock/api/jsonp_v2.php/var%20t1") + code + wxT("=/US_MinlineNService.getMinline?");

                params.push_back(L"symbol=" + code);
                params.push_back(L"day=1");
                // params.push_back(L"random=1");

                // /*<script>location.href='//sina.com';</script>*/
                // var t1aapl=("09:30:00,4534779,331.859,332.4800;09:31:00,414814,331.867,331.5050;09:32:00,217974,331.886,332.8530;09:33:00,307562,331.917,332.9920;09:34:00,323419,331.977,333.4400;09:35:00,283580

                break;
            }
            case MarketType::MarketType_HK:
                //     HK: {
                //         T_Head_STR: "t1",
                //         T_URL: "http://stock.finance.sina.com.cn/hkstock/api/openapi.php/HK_StockService.getHKMinline?symbol=$symbol&random=$rn&callback=$cb=",
                //         T5_URL: "http://quotes.sina.cn/hk/api/openapi.php/HK_MinlineService.getMinline?symbol=$symbol&day=5&callback=$cb=",
                //         LAST5_URL: "http://stock.finance.sina.com.cn/hkstock/api/jsonp_v2.php/$cb/HK_StockService.getStock5DayAvgVolume?symbol=$symbol",
                //         TRADING_DATES_URL: c
                //     },
            case MarketType::MarketType_HKAP:
                //     HKAP: {
                //         T_Head_STR: "t1",
                //         T_URL: "http://quotes.sina.cn/hk/api/openapi.php/HK_MinlineService.getMinlineDp?symbol=$symbol&callback=$cb=",
                //         T5_URL: "http://quotes.sina.cn/hk/api/openapi.php/HK_MinlineService.getMinline?symbol=$symbol&day=5&callback=$cb=",
                //         LAST5_URL: "http://stock.finance.sina.com.cn/hkstock/api/jsonp_v2.php/$cb/HK_StockService.getStock5DayAvgVolume?symbol=$symbol",
                //         TRADING_DATES_URL: c
                //     },
                url_prefix = "https://quotes.sina.cn/hk/api/openapi.php/HK_MinlineService.getMinline?";

                params.push_back(L"symbol=" + stock->code);
                params.push_back(L"day=1");
                // {
                //     "result": {
                //         "status": {
                //             "code": 0
                //         },
                //         "data": [
                //             [
                //                 {
                //                     "date": "2026-07-17",
                //                     "prevclose": "70.45000",
                //                     "m": "09:30:00",
                //                     "price": "71.10000",
                //                     "volume": "83500",
                //                     "avg_p": "71.055"
                //                 },
                //                 {
                //                     "m": "09:31:00",
                //                     "price": "71.35000",
                //                     "volume": "122500",
                //                     "avg_p": "71.233"
                //                 },
                //             ]
                //         ]
                //     }
                // }

                break;
            case MarketType::MarketType_HF:
            {
                //     HF: {
                //         T_Head_STR: "t1",
                //         T_URL: "http://stock2.finance.sina.com.cn/futures/api/openapi.php/GlobalFuturesService.getGlobalFuturesMinLine?symbol=$symbol&callback=$cb=",
                //         T5_URL: "http://stock2.finance.sina.com.cn/futures/api/openapi.php/GlobalFuturesService.getGlobalFutures5MLine?symbol=$symbol&callback=$cb=",
                //         TRADING_DATES_URL: c
                //     },
                url_prefix = "https://stock2.finance.sina.com.cn/futures/api/openapi.php/GlobalFuturesService.getGlobalFuturesMinLine?";

                wxString code = UtilStringHlp::remove(stock->code, "hf_");

                params.push_back(L"symbol=" + code);

                // {
                //     "result": {
                //         "status": {
                //             "code": 0
                //         },
                //         "data": {
                //             "minLine_1d": [
                //                 [
                //                     "2026-07-17",
                //                     "3992.100",
                //                     "cme",
                //                     "",
                //                     "06:00",
                //                     "3981.260",
                //                     "0",
                //                     "0",
                //                     "3981.259",
                //                     "2026-07-17 06:00:00"
                //                 ],
                //                 [
                //                     "06:01",
                //                     "3982.021",
                //                     "0",
                //                     "0",
                //                     "3981.442",
                //                     "2026-07-17 06:01:00"
                //                 ],
                //             ]
                //         }
                //     }
                // }

                break;
            }
            case MarketType::MarketType_globalbd:
                //     globalbd: {
                //         T_Head_STR: "t1",
                //         T_URL: "http://bond.finance.sina.com.cn/hq/gb/min?symbol=$symbol&callback=$cb=",
                //         TRADING_DATES_URL: c
                //     },
                break;
            case MarketType::MarketType_LSE:
                //     LSE: {
                //         T_Head_STR: "t1",
                //         T_URL: "http://quotes.sina.cn/lse/api/openapi.php/LSEService.minline?symbol=$symbol&type=1&callback=$cb=",
                //         TRADING_DATES_URL: c
                //     }
                break;
            case MarketType::MarketType_NF:
                //     NF: {
                //         T_Head_STR: "t1",
                //         T_URL: "http://stock2.finance.sina.com.cn/futures/api/jsonp.php/$cb=/InnerFuturesNewService.getMinLine?symbol=$symbol",
                //         T5_URL: "http://stock2.finance.sina.com.cn/futures/api/jsonp.php/$cb=/InnerFuturesNewService.getFourDaysLine?symbol=$symbol",
                //         TRADING_DATES_URL: c
                //     },
                break;
            case MarketType::MarketType_GOODS:
                //     GOODS: {
                //         T_Head_STR: "t1",
                //         T_URL: "http://stock2.finance.sina.com.cn/futures/api/openapi.php/SpotService.getMinLine?symbol=$symbol&callback=$cb=",
                //         TRADING_DATES_URL: c
                //     },
                break;
            case MarketType::MarketType_fund:
                //     fund: {
                //         T_Head_STR: "t1",
                //         T_URL: "http://app.xincai.com/fund/api/jsonp.json/$cb=/XinCaiFundService.getFundYuCeNav?symbol=$symbol&___qn=3",
                //         TRADING_DATES_URL: c
                //     },
                break;
            case MarketType::MarketType_option_cn:
                //     option_cn: {
                //         T_Head_STR: "t1",
                //         T_URL: "http://stock.finance.sina.com.cn/futures/api/openapi.php/StockOptionDaylineService.getOptionMinline?symbol=$symbol&random=$rn&callback=$cb=",
                //         T5_URL: "http://stock.finance.sina.com.cn/futures/api/openapi.php/StockOptionDaylineService.getFiveDayLine?symbol=$symbol&random=$rn&callback=$cb=",
                //         TRADING_DATES_URL: c
                //     },
                break;
            case MarketType::MarketType_op_m:
                //     op_m: {
                //         T_Head_STR: "t1",
                //         T_URL: "http://stock.finance.sina.com.cn/futures/api/openapi.php/FutureOptionAllService.getOptionMinline?symbol=$symbol&random=$rn&callback=$cb=",
                //         TRADING_DATES_URL: c
                //     },
                break;
            case MarketType::MarketType_global_index:
                //     global_index: {
                //         T_Head_STR: "t1",
                //         T_URL: "http://gi.finance.sina.com.cn/hq/min/?symbol=$symbol&callback=$cb=",
                //         TRADING_DATES_URL: c
                //     },
                break;
            case MarketType::MarketType_forex:
                break;
            case MarketType::MarketType_forex_yt:
                break;
            case MarketType::MarketType_CFF:
                //     CFF: {
                //         T_Head_STR: "t1",
                //         T_URL: "http://stock2.finance.sina.com.cn/futures/api/jsonp.php/$cb=/InnerFuturesNewService.getMinLine?symbol=$symbol",
                //         T5_URL: "http://stock2.finance.sina.com.cn/futures/api/jsonp.php/$cb=/InnerFuturesNewService.getFourDaysLine?symbol=$symbol",
                //         TRADING_DATES_URL: c
                //     },
                break;
            case MarketType::MarketType_MSCI:
                //     MSCI: {
                //         T_Head_STR: "t1",
                //         T_URL: "http://quotes.sina.cn/msci/api/openapi.php/MSCIService.getMinLine?symbol=$symbol&callback=$cb=",
                //         TRADING_DATES_URL: c
                //     },
                break;
            default:
                break;
            }

            if (url_prefix.empty())
            {
                LLOG_WARN_F("not support");
                return;
            }

            wxString url = url_prefix + CommonUtils::StringHelper::vectorJoinString(params, "&");
            CString strHeaders = (wxT("Referer: ") + stock->url).wc_str();

            std::string data;
            if (UtilNetHlp::GetURL(url.ToStdWstring(), data, false, WEB_USERAGENT, strHeaders, strHeaders.GetLength()))
            {
                // stock->addTimelineData(data);

                stock->updatePeriodRawData(type, data);

                LStockServerSocket::GetInstance().GetBridge()->BridgeSendDataWithCallback(callback_id, stock->GetBridgData(type).ToStdString());
            }
        }
        else
        {
            LStockServerSocket::GetInstance().GetBridge()->BridgeSendDataWithCallback(callback_id, stock->GetBridgData(type).ToStdString());
        }
    }
    else
    {
        wxLogDebug("OnRefreshStockTimelineData: not found stock %s", code);
    }

    yyjson_doc_free(doc);
}
