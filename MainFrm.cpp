#include "stdafx.h"
#include "DEMEditor.h"

#include "MainFrm.h"
#include "Resource.h"
IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnApplicationLook)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnUpdateApplicationLook)
END_MESSAGE_MAP()

CMainFrame::CMainFrame()
{
	theApp.m_nAppLook = theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_OFF_2007_BLUE);
}

CMainFrame::~CMainFrame()
{
}

void  CMainFrame::AddMainCategory()
{
	m_MainButton.SetImage(IDB_MAIN);
	m_MainButton.SetToolTipText(_T("主菜单"));
	m_wndRibbonBar.SetApplicationButton(&m_MainButton, CSize(48, 48));
	CMFCRibbonMainPanel* pMainPanel = m_wndRibbonBar.AddMainCategory(_T("主菜单"), IDB_MOSAICKER_LARGE, IDB_MOSAICKER_LARGE);
	pMainPanel->Add(new CMFCRibbonButton(ID_OPEN_DEM, _T("打开DEM"), 1, 1));
	//pMainPanel->Add(new CMFCRibbonButton(ID_POLYGON_SELECT, _T("多边形选择"), -1, 4));
	//pMainPanel->Add(new CMFCRibbonButton(ID_RECT_SELECT, _T("矩形选择"), -1, 5));
}

void  CMainFrame::AddFunctionCategory()
{
	CMFCRibbonCategory *pCategory = m_wndRibbonBar.AddCategory(_T("主功能区"), IDB_MOSAICKER_SMALL, IDB_MOSAICKER_LARGE);
	/*-----文件面板-----*/
	CMFCRibbonPanel* pFilePanel = pCategory->AddPanel(_T("文件"));
	pFilePanel->Add(new CMFCRibbonButton(ID_OPEN_DEM, _T("打开DEM"), -1, 1));
	/*-----操作面板-----*/
	CMFCRibbonPanel* pOperPanel = pCategory->AddPanel(_T("操作"));
	pOperPanel->Add(new CMFCRibbonButton(ID_UNDO, _T("撤销"), -1, 3));
	/*-----显示面板-----*/
	CMFCRibbonPanel* pShowPanel = pCategory->AddPanel(_T("显示"));
	pShowPanel->Add(new CMFCRibbonButton(ID_BUTTON_ZOOM_OUT, _T("放大"), 0, -1));
	pShowPanel->Add(new CMFCRibbonButton(ID_BUTTON_ZOOM_IN, _T("缩小"), 1, -1));
	pShowPanel->AddSeparator();
	pShowPanel->Add(new CMFCRibbonButton(ID_BUTTON_RECT_ZOOM_OUT, _T("区域放大"), 2, -1));
	pShowPanel->AddSeparator();
	pShowPanel->Add(new CMFCRibbonButton(ID_BUTTON_ZOOM_FIT_CLIENTSCREEN, _T("适应窗口"), 3, -1));
	pShowPanel->Add(new CMFCRibbonButton(ID_BUTTON_ZOOM_ORIGIN, _T("原始比例"), 3, -1));
	/*-----选择面板-----*/
	CMFCRibbonPanel* pSelectPanel = pCategory->AddPanel(_T("选择"));
	pSelectPanel->Add(new CMFCRibbonButton(ID_POLYGON_SELECT, _T("多边形选择"), -1, 4));
	//pEditPanel->Add(new CMFCRibbonButton(ID_RECT_SELECT, _T("矩形选择"), -1, 5));
	/*-----编辑面板-----*/
	CMFCRibbonPanel* pEditPanel = pCategory->AddPanel(_T("编辑"));
	m_pMakeFlatHeight = new CMFCRibbonEdit(ID_EDIT_MAKE_FLAT_HEIGHT, 70, _T("置平高度"), 2);
	m_pMakeAddHeight  = new CMFCRibbonEdit(ID_EDIT_MAKE_ADD_HEIGHT, 70,  _T("抬升高度"), 2);
	m_pMakeSubHeight  = new CMFCRibbonEdit(ID_EDIT_MAKE_SUB_HEIGHT, 70,  _T("压低高度"), 2);

	m_pMakeFlat = new CMFCRibbonButton(ID_EDIT_TEXT_FLAT, _T("置平"), 2, -1);
	m_pMakeAdd  = new CMFCRibbonButton(ID_EDIT_TEXT_ADD,  _T("抬升"), 2, -1);
	m_pMakeSub  = new CMFCRibbonButton(ID_EDIT_TEXT_SUB,  _T("压低"), 2, -1);

	pEditPanel->Add(m_pMakeFlat);
	pEditPanel->Add(m_pMakeAdd);
	pEditPanel->Add(m_pMakeSub);

	pEditPanel->AddSeparator();

	pEditPanel->Add(m_pMakeFlatHeight);
	pEditPanel->Add(m_pMakeAddHeight);
	pEditPanel->Add(m_pMakeSubHeight);


}

void  CMainFrame::AddStatusBar()
{
	m_wndStatusBar.AddElement(new CMFCRibbonButton(0, "坐标X:0.xxxxxxxxxxxxxxx"), "");
	m_wndStatusBar.AddElement(new CMFCRibbonButton(0, "坐标Y:0.xxxxxxxxxxxxxxx"), "");
	m_wndStatusBar.AddElement(new CMFCRibbonButton(0, "高程Z:0.xxxxxxxxxxxxxxx"), "");
}

void CMainFrame::SetXYZ(float x, float y, float z)
{
	char str[32];
	sprintf(str, "坐标X:%-8f", x);
	m_wndStatusBar.GetElement(0)->SetText(str);
	m_wndStatusBar.GetElement(0)->Redraw();
	sprintf(str, "坐标Y:%-8f", y);
	m_wndStatusBar.GetElement(1)->SetText(str);
	m_wndStatusBar.GetElement(1)->Redraw();
	sprintf(str, "高程Z:%-8f", z);
	m_wndStatusBar.GetElement(2)->SetText(str);
	m_wndStatusBar.GetElement(2)->Redraw();
	m_wndStatusBar.UpdateWindow();
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	CDockingManager::SetDockingMode(DT_SMART);

	m_wndRibbonBar.Create(this);
	m_wndStatusBar.Create(this);

	AddMainCategory();
	AddFunctionCategory();
	AddStatusBar();

	m_pMakeAddHeight->SetEditText(_T("0.2"));
	m_pMakeSubHeight->SetEditText(_T("-0.2"));

	OnApplicationLook(theApp.m_nAppLook);
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWndEx::PreCreateWindow(cs) )
		return FALSE;
	return TRUE;
}

void CMainFrame::OnApplicationLook(UINT id)
{
	CWaitCursor wait;

	theApp.m_nAppLook = id;

	switch (theApp.m_nAppLook)
	{
	case ID_VIEW_APPLOOK_WIN_2000:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));
		m_wndRibbonBar.SetWindows7Look(FALSE);
		break;

	case ID_VIEW_APPLOOK_OFF_XP:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOfficeXP));
		m_wndRibbonBar.SetWindows7Look(FALSE);
		break;

	case ID_VIEW_APPLOOK_WIN_XP:
		CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
		m_wndRibbonBar.SetWindows7Look(FALSE);
		break;

	case ID_VIEW_APPLOOK_OFF_2003:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
		CDockingManager::SetDockingMode(DT_SMART);
		m_wndRibbonBar.SetWindows7Look(FALSE);
		break;

	case ID_VIEW_APPLOOK_VS_2005:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2005));
		CDockingManager::SetDockingMode(DT_SMART);
		m_wndRibbonBar.SetWindows7Look(FALSE);
		break;

	case ID_VIEW_APPLOOK_VS_2008:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2008));
		CDockingManager::SetDockingMode(DT_SMART);
		m_wndRibbonBar.SetWindows7Look(FALSE);
		break;

	case ID_VIEW_APPLOOK_WINDOWS_7:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));
		CDockingManager::SetDockingMode(DT_SMART);
		m_wndRibbonBar.SetWindows7Look(TRUE);
		break;

	default:
		switch (theApp.m_nAppLook)
		{
		case ID_VIEW_APPLOOK_OFF_2007_BLUE:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_BLACK:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_SILVER:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_AQUA:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
			break;
		}

		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
		CDockingManager::SetDockingMode(DT_SMART);
		m_wndRibbonBar.SetWindows7Look(FALSE);
	}

	RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);

	theApp.WriteInt(_T("ApplicationLook"), theApp.m_nAppLook);
}

void CMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(theApp.m_nAppLook == pCmdUI->m_nID);
}

