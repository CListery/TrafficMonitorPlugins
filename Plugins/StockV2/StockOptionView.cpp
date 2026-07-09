#include "pch.h"
#include "StockOptionView.h"
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
#include "StockSearchView.h"
#include <wx/wrapsizer.h>
#include "StockV2.h"

// 菜单ID
enum MenuID
{
    ID_MENU_EDIT = wxID_HIGHEST + 1,
    ID_MENU_DEL,
    ID_MENU_TOP,
    ID_MENU_IMPORT,
    ID_MENU_EXPORT
};

// control ids
enum
{
    LStockOptionView_StockListView = wxID_HIGHEST,
};

LStockOptionView::LStockOptionView(const wxString &title, const wxPoint &pos, const wxSize &size, const int& close_evt_id)
    : wxFrame(NULL, wxID_ANY, title, pos, size,
              (wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxRESIZE_BORDER | wxSYSTEM_MENU | wxCAPTION |
               wxTAB_TRAVERSAL | wxCLOSE_BOX))
{
    m_close_evt_id = close_evt_id;
    SetIcon(ResIcon(IDI_STOCK));

    InitUI();
    BindAllEvents();
    Center();
    // 窗口置顶
    this->Raise();
    this->SetFocus();
#ifdef __WXMSW__
    ::BringWindowToTop((HWND)this->GetHandle());
    ::SetForegroundWindow((HWND)this->GetHandle());
#endif

    LoadOptions();
}

LStockOptionView::~LStockOptionView()
{
}

void LStockOptionView::InitUI()
{
    wxPanel *rootPanel = new wxPanel(this, wxID_ANY);
    wxBoxSizer *rootSizer = new wxBoxSizer(wxVERTICAL);

    rootSizer->AddSpacer(10);

    wxSizer *sizer;

    // 股票列表
    wxStaticBox *stockListBox = new wxStaticBox(rootPanel, wxID_ANY, "股票列表");
    wxSizer *stockBoxSizer = new wxStaticBoxSizer(stockListBox, wxVERTICAL);

    // 股票列表
    m_stockListCtrl = new wxDataViewCtrl(rootPanel, LStockOptionView_StockListView, wxDefaultPosition,
                                         wxDefaultSize, wxDV_VARIABLE_LINE_HEIGHT | wxDV_ROW_LINES | wxDV_SINGLE);
    m_solVM = new STOCK::LStockListVM();
    m_stockListCtrl->AssociateModel(m_solVM.get());
    m_stockListCtrl->SetMinSize(wxSize(-1, GetSize().GetHeight() * 0.5));

    const int alignment = wxALIGN_LEFT & wxALIGN_MASK;
    const int col_flag = wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE /*| wxDATAVIEW_COL_SORTABLE*/;
    wxDataViewColumn *const colMarket = m_stockListCtrl->AppendTextColumn(
        "市场",
        STOCK::LStockListVM::Col_MarketText,
        wxDATAVIEW_CELL_INERT,
        wxCOL_WIDTH_AUTOSIZE,
        wxALIGN_NOT,
        col_flag);
    colMarket->GetRenderer()->SetAlignment(alignment);
    wxDataViewColumn *const colName = m_stockListCtrl->AppendTextColumn(
        "交易名称",
        STOCK::LStockListVM::Col_NameText,
        wxDATAVIEW_CELL_INERT,
        wxCOL_WIDTH_AUTOSIZE,
        wxALIGN_NOT,
        col_flag);
    colName->GetRenderer()->SetAlignment(alignment);
    wxDataViewColumn *const colCode = m_stockListCtrl->AppendTextColumn(
        "交易代码",
        STOCK::LStockListVM::Col_CodeText,
        wxDATAVIEW_CELL_INERT,
        wxCOL_WIDTH_AUTOSIZE,
        wxALIGN_NOT,
        col_flag);
    colCode->GetRenderer()->SetAlignment(alignment);

    stockBoxSizer->Add(m_stockListCtrl, 1, wxEXPAND);

    stockBoxSizer->AddSpacer(2);

    m_btnAdd = new wxButton(rootPanel, wxID_ANY, "添加");
    m_btnRemove = new wxButton(rootPanel, wxID_ANY, "删除");
    m_btnClean = new wxButton(rootPanel, wxID_ANY, "清空");
    m_btnMoveUP = new wxButton(rootPanel, wxID_ANY, "上移");
    m_btnMoveDown = new wxButton(rootPanel, wxID_ANY, "下移");
    m_btnMoveTop = new wxButton(rootPanel, wxID_ANY, "置顶");

    sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(m_btnAdd, 0, wxALIGN_CENTER_VERTICAL | wxALL);
    sizer->AddStretchSpacer();
    sizer->Add(m_btnRemove, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    sizer->AddStretchSpacer();
    sizer->Add(m_btnClean, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    sizer->AddStretchSpacer();
    sizer->Add(m_btnMoveUP, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    sizer->AddStretchSpacer();
    sizer->Add(m_btnMoveDown, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    sizer->AddStretchSpacer();
    sizer->Add(m_btnMoveTop, 0, wxALIGN_CENTER_VERTICAL | wxALL);
    stockBoxSizer->Add(sizer, 0, wxALL | wxEXPAND);

    rootSizer->Add(stockBoxSizer, 1, wxLEFT | wxRIGHT | wxEXPAND, 10);

    rootSizer->AddSpacer(10);

    wxStaticBox *settingsBox = new wxStaticBox(rootPanel, wxID_ANY, "设置");
    wxSizer *settingsBoxSizer = new wxStaticBoxSizer(settingsBox, wxVERTICAL);

    // 刷新频率
    sizer = new wxBoxSizer(wxHORIZONTAL);
    // m_autoUpdateCheck = new wxCheckBox(rootPanel, wxID_ANY, "交易时段全天刷新");
    // sizer->Add(m_autoUpdateCheck, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 0);
    // sizer->AddSpacer(5);
    sizer->Add(new wxStaticText(rootPanel, wxID_ANY, "刷新间隔:"), 0, wxALIGN_CENTER, 0);
    m_cmbRefreshFreq = new wxComboBox(rootPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, COMB_FREQ_OPTS, wxCB_READONLY);
    m_cmbRefreshFreq->SetMinSize(wxSize(50, -1));
    sizer->Add(m_cmbRefreshFreq, 0, wxLEFT | wxRIGHT | wxEXPAND, 5);
    sizer->Add(new wxStaticText(rootPanel, wxID_ANY, "秒"), 0, wxALIGN_CENTER, 0);
    settingsBoxSizer->Add(sizer, 0);

    settingsBoxSizer->AddSpacer(8);

    // 走势图尺寸
    sizer = new wxBoxSizer(wxHORIZONTAL);
    int flags = wxSP_VERTICAL | wxSP_ARROW_KEYS | wxSP_WRAP;
    int textFlags = wxALIGN_LEFT;
    m_KLineViewWidthSpinCtrl = new wxSpinCtrl(rootPanel, wxID_ANY,
                                              wxString::Format("%d", DEFAULT_KLINE_VIEW_W),
                                              wxDefaultPosition, wxDefaultSize,
                                              flags | textFlags,
                                              MIN_KLINE_VIEW_SIZE, MAX_KLINE_VIEW_SIZE, DEFAULT_KLINE_VIEW_W);
    m_KLineViewHeightSpinCtrl = new wxSpinCtrl(rootPanel, wxID_ANY,
                                               wxString::Format("%d", DEFAULT_KLINE_VIEW_H),
                                               wxDefaultPosition, wxDefaultSize,
                                               flags | textFlags,
                                               MIN_KLINE_VIEW_SIZE, MAX_KLINE_VIEW_SIZE, DEFAULT_KLINE_VIEW_H);
    sizer->Add(new wxStaticText(rootPanel, wxID_ANY, "行情页面宽:"), 0, wxALIGN_CENTER);
    sizer->Add(m_KLineViewWidthSpinCtrl, 0, wxLEFT | wxRIGHT, 0);
    sizer->Add(new wxStaticText(rootPanel, wxID_ANY, "高:"), 0, wxALIGN_CENTER | wxLEFT, 0);
    sizer->Add(m_KLineViewHeightSpinCtrl, 0, wxLEFT | wxRIGHT, 5);
    settingsBoxSizer->Add(sizer, 0);

    settingsBoxSizer->AddSpacer(8);

    // 功能勾选
    wxBoxSizer *optSizer = new wxBoxSizer(wxHORIZONTAL);
    m_colorCheck = new wxCheckBox(rootPanel, wxID_ANY, "开启涨跌颜色");
    m_priorityDisplayChangedCheck = new wxCheckBox(rootPanel, wxID_ANY, "优先显示价格变动");
    m_isDisplayStockNameCheck = new wxCheckBox(rootPanel, wxID_ANY, "显示股票名称");
    // m_alertCheck = new wxCheckBox(rootPanel, wxID_ANY, "价格涨跌幅提醒");
    optSizer->Add(m_colorCheck, 0);
    optSizer->Add(m_priorityDisplayChangedCheck, 0, wxLEFT, 15);
    optSizer->Add(m_isDisplayStockNameCheck, 0, wxLEFT, 15);
    // optSizer->Add(m_alertCheck, 0, wxLEFT, 15);
    settingsBoxSizer->Add(optSizer, 0);

    settingsBoxSizer->AddSpacer(8);

    // 涨跌颜色选择器
    // sizer = new wxBoxSizer(wxHORIZONTAL);
    // m_colourRise = new wxColourPickerCtrl(rootPanel, wxID_ANY, wxColour(255, 0, 0));
    // m_colourFall = new wxColourPickerCtrl(rootPanel, wxID_ANY, wxColour(0, 128, 0));
    // sizer->Add(new wxStaticText(rootPanel, wxID_ANY, "上涨颜色:"), 0, wxALIGN_CENTER);
    // sizer->Add(m_colourRise, 0, wxLEFT | wxRIGHT, 5);
    // sizer->Add(new wxStaticText(rootPanel, wxID_ANY, "下跌颜色:"), 0, wxALIGN_CENTER | wxLEFT, 15);
    // sizer->Add(m_colourFall, 0, wxLEFT, 5);
    // settingsBoxSizer->Add(sizer, 0);

    rootSizer->Add(settingsBoxSizer, 0, wxLEFT | wxRIGHT | wxEXPAND, 10);

    // 底部按钮
    sizer = new wxBoxSizer(wxHORIZONTAL);
    m_btnRestoreDefault = new wxButton(rootPanel, wxID_ANY, "恢复");
    m_btnOk = new wxButton(rootPanel, wxID_ANY, "确定");
    m_btnCancel = new wxButton(rootPanel, wxID_ANY, "取消");
    sizer->AddStretchSpacer();
    sizer->Add(m_btnRestoreDefault);
    sizer->Add(m_btnOk, 0, wxLEFT, 15);
    sizer->Add(m_btnCancel, 0, wxLEFT, 10);
    rootSizer->Add(sizer, 0, wxALL | wxEXPAND, 10);

    rootPanel->SetSizerAndFit(rootSizer);

    wxSizer *fsizer = new wxBoxSizer(wxVERTICAL);
    fsizer->Add(rootPanel, wxSizerFlags(1).Expand());
    SetSizerAndFit(fsizer);
}

void LStockOptionView::LoadOptions()
{
    int freqIndex = COMB_FREQ_OPTS.Index(wxString::FromDouble(g_data.RealtimeRefreshFreq()));
    m_cmbRefreshFreq->Select(freqIndex);
    m_colorCheck->SetValue(g_data.IsDisplayColor());
    m_priorityDisplayChangedCheck->SetValue(g_data.IsPriorityDisplayChanged());
    m_isDisplayStockNameCheck->SetValue(g_data.IsDisplayName());
    m_solVM->SetData(g_data.AllStocks());
}

void LStockOptionView::BindAllEvents()
{
    // 按钮
    m_btnAdd->Bind(wxEVT_BUTTON, &LStockOptionView::OnAddStock, this);
    m_btnRemove->Bind(wxEVT_BUTTON, &LStockOptionView::OnDeleteSel, this);
    m_btnClean->Bind(wxEVT_BUTTON, &LStockOptionView::OnBatchClear, this);
    m_btnMoveUP->Bind(wxEVT_BUTTON, &LStockOptionView::OnMoveUp, this);
    m_btnMoveDown->Bind(wxEVT_BUTTON, &LStockOptionView::OnMoveDown, this);
    m_btnMoveTop->Bind(wxEVT_BUTTON, &LStockOptionView::OnTopSel, this);

    m_btnRestoreDefault->Bind(wxEVT_BUTTON, &LStockOptionView::OnRestoreDefault, this);

    m_btnOk->Bind(wxEVT_BUTTON, &LStockOptionView::OnSaveConfig, this);
    m_btnCancel->Bind(wxEVT_BUTTON, &LStockOptionView::OnCancel, this);

    // 列表右键菜单
    m_stockListCtrl->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &LStockOptionView::OnListRightMenu, this);

    m_KLineViewWidthSpinCtrl->Bind(wxEVT_COMMAND_SPINCTRL_UPDATED, [](wxSpinEvent &event)
                                   {
        int w = event.GetValue();
        if (w < MIN_KLINE_VIEW_SIZE || w > MAX_KLINE_VIEW_SIZE) {
            wxMessageBox("无效宽度", "错误");
            return;
        }
        g_data.KLineW(w); });
    m_KLineViewWidthSpinCtrl->Bind(wxEVT_COMMAND_SPINCTRL_UPDATED, [](wxSpinEvent &event)
                                   {
        int h = event.GetValue();
        if (h < MIN_KLINE_VIEW_SIZE || h > MAX_KLINE_VIEW_SIZE) {
            wxMessageBox("无效高度", "错误");
            return;
        }
        g_data.KLineH(h); });
    m_colorCheck->Bind(wxEVT_CHECKBOX, [](wxCommandEvent &event)
                       { g_data.IsDisplayColor(event.IsChecked()); });
    m_priorityDisplayChangedCheck->Bind(wxEVT_CHECKBOX, [](wxCommandEvent &event)
                                        { g_data.IsPriorityDisplayChanged(event.IsChecked()); });
    m_isDisplayStockNameCheck->Bind(wxEVT_CHECKBOX, [](wxCommandEvent &event)
                                    { g_data.IsDisplayName(event.IsChecked()); });
    m_cmbRefreshFreq->Bind(wxEVT_COMMAND_COMBOBOX_SELECTED, [this](wxCommandEvent &event)
                           {
        int freq = UtilStringHlp::parseInt(m_cmbRefreshFreq->GetValue());
        if (UtilStringHlp::isValidNum(freq))
        {
            g_data.RealtimeRefreshFreq(freq);
        }
        else {
            wxMessageBox("无效刷新速率", "错误");
            return;
        } });

    Bind(wxEVT_COMMAND_MENU_SELECTED, &LStockOptionView::OnMenuSelect, this);
}

void LStockOptionView::OnAddStock(wxCommandEvent &)
{
    LStockSearchView searchView(this);

    if (searchView.ShowModal() == wxID_OK)
    {
        auto data = searchView.GetSelectStocks();
        if (data)
        {
            if (g_data.GetStockByCode(data->code))
            {
                wxMessageBox("该股票已在列表中，无需重复添加", "提示", wxICON_INFORMATION);
            }
            else
            {
                m_solVM->AppendRowData(data);
                g_data.AddStock(data);
            }
        }
    }
}

void LStockOptionView::OnDeleteSel(wxCommandEvent &)
{
    auto item = m_stockListCtrl->GetSelection();
    if (!item.IsOk())
    {
        return;
    }
    WXUINT row = m_solVM->GetRow(item);
    if (row >= 0)
    {
        auto data = m_solVM->GetRowData(item);
        if (data)
        {
            if (g_data.DeleteStockByCode(data->code))
            {
                m_solVM->EraseRow(row);
            }
        }
    }
}

void LStockOptionView::OnBatchClear(wxCommandEvent &)
{
    if (wxMessageBox("确定清空全部自选股票？", "确认", wxYES_NO | wxICON_QUESTION) != wxYES)
        return;
    g_data.ClearAllCodes();
    m_solVM->Clear();
}

void LStockOptionView::OnMoveUp(wxCommandEvent &)
{
    auto item = m_stockListCtrl->GetSelection();
    if (!item.IsOk())
    {
        return;
    }
    WXUINT row = m_solVM->GetRow(item);
    if (row > 0)
    {
        auto data = m_solVM->GetRowData(item);
        if (data)
        {
            WXUINT targetPos = row - 1;
            if (g_data.SetStockPosition(data, targetPos))
            {
                m_solVM->MoveRow(row, targetPos);
                m_stockListCtrl->Select(m_solVM->GetItem(targetPos));
            }
        }
    }
}

void LStockOptionView::OnMoveDown(wxCommandEvent &)
{
    auto item = m_stockListCtrl->GetSelection();
    if (!item.IsOk())
    {
        return;
    }
    WXUINT row = m_solVM->GetRow(item);
    if (row < m_solVM->GetCount() - 1)
    {
        auto data = m_solVM->GetRowData(item);
        if (data)
        {
            WXUINT targetPos = row + 1;
            if (g_data.SetStockPosition(data, targetPos))
            {
                m_solVM->MoveRow(row, targetPos);
                m_stockListCtrl->Select(m_solVM->GetItem(targetPos));
            }
        }
    }
}

void LStockOptionView::OnTopSel(wxCommandEvent &)
{
    auto item = m_stockListCtrl->GetSelection();
    if (!item.IsOk())
    {
        return;
    }
    WXUINT row = m_solVM->GetRow(item);
    if (row > 0)
    {
        auto data = m_solVM->GetRowData(item);
        if (data)
        {
            if (g_data.SetStockPosition(data, 0))
            {
                m_solVM->MoveRow(row, 0);
                m_stockListCtrl->Select(m_solVM->GetItem(0));
            }
        }
    }
}

void LStockOptionView::OnRestoreDefault(wxCommandEvent &)
{
    g_data.ReLoadConfig();
    g_data.ReLoadAllStocks();
    LoadOptions();
}

void LStockOptionView::OnSaveConfig(wxCommandEvent &)
{
    if (g_data.IsChangeStockList())
    {
        wxMessageBox("增删股票条数需要重启宿主程序后才能刷新界面", "温馨提示", wxICON_INFORMATION);
    }
    g_data.SaveConfig();
    Close();
}

void LStockOptionView::OnCancel(wxCommandEvent &)
{
    g_data.ReLoadConfig();
    g_data.ReLoadAllStocks();
    Close();
}

// 列表右键菜单
void LStockOptionView::OnListRightMenu(wxDataViewEvent &event)
{
    auto item = event.GetItem();
    if (!item.IsOk())
    {
        return;
    }
    wxMenu menu;
    menu.Append(ID_MENU_TOP, "置顶");
    menu.Append(ID_MENU_DEL, "删除");
    // menu.AppendSeparator();
    // menu.Append(ID_MENU_IMPORT, "批量导入");
    // menu.Append(ID_MENU_EXPORT, "导出列表");
    PopupMenu(&menu);
}

void LStockOptionView::OnMenuSelect(wxCommandEvent &e)
{
    int id = e.GetId();

    if (id == ID_MENU_DEL)
    {
        OnDeleteSel(e);
    }
    else if (id == ID_MENU_TOP)
    {
        OnTopSel(e);
    }
    // else if (id == ID_MENU_IMPORT)
    //{
    //     wxFileDialog dlg(this, "导入股票列表", "", "", "文本文件(*.txt)|*.txt", wxFD_OPEN);
    //     if (dlg.ShowModal() != wxID_OK)
    //         return;

    //    wxFileInputStream fis(dlg.GetPath());
    //    if (!fis.IsOk())
    //        return;
    //    wxTextInputStream tis(fis);
    //    wxString line;
    //    while (!fis.Eof())
    //    {
    //        line = tis.ReadLine().Trim();
    //        if (!line.IsEmpty())
    //        {
    //            CheckStockCode(line);
    //            if (m_stockList->FindString(line) == wxNOT_FOUND)
    //                m_stockList->Append(line);
    //        }
    //    }
    //    wxMessageBox("批量导入完成", "导入成功");
    //}
    // else if (id == ID_MENU_EXPORT)
    //{
    //    wxFileDialog dlg(this, "导出股票列表", "", "stock_list.txt", "文本文件(*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    //    if (dlg.ShowModal() != wxID_OK)
    //        return;

    //    wxFileOutputStream fos(dlg.GetPath());
    //    wxTextOutputStream tos(fos);
    //    for (unsigned int i = 0; i < m_stockList->GetCount(); ++i)
    //        tos << m_stockList->GetString(i) << endl;

    //    wxMessageBox("导出完成", "导出成功");
    //}
}

void LStockOptionView::OnClose(wxCloseEvent& event)
{
    wxQueueEvent(wxApp::GetInstance(), new wxThreadEvent(wxEVT_THREAD, m_close_evt_id));

    event.Skip();
}

wxBEGIN_EVENT_TABLE(LStockOptionView, wxFrame)
EVT_CLOSE(LStockOptionView::OnClose)
    wxEND_EVENT_TABLE()
