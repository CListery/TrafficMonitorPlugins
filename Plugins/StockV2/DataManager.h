#pragma once
#include "pch.h"
#include <wx/sharedptr.h>
#include "StockDef.h"

#define g_data LDataManager::Instance()

#define TASK_KEY_INIT_LOAD "INIT_STOCK_DATA"
#define TASK_KEY_STOCK_AUTO_REFRESH "TASK_AUTO_REFRESH_STOCK_DATA"
#define TASK_KEY_STOCK_TIMELINE_AUTO_REFRESH "TASK_AUTO_REFRESH_STOCK_TIMELINE_DATA"

#define STOCK_DISPLAY_ITEM_MAX 99

// 默认刷新频率
constexpr unsigned int DEFAULT_STOCK_REFRESH_FREQ = 3;
// 默认K线图宽度
constexpr unsigned int DEFAULT_KLINE_VIEW_W = 600;
// 默认K线图高度
constexpr unsigned int DEFAULT_KLINE_VIEW_H = 400;
// 最大K线图大小
constexpr unsigned int MAX_KLINE_VIEW_SIZE = 1000;
// 最小K线图大小
constexpr unsigned int MIN_KLINE_VIEW_SIZE = 100;

using namespace STOCK;

namespace {
    WX_DECLARE_HASH_MAP(wxString, size_t, wxStringHash, wxStringEqual, LCacheIndexMap);

    using StockDataPtr = wxSharedPtr<LStockData>;
    using CachePair = std::pair<wxString, StockDataPtr>;
    using CachePairPtr = wxSharedPtr<CachePair>;

    class LStockCacheManager {
    private:
        // 存储真实数据，可自由调整顺序
        wxVector<CachePairPtr> data_;
        // 字符串唯一索引：key -> data_下标
        LCacheIndexMap cache_index_;

    private:
        // 全量刷新索引（移动/删除后调用）
        void rebuild_index()
        {
            cache_index_.clear();
            for (size_t i = 0; i < data_.size(); ++i)
            {
                const auto& item = data_[i];
                cache_index_[item->first] = i;
            }
        }

    public:
        bool insert(StockDataPtr data) {
            if (data && !data->code.empty()) {
                wxString key = data->code;
                if (cache_index_.count(key)) {
                    return false;
                }
                CachePairPtr item(new CachePair(key, data));
                data_.push_back(item);
                cache_index_[key] = data_.size() - 1;
                return true;
            }
            return false;
        }

        StockDataPtr indexAt(size_t pos) {
            if (data_.size() > pos) {
                return data_[pos]->second;
            }
            return StockDataPtr(nullptr);
        }

        // 根据字符串快速查找，返回对象指针，nullptr不存在
        StockDataPtr find(const wxString& key) {
            if (key.empty()) {
                return StockDataPtr(nullptr);
            }
            auto idx_it = cache_index_.find(key);
            if (idx_it == cache_index_.end()) {
                return StockDataPtr(nullptr);
            }
            return indexAt(idx_it->second);
        }

        // 将指定key移动到目标下标pos位置
        bool move_to(const wxString& key, size_t target_pos) {
            if (key.empty()) {
                return false;
            }
            auto idx_it = cache_index_.find(key);
            if (idx_it == cache_index_.end()) {
                return false;
            }

            // 取出当前所在位置
            size_t origin_pos = idx_it->second;
            if (origin_pos == target_pos) {
                return true;
            }

            // 取出要移动的元素
            auto target_item = data_[origin_pos];
            // 删除原位置元素
            data_.erase(data_.begin() + origin_pos);
            // 插入到目标位置
            data_.insert(data_.begin() + target_pos, target_item);

            // 重建索引：erase+insert会改变所有下标，必须刷新哈希表
            rebuild_index();
            return true;
        }

        // 删除指定股票
        bool remove(const wxString& key)
        {
            if (key.empty()) {
                return false;
            }
            auto idx_it = cache_index_.find(key);
            if (idx_it == cache_index_.end())
                return false;

            size_t pos = idx_it->second;
            data_.erase(data_.begin() + pos);
            rebuild_index();
            return true;
        }

        void clear()
        {
            data_.clear();
            cache_index_.clear();
        }

        // 回调遍历所有股票
        template<typename Func>
        void ForEach(Func func)
        {
            for (auto&& item : data_)
                func(item->first, item->second);
        }

        template<typename Func>
        void ForEach(Func func) const
        {
            for (const auto&& item : data_)
                func(item->first, item->second);
        }

        // 获取当前元素总数
        size_t size() const { return data_.size(); }
    };
}

class LDataManager
{
public:
    LDataManager(const LDataManager &) = delete;
    LDataManager &operator=(const LDataManager &) = delete;

    static LDataManager &Instance();
    ~LDataManager();

public:
    const wxString GetModuleParentPath();
    const wxString GetModuleName();
    const wxArrayString GetAllCodes();
    wxVector<StockDataPtr> AllStocks();
    long RealtimeRefreshFreq() const;
    void RealtimeRefreshFreq(long freq);
    bool IsDisplayName() const;
    void IsDisplayName(bool flag);
    bool IsDisplayColor() const;
    void IsDisplayColor(bool flag);
    bool IsDisplayRightAlign() const;
    void IsDisplayRightAlign(bool flag);
    bool IsPriorityDisplayChanged() const;
    void IsPriorityDisplayChanged(bool flag);
    void KLineWH(int w, int h);
    void KLineW(int w);
    int KLineW() const;
    void KLineH(int h);
    int KLineH() const;
    std::pair<int, int> KLineWH() const;

    void InitConfig();
    void InitTasks();
    void ReLoadConfig();
    void ReLoadAllStocks();
    void SaveConfig();

    bool IsChangeStockList();
    
    void ClearAllCodes();
    void AddStock(StockDataPtr data);
    bool HandleStockRealtimeData(wxString jsonp);

    StockDataPtr GetStockByCode(const wxString& code);
    StockDataPtr GetStockByIndex(WXUINT index);
    bool DeleteStockByCode(const wxString& code);
    bool SetStockPosition(const StockDataPtr ptr, size_t pos);

    static void OnRefreshStockTimelineData(const std::string& callback_id, const std::string& data);

private:
    LDataManager();

private:
    struct StockConfgInfo
    {
        // 实时数据刷新频率，单位秒
        long realtimeRefreshFreq;
        // 所有股票数据
        wxVector<StockDataPtr> datas;
        // 是否显示股票名称
        bool isDisplayName;
        // 是否开启颜色显示
        bool isDisplayColor;
        // 是否优先显示变动
        bool isPriorityDisplayChanged;
        int klineW;
        int klineH;
    };

    static LDataManager m_instance;

    wxString m_moduleParentPath;
    wxString m_moduleName;

    StockConfgInfo m_stockConfig;
    // 数值右对齐
    bool m_isDisplayRightAlign;

    LStockCacheManager m_stock_cache;
    //wxVector<wxSharedPtr<LStockData>> m_cacheStocks;
    //StockDataMap m_cacheStockMap;
};
