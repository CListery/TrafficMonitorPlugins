#pragma once

#include "pch.h"
#include "PluginInterface.h"
#include "StockItem.h"
#include <vector>
#include <string>
#include <windows.h>
#include <wx/dynlib.h>

constexpr int STOCK_ITEM_MAX_COUNT = 1;

// 股票插件接口
class LStockPlugin : public ITMPlugin
{
public:
    LStockPlugin(const LStockPlugin &) = delete;
    LStockPlugin &operator=(const LStockPlugin &) = delete;

    static LStockPlugin *Instance();
    ~LStockPlugin();

public:
    void ShowStockView(const int stock_index);
    void ShowStockViewMenu(void* hWnd, CPoint ptScreen, wxSharedPtr<STOCK::LStockData> stock);
    void ShowStockWxFrame();
    void LoadDisplayItems();

private:
    LStockPlugin();

private:
    static LStockPlugin *m_pInstance;          // 单例实例
    static std::recursive_mutex m_mtxInstance; // 单例线程锁

    std::vector<LStockItem> m_displayStocks;

public:
    /**
     * @brief   获取插件显示项目的对象
     * @detail  一个插件dll可以提供多个实现IPluginItem接口的对象，对应多个显示项目。
     *  当index的值大于或等于0且小于IPluginItem接口的对象的个数时，返回对象的IPluginItem接口的指针，其他情况应该返回空指针。
     *  例如插件提供两个显示项目，则当index等于0或1时返回对应IPluginItem接口的对象，其他值时必须返回空指针。
     * @param   int index 对象的索引
     * @return  IPluginItem* 插件显示项目的对象
     */
    virtual IPluginItem *GetItem(int index) override;

    /**
     * @brief   主程序会每隔一定时间调用此函数，插件需要在函数里获取一次监控的数据
     */
    virtual void DataRequired() override;

    /**
     * @brief   获取此插件的信息，根据index的值返回对应的信息
     */
    virtual const wchar_t *GetInfo(PluginInfoIndex index) override;

    /**
     * @brief   主程序调用此函数以打开插件的选项设置对话框
     * @detail  此函数不一定要重写。如果插件提供了选项设置界面，则应该重写此函数，并在最后返回OR_OPTION_CHANGED或OR_OPTION_UNCHANGED。
     * @param   void * hParent 父窗口的句柄
     *  返回值为OR_OPTION_NOT_PRVIDED则认为插件不提供选项设置对话框。
     * @return  ITMPlugin::OptionReturn
     */
    virtual OptionReturn ShowOptionsDialog(void *hParent) override;

    // /**
    //  * @brief   主程序调用此函数以向插件传递所有获取到的监控信息
    //  */
    // virtual void OnMonitorInfo(const MonitorInfo& monitor_info) override;

    // /**
    //  * @brief   获取插件要在鼠标提示中显示的文本
    //  */
    // virtual const wchar_t* GetTooltipInfo() override;

    /**
     * @brief   主程序调用此函数以向插件传递更多信息
     * @param   ExtendedInfoIndex index 信息的索引，用于区分向插件传递的信息
     * @param   const wchar_t* data 传递的数据
     * @return  void
     */
    virtual void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t *data) override;

    /**
     * @brief   获取插件的图标，HICON格式
     */
    virtual void *GetPluginIcon() override;

    /**
     * @brief   获取插件的命令的数量
     * @return  int 插件的命令的数量
     */
    virtual int GetCommandCount() override;

    /**
     * @brief   获取插件的命令名称
     * @param   int command_index 插件命令的索引（从0开始，小于GetCommandCount()的返回值）
     * @return  wchar_t* 插件命令的名称
     */
    virtual const wchar_t *GetCommandName(int command_index) override;

    // /**
    //  * @brief   获取插件的命令图标
    //  * @param   int command_index 插件命令的索引（从0开始，小于GetCommandCount()的返回值）
    //  * @return  void* 插件命令的图标，HICON格式
    //  */
    // virtual void* GetCommandIcon(int command_index) override;

    /**
     * @brief   执行一个插件命令时由框架调用
     * @param   int command_index 插件命令的索引（从0开始，小于GetCommandCount()的返回值）
     * @param   void* hWnd 发送此命令的窗口句柄
     * @param   void* para 预留参数
     */
    virtual void OnPluginCommand(int command_index, void *hWnd, void *para) override;

    // /**
    // * @brief   获取插件命令是否处于勾选状态
    // * @param   int command_index 插件命令的索引（从0开始，小于GetCommandCount()的返回值）
    // * @return  1：已勾选；0：未勾选
    // */
    // virtual int IsCommandChecked(int command_index) override;

    /**
     * @brief   插件初始化
     * @detail  当插件被加载时被调用，传递ITrafficMonitor接口的指针。插件可以保存此指针以调用ITrafficMonitor接口中的函数
     * @param   pApp
     */
    virtual void OnInitialize(ITrafficMonitor *pApp) override;
};
