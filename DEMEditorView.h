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
	/*						         ��ǰѡ��������                          */
	/************************************************************************/
	SelectRange* m_pCurSelectRange;    
	SelectRange* m_pCurSelectOld;
	/************************************************************************/
	/*								  OpenGL����                            */
	/************************************************************************/
	HDC   m_hDC;
	HGLRC m_hRC;
	 
	/************************************************************************/
	/*								 ��ʷ������¼                            */
	/************************************************************************/
	std::vector<SelectRange*> m_HistoryOp;

	/************************************************************************/
	/*						    ����߳�ʱ����Ļ���ռ�                     */
	/************************************************************************/
	float **m_pBufferData;

	/************************************************************************/
	/*					��ǰѡ�������ƽ���߳�(��ѡ������)                  */
	/************************************************************************/
	double m_CurSumHeight;
	int    m_nValidSelPointCount;

private:
	/************************************************************************/
	/*					 �ı�Ӱ�����ݣ��ײ����� + ���������ݣ�                */
	/************************************************************************/
	void UpdateData(SelectRange* pSelRange);

	/************************************************************************/
	/*					       ǿ������ĳ������ĸ߳�����                     */
	/************************************************************************/
	void ForceReloadTiles(const OGREnvelope& enve);

	/************************************************************************/
	/*					     ����ǰ����������ʷ������¼��ȥ                   */
	/************************************************************************/
	void Add2History();
	/************************************************************************/
	/*							   ���ñ༭״̬                              */
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

	//�����༭
	afx_msg void OnUndo();
	afx_msg void OnUpdateUndo(CCmdUI *pCmdUI);

	//��ƽ
	afx_msg void OnEditMakeFlat();
	afx_msg void OnEditMakeFlatHeight();

	//̧��
	afx_msg void OnEditMakeAdd();
	afx_msg void OnEditMakeAddHeight();

	//ѹ��
	afx_msg void OnEditMakeSub();
	afx_msg void OnEditMakeSubHeight();
};
