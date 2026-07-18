#pragma once
#include "pch.h"
#include <wx/dataview.h>
#include <wx/hashmap.h>
#include <memory>
#include <wx/sharedptr.h>
#include <utilities/yyjson/yyjson.h>
#include "StockSockets.h"
#include <unordered_set>
#include "Logger.h"

namespace STOCK
{
    static constexpr char EMPTY_VAL[3] = "--";

    static const auto CFG_REPLACE_STR = "_$";

    // 价格
    using Price = double;
    // 数量
    using Volume = long long;
    // 金额
    using Amount = double;
    using TimePoint = wxString;

    using Clock = std::chrono::steady_clock;
    using ClockTimePoint = Clock::time_point;

    enum MarketType
    {
        MarketType_REPO,
        MarketType_SI,
        MarketType_SZ_SH,
        MarketType_BJ,
        MarketType_GN,
        MarketType_HY,
        MarketType_DY,
        MarketType_CNI,
        MarketType_OTC,
        MarketType_BTC,
        MarketType_US,
        MarketType_HKAP,
        MarketType_HK,
        MarketType_HF,
        MarketType_globalbd,
        MarketType_LSE,
        MarketType_NF,
        MarketType_GOODS,
        MarketType_fund,
        MarketType_option_cn,
        MarketType_op_m,
        MarketType_global_index,
        MarketType_forex,
        MarketType_forex_yt,
        MarketType_CFF,
        MarketType_MSCI,
        MarketType_UNKNOWN,
    };
    MarketType market(const wxString &code);
    bool isCN(MarketType type)
    {
        return type == MarketType::MarketType_SZ_SH ||
               type == MarketType::MarketType_BJ ||
               type == MarketType::MarketType_GN ||
               type == MarketType::MarketType_HY ||
               type == MarketType::MarketType_SI ||
               type == MarketType::MarketType_CNI ||
               type == MarketType::MarketType_DY;
    }

    enum GetMarketType
    {
        GetMarketType_A,
        GetMarketType_US,
        GetMarketType_HF,
        GetMarketType_NF,
        GetMarketType_SI,
        GetMarketType_DINIW,
        GetMarketType_FX,
        GetMarketType_B,
        GetMarketType_LSE,
        GetMarketType_GOODS,
        GetMarketType_ZNB,
        GetMarketType_HK,
        GetMarketType_SB,
        GetMarketType_BT,
        GetMarketType_FUND,
        GetMarketType_MSCI,
        GetMarketType_RTHK,
        GetMarketType_BLOCK,
        GetMarketType_GlobalBD,
        GetMarketType_UNKNOWN,
    };
    GetMarketType getMarket(const wxString &code);

    //------------------------------------------------------------
    // LStockPeriod*
    //------------------------------------------------------------

    // 定义不同的数据周期
    enum class LStockPeriodType : int
    {
        UNKNOWN = -1,
        TIMELINE, // 分时
        MIN1,     // 1分钟
        MIN5,     // 5分钟
        MIN15,    // 15分钟
        MIN30,    // 30分钟
        HOUR1,    // 1小时
        DAY,      // 日线
        WEEK,     // 周线
        MONTH,    // 月线
        YEAR      // 年线
    };

    std::unordered_set<int> GetValidPeriodTypeValues();

    LStockPeriodType IntToStockPeriodType(int val);

    // // 数据周期基类
    // class LStockPeriodDataBase
    // {
    // public:
    //     TimePoint time;            // 时间点
    //     Volume volume;             // 成交量
    //     Price price;               // 价格
    //     Price averagePrice;        // 均价
    //     Volume accumulationVolume; // 累计成交量
    // public:
    //     virtual ~LStockPeriodDataBase() = default;
    //     virtual LStockPeriodType GetType() const = 0;
    //     // virtual TimePoint GetStartTime() const = 0;
    //     // virtual TimePoint GetEndTime() const = 0;
    //     //  对外暴露模板接口，内部调用私有虚分发函数
    //     template <typename T>
    //     void HandleByData(const T &data)
    //     {
    //         DispatchHandle(data);
    //     }

    // private:
    //     virtual void DispatchHandle(yyjson_val *item) = 0;
    // };

    // class LStockPeriodTimelineData : public LStockPeriodDataBase
    // {
    // public:
    //     LStockPeriodType GetType() const wxOVERRIDE { return LStockPeriodType::TIMELINE; }

    // public:
    //     static bool LoadJsonIterator(MarketType type, wxString json_data, yyjson_arr_iter *iter, yyjson_doc **out_doc);

    // private:
    //     void DispatchHandle(yyjson_val *item) wxOVERRIDE;
    // };

    class LStockPeriodTypeHash
    {
    public:
        LStockPeriodTypeHash() {}

        unsigned long operator()(const LStockPeriodType &k) const
        {
            return static_cast<unsigned long>(k);
        }

        LStockPeriodTypeHash &operator=(const LStockPeriodTypeHash &) { return *this; }
    };
    class LStockPeriodTypeEqual
    {
    public:
        LStockPeriodTypeEqual() {}
        bool operator()(const LStockPeriodType &a, const LStockPeriodType &b) const
        {
            return a == b;
        }

        LStockPeriodTypeEqual &operator=(const LStockPeriodTypeEqual &) { return *this; }
    };

    // WX_DECLARE_HASH_MAP(LStockPeriodType, wxVector<wxSharedPtr<LStockPeriodDataBase>>, LStockPeriodTypeHash, LStockPeriodTypeEqual, StockPeriodDataMap);
    WX_DECLARE_HASH_MAP(LStockPeriodType, wxString, LStockPeriodTypeHash, LStockPeriodTypeEqual, StockPeriodRawDataMap);
    WX_DECLARE_HASH_MAP(LStockPeriodType, ClockTimePoint, LStockPeriodTypeHash, LStockPeriodTypeEqual, StockPeriodTimeMap);

    //------------------------------------------------------------
    // LStockData
    //------------------------------------------------------------

    class LStockData
    {
    public:
        LStockData() = default;

        LStockData(const LStockData &) = default;
        LStockData &operator=(const LStockData &) = default;

    public:
        wxString type;
        wxString name;
        wxString code;
        wxString url;
        unsigned int decimals; // 小数点位数

        wxString changePrice;       // change
        wxString changeFluctuation; // percent

    private:
        Price open;           // 今日开盘价
        Price prevclose;      // 昨日收盘价
        Price price;          // 当前价格
        Price high;           // 最高价
        Price low;            // 最低价
        Volume totalVolume_i; // 成交量(股)
        Amount totalAmount_i; // 成交额(元)

        Price priceLimit; // 价格限制

        // StockPeriodDataMap period_data_map;
        StockPeriodRawDataMap period_raw_data_map;
        StockPeriodTimeMap period_time_map;

    public:
        wxString GetCurrentPrice() const
        {
            return UtilStringHlp::toFixed(price, decimals);
        }

        wxString GetChangePrice() const
        {
            return this->changePrice;
        }

    public:
        void LoadByRealtimeData(const wxString &code, const wxString &raw_data);
        void LoadBySearchData(const wxString &raw_data);
        bool LoadByConfig(const wxString &raw_data);
        wxString ToConfig() const;
        wxString GetBridgData(LStockPeriodType type) const;

        // wxVector<wxSharedPtr<LStockPeriodDataBase>> &MakesureStockPeriodData(LStockPeriodType type)
        // {
        //     auto dataIt = period_data_map.find(type);
        //     if (dataIt != period_data_map.end())
        //     {
        //         return dataIt->second;
        //     }
        //     auto &data = period_data_map[type];
        //     auto &time = period_time_map[type];
        //     return data;
        // }

        // wxVector<wxSharedPtr<LStockPeriodDataBase>> &ResetStockPeriodData(LStockPeriodType type)
        // {
        //     auto &&dataIt = MakesureStockPeriodData(type);
        //     dataIt.clear();
        //     period_time_map.erase(type);
        //     return dataIt;
        // }

        void ResetStockPeriodData(LStockPeriodType type)
        {
            period_raw_data_map.erase(type);
            period_time_map.erase(type);
        }

        BOOL CanUpdateStockPeriodData(LStockPeriodType type)
        {
            auto &&timeIt = period_time_map.find(type);
            if (timeIt == period_time_map.end())
            {
                return -1;
            }
            auto &m_lastTime = timeIt->second;
            ClockTimePoint now = Clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - m_lastTime);
            double interval = static_cast<double>(duration.count()) / 1000.0;
            return interval > 5;
        }

        void updatePeriodRawData(LStockPeriodType type, wxString periodRawData)
        {
            period_raw_data_map[type] = periodRawData;
        }

        // void addTimelineData(wxString json_data)
        // {
        //     yyjson_doc *doc = NULL;
        //     yyjson_arr_iter iter;
        //     MarketType type = STOCK::market(this->code);

        //     switch (type)
        //     {
        //     case MarketType::MarketType_REPO:
        //         // {
        //         //     "result": {
        //         //         "status": {
        //         //             "code": 0,
        //         //             "msg": "MySql:lv1:success"
        //         //         },
        //         //         "data": [
        //         //             {
        //         //                 "m": "09:30:00",
        //         //                 "v": "477000",
        //         //                 "p": "7.19",
        //         //                 "avg_p": "7.19",
        //         //                 "tot_v": "477000"
        //         //             },
        //         //         ]
        //         //     }
        //         // }
        //     case MarketType::MarketType_BJ:
        //     case MarketType::MarketType_SI:
        //     case MarketType::MarketType_CNI:
        //     case MarketType::MarketType_DY:
        //     case MarketType::MarketType_GN:
        //     case MarketType::MarketType_HY:
        //     case MarketType::MarketType_SZ_SH:
        //         // {
        //         //     "result": {
        //         //         "status": {
        //         //             "code": 0,
        //         //             "msg": "MySql:lv1:success"
        //         //         },
        //         //         "data": [
        //         //             {
        //         //                 "m": "09:30:00",
        //         //                 "v": "5513800",
        //         //                 "p": "4.96",
        //         //                 "avg_p": "4.96",
        //         //                 "tot_v": "5513800"
        //         //             }
        //         //         ]
        //         //     }
        //         // }

        //         if (LStockPeriodTimelineData::LoadJsonIterator(type, json_data, &iter, &doc))
        //         {
        //             auto &timelineData = ResetStockPeriodData(LStockPeriodType::TIMELINE);
        //             yyjson_val *item;
        //             while ((item = yyjson_arr_iter_next(&iter)))
        //             {
        //                 wxSharedPtr<LStockPeriodDataBase> data(new LStockPeriodTimelineData());
        //                 data->HandleByData(item);
        //                 timelineData.push_back(data);
        //             }
        //         }
        //         period_time_map[LStockPeriodType::TIMELINE] = Clock::now();
        //         // LLOG_DEBUG("addTimelineData: %zu", MakesureStockPeriodData(LStockPeriodType::TIMELINE).size());
        //         yyjson_doc_free(doc);
        //         break;
        //     case MarketType::MarketType_OTC:
        //         break;
        //     case MarketType::MarketType_BTC:
        //         break;
        //     case MarketType::MarketType_US:
        //         // /*<script>location.href='//sina.com';</script>*/
        //         // var t1aapl=("09:30:00,4534779,331.859,332.4800;09:31:00,414814,331.867,331.5050;09:32:00,217974,331.886,332.8530;09:33:00,307562,331.917,332.9920;09:34:00,323419,331.977,333.4400;09:35:00,283580

        //         break;
        //     case MarketType::MarketType_HK:
        //     case MarketType::MarketType_HKAP:
        //         // {
        //         //     "result": {
        //         //         "status": {
        //         //             "code": 0
        //         //         },
        //         //         "data": [
        //         //             [
        //         //                 {
        //         //                     "date": "2026-07-17",
        //         //                     "prevclose": "70.45000",
        //         //                     "m": "09:30:00",
        //         //                     "price": "71.10000",
        //         //                     "volume": "83500",
        //         //                     "avg_p": "71.055"
        //         //                 },
        //         //                 {
        //         //                     "m": "09:31:00",
        //         //                     "price": "71.35000",
        //         //                     "volume": "122500",
        //         //                     "avg_p": "71.233"
        //         //                 },
        //         //             ]
        //         //         ]
        //         //     }
        //         // }

        //         if (LStockPeriodTimelineData::LoadJsonIterator(type, json_data, &iter, &doc))
        //         {
        //             auto &timelineData = ResetStockPeriodData(LStockPeriodType::TIMELINE);
        //             yyjson_val *item;
        //             while ((item = yyjson_arr_iter_next(&iter)))
        //             {
        //                 wxSharedPtr<LStockPeriodDataBase> data(new LStockPeriodTimelineData());
        //                 data->HandleByData(item);
        //                 timelineData.push_back(data);
        //             }
        //         }
        //         period_time_map[LStockPeriodType::TIMELINE] = Clock::now();
        //         // LLOG_DEBUG("addTimelineData: %zu", MakesureStockPeriodData(LStockPeriodType::TIMELINE).size());
        //         yyjson_doc_free(doc);
        //         break;
        //     case MarketType::MarketType_HF:
        //         // {
        //         //     "result": {
        //         //         "status": {
        //         //             "code": 0
        //         //         },
        //         //         "data": {
        //         //             "minLine_1d": [
        //         //                 [
        //         //                     "2026-07-17",
        //         //                     "3992.100",
        //         //                     "cme",
        //         //                     "",
        //         //                     "06:00",
        //         //                     "3981.260",
        //         //                     "0",
        //         //                     "0",
        //         //                     "3981.259",
        //         //                     "2026-07-17 06:00:00"
        //         //                 ],
        //         //                 [
        //         //                     "06:01",
        //         //                     "3982.021",
        //         //                     "0",
        //         //                     "0",
        //         //                     "3981.442",
        //         //                     "2026-07-17 06:01:00"
        //         //                 ],
        //         //             ]
        //         //         }
        //         //     }
        //         // }

        //         break;
        //     case MarketType::MarketType_globalbd:
        //         break;
        //     case MarketType::MarketType_LSE:
        //         break;
        //     case MarketType::MarketType_NF:
        //         break;
        //     case MarketType::MarketType_GOODS:
        //         break;
        //     case MarketType::MarketType_fund:
        //         break;
        //     case MarketType::MarketType_option_cn:
        //         break;
        //     case MarketType::MarketType_op_m:
        //         break;
        //     case MarketType::MarketType_global_index:
        //         break;
        //     case MarketType::MarketType_forex:
        //         break;
        //     case MarketType::MarketType_forex_yt:
        //         break;
        //     case MarketType::MarketType_CFF:
        //         break;
        //     case MarketType::MarketType_MSCI:
        //         break;
        //     default:
        //         break;
        //     }
        // }

    private:
        void LoadUrl(const wxString &t, const wxString &s);

        void StockObj(wxString code, wxArrayString latest_data_arr);
        void USStockObj(wxString code, wxArrayString latest_data_arr);
        void FuturesObj(wxString code, wxArrayString latest_data_arr);
        void NffuturesObj(wxString code, wxArrayString latest_data_arr);
        void DINIWObj(wxString code, wxArrayString latest_data_arr);
        void ForexObj(wxString code, wxArrayString latest_data_arr);
        void GlobalObj(wxString code, wxArrayString latest_data_arr);
        void SBstockObj(wxString code, wxArrayString latest_data_arr);
        void HkstockObj(wxString code, wxArrayString latest_data_arr);
        void GlobalBDObj(wxString code, wxArrayString latest_data_arr);
        void BitcoinObj(wxString code, wxArrayString latest_data_arr);
        void FUNDObj(wxString code, wxArrayString latest_data_arr);
        void ZNBGBObj(wxString code, wxArrayString latest_data_arr);
        void BlockIndexObj(wxString code, wxArrayString latest_data_arr);
        void UKLSEObj(wxString code, wxArrayString latest_data_arr);
        void GOODSObj(wxString code, wxArrayString latest_data_arr);
        void MSCIObj(wxString code, wxArrayString latest_data_arr);

    public:
        wxString GetDisplayMarket()
        {
            wxString item_display_code;
            if (type == "11" || type == "203" || type == "204")
            {
                // item_display_code = "istock";
                item_display_code = "A股";
            }
            else if (type == "12")
            {
                // item_display_code = "istockB";
                item_display_code = "B股";
            }
            else if (type == "202" || type == "201")
            {
                // item_display_code = "ifund";
                item_display_code = "基金";
            }
            else if (type == "31" || type == "32" || type == "33")
            {
                // item_display_code = "ihkstock";
                item_display_code = "港股";
            }
            else if (type == "41")
            {
                // item_display_code = "iusstock";
                item_display_code = "美股";
            }
            else if (type == "77" || type == "78" || type == "79" || type == "102")
            {
                // item_display_code = "iban";
                item_display_code = "理财";
            }
            else if (type == "71")
            {
                // item_display_code = "iforex";
                item_display_code = "外汇";
            }
            else if (type == "73")
            {
                // item_display_code = "sanban";
                item_display_code = "新三板";
            }
            else if (type == "81" || type == "120")
            {
                // item_display_code = "ibond";
                item_display_code = "债券";
            }
            else if (type == "85" || type == "86" || type == "87" || type == "88")
            {
                // item_display_code = "ifutures";
                item_display_code = "期货";
            }
            else if (type == "100")
            {
                // item_display_code = "izhi";
                item_display_code = "指数";
            }
            else if (type == "103")
            {
                // item_display_code = "iukstock";
                item_display_code = "英股";
            }
            else if (type == "114")
            {
                // item_display_code = "ibond";
                item_display_code = "债券";
            }
            return item_display_code;
        }
    };

    WX_DECLARE_HASH_MAP(wxString, wxSharedPtr<LStockData>, wxStringHash, wxStringEqual, StockDataMap);

    //------------------------------------------------------------
    // LStockListVM
    //------------------------------------------------------------

    class LStockListVM : public wxDataViewVirtualListModel
    {
    public:
        enum
        {
            Col_MarketText,
            Col_NameText,
            Col_DecimalsText,
            Col_CodeText,
        };

        LStockListVM() : wxDataViewVirtualListModel(0)
        {
        }
        LStockListVM(wxVector<wxSharedPtr<LStockData>> dataPtr) : wxDataViewVirtualListModel(dataPtr.size())
        {
            m_row_data = dataPtr;
        }

    public:
        wxSharedPtr<LStockData> GetRowData(const wxDataViewItem &item);
        void EraseRow(WXUINT row)
        {
            m_row_data.erase(m_row_data.begin() + row);
            RowDeleted(row);
        }
        void SetData(wxVector<wxSharedPtr<LStockData>> dataPtr)
        {
            m_row_data = dataPtr;
            Reset(dataPtr.size());
        }
        void Clear()
        {
            m_row_data.clear();
            Reset(0);
        }
        void AppendRowData(const wxSharedPtr<LStockData> data)
        {
            m_row_data.push_back(data);
            RowAppended();
        }

        virtual void GetValueByRow(wxVariant &variant, unsigned int row, unsigned int col) const wxOVERRIDE;
        virtual bool GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr &attr) const wxOVERRIDE;
        virtual bool SetValueByRow(const wxVariant &variant, unsigned int row, unsigned int col) wxOVERRIDE;

        void MoveRow(int src, int dst)
        {
            if (src == dst)
                return;
            // 内存数组移动元素（纯数据操作，无UI）
            auto temp = m_row_data[src];
            m_row_data.erase(m_row_data.begin() + src);
            m_row_data.insert(m_row_data.begin() + dst, temp);
            // 通知UI刷新视图
            RowChanged(src);
            RowChanged(dst);
        }

    private:
        wxVector<wxSharedPtr<LStockData>> m_row_data;
    };

}
