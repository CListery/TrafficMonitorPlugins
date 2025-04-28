#pragma once

#include <StockDef.h>
#include <TransparentWnd.h>

//class CTransparentWnd : public CWnd
//{
//public:
//    CTransparentWnd();
//    void SetParent(CWnd *pParent) { m_pParent = pParent; } // 添加设置父窗口的方法
//
//protected:
//    DECLARE_MESSAGE_MAP()
//    //afx_msg void OnMouseMove(UINT nFlags, CPoint point);
//    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
//    afx_msg BOOL OnEraseBkgnd(CDC *pDC);
//    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
//
//private:
//    CWnd *m_pParent;
//};

class CFloatingWnd : public CWnd
{
public:
    CFloatingWnd();
    virtual ~CFloatingWnd();

    BOOL Create(CFont* font, CPoint pt, std::wstring stock_id);

protected:
    DECLARE_MESSAGE_MAP()
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC *pDC);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    LRESULT OnUpdateStatus(WPARAM wParam, LPARAM lParam);
    LRESULT OnUpdateData(WPARAM wParam, LPARAM lParam);

private:
    void PositionWindow(CPoint pt);
    void RequestData();                           // 网络请求函数
    static UINT NetworkThreadProc(LPVOID pParam); // 线程函数
    CPoint Stock2Point(int x, int y, int w, int h, float unitY, const STOCK::TimelinePoint& item, const STOCK::Price prevClosePrice);

    CTransparentWnd m_CTransparentWnd;
    std::wstring m_stock_id;
    volatile BOOL m_isRequesting;
    volatile BOOL m_isDestroying; // 添加销毁标志
    CFont* m_pfont{};
};
