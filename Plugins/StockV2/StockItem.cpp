#include "pch.h"
#include "StockItem.h"
#include "StockV2.h"
#include <wx/dcgraph.h>

const wchar_t *LStockItem::GetItemName() const
{
    auto data = g_data.GetStockByIndex(index);
    if (data) {
        return data->name;
    }
    return UtilResHlp.StringRes(IDS_LOADING);
}

const wchar_t *LStockItem::GetItemId() const
{
    static std::wstring item_id;
    item_id = L"qL0KmmYi";
    item_id += std::to_wstring(index);
    return item_id.c_str();
}

const wchar_t *LStockItem::GetItemLableText() const
{
    return L"";
}

const wchar_t *LStockItem::GetItemValueText() const
{
    return L"";
}

bool LStockItem::IsCustomDraw() const
{
    return true;
}

CString LStockItem::GetDisplayContent(wxSharedPtr<STOCK::LStockData> data, bool include_name) const
{
    if (data == nullptr) {
        return L"";
    }
    wxString content;
    if (include_name)
        content = content + data->name + ": ";
    content += g_data.IsPriorityDisplayChanged() ? data->GetChangePrice() : data->GetCurrentPrice();
    content += " ";
    content += data->changeFluctuation;
    return content.wc_str();
}

int LStockItem::GetItemWidthEx(void* hDC) const
{
    CDC* pDC = CDC::FromHandle((HDC)hDC);

    auto data = g_data.GetStockByIndex(index);

    int width = pDC->GetTextExtent(GetDisplayContent(data, g_data.IsDisplayName())).cx;

    LLOG_DEBUG("GetItemWidthEx: %d", width);
    return width;
}

void LStockItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    auto data = g_data.GetStockByIndex(index);
    if (data == nullptr) {
        return;
    }

    // 绘图句柄
    CDC* pDC = CDC::FromHandle((HDC)hDC);

    // 矩形区域
    CRect rect(CPoint(x, y), CSize(w, h));

    // 文本颜色
    COLORREF color_default;
    COLORREF color_red;
    COLORREF color_green;
    if (dark_mode)
    {
        color_default = RGB(255, 255, 255);
        color_red = RGB(255, 121, 120);
        color_green = RGB(111, 215, 149);
    }
    else
    {
        color_default = RGB(0, 0, 0);
        color_red = RGB(195, 0, 0);
        color_green = RGB(46, 139, 87);
    }

    CRect rect_value{ rect };
    if (g_data.IsDisplayName())
    {
        // 绘制名称
        pDC->SetTextColor(color_default);
        CString displayContent = (data->name + ": ").wc_str();
        CRect rect_name{ rect };
        rect_name.right = rect_name.left + pDC->GetTextExtent(displayContent).cx;
        pDC->DrawText(displayContent, rect_name, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        rect_value.left = rect_name.right;
    }

    // 绘制数值
    if (g_data.IsDisplayColor())
    {
        bool isUp = data->changeFluctuation.find('+') != wxString::npos;
        if (isUp)
            pDC->SetTextColor(color_red);
        else
            pDC->SetTextColor(color_green);
    }
    else
    {
        pDC->SetTextColor(color_default);
    }

    UINT flags = DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX;
    if (g_data.IsDisplayRightAlign())
        flags |= DT_RIGHT;
    pDC->DrawText(GetDisplayContent(data, false), rect_value, flags);
}

const wchar_t *LStockItem::GetItemValueSampleText() const
{
    return L"--";
}

int LStockItem::OnMouseEvent(MouseEventType type, int x, int y, void *hWnd, int flag)
{
    CWnd *pWnd = CWnd::FromHandle((HWND)hWnd);
    LLOG_DEBUG("OnMouseEvent: %d", type);
    CPoint ptScreen = CPoint(x, y);
    switch (type)
    {
    case IPluginItem::MT_RCLICKED:
        LStockPlugin::Instance()->ShowStockViewMenu(hWnd, ptScreen, g_data.GetStockByIndex(index));
        return 1;

    case IPluginItem::MT_LCLICKED:
    {
        LStockPlugin::Instance()->ShowStockView(index);
    }
    default:
        break;
    }
    return 0;
}
