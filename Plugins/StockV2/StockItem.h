#pragma once
#include <PluginInterface.h>
class LStockItem : public IPluginItem
{
public:
    int index;
    bool enable;

private:
    CString GetDisplayContent(wxSharedPtr<STOCK::LStockData> data, bool include_name) const;

public:
    /**
     * @brief   获取显示项目的名称
     * @return  const wchar_t*
     */
    virtual const wchar_t* GetItemName() const override;

    /**
     * @brief   获取显示项目的唯一ID
     * @return  const wchar_t*
     */
    virtual const wchar_t* GetItemId() const override;

    /**
     * @brief   获取项目标签的文本
     * @return  const wchar_t*
     */
    virtual const wchar_t* GetItemLableText() const override;

    /**
     * @brief   获取项目数值的文本
     * @detail  由于此函数可能会被频繁调用，因此不要在这里获取监控数据，
     *  而是在ITMPlugin::DataRequired函数中获取数据后保存起来，然后在这里返回获取的数值
     * @return  const wchar_t*
     */
    virtual const wchar_t* GetItemValueText() const override;

    /**
     * @brief   获取项目数值的示例文本
     * @detail  此函数返回的字符串的长度会用于计算显示区域的宽度
     * @return  const wchar_t*
     */
    virtual const wchar_t* GetItemValueSampleText() const override;

     /**
      * @brief   显示区域是否由插件自行绘制
      * @detail
      *  如果返回false，则根据GetItemLableText和GetItemValueText返回的文本由主程序绘制显示区域，重写DrawItem函数将不起作用。
      *  如果重写此函数并返回true，则必须重写DrawItem函数并在里面添加绘制显示区域的代码，
      *  此时GetItemLableText、GetItemValueText和GetItemValueSampleText的返回值将被主程序忽略
      * @return  bool
      */
     virtual bool IsCustomDraw() const override;

     ///**
     // * @brief   获取显示区域的宽度
     // * @detail
     // *  只有当CustomDraw()函数返回true时重写此函数才有效。
     // *  返回的值为DPI为96（100%）时的宽度，主程序会根据当前系统DPI的设置自动按比例放大，
     // *  因此你不需要为不同的DPI设置返回不同的值。
     // *  需要注意的是，这里的返回值代表了自绘区域所需要的最小宽度，DrawItem函数中的参数w的值可能会大于这个值
     // * @return  int
     // */
     //virtual int GetItemWidth() const override;

     /**
      * @brief   自定义绘制显示区域的函数，只有当CustomDraw()函数返回true时重写此函数才有效
      * @param   void * hDC 绘图的上下文句柄
      * @param   int x 绘图的矩形区域
      * @param   int y
      * @param   int w
      * @param   int h
      * @param   bool dark_mode 深色模式为true，浅色模式为false
      * @return  void
      */
     virtual void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override;

     /**
      * @brief   获取显示区域的宽度
      * @detail
      *  只有当CustomDraw()函数返回true时重写此函数才有效。
      *  此函数和GetItemWidth不同，插件可以根据参数hDC来计算需要的宽度，
      *  它返回的是实际的宽度，主程序不会根据当前系统的DPI对返回值进行放大。
      *  需要注意的是，这里的返回值代表了自绘区域所需要的最小宽度，DrawItem函数中的参数w的值可能会大于这个值
      * @param   void * hDC 绘图的上下文句柄
      * @return  int
      */
     virtual int GetItemWidthEx(void* hDC) const override;

    /**
     * @brief   当插件显示区域有鼠标事件时由主程序调用
     * @param   MouseEventType type 鼠标事件的类型
     * @param   int x 鼠标指针所在的x坐标
     * @param   int y 鼠标指针所在的y坐标
     * @param   void* hWnd 产生此鼠标事件的窗口的句柄（主窗口或任务栏窗口）
     * @param   int flag 为若干MouseEventFlag枚举常量的组合
     * @return  int 如果返回非0，则主程序认为插件已经对此鼠标事件作出了全部的响应，主程序将不会再对此鼠标事件做额外的响应。
     *   例如当type为MT_RCLICKED时，如果程序返回0，则会弹出主程序提供的右键菜单；而返回非0时，主程序不会再做任何处理。
     */
    virtual int OnMouseEvent(MouseEventType type, int x, int y, void* hWnd, int flag) override;

    // /**
    //  * @brief   当插件显示区域有键盘事件时由主程序调用
    //  * @param   int key 按下的键
    //  * @param   bool ctrl 是否按下了Ctrl键
    //  * @param   bool shift 是否按下了Shift键
    //  * @param   bool alt 是否按下了Alt键
    //  * @param   void* hWnd 产生此键盘事件的窗口的句柄（主窗口或任务栏窗口）
    //  * @param   int flag 为若干KeyboardEventFlag枚举常量的组合
    //  * @return  int 如果返回非0，则主程序认为插件已经对此键盘事件作出了全部的响应，主程序将不会再对此键盘事件做额外的响应。
    //  */
    // virtual int OnKeboardEvent(int key, bool ctrl, bool shift, bool alt, void* hWnd, int flag) override;

    // //预留的接口
    // virtual void* OnItemInfo(ItemInfoType, void* para1, void* para2) override;

    // /**
    //  * @brief 是否在在任务栏中显示此项目的资源占用图
    //  * @return 1：显示，0：不显示
    //  */
    // virtual int IsDrawResourceUsageGraph() const override;

    // /**
    //  * @brief 获取资源占用图的值。当IsDrawResourceUsageGraphType返回值不为0时有效
    //  * @return float 资源占用图的值，范围为0.0~1.0。
    //  */
    // virtual float GetResourceUsageGraphValue() const override;
};

