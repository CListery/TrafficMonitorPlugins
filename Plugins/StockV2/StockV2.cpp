#include "pch.h"
#include "framework.h"
#include "StockV2.h"
#include "StockView.h"
#include "TaskManager.h"

#include "StockOptionView.h"

#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/app.h>
#include <wx/thread.h>
#include <wx/msgdlg.h>
#include <process.h>
#include <mutex>

#include <wx/filesys.h>
#include <wx/fs_arc.h>
#include <wx/fs_mem.h>
#include <wx/fileconf.h>
#include <StockSockets.h>

// 插件导出接口
extern "C" ITMPlugin *PASCAL EXPORT TMPluginGetInstance()
{
    AFX_MANAGE_STATE_MODULE();
    return LStockPlugin::Instance();
}

LStockPlugin *LStockPlugin::m_pInstance = nullptr;
std::recursive_mutex LStockPlugin::m_mtxInstance;

// wxWidgets
namespace
{
    static const int CMD_SHOW_WINDOW = wxNewId();
    static const int CMD_TERMINATE = wxNewId();
    static const int CMD_SHOW_STOCK_VIEW = wxNewId();
    static const int CMD_SHOW_STOCK_OPTION_VIEW = wxNewId();
    static const int CMD_CLOSE_STOCK_OPTION_VIEW = wxNewId();

    // IDs for the controls and the menu commands
    enum
    {
        // menu items
        Event_Menu_OpenWeb = 1,
        Event_Menu_OpenStockOption,
    };

    // ----------------------------------------------------------------------------
    // GUI classes
    // ----------------------------------------------------------------------------

    class LStockMenuEvtHandler : public wxEvtHandler
    {
    public:
        LStockMenuEvtHandler(wxSharedPtr<STOCK::LStockData> stock) { m_stock = stock; }

        void OnOpenWeb(wxCommandEvent &event)
        {
            wxLogDebug("OnOpenWeb");

            if (m_stock && !m_stock->url.empty()) {
                wxLaunchDefaultBrowser(m_stock->url);
            }

            // if we don't skip the event, the other event handlers won't get it:
            // try commenting out this line and see what changes
            event.Skip();
        }

        void OnOpenStockOption(wxCommandEvent &event)
        {
            wxLogDebug("OnOpenStockOption");

            wxQueueEvent(wxApp::GetInstance(), new wxThreadEvent(wxEVT_THREAD, CMD_SHOW_STOCK_OPTION_VIEW));

            // if we don't skip the event, the other event handlers won't get it:
            // try commenting out this line and see what changes
            event.Skip();
        }

    private:
        wxSharedPtr<STOCK::LStockData> m_stock;

        wxDECLARE_EVENT_TABLE();
    };

    class StockWxFrame : public wxFrame
    {
    public:
        StockWxFrame(const wxString &title);

        void OnAbout(wxCommandEvent &event);

        wxDECLARE_EVENT_TABLE();
    };

    class wxStockWindow : public wxFrame
    {
    private:
        wxLogWindow *m_log_window;

    public:
        wxStockWindow() : wxFrame(NULL, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0)
        {
#ifdef DEBUG
            // Create log window
            m_log_window = new wxLogWindow(this, "Log Messages", false);
            // m_log_window->GetFrame()->Move(mousePt.x, GetPosition().y - m_log_window->GetFrame()->GetSize().y - 10);
            m_log_window->Show();
            m_log_window->GetFrame()->Raise();
#endif
        }

        ~wxStockWindow()
        {
#ifdef DEBUG
            delete m_log_window;
#endif
        }

        // WXLRESULT MSWWindowProc(WXUINT msg, WXWPARAM wParam, WXLPARAM lParam) wxOVERRIDE
        //{
        //     return wxFrame::MSWWindowProc(msg, wParam, lParam);
        // }
    };

    // ----------------------------------------------------------------------------
    // StockWxApp
    // ----------------------------------------------------------------------------

    class LStockWxMgr
    {
    public:
        static LStockWxMgr *Instance();
        bool InitWxWidgets();
        void CleanupWxWidgets();

    private:
        LStockWxMgr()
        {
            m_hInitEvent = NULL;
            m_wxMainThreadHandle = NULL;
            m_win = NULL;
        }

    private:
        HANDLE m_hInitEvent;
        // Handle of wx "main" thread if running, NULL otherwise
        HANDLE m_wxMainThreadHandle;
        // Critical section that guards everything related to wxWidgets "main" thread startup or shutdown.
        wxCriticalSection m_wxStartupMutex;

        wxStockWindow *m_win;

    public:
        wxWindow *GetStockWnd()
        {
            wxASSERT_MSG(m_win != NULL, wxT("wxWidgets engine not initialized"));
            return m_win;
        }

    private:
        struct ThreadParam
        {
            LStockWxMgr *pThis;
            HANDLE hEvent;
        };

        // ========================== wxWidgets 线程启动函数 ==========================
        // wx 应用程序启动代码 -- 运行在独立线程中
        static unsigned wxSTDCALL StockWxAppLauncher(void *eventHandle)
        {
            LLOG_INFO_F("StockWxAppLauncher");

            ThreadParam *param = static_cast<ThreadParam *>(eventHandle);
            LStockWxMgr *pThis = static_cast<LStockWxMgr *>(param->pThis);
            HANDLE hEvent = static_cast<HANDLE *>(param->hEvent);

            // 注：调用 InitWxWidgets() 的线程当前持有 m_wxStartupMutex 互斥锁，
            //      且在我们发送信号前不会释放该锁。

            // 我们需要向 wxEntry() 传入正确的实例句柄 HINSTANCE，
            // 正确值为本 DLL 的实例句柄，而非主程序 exe 的句柄，
            // 调用这个仅适用于 Windows 平台的 wxWidgets 函数来获取该句柄。
            // 注意：Windows XP 及以上系统无需传入/使用模块名称，程序会通过 DLL 内部地址获取模块句柄；
            // 但如果要让这段代码在老旧系统上也能正常运行，则必须传入正确的模块名称。
            const HINSTANCE hInstance = wxDynamicLibrary::MSWGetModuleHandle(g_data.GetModuleName(), &pThis->m_wxMainThreadHandle);
            if (!hInstance)
            {
                LLOG_ERROR_F("StockWxAppLauncher ModuleHandle Error!");
                return 0; // failed to get DLL's handle
            }
#ifndef DEBUG
            // 使用该宏可在发行版构建中禁用所有调试代码，适用于未使用 wxIMPLEMENT_APP() 的场景。
            wxDISABLE_DEBUG_SUPPORT();
#endif // DEBUG

            // 我们提前手动执行该操作，尽管 wxEntry() 内部本身也会执行，
            // 这样做是为了能确认 wxWidgets 初始化完成，并在启动事件循环**之前**，
            // 向 InitWxWidgets() 发送初始化完成信号。
            wxInitializer initializer;
            if (!initializer.IsOk())
            {
                LLOG_ERROR_F("StockWxAppLauncher wxInitializer Error!");
                return 0; // failed to init wx
            }
            // 通知 InitWxWidgets() 函数可以继续执行了
            if (!SetEvent(hEvent))
            {
                LLOG_ERROR_F("StockWxAppLauncher Event Error!");
                return 0; // failed setting up the mutex
            }
            // Run the app:
            wxEntry(hInstance);
            return 1;
        }
    };

    class StockWxApp : public wxApp
    {
    public:
        StockWxApp();

    private:
        void OnShowWindow(wxThreadEvent &event);
        void OnTerminate(wxThreadEvent &event);
        void OnShowStockView(wxThreadEvent &event);
        void OnShowStockOptionView(wxThreadEvent &event);
        void OnCloseShowStockOptionView(wxThreadEvent& event);
    private:
        wxSharedPtr<LStockOptionView> optView;
    };

    // --------------------------------------------------
    // LStockWxMgr
    // --------------------------------------------------

    LStockWxMgr *LStockWxMgr::Instance()
    {
        static LStockWxMgr *instance = nullptr;
        if (instance == nullptr)
        {
            instance = new LStockWxMgr();
        }
        return instance;
    }

    bool LStockWxMgr::InitWxWidgets()
    {
        LLOG_INFO_F("InitWxWidgets");

        // std::lock_guard<std::recursive_mutex> lock(m_wxStartupMutex);
        wxCriticalSectionLocker lock(m_wxStartupMutex);

        if (m_wxMainThreadHandle)
        {
            return true;
        }

        // 创建事件，等待wx初始化完成
        m_hInitEvent = CreateEvent(
            NULL,  // default security attributes
            FALSE, // auto-reset
            FALSE, // initially non-signaled
            NULL   // anonymous
        );

        if (!m_hInitEvent)
            return false; // error

        ThreadParam *param = new ThreadParam;
        param->pThis = this;
        param->hEvent = m_hInitEvent;

        // 启动wx主线程
        // 注意：如果你的编译器不支持 _beginthreadex()，请改用 CreateThread()
        m_wxMainThreadHandle = (HANDLE)_beginthreadex(
            NULL, // default security
            0,    // default stack size
            &LStockWxMgr::StockWxAppLauncher,
            param, // arguments
            0,     // create running
            NULL);

        if (!m_wxMainThreadHandle)
        {
            CloseHandle(m_hInitEvent);
            return false; // error
        }

        // 等待 StockWxAppLauncher 通知我们 wxWidgets 已完成初始化。
        // 之所以要这样做，是因为后续代码会用到 wxMessageQueue<> 和 wxString，
        // 因此必须确保这两个组件能够正常工作。
        WaitForSingleObject(m_hInitEvent, INFINITE);
        CloseHandle(m_hInitEvent);
        m_hInitEvent = nullptr;

        // NB: we have to create the window lazily because of backward compatibility,
        //     old applications may create a wxTaskBarIcon instance before wxApp
        //     is initialized (as samples/taskbar used to do)
        if (!m_win)
        {
            m_win = new wxStockWindow();
        }

        //wxString cfgPath = g_data.GetModuleName() + wxFILE_SEP_EXT + L"ini";
        wxString cfgPath = g_data.GetModuleParentPath() + wxFILE_SEP_PATH + g_data.GetModuleName() + wxFILE_SEP_EXT + L"ini";
        LLOG_DEBUG_F("config path: %s", cfgPath);
        wxFileConfig* pConfig = new wxFileConfig(ResString(IDS_PLUGIN_AUTHOR), g_data.GetModuleName(), cfgPath);
        wxConfigBase::Set(pConfig);
        //pConfig->SetRecordDefaults();

        g_data.InitConfig();
        g_data.InitTasks();

        //LStockServerSocket& sss = LStockServerSocket::GetInstance();
        //sss.StartSocketServer();
        //sss.GetBridge()->RegisterCallFunc("request_kline_data", LDataManager::OnRefreshStockTimelineData);
        //wxLogDebug("StockServer StartUp: %d", sss.GetWebBridgePort());

        LLOG_INFO_F("InitWxWidgets DONE!");

        return true;
    }

    void LStockWxMgr::CleanupWxWidgets()
    {
        LLOG_INFO_F("CleanupWxWidgets");

        // std::lock_guard<std::recursive_mutex> lock(m_wxStartupMutex);

        wxCriticalSectionLocker lock(m_wxStartupMutex);

        if (!m_wxMainThreadHandle)
            return;

        // If wx main thread is running, we need to stop it. To accomplish this,
        // send a message telling it to terminate the app.
        // 如果 wxWidgets 主线程正在运行，我们需要将其终止。实现方式为：
        // 向该线程发送消息，通知其退出应用程序。
        wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD, CMD_TERMINATE);
        wxQueueEvent(wxApp::GetInstance(), event);

        wxDELETE(m_win);

        // We must then wait for the thread to actually terminate.
        // 等待线程退出并释放资源
        WaitForSingleObject(m_wxMainThreadHandle, INFINITE);
        CloseHandle(m_wxMainThreadHandle);
        m_wxMainThreadHandle = nullptr;

        LLOG_INFO_F("CleanupWxWidgets DONE!");
    }

    // ========================== StockWxApp 实现 ==========================

    StockWxApp::StockWxApp()
    {
        LLOG_INFO_F("StockWxApp");

        // 即便没有窗口，也保持 wxWidgets 主线程持续运行。
        // 这能极大简化线程管理逻辑，我们无需复杂实现 wx 主线程重启相关逻辑。
        //
        // 注意：仅当你不会主动调用 ExitMainLoop() 时该机制才能正常工作，
        // 仅在响应 CleanupWxWidgets() 发来的消息时除外。
        // CleanupWxWidgets() 需要 wxApp 应用实例保持可用状态；
        // 一旦事件循环退出，wxEntry() 函数就会返回，wxApp 实例也会随之销毁。
        //
        // 另外该方案运行效率很高：当不存在任何窗口时，主线程会进入休眠，等待新事件到来。
        // 当然，我们也可以在线程不再使用时将其关闭，以此节省一部分内存。
        SetExitOnFrameDelete(false);

        // Required for virtual file system archive and memory support
        wxFileSystem::AddHandler(new wxArchiveFSHandler);
        wxFileSystem::AddHandler(new wxMemoryFSHandler);
        // Create the memory files
        wxImage::AddHandler(new wxPNGHandler);

        // 我们使用了wxConfig的“按需创建”特性：配置对象会在首次被使用时才创建。
        // 相较于手动显式创建wxConfig对象，该特性具备多项优势：
        //  1) 若程序全程未使用配置功能，则不会产生任何资源开销
        //  2) 不存在重复创建配置对象的风险

        // wxConfig会借助应用名称与厂商名称来生成配置文件名称或注册表项名称；
        // 如果你需要覆盖默认名称，必须在第一次调用Get()方法前完成二者的设置。
        // 默认情况下，应用名称为可执行文件名，厂商名称与应用名称一致。
        SetVendorName(ResString(IDS_PLUGIN_AUTHOR));
        SetAppName(g_data.GetModuleName()); // not needed, it's the default value

        Bind(wxEVT_THREAD, &StockWxApp::OnShowWindow, this, CMD_SHOW_WINDOW);
        Bind(wxEVT_THREAD, &StockWxApp::OnTerminate, this, CMD_TERMINATE);
        Bind(wxEVT_THREAD, &StockWxApp::OnShowStockView, this, CMD_SHOW_STOCK_VIEW);
        Bind(wxEVT_THREAD, &StockWxApp::OnShowStockOptionView, this, CMD_SHOW_STOCK_OPTION_VIEW);
        Bind(wxEVT_THREAD, &StockWxApp::OnCloseShowStockOptionView, this, CMD_CLOSE_STOCK_OPTION_VIEW);

        LLOG_INFO_F("StockWxApp DONE!");
    }

    void StockWxApp::OnTerminate(wxThreadEvent &)
    {
        LLOG_INFO_F("OnTerminate");

        // 资源清理说明：Set() 和 Get() 一样会返回当前正在使用的配置对象，但和 Get() 不同的是，
        // 当不存在配置对象时，Set() 不会自动新建（这正是我们此处需要的行为）
        delete wxConfigBase::Set((wxConfigBase *)NULL);

        ExitMainLoop();
    }

    void StockWxApp::OnCloseShowStockOptionView(wxThreadEvent& event)
    {
        optView.reset();
    }

    void StockWxApp::OnShowStockOptionView(wxThreadEvent &event)
    {
        if (optView && optView->IsShown()) {
            // 窗口置顶
            optView->Raise();
            optView->SetFocus();
#ifdef __WXMSW__
            ::BringWindowToTop((HWND)optView->GetHandle());
            ::SetForegroundWindow((HWND)optView->GetHandle());
#endif
            return;
        }
        wxSize frameSize((wxSystemSettings::GetMetric(wxSYS_SCREEN_X) / 10) * 4,
                         (wxSystemSettings::GetMetric(wxSYS_SCREEN_Y) / 10) * 3);
        if (frameSize.x > 580)
            frameSize.x = 580;

        if (frameSize.y > 500)
            frameSize.y = 500;

        optView.reset(new LStockOptionView(ResString(IDS_OPTION_VIEW_TITLE), wxPoint(0, 0), frameSize, CMD_CLOSE_STOCK_OPTION_VIEW));
        optView->Show(true);
    }

    void StockWxApp::OnShowWindow(wxThreadEvent &event)
    {
        wxFrame *frame = new StockWxFrame(event.GetString());
        frame->Show(true);
    }

    void StockWxApp::OnShowStockView(wxThreadEvent &event)
    {
        const int stock_index = event.GetInt();
        if (stock_index < 0) {
            return;
        }
        // 显示走势图
        wxFrame *frame = new wxFrame(NULL, wxID_ANY, "StockView");
        wxPoint pt = wxGetMousePosition();
        LStockViewer::LStockView *stock_view = new LStockViewer::LStockView(frame);
        stock_view->Setup(pt, stock_index);
    }

    // we can't have WinMain() in a DLL and want to start the app ourselves
    // DLL中不能实现WinMain()，因此需要我们自行启动应用程序
    wxIMPLEMENT_APP_NO_MAIN(StockWxApp);

    // ----------------------------------------------------------------------------
    // StockWxFrame
    // ----------------------------------------------------------------------------

    StockWxFrame::StockWxFrame(const wxString &label) : wxFrame(NULL, wxID_ANY, label)
    {
        wxPanel *p = new wxPanel(this, wxID_ANY);
        wxSizer *sizer = new wxBoxSizer(wxVERTICAL);

        // 版本信息文本
        sizer->Add(
            new wxStaticText(
                p, wxID_ANY,
                wxString::Format(
                    "Running using %s\n"
                    "wxApp instance is %p, thread ID %ld",
                    wxVERSION_STRING,
                    wxApp::GetInstance(),
                    wxThread::GetCurrentId())),
            wxSizerFlags(1).Expand().Border(wxALL, 10));

        // 关于按钮
        sizer->Add(
            new wxButton(p, wxID_ABOUT, "Show info"),
            wxSizerFlags(0).Right().Border(wxALL, 10));

        p->SetSizerAndFit(sizer);

        // 窗口总布局
        wxSizer *fsizer = new wxBoxSizer(wxVERTICAL);
        fsizer->Add(p, wxSizerFlags(1).Expand());
        SetSizerAndFit(fsizer);

        // 窗口置顶
        this->Raise();
        this->SetFocus();
#ifdef __WXMSW__
        ::BringWindowToTop((HWND)this->GetHandle());
        ::SetForegroundWindow((HWND)this->GetHandle());
#endif
    }

    void StockWxFrame::OnAbout(wxCommandEvent &)
    {
        wxMessageBox("该窗口在独立线程中运行，使用编译到动态链接库中的专属wxWidgets实例。",
                     "About",
                     wxOK | wxICON_INFORMATION);
    }

    // GUI Event
    wxBEGIN_EVENT_TABLE(StockWxFrame, wxFrame)        //
        EVT_BUTTON(wxID_ABOUT, StockWxFrame::OnAbout) //
        wxEND_EVENT_TABLE()                           //

        // Menu Event
        wxBEGIN_EVENT_TABLE(LStockMenuEvtHandler, wxEvtHandler) //
        EVT_MENU(Event_Menu_OpenWeb, LStockMenuEvtHandler::OnOpenWeb)   //
        EVT_MENU(Event_Menu_OpenStockOption, LStockMenuEvtHandler::OnOpenStockOption)   //
        wxEND_EVENT_TABLE()                                     //
}

// --------------------------------------------------
// LStockPlugin
// --------------------------------------------------

LStockPlugin *LStockPlugin::Instance()
{
    if (m_pInstance == nullptr)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mtxInstance);
        if (m_pInstance == nullptr)
            m_pInstance = new LStockPlugin();
    }
    return m_pInstance;
}

// LStockPlugin 构造
LStockPlugin::LStockPlugin()
{
    AFX_MANAGE_STATE_MODULE();
    // 初始化wxWidgets GUI
    LStockWxMgr::Instance()->InitWxWidgets();
    // 启动任务管理器
    LTaskManager::Instance()->StartUp();

    m_displayStocks = std::vector<LStockItem>(STOCK_DISPLAY_ITEM_MAX);
    std::fill(m_displayStocks.begin(), m_displayStocks.end(), LStockItem());
    for (size_t index = 0; index < m_displayStocks.size(); index++)
    {
        m_displayStocks[index].index = index;
    }

    LoadDisplayItems();
}

LStockPlugin::~LStockPlugin()
{
    AFX_MANAGE_STATE_MODULE();
    // 清理任务管理器
    LTaskManager::Instance()->ShutDown();
    // 清理wxWidgets
    LStockWxMgr::Instance()->CleanupWxWidgets();
}

void LStockPlugin::OnInitialize(ITrafficMonitor *pApp)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState())
}

void LStockPlugin::LoadDisplayItems()
{
    for (LStockItem item : m_displayStocks)
    {
        item.enable = false;
    }
    for (int i = 0; i < g_data.GetAllCodes().size(); ++i)
    {
        m_displayStocks[i].index = i;
        m_displayStocks[i].enable = true;
    }
}

IPluginItem *LStockPlugin::GetItem(int index)
{
    AFX_MANAGE_STATE_MODULE();

    // 不支持动态创建
    size_t item_size = m_displayStocks.size();
    if (g_data.GetAllCodes().size() < item_size)
        item_size = g_data.GetAllCodes().size();
    // if (item_size == 0)
    //     item_size = 1;
    if (index >= item_size)
        return nullptr;
    return &(m_displayStocks[index]);
}

void LStockPlugin::DataRequired()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
}

const wchar_t *LStockPlugin::GetInfo(PluginInfoIndex index)
{
    AFX_MANAGE_STATE_MODULE();
    switch (index)
    {
    case TMI_NAME:
        return ResString(IDS_PLUGIN_NAME);
    case TMI_DESCRIPTION:
        return ResString(IDS_PLUGIN_DESCRIPTION);
    case TMI_AUTHOR:
        return ResString(IDS_PLUGIN_AUTHOR);
    case TMI_COPYRIGHT:
        return ResString(IDS_PLUGIN_COPYRIGHT);
    case TMI_URL:
        return ResString(IDS_PLUGIN_URL);
    case TMI_VERSION:
        return ResString(IDS_PLUGIN_VERSION);
    default:
        return L"";
    }
}

void LStockPlugin::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t *data)
{
    AFX_MANAGE_STATE_MODULE();
    switch (index)
    {
    case ITMPlugin::ExtendedInfoIndex::EI_CONFIG_DIR:
        // 配置文件的目录
        // g_data.LoadConfig(data);
        break;
    case ITMPlugin::EI_TASKBAR_WND_SPERATE_WITH_SPACE:
        // 数值和单位使用空格分隔
        break;
    case ITMPlugin::EI_TASKBAR_WND_VALUE_RIGHT_ALIGN:
        // 数值右对齐
        // 获取TrafficMonitor任务栏窗口中“数值右对齐”设置
        g_data.IsDisplayRightAlign(_wtoi(data) != 0);
        break;
    default:
        break;
    }
}

void *LStockPlugin::GetPluginIcon()
{
    AFX_MANAGE_STATE_MODULE();
    return ResHicon(IDI_STOCK);
}

int LStockPlugin::GetCommandCount()
{
    return 1;
}

const wchar_t *LStockPlugin::GetCommandName(int command_index)
{
    AFX_MANAGE_STATE_MODULE();
    return ResString(IDS_MENU_STOCK_MANAGE);
}

void LStockPlugin::OnPluginCommand(int command_index, void *hWnd, void *para)
{
    AFX_MANAGE_STATE_MODULE();
    wxThreadEvent* event = new wxThreadEvent(wxEVT_THREAD, CMD_SHOW_STOCK_OPTION_VIEW);
    wxQueueEvent(wxApp::GetInstance(), event);
}

LStockPlugin::OptionReturn LStockPlugin::ShowOptionsDialog(void *hParent)
{
    AFX_MANAGE_STATE_MODULE();

    wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD, CMD_SHOW_STOCK_OPTION_VIEW);
    wxQueueEvent(wxApp::GetInstance(), event);

    return ITMPlugin::OR_OPTION_UNCHANGED;
}

void LStockPlugin::ShowStockWxFrame()
{
    wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD, CMD_SHOW_WINDOW);
    event->SetString("StockV2");
    wxQueueEvent(wxApp::GetInstance(), event);
}

void LStockPlugin::ShowStockView(const int stock_index)
{
    if (stock_index < 0) {
        return;
    }

    wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD, CMD_SHOW_STOCK_VIEW);
    event->SetInt(stock_index);
    wxQueueEvent(wxApp::GetInstance(), event);
}

void LStockPlugin::ShowStockViewMenu(void *hWnd, CPoint ptScreen, wxSharedPtr<STOCK::LStockData> stock)
{
    if (!stock) {
        return;
    }

    wxWindow *m_win = LStockWxMgr::Instance()->GetStockWnd();

    static bool s_inPopup = false;

    if (s_inPopup)
        return;

    s_inPopup = true;

    int x, y;
    wxGetMousePosition(&x, &y);

    m_win->Move(x, y);

    m_win->PushEventHandler(new LStockMenuEvtHandler(stock));

    wxMenu *menu = new wxMenu;
    menu->Append(Event_Menu_OpenWeb, "打开Web页面");
    menu->AppendSeparator();
    menu->Append(Event_Menu_OpenStockOption, "管理股票列表");

    menu->UpdateUI();

    m_win->Raise();
    m_win->SetFocus();
#ifdef __WXMSW__
    ::BringWindowToTop((HWND)m_win->GetHWND());
    ::SetForegroundWindow((HWND)m_win->GetHWND());
#endif

    m_win->PopupMenu(menu, 0, 0);

    //::PostMessage((HWND)m_win->GetHWND(), WM_NULL, 0, 0L);

    m_win->PopEventHandler(true);

    s_inPopup = false;
}
