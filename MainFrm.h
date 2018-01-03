#pragma once

class CMainFrame : public CFrameWndEx
{
	
protected: // 仅从序列化创建
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

public:
	virtual ~CMainFrame();

	CMFCRibbonEdit* GetMakeFlatHeightEdit() { return m_pMakeFlatHeight; }
	CMFCRibbonEdit* GetMakeAddHeightEdit()  { return m_pMakeAddHeight; }
	CMFCRibbonEdit* GetMakeSubHeightEdit()  { return m_pMakeSubHeight; }
public:
	void SetXYZ(float x, float y, float z);

protected:  // 控件条嵌入成员
	CMFCRibbonBar     m_wndRibbonBar;
	CMFCRibbonApplicationButton m_MainButton;
	CMFCToolBarImages m_PanelImages;
	CMFCRibbonStatusBar  m_wndStatusBar;


	CMFCRibbonEdit* m_pMakeFlatHeight;
	CMFCRibbonEdit* m_pMakeAddHeight;
	CMFCRibbonEdit* m_pMakeSubHeight;

	CMFCRibbonButton* m_pMakeFlat;
	CMFCRibbonButton* m_pMakeSub;
	CMFCRibbonButton* m_pMakeAdd;

protected:

	void	AddMainCategory();
	void    AddFunctionCategory();
	void    AddStatusBar();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnApplicationLook(UINT id);
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()

};


