#include "pch.h"
#include "FloatingWnd.h"
#include <afxinet.h>
#include <memory>
#include "Common.h"
#include "DataManager.h"

// 定义自定义消息
#define WM_UPDATE_STATUS (WM_USER + 100)
#define WM_UPDATE_DATA (WM_USER + 101)

constexpr auto WEB_USERAGENT = _T("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/135.0.0.0 Safari/537.36 Edg/135.0.0.0");

BEGIN_MESSAGE_MAP(CTransparentWnd, CWnd)
ON_WM_LBUTTONDOWN()
ON_WM_ERASEBKGND()
ON_WM_CREATE()
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CFloatingWnd, CWnd)
ON_WM_PAINT()
ON_WM_ERASEBKGND()
ON_WM_LBUTTONDOWN()
ON_WM_CREATE()
// ... 其他消息映射
ON_MESSAGE(WM_UPDATE_STATUS, OnUpdateStatus)
ON_MESSAGE(WM_UPDATE_DATA, OnUpdateData)
END_MESSAGE_MAP()

int CTransparentWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    // 添加调试输出
    TRACE(L"CTransparentWnd Created\n");
    return 0;
}

int CFloatingWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    // 开始网络请求
    RequestData();
    return 0;
}

// 处理消息
LRESULT CFloatingWnd::OnUpdateStatus(WPARAM wParam, LPARAM lParam)
{
    m_isRequesting = (BOOL)wParam;
    Invalidate();
    return 0;
}

LRESULT CFloatingWnd::OnUpdateData(WPARAM wParam, LPARAM lParam)
{
    CString *pData = (CString *)wParam;
    if (pData)
    {
        m_data = *pData;
        delete pData;
        Invalidate();
    }
    return 0;
}

CTransparentWnd::CTransparentWnd() : m_pParent(nullptr)
{
}

CFloatingWnd::CFloatingWnd()
    : m_isRequesting(FALSE), m_isDestroying(FALSE)
{
}

CFloatingWnd::~CFloatingWnd()
{
    // 标记窗口正在销毁
    m_isDestroying = TRUE;
    if (m_CTransparentWnd.GetSafeHwnd())
        m_CTransparentWnd.DestroyWindow();
}

BOOL CFloatingWnd::Create(CPoint pt, std::wstring stock_id)
{
    m_stock_id = stock_id;
    // 注册窗口类
    WNDCLASS wndcls;
    HINSTANCE hInst = AfxGetInstanceHandle();
    if (!(::GetClassInfo(hInst, L"CTransparentWnd", &wndcls)))
    {
        wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
        wndcls.lpfnWndProc = ::DefWindowProc;
        wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
        wndcls.hInstance = hInst;
        wndcls.hIcon = NULL;
        wndcls.hCursor = LoadCursor(NULL, IDC_ARROW);
        // wndcls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wndcls.hbrBackground = NULL; // 重要：设置为NULL
        wndcls.lpszMenuName = NULL;
        wndcls.lpszClassName = L"CTransparentWnd";
        if (!AfxRegisterClass(&wndcls))
            return FALSE;
    }

    // 设置父窗口指针
    m_CTransparentWnd.SetParent(this);

    // 获取包含鼠标点的显示器
    HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = {sizeof(MONITORINFO)};
    GetMonitorInfo(hMonitor, &mi);
    CRect screenRect = mi.rcWork; // 工作区域

    // 创建透明全屏窗口
    if (!m_CTransparentWnd.CreateEx(WS_EX_TOOLWINDOW /* | WS_EX_LAYERED */ /* | WS_EX_TRANSPARENT */,
                                    L"CTransparentWnd", L"", WS_POPUP | WS_VISIBLE,
                                    screenRect, NULL, 0, NULL))
    {
        TRACE(L"Failed to create transparent window\n");
        return FALSE;
    }

    const int WIDTH = 300;
    const int HEIGHT = 200;
    int x = pt.x;
    int y = pt.y;

    // 调整位置
    if (x + WIDTH > screenRect.right)
        x = screenRect.right - WIDTH;
    if (y + HEIGHT > screenRect.bottom)
        y = screenRect.bottom - HEIGHT;
    x = max(screenRect.left, x);
    y = max(screenRect.top, y);

    CRect rect(pt.x, pt.y, pt.x + WIDTH, pt.y + HEIGHT);

    // 创建实际的浮动窗口
    if (!CreateEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                  AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW),
                  L"", WS_POPUP | WS_VISIBLE | WS_BORDER,
                  rect, &m_CTransparentWnd, 0))
    {
        TRACE(L"Failed to create floating window\n");
        m_CTransparentWnd.DestroyWindow();
        return FALSE;
    }

    // 确保浮动窗口在最顶层
    BringWindowToTop();
    SetForegroundWindow();

    // 设置完全透明
    m_CTransparentWnd.SetLayeredWindowAttributes(0, 0, LWA_ALPHA);
    m_CTransparentWnd.ShowWindow(SW_SHOW);

    TRACE(L"Windows created successfully\n");
    return TRUE;
}

void CFloatingWnd::OnPaint()
{
    CPaintDC dc(this);
    CRect rect;
    GetClientRect(&rect);

    // 双缓冲绘制
    CDC memDC;
    CBitmap memBitmap;
    memDC.CreateCompatibleDC(&dc);
    memBitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());
    CBitmap *pOldBitmap = memDC.SelectObject(&memBitmap);

    // 绘制背景
    memDC.FillSolidRect(rect, RGB(255, 255, 255));

    // 绘制内容
    memDC.SetBkMode(TRANSPARENT);
    // memDC.TextOut(10, 10, L"这是一个测试文本");
    memDC.TextOut(10, 10, m_isRequesting ? L"Loading..." : m_data);

    // 复制到屏幕
    dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);

    memDC.SelectObject(pOldBitmap);
}

BOOL CFloatingWnd::OnEraseBkgnd(CDC *pDC)
{
    return TRUE; // 不擦除背景
}

void CFloatingWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
    // DestroyWindow();
}

void CFloatingWnd::RequestData()
{
    if (!m_isRequesting)
    {
        m_isRequesting = TRUE;
        AfxBeginThread(NetworkThreadProc, this);
    }
}

UINT CFloatingWnd::NetworkThreadProc(LPVOID pParam)
{
    CFloatingWnd *pWnd = (CFloatingWnd *)pParam;

    try
    {
        // 检查窗口是否正在销毁
        if (pWnd->m_isDestroying)
        {
            return 0;
        }

        std::wstring url{L"https://cn.finance.sina.com.cn/minline/getMinlineData?"};
        // https://cn.finance.sina.com.cn/minline/getMinlineData?symbol=sz000100&version=7.11.0&dpc=1
        std::vector<std::wstring> params;
        params.push_back(L"symbol=" + pWnd->m_stock_id);
        params.push_back(L"version=7.11.0");
        params.push_back(L"dpc=1");

        url += CCommon::vectorJoinString(params, L"&");
        CCommon::WriteLog(url.c_str(), g_data.m_log_path.c_str());

        // CString strHeaders = L"Referer: https://finance.sina.com.cn/realstock/company/" + m_stock_id + L"/nc.shtml";
        std::wstring strHeaders{L"Referer: https://finance.sina.com.cn/realstock/company/"};
        strHeaders += pWnd->m_stock_id;
        strHeaders += L"/nc.shtml";
        CString headers = strHeaders.c_str();

        CInternetSession *session = new CInternetSession(WEB_USERAGENT);
        CHttpFile *pFile = (CHttpFile *)session->OpenURL(url.c_str(), 1, INTERNET_FLAG_TRANSFER_ASCII, headers, headers.GetLength());

        // 检查窗口是否有效
        if (pWnd->m_isDestroying)
        {
            return 0;
        }

        // pFile->SendRequest();

        DWORD dwStatusCode;
        pFile->QueryInfoStatusCode(dwStatusCode);

        if (dwStatusCode == HTTP_STATUS_OK)
        {
            CString strData;
            char szBuffer[1025];
            int nRead;
            while ((nRead = pFile->Read(szBuffer, 1024)) > 0)
            {
                szBuffer[nRead] = 0;
                strData += CString(szBuffer);
            }

            if (!pWnd->m_isDestroying)
            {
                // 使用消息更新状态
                pWnd->PostMessage(WM_UPDATE_STATUS, FALSE, 0);
                pWnd->PostMessage(WM_UPDATE_DATA, (WPARAM) new CString(strData), 0);
            }
        }

        // 清理资源
        pFile->Close();
        delete pFile;
        session->Close();
    }
    catch (CInternetException *e)
    {
        e->Delete();
        if (!pWnd->m_isDestroying)
        {
            pWnd->PostMessage(WM_UPDATE_STATUS, FALSE, 0);
        }
    }

    return 0;
}

void CTransparentWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
    // 添加调试输出
    TRACE(L"CTransparentWnd OnLButtonDown\n");

    // 获取鼠标位置
    CPoint ptScreen;
    GetCursorPos(&ptScreen);

    // 获取浮动窗口区域
    CRect rcFloat;
    if (m_pParent && m_pParent->GetSafeHwnd())
    {
        m_pParent->GetWindowRect(rcFloat);
        if (!rcFloat.PtInRect(ptScreen))
        {
            TRACE(L"Destroying floating window\n");
            m_pParent->DestroyWindow();
            DestroyWindow();
        }
        else
        {
            // 如果点击在浮动窗口内部，将消息传递给浮动窗口
            m_pParent->SendMessage(WM_LBUTTONDOWN, nFlags, MAKELPARAM(point.x, point.y));
        }
    }
    // else
    // {
    //     TRACE(L"Destroying transparent window\n");
    //     DestroyWindow();
    // }
}

BOOL CTransparentWnd::OnEraseBkgnd(CDC *pDC)
{
    return TRUE; // 不擦除背景
}
