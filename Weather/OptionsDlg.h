﻿#pragma once
#include "DataManager.h"

// COptionsDlg 对话框

class COptionsDlg : public CDialog
{
    DECLARE_DYNAMIC(COptionsDlg)

public:
    COptionsDlg(CWnd* pParent = nullptr);   // 标准构造函数
    virtual ~COptionsDlg();
    void EnableUpdateBtn(bool enable);

    SettingData m_data;

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_OPTIONS_DIALOG };
#endif

private:
    CComboBox m_weather_type_combo;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedSelectCityButton();
    afx_msg void OnCbnSelchangeWeatherTypeCombo();
    afx_msg void OnBnClickedShowTooltipsCheck();
    afx_msg void OnBnClickedUseWeatherIconCheck();
    afx_msg void OnBnClickedUpdateWeatherButton();
};
