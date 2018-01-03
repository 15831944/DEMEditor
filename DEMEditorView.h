#pragma once
#include "OSGView.h"

const int MAX_SELECT_RANGE = 4096;

class SelectRange
{
public:
	SelectRange()
	{
		m_nStartCol = m_nStartRow = m_nCols = m_nRows = -1;
		m_pHeightData = new BYTE[MAX_SELECT_RANGE * MAX_SELECT_RANGE * 4];
		m_pHeightMask = new BYTE[MAX_SELECT_RANGE * MAX_SELECT_RANGE * 1];
	}

	~SelectRange()
	{
		if (m_pHeightData)
		{
			delete m_pHeightData;  m_pHeightData = NULL;
		}
		if (m_pHeightMask)
		{
			delete m_pHeightMask;  m_pHeightMask = NULL;
		}
	}
	int m_nStartCol, m_nStartRow, m_nCols, m_nRows;
	
	BYTE* m_pHeightData;
	BYTE* m_pHeightMask;

};


class CDEMEditorView : public CView
{
protected:
	CDEMEditorView();
	DECLARE_DYNCREATE(CDEMEditorView)

public:
	virtual ~CDEMEditorView();
	virtual void OnDraw(CDC* pDC);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	CDEMEditorDoc* GetDocument() const;

	enum SELECT_OPERATION
	{
		RECT_SELECT,
		POLYGON_SELECT,
		NULL_SELECT
	};

	DECLARE_MESSAGE_MAP()

private:
	OSGView* m_pOSGView;
	CRenderingThread* m_ThreadHandle;
	SELECT_OPERATION  m_CurSelectOperation;
	CString m_DemName;
	bool m_bStartGather;
	bool m_bStartDrage;
	int  m_PointsCount;
	
	/************************************************************************/
	/*						         当前选定的区域                          */
	/************************************************************************/
	SelectRange* m_pCurSelectRange;    
	SelectRange* m_pCurSelectOld;
	/************************************************************************/
	/*								  OpenGL环境                            */
	/************************************************************************/
	HDC   m_hDC;
	HGLRC m_hRC;
	 
	/************************************************************************/
	/*								 历史操作记录                            */
	/************************************************************************/
	std::vector<SelectRange*> m_HistoryOp;

	/************************************************************************/
	/*						    处理高程时所需的缓存空间                     */
	/************************************************************************/
	float **m_pBufferData;

	/************************************************************************/
	/*					当前选中区域的平均高程(按选择点计算)                  */
	/************************************************************************/
	double m_CurSumHeight;
	int    m_nValidSelPointCount;

private:
	/************************************************************************/
	/*					 改变影像数据（底层数据 + 金字塔数据）                */
	/************************************************************************/
	void UpdateData(SelectRange* pSelRange);

	/************************************************************************/
	/*					       强制重载某个区域的高程数据                     */
	/************************************************************************/
	void ForceReloadTiles(const OGREnvelope& enve);

	/************************************************************************/
	/*					     将当前操作加入历史操作记录中去                   */
	/************************************************************************/
	void Add2History();
	/************************************************************************/
	/*							   重置编辑状态                              */
	/************************************************************************/
	void ResetEditState();

public:
	virtual void OnInitialUpdate();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnOpenDem();
	afx_msg void OnUpdateOpenDem(CCmdUI *pCmdUI);
	

	afx_msg void OnRectSelect();
	afx_msg void OnUpdateRectSelect(CCmdUI *pCmdUI);
	afx_msg void OnPolygonSelect();
	afx_msg void OnUpdatePolygonSelect(CCmdUI *pCmdUI);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

	void UpdateFlatZ();

	//撤销编辑
	afx_msg void OnUndo();
	afx_msg void OnUpdateUndo(CCmdUI *pCmdUI);

	//置平
	afx_msg void OnEditMakeFlat();
	afx_msg void OnEditMakeFlatHeight();

	//抬升
	afx_msg void OnEditMakeAdd();
	afx_msg void OnEditMakeAddHeight();

	//压低
	afx_msg void OnEditMakeSub();
	afx_msg void OnEditMakeSubHeight();
};
