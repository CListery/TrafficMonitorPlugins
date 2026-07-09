#include "pch.h"
#include "StockSearchView.h"
#include "pch.h"
#include <wx/wx.h>
#include <wx/listbox.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/valtext.h>
#include <wx/menu.h>
#include <wx/combobox.h>
#include <wx/filedlg.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/clrpicker.h>
#include <wx/srchctrl.h>
#include <wx/webrequest.h>
#include <wx/dataview.h>
#include <wx/tokenzr.h>

// control ids
enum
{
    LStockSearchView_StockListView = wxID_HIGHEST,
};

LStockSearchView::LStockSearchView(wxWindow *parent) : wxDialog(parent, wxID_ANY, wxString("搜索"), wxDefaultPosition, wxSize(600, 500))
{
    SetIcon(ResIcon(IDI_STOCK));

    InitUI();
    BindAllEvents();
    Center();

    m_selectStock = nullptr;

#ifdef DEBUG
    wxString test_content{"var suggest=\"btc,103,btc,btc,BTC,,BTC,99,1,,,;btc,86,btc,btc,比特币期货,,比特币期货,99,1,,,;博泰车联,31,02889,02889,博泰车联,,博泰车联,99,1,,,;比特策略,31,06113,06113,比特策略,,比特策略,99,1,,,;BTC,41,btc,btc,Community Bankers Trust Corp,,Community Bankers Trust Corp,99,0,,,;btc1,103,btc1,btc1,BTC1,,BTC1,99,1,,,;btcf,103,btcf,btcf,BTCF,,BTCF,99,1,,,;btc3,103,btc3,btc3,BTC3,,BTC3,99,1,,,;btcu,103,btcu,btcu,BTCU,,BTCU,99,1,,,;btcw,103,btcw,btcw,BTCW,,BTCW,99,1,,,;btce,103,btce,btce,BTCE,,BTCE,99,1,,,;BTCUSD,71,btcusd,btcusd,比特币美元,,比特币美元,99,1,,,;万邦特材,73,836779,sb836779,万邦特材,,万邦特材,99,1,,,;博通,41,avgo,avgo,博通,,博通,99,1,ESG,,;BTCTW,41,btctw,btctw,BTC Digital Ltd.,,BTC Digital Ltd.,99,1,,,;btc2606,86,btc2606,btc2606,比特币期货2606,,比特币期货2606,99,1,,,;白糖,87,sr0,sr0,白糖,,白糖,99,1,,,;本田汽车,41,hmc,hmc,本田汽车,,本田汽车,99,1,ESG,,;btc2608,86,btc2608,btc2608,比特币期货2608,,比特币期货2608,99,1,,,;btc2605,86,btc2605,btc2605,比特币期货2605,,比特币期货2605,99,1,,,;贝特瑞,11,920185,bj920185,贝特瑞,,贝特瑞,99,1,,,;伯特利,11,603596,sh603596,伯特利,,伯特利,99,1,ESG,,;宝泰隆,11,601011,sh601011,宝泰隆,,宝泰隆,99,1,,,;BTCT,41,btct,btct,BTC Digital Ltd.,,BTC Digital Ltd.,99,1,,,;BTCS,41,btcs,btcs,BTCS Inc.,,BTCS Inc.,99,1,,,;BTCSW,41,btcsw,btcsw,BTCSW,,BTCSW,99,1,,,;btc2607,86,btc2607,btc2607,比特币期货2607,,比特币期货2607,99,1,,,;BTCBTCDOLLAR,71,btcbtcdollar,btcbtcdollar,比特币兑美元,,比特币兑美元,99,1,,,;团车,41,tc,tc,团车,,团车,99,1,,,;BTCO,41,btco,btco,Invesco Galaxy Bitcoin ETF,,Invesco Galaxy Bitcoin ETF,99,1,,,;BTCL,41,btcl,btcl,T-Rex 2X Long Bitcoin Daily Target ETF,,T-Rex 2X Long Bitcoin Daily Target ETF,99,1,,,;BTC Digital Ltd.,41,metxw,metxw,BTC Digital Ltd.,,BTC Digital Ltd.,99,1,,,;BTCK,41,btck,btck,Nexo 7RCC Spot Bitcoin & Carbon Credit Futures ETF,,Nexo 7RCC Spot Bitcoin & Carbon Credit Futures ETF,99,1,,,;BTC Development Corp.,41,bdci,bdci,BTC Development Corp.,,BTC Development Corp.,99,1,,,;BTCR,41,btcr,btcr,Volt Crypto Industry Revolution & Tech ETF,,Volt Crypto Industry Revolution & Tech ETF,99,1,,,;BTCC,41,btcc,btcc,Grayscale Bitcoin Covered Call ETF,,Grayscale Bitcoin Covered Call ETF,99,1,,,;BTCI,41,btci,btci,Neos Bitcoin High Income ETF,,Neos Bitcoin High Income ETF,99,1,,,;BTCWF,41,btcwf,btcwf,Bluesky Digital Assets Corp.,,Bluesky Digital Assets Corp.,99,1,,,;BTCW,41,btcw,btcw,Wisdomtree Bitcoin Fund,,Wisdomtree Bitcoin Fund,99,1,,,;BTC Development Corp.,41,bdciw,bdciw,BTC Development Corp.,,BTC Development Corp.,99,1,,,;BTC Development Corp.,41,bdciu,bdciu,BTC Development Corp.,,BTC Development Corp.,99,1,,,;Bitwise Trendwise BTC/ETH & Treasuries Rotation Strategy,41,btop,btop,Bitwise Trendwise BTC/ETH & Treasuries Rotation Strategy,,Bitwise Trendwise BTC/ETH & Treasuries Rotation Strategy,99,1,,,;21Shares FTSE Crypto 10 ex-BTC Index ETF,41,txbc,txbc,21Shares FTSE Crypto 10 ex-BTC Index ETF,,21Shares FTSE Crypto 10 ex-BTC Index ETF,99,1,,,;BTCZ,41,btcz,btcz,T-Rex 2X Inverse Bitcoin Daily Target ETF,,T-Rex 2X Inverse Bitcoin Daily Target ETF,99,1,,,;BTCY,41,btcy,btcy,Biotricity Inc.,,Biotricity Inc.,99,1,,,;BTCM,41,btcm,btcm,比特矿业,,比特矿业,99,1,,,;贝泰妮,11,300957,sz300957,贝泰妮,,贝泰妮,99,1,ESG,,;博通集成,11,603068,sh603068,博通集成,,博通集成,99,1,,,;保诚,31,02378,02378,保诚,,保诚,99,1,ESG,,;美联国际教育集团,41,metx,metx,美联国际教育集团,,美联国际教育集团,99,1,,,\";"};
    HandleSearchResult(test_content);
#endif // DEBUG
}

LStockSearchView::~LStockSearchView()
{
    // We have to block until the web request completes, but we need to
    // process events while doing it.
    Hide();

    while (m_currentRequest.IsOk())
    {
        wxYield();
    }
}

void LStockSearchView::InitUI()
{
    // 输入区
    m_stockSearchCtrl = new wxSearchCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBORDER_DEFAULT);
    m_stockSearchCtrl->ShowSearchButton(false);
    m_stockSearchCtrl->ShowCancelButton(true);
    m_stockSearchCtrl->SetHint("输入交易代码按回车进行搜索");
    m_stockSearchCtrl->SetToolTip("输入交易代码按回车进行搜索");

    // 股票列表
    m_stockSearchResultCtrl = new wxDataViewCtrl(this, LStockSearchView_StockListView, wxDefaultPosition,
                                                 wxDefaultSize, wxDV_VARIABLE_LINE_HEIGHT | wxDV_ROW_LINES | wxDV_SINGLE);
    m_search_datas = wxVector<wxSharedPtr<STOCK::LStockData>>();
    m_ssrVM = new STOCK::LStockListVM(m_search_datas);
    m_stockSearchResultCtrl->AssociateModel(m_ssrVM.get());

    const int alignment = wxALIGN_LEFT & wxALIGN_MASK;

    wxDataViewColumn* const colMarket = m_stockSearchResultCtrl->AppendTextColumn(
        "市场",
        STOCK::LStockListVM::Col_MarketText,
        wxDATAVIEW_CELL_INERT,
        wxCOL_WIDTH_AUTOSIZE,
        wxALIGN_NOT,
        wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);
    colMarket->GetRenderer()->SetAlignment(alignment);
    wxDataViewColumn *const colName = m_stockSearchResultCtrl->AppendTextColumn(
        "交易名称",
        STOCK::LStockListVM::Col_NameText,
        wxDATAVIEW_CELL_INERT,
        wxCOL_WIDTH_AUTOSIZE,
        wxALIGN_NOT,
        wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);
    colName->GetRenderer()->SetAlignment(alignment);
    wxDataViewColumn *const colCode = m_stockSearchResultCtrl->AppendTextColumn(
        "交易代码",
        STOCK::LStockListVM::Col_CodeText,
        wxDATAVIEW_CELL_INERT,
        wxCOL_WIDTH_AUTOSIZE,
        wxALIGN_NOT,
        wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);
    colCode->GetRenderer()->SetAlignment(alignment);

    BuildMainLayout();
}

void LStockSearchView::BuildMainLayout()
{
    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(mainSizer);

    // 股票输入行
    wxBoxSizer *inputSizer = new wxBoxSizer(wxHORIZONTAL);
    inputSizer->Add(new wxStaticText(this, wxID_ANY, "交易代码: "), 0, wxALIGN_CENTER | wxRIGHT, 5);
    inputSizer->Add(m_stockSearchCtrl, 1, wxRIGHT, 8);
    mainSizer->Add(inputSizer, 0, wxALL | wxEXPAND, 10);

    // 股票列表
    mainSizer->Add(m_stockSearchResultCtrl, 1, wxGROW | wxALL, 5);
    // mainSizer->Add(m_stockList, 0, wxLEFT | wxRIGHT | wxEXPAND, 5);

    // 底部按钮
    mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 15);
}

void LStockSearchView::BindAllEvents()
{
    Bind(wxEVT_CLOSE_WINDOW, &LStockSearchView::OnClose, this);

    // 列表右键菜单
    // m_stockList->Bind(wxEVT_CONTEXT_MENU, &LStockSearchView::OnListRightMenu, this);
}

void LStockSearchView::RequestStockByKeyword(const wxString &keyword)
{
    if (keyword.empty())
    {
        return;
    }

    if (m_currentRequest.IsOk())
    {
        m_currentRequest.Cancel();
    }

    static const wxString url_fmt = "https://suggest3.sinajs.cn/suggest/?key=%s&type=11,41,31,201,202,203,204,71,85,86,87,88,102,12,33,32,73,81,100,120,103,114&name=suggest&num=50";
    const wxString search_url = wxString::Format(url_fmt, keyword);

    m_currentRequest = wxWebSession::GetDefault().CreateRequest(this, search_url);

    // Bind event for state change
    Bind(wxEVT_WEBREQUEST_STATE, &LStockSearchView::OnWebRequestState, this);

    m_currentRequest.SetHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/135.0.0.0 Safari/537.36 Edg/135.0.0.0");
    m_currentRequest.SetHeader("Referer", "https://finance.sina.com.cn");

    m_selectStock = nullptr;
    // Start the request (events will be sent on success or failure)
    m_currentRequest.Start();
}

void LStockSearchView::HandleSearchResult(const wxString &jsonp)
{
    wxLogDebug("HandleSearchResult: %s", jsonp);

    if (jsonp.empty())
    {
        return;
    }

    m_ssrVM->Clear();

    wxString content = jsonp;
    content.Replace("var suggest=", "");
    content.Replace('\"', "");

    for (wxString line : CommonUtils::StringHelper::split(content, ";"))
    {
        if (line.empty())
        {
            continue;
        }

        wxSharedPtr<STOCK::LStockData> ptr(new STOCK::LStockData());
        ptr->LoadBySearchData(line);
        m_ssrVM->AppendRowData(ptr);
    }
}

void LStockSearchView::OnWebRequestState(wxWebRequestEvent &evt)
{

    bool stillActive = false;

    switch (evt.GetState())
    {
    case wxWebRequest::State_Completed:
    {
        wxLogDebug("Completed");
        wxWebResponse resp = evt.GetResponse();
        wxString content = resp.AsString();
        HandleSearchResult(content);
        break;
    }

    case wxWebRequest::State_Failed:
        wxLogDebug("Web Request failed: %s", evt.GetErrorDescription());
        break;

    case wxWebRequest::State_Cancelled:
        wxLogDebug("Cancelled");
        break;

    case wxWebRequest::State_Unauthorized:
        wxLogDebug("Unauthorized");
        break;

    case wxWebRequest::State_Active:
        stillActive = true;
        wxLogDebug("Active");
        break;

    case wxWebRequest::State_Idle:
        // Nothing special to do for this state.
        break;
    }
    if (!stillActive)
    {
        m_currentRequest = wxWebRequest();
    }
}

void LStockSearchView::OnSaveConfig(wxCommandEvent &event)
{
    wxLogDebug("OnSaveConfig");
    SetEscapeId(wxID_OK);
    Close();
}

void LStockSearchView::OnText(wxCommandEvent &event)
{
    wxLogDebug("Search control: text changes, contents is \"%s\".",
               event.GetString());
}

void LStockSearchView::OnTextEnter(wxCommandEvent &event)
{
    wxLogDebug("Search control: enter pressed, contents is \"%s\".",
               event.GetString());
    RequestStockByKeyword(event.GetString());
}

void LStockSearchView::OnSearch(wxCommandEvent &event)
{
    wxLogDebug("Search button: search for \"%s\".", event.GetString());
    RequestStockByKeyword(event.GetString());
}

void LStockSearchView::OnSearchCancel(wxCommandEvent &event)
{
    wxLogDebug("Cancel button pressed.");
    if (m_currentRequest.IsOk())
    {
        m_currentRequest.Cancel();
    }
    event.Skip();
}

void LStockSearchView::OnClose(wxCloseEvent &event)
{
    if (m_currentRequest.IsOk())
    {
        m_currentRequest.Cancel();
    }
    event.Skip();
}

void LStockSearchView::OnSelectItem(wxDataViewEvent &event)
{
    auto rowData = m_ssrVM->GetRowData(event.GetItem());

    if (rowData)
    {
        m_selectStock = rowData;
        wxLogDebug("OnSelectItem: %s", rowData->name);
    }
}

void LStockSearchView::OnItemActivated(wxDataViewEvent &event)
{
    auto rowData = m_ssrVM->GetRowData(event.GetItem());

    if (rowData)
    {
        m_selectStock = rowData;
        wxLogDebug("OnItemActivated: %s", rowData->name);
        SetEscapeId(wxID_OK);
        Close();
    }
}

wxBEGIN_EVENT_TABLE(LStockSearchView, wxDialog)
    //
    EVT_TEXT(wxID_ANY, LStockSearchView::OnText)
    //
    EVT_TEXT_ENTER(wxID_ANY, LStockSearchView::OnTextEnter)
    //
    EVT_SEARCH(wxID_ANY, LStockSearchView::OnSearch)
    //
    EVT_SEARCH_CANCEL(wxID_ANY, LStockSearchView::OnSearchCancel)
    //
    EVT_DATAVIEW_ITEM_ACTIVATED(LStockSearchView_StockListView, LStockSearchView::OnItemActivated)
    //
    EVT_DATAVIEW_SELECTION_CHANGED(LStockSearchView_StockListView, LStockSearchView::OnSelectItem)
    //
    wxEND_EVENT_TABLE()
