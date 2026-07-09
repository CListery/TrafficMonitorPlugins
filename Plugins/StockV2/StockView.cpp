#include "pch.h"
#include "StockView.h"
#include <windows.h>
#include <wx/frame.h>
#include <sstream>
#include "StockV2.h"
#include "StockSockets.h"
#include <wx/display.h>
#include <wx/sstream.h>

namespace LStockViewer
{
    //----------------------------------------------------------------------------
    // LStockView
    //----------------------------------------------------------------------------

#ifdef DEBUG_STOCK_VIEW
    wxIMPLEMENT_CLASS(LStockView, wxFrame);
    
    wxBEGIN_EVENT_TABLE(LStockView, wxFrame)
        EVT_MOUSE_EVENTS(LStockView::OnMouse)
    wxEND_EVENT_TABLE()
#else
    wxIMPLEMENT_CLASS(LStockView, wxPopupTransientWindow);

    wxBEGIN_EVENT_TABLE(LStockView, wxPopupTransientWindow)
        EVT_MOUSE_EVENTS(LStockView::OnMouse)
    wxEND_EVENT_TABLE()

#endif

    void LStockView::OnMouse(wxMouseEvent& WXUNUSED(event))
    {
        LLOG_INFO("OnMouse");
    }

#ifdef DEBUG_STOCK_VIEW
    LStockView::LStockView(wxWindow* parent) : wxFrame(parent, wxID_ANY, "StockViewer"), m_is_cleaned(false)
#else
    LStockView::LStockView(wxWindow* parent) : wxPopupTransientWindow(parent, wxBORDER_NONE | wxPU_CONTAINS_CONTROLS), m_is_cleaned(false)
#endif
    {
    }

    LStockView::~LStockView()
    {
        Clean();
    }

#ifndef DEBUG_STOCK_VIEW
    void LStockView::OnDismiss()
    {
        LLOG_DEBUG("%p LStockView::OnDismiss", this);
        wxPopupTransientWindow::OnDismiss();
        Clean();
    }
#endif

    void LStockView::Clean()
    {
        LLOG_DEBUG("Clean");

        if (m_is_cleaned)
            return;
        m_is_cleaned = TRUE;

        LStockServerSocket& sss = LStockServerSocket::GetInstance();
        sss.StopSocketServer();
        sss.GetBridge()->UnregisterCallFunc("request_kline_data");

        m_webView->Destroy();
        //Destroy();
    }

    // 计算窗口最终位置（屏幕边界适配）
    wxRect LStockView::CalculateWindowPosition(wxPoint pt, const int width, const int height)
    {
        const wxPoint ptScreen = ClientToScreen(pt);
        int dpy = wxDisplay::GetFromPoint(ptScreen);
        if (dpy == wxNOT_FOUND) {
            return wxRect(ptScreen.x, ptScreen.y, ptScreen.x + width, ptScreen.y + height);
        }
        else {
            wxDisplay display(dpy);
            const wxRect screenRect(display.GetGeometry());

            int x = screenRect.x;
            int y = screenRect.y;

            if (x + width > screenRect.GetRight())
                x -= width;
            if (y + height > screenRect.GetBottom())
                y -= height;

            x = max(screenRect.GetLeft(), x);
            y = max(screenRect.GetTop(), y);

            return wxRect(x, y, x + width, y + height);
        }
    }

    // 创建WebView组件
    wxWebView *LStockView::CreateWebViewComponent(wxWindow* parent, int wxWindowStyle)
    {
        wxWebView *webView = nullptr;
        if (wxWebView::IsBackendAvailable(wxWebViewBackendEdge))
        {
            webView = wxWebView::New(wxWebViewBackendEdge);

            webView->Create(parent, wxID_ANY, wxWebViewDefaultURLStr, wxDefaultPosition, wxDefaultSize);

            //m_webView = wxWebView::New(parent,
            //                         wxID_ANY,
            //                         wxASCII_STR(wxWebViewDefaultURLStr),
            //                         wxDefaultPosition,
            //                         wxDefaultSize,
            //                         wxWebViewBackendEdge,
            //                         wxWindowStyle,
            //                         wxASCII_STR(wxWebViewNameStr));
        }

        if (!webView)
        {
            return nullptr;
        }

        return webView;
    }

    BOOL LStockView::Setup(wxPoint pt, const int stock_index)
    {
        m_stock = g_data.GetStockByIndex(stock_index);
        m_is_cleaned = false;

        int wxWindowStyle = wxNO_BORDER | wxTRANSPARENT_WINDOW;

        const int width = FromDIP(g_data.KLineW());  // g_data.RDPI(g_data.m_setting_data.m_kline_width);
        const int height = FromDIP(g_data.KLineH()); // g_data.RDPI(g_data.m_setting_data.m_kline_height);

        wxRect windowRect = CalculateWindowPosition(pt, width, height);

        wxLogDebug("StockView: %p Shown pos(%d, %d) size(%d, %d)", this, windowRect.GetLeft(), windowRect.GetBottom(), windowRect.width, windowRect.height);

        //wxScrolledWindow* m_panel = new wxScrolledWindow(this, wxID_ANY);
        wxPanel* m_panel = new wxPanel(this, wxID_ANY);

        // Keep this code to verify if mouse events work, they're required if
        // you're making a control like a combobox where the items are highlighted
        // under the cursor, the m_panel is set focus in the Popup() function
        m_panel->Bind(wxEVT_MOTION, &LStockView::OnMouse, this);

        m_panel->SetWindowStyle(wxWindowStyle);
        m_panel->SetBackgroundColour(*wxLIGHT_GREY);

        m_panel->SetSize(0, 0, windowRect.width, windowRect.height);

        // 创建WebView
        m_webView = CreateWebViewComponent(m_panel, wxWindowStyle);
        if (!m_webView)
        {
            LLOG_ERROR("Webview init failed!");
            Clean();
            return FALSE;
        }

        wxLogDebug("Backend: %s Version: %s", m_webView->GetClassInfo()->GetClassName(), wxWebView::GetBackendVersionInfo().ToString().wc_str());

        m_webView->SetSize(0, 0, windowRect.width, windowRect.height);

        wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
        //topSizer->Add(m_webView, 0, wxALL, 0);
        //topSizer->Add(m_webView, wxSizerFlags().Expand().Proportion(1));
        //topSizer->Add(m_webView, wxALL);
        topSizer->Add(m_webView, 1, wxEXPAND | wxALL, 0);
        //topSizer->Add(text, 0, wxALL, 5);
        m_panel->SetSizer(topSizer);
        // Use the fitting size for the panel if we don't need scrollbars.
        //topSizer->Fit(m_panel);
        SetClientSize(m_panel->GetSize());

#ifdef DEBUG
        m_webView->EnableAccessToDevTools(TRUE);
        m_webView->EnableContextMenu(TRUE);
#endif

        // m_webView->LoadURL("https://www.baidu.com");

        // 加载HTML资源 + 注入数据
        wxString htmlContent = ResHtml(IDR_HTML_STOCK_SF);
        LLOG_DEBUG("html: %d", htmlContent.length());
        // LLOG_DEBUG(htmlContent);
        if (htmlContent.empty())
        {
            LLOG_ERROR("Load HTML resource failed!");
            Clean();
            return FALSE;
        }

        LStockServerSocket &sss = LStockServerSocket::GetInstance();
        sss.StartSocketServer();
        sss.GetBridge()->RegisterCallFunc("request_kline_data", LDataManager::OnRefreshStockTimelineData);
        wxLogDebug("StockServer StartUp: %d", sss.GetWebBridgePort());

        wxString dataScript = BuildDataScript(sss.GetWebBridgePort());
        wxLogDebug(dataScript);
        htmlContent.Replace("<!-- __DATA_INJECT__ -->", dataScript);
        m_webView->SetPage(htmlContent, "about:blank");

#ifdef DEBUG_STOCK_VIEW
        this->Move(ClientToScreen(pt));
        this->Show(true);
#else
        this->Position(ClientToScreen(pt), wxSize(0, 0));
        this->Popup();
#endif

        // 窗口置顶
        this->Raise();
        this->SetFocus();
#ifdef __WXMSW__
        ::BringWindowToTop((HWND)this->GetHandle());
        ::SetForegroundWindow((HWND)this->GetHandle());
#endif

        wxLogDebug("StockView setup success, stock: %s", m_stock->code);
        return TRUE;
    }

    wxString LStockView::BuildDataScript(UINT bridgePort)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(nullptr);
        yyjson_mut_val* root = yyjson_mut_obj(doc);
        yyjson_mut_doc_set_root(doc, root);

        wxScopedCharBuffer utf8Name = m_stock->name.ToUTF8();
        yyjson_mut_obj_add_strcpy(doc, root, "title", utf8Name.data());
        yyjson_mut_obj_add_strcpy(doc, root, "code", m_stock->code.ToUTF8());

        yyjson_mut_val* bridgeObj = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_strcpy(doc, bridgeObj, "host", "127.0.0.1");
        yyjson_mut_obj_add_int(doc, bridgeObj, "port", bridgePort);
        //yyjson_mut_obj_add_strcpy(doc, bridgeObj, "token", "");
        yyjson_mut_obj_add_val(doc, root, "bridge", bridgeObj);

        size_t jsonLen = 0;
        char* jsonStr = yyjson_mut_write(doc, 0, &jsonLen);

        wxString result;
        result << R"(<script id="__WX_DATA__" type="application/json" crossorigin="anonymous">)";
        if (jsonStr && jsonLen > 0)
        {
            result += wxString::FromUTF8(jsonStr, jsonLen);
            free(jsonStr);
        }
        result << R"(</script>)";

        yyjson_mut_doc_free(doc);

        return result;
    }

}