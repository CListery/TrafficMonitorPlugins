#pragma once
#include "pch.h"
#include <afxwin.h>
#include <wx/webview.h>
#include <string>
#include <wx/popupwin.h>

#ifdef DEBUG
#define DEBUG_STOCK_VIEW 1
#endif // DEBUG

namespace LStockViewer
{
    // 股票视图面板
    class LStockView : 
#ifdef DEBUG_STOCK_VIEW
        public wxFrame
#else
        public wxPopupTransientWindow
#endif // DEBUG_STOCK_VIEW

    {
    public:
        LStockView(wxWindow* parent);
        ~LStockView() override;
        // 初始化接口
        BOOL Setup(wxPoint pt, const int stock_index);
        // 资源清理
        void Clean();

#ifndef DEBUG_STOCK_VIEW
        // wxPopupTransientWindow virtual methods are all overridden to log them
        virtual void OnDismiss() wxOVERRIDE;
#endif // !DEBUG_STOCK_VIEW
    public:
        void OnMouse(wxMouseEvent &WXUNUSED(event));

    private:
        wxRect CalculateWindowPosition(wxPoint pt, const int width, const int height);
        wxString BuildDataScript(UINT bridgePort);
        wxWebView *CreateWebViewComponent(wxWindow* parent, int wxWindowStyle);

    private:
        wxWebView* m_webView;
        wxSharedPtr<STOCK::LStockData> m_stock;
        bool m_is_cleaned;               // 防止重复清理
    private:
        wxDECLARE_ABSTRACT_CLASS(LStockView);
        wxDECLARE_EVENT_TABLE();
    };

}