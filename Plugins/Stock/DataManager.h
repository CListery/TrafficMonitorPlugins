#pragma once
#include <string>
#include <map>
#include "resource.h"
#include "StockDef.h"

using namespace STOCK;

#define g_data CDataManager::Instance()


struct SettingData
{
    vector<std::wstring> m_stock_codes; // 代码
    bool m_full_day{}; // 全天更新
    bool m_show_stock_name{}; // 显示股票名称
    bool m_color_with_price{}; // 涨跌颜色标识
};

// Stock显示数据
// struct StockInfo
// {
//     std::wstring pc = L"--%";
//     std::wstring p = L"--";
//     std::wstring name = L"";
//     std::wstring ToString(bool include_name = true) const;
//     bool IsEmpty() const;
// };

class CDataManager
{
private:
    CDataManager();
    ~CDataManager();

public:
    static CDataManager& Instance();

    void LoadConfig(const std::wstring& config_dir);
    void SaveConfig();
    const CString& StringRes(UINT id);      //根据资源id获取一个字符串资源
    void DPIFromWindow(CWnd* pWnd);
    int DPI(int pixel);
    float DPIF(float pixel);
    int RDPI(int pixel);
    HICON GetIcon(UINT id);
    void ResetText();
    std::shared_ptr<StockData> GetStockData(const std::wstring& code);

    // 获取最新数据
    void RequestRealtimeData();
    void RequestTimelineData();

    SettingData m_setting_data;
    std::wstring m_log_path;
    bool m_right_align{};       //数值是否右对齐

private:
    // std::map<std::wstring, StockInfo> m_stock_info_map;
    STOCK::StockMarket stockMarket;

    std::wstring m_config_dir;

    static CDataManager m_instance;
    std::wstring m_config_path;
    std::map<UINT, CString> m_string_table;
    std::map<UINT, HICON> m_icons;
    int m_dpi{ 96 };
};
