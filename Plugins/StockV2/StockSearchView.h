#pragma once

#include "pch.h"
#include <wx/srchctrl.h>
#include <wx/webrequest.h>
#include <wx/dataview.h>
#include "StockDef.h"
#include <wx/sharedptr.h>

class LStockSearchView : public wxDialog
{
public:
    LStockSearchView(wxWindow *parent);
    ~LStockSearchView();

public:
    wxSharedPtr<STOCK::LStockData> GetSelectStocks() {
        return m_selectStock;
    }

protected:
    void OnText(wxCommandEvent &event);
    void OnTextEnter(wxCommandEvent &event);

    void OnSaveConfig(wxCommandEvent &event);

    void OnSearch(wxCommandEvent &event);
    void OnSearchCancel(wxCommandEvent &event);

    void OnClose(wxCloseEvent &event);

private:
    void InitUI();
    void BuildMainLayout();
    void BindAllEvents();

private:
    void RequestStockByKeyword(const wxString &keyword);
    void OnWebRequestState(wxWebRequestEvent &evt);
    void HandleSearchResult(const wxString &jsonp);

    void OnSelectItem(wxDataViewEvent &event);
    void OnItemActivated(wxDataViewEvent &event);
private:
    // 控件
    wxSearchCtrl *m_stockSearchCtrl;

    wxDataViewCtrl *m_stockSearchResultCtrl;
    wxObjectDataPtr<STOCK::LStockListVM> m_ssrVM;

    wxSharedPtr<STOCK::LStockData> m_selectStock;

    wxVector<wxSharedPtr<STOCK::LStockData>> m_search_datas;

private:
    wxWebRequest m_currentRequest;

private:
    wxDECLARE_EVENT_TABLE();
};
