#include "stdafx.h"
#include "PaintWindow.h"
#include "DEMEditor.h"
#include "OpenDEMDlg.h"
#include "DEMEditorDoc.h"
#include "DEMEditorView.h"
#include "MainFrm.h"
#include "Resource.h"
#include "GL/GLObjects.h"
#include "MosaicShader.h"
#include "Raster2File.h"
#include "HeightProcess.h"
#include "opencv2/imgproc.hpp"
#include "opencv2/opencv_modules.hpp"  
#include "opencv2/highgui/highgui.hpp"

#ifdef _DEBUG
#pragma comment(lib, "opencv_world310d.lib")
#else
#pragma comment(lib, "opencv_world310.lib")
#endif


IMPLEMENT_DYNCREATE(CDEMEditorView, CView)
BEGIN_MESSAGE_MAP(CDEMEditorView, CView)
	ON_COMMAND(ID_OPEN_DEM, &CDEMEditorView::OnOpenDem)
	ON_UPDATE_COMMAND_UI(ID_OPEN_DEM, &CDEMEditorView::OnUpdateOpenDem)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_RECT_SELECT, &CDEMEditorView::OnRectSelect)
	ON_UPDATE_COMMAND_UI(ID_RECT_SELECT, &CDEMEditorView::OnUpdateRectSelect)
	ON_COMMAND(ID_POLYGON_SELECT, &CDEMEditorView::OnPolygonSelect)
	ON_UPDATE_COMMAND_UI(ID_POLYGON_SELECT, &CDEMEditorView::OnUpdatePolygonSelect)
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONUP()

	ON_COMMAND(ID_UNDO, &CDEMEditorView::OnUndo)
	ON_UPDATE_COMMAND_UI(ID_UNDO, &CDEMEditorView::OnUpdateUndo)

	ON_COMMAND(ID_EDIT_TEXT_FLAT, &CDEMEditorView::OnEditMakeFlat)
	ON_COMMAND(ID_EDIT_MAKE_FLAT_HEIGHT, &CDEMEditorView::OnEditMakeFlatHeight)
	ON_COMMAND(ID_EDIT_TEXT_ADD, &CDEMEditorView::OnEditMakeAdd)
	ON_COMMAND(ID_EDIT_MAKE_ADD_HEIGHT, &CDEMEditorView::OnEditMakeAddHeight)
	ON_COMMAND(ID_EDIT_TEXT_SUB, &CDEMEditorView::OnEditMakeSub)
	ON_COMMAND(ID_EDIT_MAKE_SUB_HEIGHT, &CDEMEditorView::OnEditMakeSubHeight)
END_MESSAGE_MAP()

int nMaxPyramidLevel = 10;

//最邻近内插
template<typename T> void pyrDown(T* pSrcData, int nSCols, int nSRows, T* pDstData, int nDCols, int nDRows)
{
	const float NoData = -99999.0;
	for (int i = 0; i < nDRows; ++i)
	{
		for (int j = 0; j < nDCols; ++j)
		{
			int iidx = 2 * i, jidx = 2 * j;
			T a = pSrcData[iidx * nSCols + jidx];
			if (a == NoData)
				pDstData[i * nDCols + j] = NoData;
			else
				pDstData[i * nDCols + j] = a;
		}
	}
}

CDEMEditorView::CDEMEditorView()
{
	m_CurSelectOperation = NULL_SELECT;
	m_ThreadHandle = NULL;
	m_pOSGView = NULL;
	m_bStartGather = false;
	m_bStartDrage = false;
	m_PointsCount = 0;
	m_pCurSelectRange = new SelectRange();
	m_pCurSelectOld   = new SelectRange();
	int nMaxBufRange = MAX_SELECT_RANGE + 128;
	int nCurBufRange = nMaxBufRange;
	m_pBufferData = new float*[nMaxPyramidLevel];
	for (int i = 0; i < nMaxPyramidLevel; ++i)
	{
		m_pBufferData[i] = new float[nCurBufRange * nCurBufRange];
		nCurBufRange = nCurBufRange / 2;
	}
}

CDEMEditorView::~CDEMEditorView()
{
	if (m_pCurSelectRange)
	{
		delete m_pCurSelectRange; m_pCurSelectRange = NULL;
	}
	if (m_pCurSelectOld)
	{
		delete m_pCurSelectOld; m_pCurSelectOld = NULL;
	}
	for (int i = 0; i < m_HistoryOp.size(); ++i)
	{
		delete m_HistoryOp[i];
	}
	for (int i = 0; i < nMaxPyramidLevel; ++i)
	{
		delete m_pBufferData[i];
	}
	delete m_pBufferData;
	wglDeleteContext(m_hRC);
}

void CDEMEditorView::UpdateData(SelectRange* pSelRange)
{
	if (!pSelRange)
	{
		return;
	}
	if (!m_pOSGView)
	{
		return;
	}
	CDem* pDem = m_pOSGView->GetDEM();
	if (!pDem)
	{
		return;
	}
	GDALDataset* pDataset = pDem->GetDataSet();
	if (!pDataset)
	{
		return;
	}

	int nOvCount = pDem->GetOverviewCount();
	int nMaxZoom = 1 << nOvCount;
	int nDemCols = pDem->GetCols();
	int nDemRows = pDem->GetRows();
	int nStartCol = pSelRange->m_nStartCol;
	int nStartRow = pSelRange->m_nStartRow;
	int nCols = pSelRange->m_nCols;
	int nRows = pSelRange->m_nRows;
	/************************************************************************/
	/*                          改变原始层级数据                             */
	/************************************************************************/
	int nBandCount = 1;
	int pBandMap = 1;

	EnterCriticalSection(&m_pOSGView->GetReaderMutex());
	CPLErr err = pDataset->RasterIO(GF_Write, nStartCol, nStartRow, nCols, nRows, pSelRange->m_pHeightData,
		nCols, nRows, GDT_Float32, 1, &pBandMap, nBandCount * sizeof(float), sizeof(float) * nBandCount * nCols, 0);
	if (err != CE_None)
	{
		AfxMessageBox(_T("影像写入错误(2)."));
	}
	pDataset->FlushCache();
	LeaveCriticalSection(&m_pOSGView->GetReaderMutex());
	/************************************************************************/
	/*					       改变影像金字塔数据                            */
	/************************************************************************/
	nStartCol = nStartCol - (nStartCol % nMaxZoom);
	nStartRow = nStartRow - (nStartRow % nMaxZoom);
	if (nCols % nMaxZoom != 0) nCols = nCols + nMaxZoom - nCols % nMaxZoom + nMaxZoom;
	if (nRows % nMaxZoom != 0) nRows = nRows + nMaxZoom - nRows % nMaxZoom + nMaxZoom;
	if (nStartCol + nCols > nDemCols)
	{
		nCols = nDemCols - nStartCol;
	}
	if (nStartRow + nRows > nDemRows)
	{
		nRows = nDemRows - nStartRow;
	}
	if (nCols <= 0 || nRows <= 0)
	{
		return;
	}

	EnterCriticalSection(&m_pOSGView->GetReaderMutex());
	err = pDataset->RasterIO(GF_Read, nStartCol, nStartRow, nCols, nRows, m_pBufferData[0],
		nCols, nRows, GDT_Float32, nBandCount, &pBandMap, nBandCount * sizeof(float), sizeof(float) * nBandCount * nCols, 0);
	if (err != CE_None)
	{
		AfxMessageBox(_T("影像读取错误(3)."));
	}
	LeaveCriticalSection(&m_pOSGView->GetReaderMutex());

	EnterCriticalSection(&m_pOSGView->GetReaderMutex());
	int nZoomCols = nCols, nZoomRows = nRows;
	for (int i = 1; i <= nOvCount; ++i)
	{
		int nCurZoomCol = nZoomCols / 2 + nZoomCols % 2;
		int nCurZoomRow = nZoomRows / 2 + nZoomRows % 2;
		pyrDown<float>(m_pBufferData[i - 1], nZoomCols, nZoomRows, m_pBufferData[i], nCurZoomCol, nCurZoomRow);
		pDataset->RasterIO(GF_Write, nStartCol, nStartRow, nCols, nRows, m_pBufferData[i],
			nCurZoomCol, nCurZoomRow, GDT_Float32, nBandCount, &pBandMap, nBandCount * sizeof(float), sizeof(float) * nBandCount * nCurZoomCol, 0);
		nZoomCols = nCurZoomCol;
		nZoomRows = nCurZoomRow;
	}
	pDataset->FlushCache();
	LeaveCriticalSection(&m_pOSGView->GetReaderMutex());
	/************************************************************************/
	/*					       重新加载更新的纹理                            */
	/************************************************************************/
	OGREnvelope enve;
	enve.MinX = pSelRange->m_nStartCol;
	enve.MinY = pSelRange->m_nStartRow;
	enve.MaxX = enve.MinX + pSelRange->m_nCols;
	enve.MaxY = enve.MinY + pSelRange->m_nRows;
	ForceReloadTiles(enve);
}

void CDEMEditorView::ForceReloadTiles(const OGREnvelope& enve)
{
	m_pOSGView->GetReader()->ReLoadTiles(enve);
}

CDEMEditorDoc* CDEMEditorView::GetDocument() const
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CDEMEditorDoc)));
	return (CDEMEditorDoc*)m_pDocument;
}

BOOL CDEMEditorView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

void CDEMEditorView::OnDraw(CDC* /*pDC*/)
{
	CDEMEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;
}

void CDEMEditorView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{

}

int  CDEMEditorView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	return 0;
}

void CDEMEditorView::OnDestroy()
{
	CView::OnDestroy();
	delete m_ThreadHandle;
	if (m_pOSGView != 0) delete m_pOSGView;
}

void CDEMEditorView::OnInitialUpdate()
{
	CView::OnInitialUpdate();
	CDEMTile::Pool.NewPool(G_TileWidth, 128, 10);
	m_pOSGView = new OSGView(m_hWnd);
	m_pOSGView->InitOSG();
	m_ThreadHandle = new CRenderingThread(m_pOSGView);
	m_ThreadHandle->start();

	/************************************************************************/
	/*					      初始化OpenGL环境                              */
	/************************************************************************/

	m_hDC = NULL; //m_hRC = NULL;
	m_hDC = GetDC()->GetSafeHdc();
	if (m_hDC == NULL) return;
	//if (SetWindowPixelFormat(m_hDC) == FALSE) //初始化OSG过程中已经设置过像素格式
	//{
	//	ASSERT(0);
	//	return;
	//}
	if (CreateViewGLContext(m_hDC, m_hRC) == FALSE)
	{
		ASSERT(0);
		return;
	}
	//m_MainShader = LoadShaders(drawvert, drawfrag, m_hDC, m_hRC);

}

void CDEMEditorView::ResetEditState()
{
	/************************************************************************/
	/*							   编辑状态复位                               */
	/************************************************************************/
	m_bStartDrage = false;
	m_bStartGather = false;
	m_nValidSelPointCount = 0;
	m_CurSumHeight = 0.0;
	m_PointsCount = 0;
	/************************************************************************/
	/*							  历史编辑清除                              */
	/************************************************************************/
	for (int i = 0; i < m_HistoryOp.size(); ++i)
	{
		delete m_HistoryOp[i];
	}
	m_HistoryOp.clear();

	/************************************************************************/
	/*					        抹掉选择出的多边形                           */
	/************************************************************************/
	osg::Geometry* pGeometry = m_pOSGView->GetSelectNode();
	EnterCriticalSection(&m_pOSGView->GetRenderMutex());
	pGeometry->removePrimitiveSet(0, pGeometry->getNumPrimitiveSets());
	LeaveCriticalSection(&m_pOSGView->GetRenderMutex());
}

void CDEMEditorView::OnOpenDem()
{
	CFileDialog dlg(TRUE, _T(".prj"), "", OFN_CREATEPROMPT | OFN_PATHMUSTEXIST, _T("DEM文件(*.tif)|*.tif||"));
	if (IDOK == dlg.DoModal())
	{
		m_DemName = dlg.GetPathName();
		COpenDEMDlg dlg(this, m_DemName);
		if (dlg.DoModal() == IDOK)
		{
			if (dlg.GetBandCount() == 1)
			{
				m_pOSGView->InitWithDEM(std::string(m_DemName));
				float meanZ = (m_pOSGView->GetDEM()->GetMaxZ() + m_pOSGView->GetDEM()->GetMinZ()) / 2.0;
				CString str;
				str.Format("%f", meanZ);
				CMainFrame* pFrm = (CMainFrame *)theApp.GetMainWnd();
				pFrm->GetMakeFlatHeightEdit()->SetEditText(str);
				ResetEditState();
			}
			else
			{
				AfxMessageBox("DEM文件波段数必须为1.");
			}
		}
	}
}

void CDEMEditorView::OnUpdateOpenDem(CCmdUI *pCmdUI)
{

}

void CDEMEditorView::OnRectSelect()
{
	if (m_CurSelectOperation == RECT_SELECT)
	{
		m_CurSelectOperation = NULL_SELECT;
	}
	else
	{
		m_CurSelectOperation = RECT_SELECT;
	}
}

void CDEMEditorView::OnUpdateRectSelect(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_CurSelectOperation == RECT_SELECT);
}

void CDEMEditorView::OnPolygonSelect()
{
	if (m_CurSelectOperation == POLYGON_SELECT)
	{
		m_CurSelectOperation = NULL_SELECT;
	}
	else
	{
		m_CurSelectOperation = POLYGON_SELECT;
	}
}

void CDEMEditorView::OnUpdatePolygonSelect(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_CurSelectOperation == POLYGON_SELECT);
}

void CDEMEditorView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_pOSGView)
	{
		DEManipulator* pManipulator = m_pOSGView->GetManipulator();
		CDem* pDEM = m_pOSGView->GetDEM();
		if (pManipulator)
		{
			RECT rt;
			GetClientRect(&rt);
			int nHei = abs(rt.top - rt.bottom);
			Vec2d wPos;
			pManipulator->Client2World(Vec2d(point.x, nHei - point.y), wPos);
			if (pDEM)
			{
				EnterCriticalSection(&m_pOSGView->GetRenderMutex());
				((CMainFrame*)theApp.GetMainWnd())->SetXYZ(wPos._v[0], wPos._v[1], m_pOSGView->GetZ(wPos._v[0] - pDEM->GetMinX(), wPos._v[1] - pDEM->GetMinY()));
				LeaveCriticalSection(&m_pOSGView->GetRenderMutex());
			}
			if (m_bStartGather)
			{
				osg::Geometry* pGeometry = m_pOSGView->GetSelectNode();
				osg::Vec3Array* pArray = (Vec3Array*)pGeometry->getVertexArray();
				if (m_CurSelectOperation == POLYGON_SELECT)
				{
					pArray->at(m_PointsCount)._v[0] = wPos._v[0] - pDEM->GetMinX();
					pArray->at(m_PointsCount)._v[1] = wPos._v[1] - pDEM->GetMinY();
					pArray->at(m_PointsCount)._v[2] = G_MAXHeight;
					pArray->dirty();
					EnterCriticalSection(&m_pOSGView->GetRenderMutex());
					pGeometry->removePrimitiveSet(0, pGeometry->getNumPrimitiveSets());
					pGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, m_PointsCount + 1));
					LeaveCriticalSection(&m_pOSGView->GetRenderMutex());
				}
				else if (m_CurSelectOperation == RECT_SELECT)
				{

				}
			}
		}
	}
	CView::OnMouseMove(nFlags, point);
}

void CDEMEditorView::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_bStartDrage = true;
	DEManipulator* pManipulator = m_pOSGView->GetManipulator();
	if (!pManipulator)
	{
		return;
	}
	CDem* pDEM = m_pOSGView->GetDEM();
	RECT rt;
	GetClientRect(&rt);
	int nHei = abs(rt.top - rt.bottom);
	Vec2d wPos;
	pManipulator->Client2World(Vec2d(point.x, nHei - point.y), wPos);

	if (m_CurSelectOperation != NULL_SELECT)
	{
		if (!m_bStartGather)
		{
			m_PointsCount = 0;
			m_nValidSelPointCount = 0;
			m_CurSumHeight = 0.0;
			m_bStartGather = true;
			osg::Geometry* pGeometry = m_pOSGView->GetSelectNode();
			osg::Vec3Array* pArray = (Vec3Array*)pGeometry->getVertexArray();
			pArray->at(m_PointsCount)._v[0] = wPos._v[0] - pDEM->GetMinX();
			pArray->at(m_PointsCount)._v[1] = wPos._v[1] - pDEM->GetMinY();
			double CurHeight = m_pOSGView->GetZ(pArray->at(m_PointsCount)._v[0], pArray->at(m_PointsCount)._v[1]);
			if (CurHeight != pDEM->GetNoDataValue())
			{
				m_CurSumHeight += CurHeight;
				m_nValidSelPointCount++;
			}
			pArray->at(m_PointsCount)._v[2] = G_MAXHeight;
			m_PointsCount++;
			pArray->dirty();
			EnterCriticalSection(&m_pOSGView->GetRenderMutex());
			pGeometry->removePrimitiveSet(0, pGeometry->getNumPrimitiveSets());
			pGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, m_PointsCount));
			LeaveCriticalSection(&m_pOSGView->GetRenderMutex());
		}
		else
		{
			if (m_CurSelectOperation == POLYGON_SELECT)
			{
				osg::Geometry* pGeometry = m_pOSGView->GetSelectNode();
				osg::Vec3Array* pArray = (Vec3Array*)pGeometry->getVertexArray();
				pArray->at(m_PointsCount)._v[0] = wPos._v[0] - pDEM->GetMinX();
				pArray->at(m_PointsCount)._v[1] = wPos._v[1] - pDEM->GetMinY();
				pArray->at(m_PointsCount)._v[2] = G_MAXHeight;

				double CurHeight = m_pOSGView->GetZ(pArray->at(m_PointsCount)._v[0], pArray->at(m_PointsCount)._v[1]);
				if (CurHeight != pDEM->GetNoDataValue())
				{
					m_CurSumHeight += CurHeight;
					m_nValidSelPointCount++;
				}

				m_PointsCount++;
				pArray->dirty();
				EnterCriticalSection(&m_pOSGView->GetRenderMutex());
				pGeometry->removePrimitiveSet(0, pGeometry->getNumPrimitiveSets());
				pGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, m_PointsCount));
				LeaveCriticalSection(&m_pOSGView->GetRenderMutex());
			}
		}
	}
	CView::OnLButtonDown(nFlags, point);
}

void CDEMEditorView::OnRButtonDown(UINT nFlags, CPoint point)
{
	DEManipulator* pManipulator = m_pOSGView->GetManipulator();
	if (!pManipulator)
	{
		return;
	}
	CDem* pDEM = m_pOSGView->GetDEM();
	RECT rt;
	GetClientRect(&rt);
	int nHei = abs(rt.top - rt.bottom);
	Vec2d wPos;
	pManipulator->Client2World(Vec2d(point.x, nHei - point.y), wPos);

	if (m_CurSelectOperation != NULL_SELECT)
	{
		if (m_bStartGather)
		{
			osg::Geometry* pGeometry = m_pOSGView->GetSelectNode();
			osg::Vec3Array* pArray = (Vec3Array*)pGeometry->getVertexArray();
			pArray->at(m_PointsCount)._v[0] = wPos._v[0] - pDEM->GetMinX();
			pArray->at(m_PointsCount)._v[1] = wPos._v[1] - pDEM->GetMinY();
			pArray->at(m_PointsCount)._v[2] = G_MAXHeight;

			double CurHeight = m_pOSGView->GetZ(pArray->at(m_PointsCount)._v[0], pArray->at(m_PointsCount)._v[1]);
			if (CurHeight != pDEM->GetNoDataValue())
			{
				m_CurSumHeight += CurHeight;
				m_nValidSelPointCount++;
			}

			m_PointsCount++;
			pArray->dirty();
			EnterCriticalSection(&m_pOSGView->GetRenderMutex());
			pGeometry->removePrimitiveSet(0, pGeometry->getNumPrimitiveSets());
			pGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_LOOP, 0, m_PointsCount));
			LeaveCriticalSection(&m_pOSGView->GetRenderMutex());
			m_bStartGather = false;
			if (m_PointsCount <= 3)
			{
				EnterCriticalSection(&m_pOSGView->GetRenderMutex());
				pGeometry->removePrimitiveSet(0, pGeometry->getNumPrimitiveSets());
				LeaveCriticalSection(&m_pOSGView->GetRenderMutex());
				m_PointsCount = 0;
			}
			else
			{
				/************************************************************************/
				/*							更新置平的Z坐标                              */
				/************************************************************************/
				UpdateFlatZ();
				/************************************************************************/
				/*					        获取当前选中的范围                           */
				/************************************************************************/
				OGREnvelope selEnve;
				selEnve.MinX = DBL_MAX, selEnve.MaxX = -DBL_MAX, selEnve.MinY = DBL_MAX, selEnve.MaxY = -DBL_MAX;
				for (int i = 0; i < m_PointsCount; ++i)
				{
					if (pArray->at(i)._v[0] > selEnve.MaxX)
						selEnve.MaxX = pArray->at(i)._v[0];
					else if (pArray->at(i)._v[0] < selEnve.MinX)
						selEnve.MinX = pArray->at(i)._v[0];
					if (pArray->at(i)._v[1] > selEnve.MaxY)
						selEnve.MaxY = pArray->at(i)._v[1];
					else if (pArray->at(i)._v[1] < selEnve.MinY)
						selEnve.MinY = pArray->at(i)._v[1];
				}
				CDem* pDem = m_pOSGView->GetDEM();
				OGREnvelope demEnve = m_pOSGView->GetDEM()->GetEnvelope();
				demEnve.MaxX -= demEnve.MinX; demEnve.MinX = 0.0; demEnve.MaxY -= demEnve.MinY; demEnve.MinY = 0.0;
				double      demGSD = m_pOSGView->GetDEM()->GetGeoTrans()[1];
				selEnve.MinX -= demGSD; selEnve.MaxX += demGSD; selEnve.MinY -= demGSD; selEnve.MaxY += demGSD;
				Intersect(demEnve, selEnve, selEnve);
				//与实际的DEM范围求交集
				int nDEMCols = m_pOSGView->GetDEM()->GetCols();
				int nDEMRows = m_pOSGView->GetDEM()->GetRows();
				m_pCurSelectRange->m_nStartCol = max(0, int(selEnve.MinX / demGSD));
				m_pCurSelectRange->m_nStartRow = max(0, int((demEnve.MaxY - selEnve.MaxY) / demGSD));
				m_pCurSelectRange->m_nCols = (selEnve.MaxX - selEnve.MinX) / demGSD;
				m_pCurSelectRange->m_nRows = (selEnve.MaxY - selEnve.MinY) / demGSD;
				while (m_pCurSelectRange->m_nStartCol + m_pCurSelectRange->m_nCols > nDEMCols)
				{
					m_pCurSelectRange->m_nCols--;
				}
				while (m_pCurSelectRange->m_nStartRow + m_pCurSelectRange->m_nRows > nDEMRows)
				{
					m_pCurSelectRange->m_nRows--;
				}

				OGRPolygon* pPolygon = (OGRPolygon*)OGRGeometryFactory::createGeometry(wkbPolygon);
				OGRLinearRing* pRing = (OGRLinearRing*)OGRGeometryFactory::createGeometry(wkbLinearRing);

				for (int i = 0; i < m_PointsCount; ++i)
				{
					pRing->addPoint(pArray->at(i)._v[0], pArray->at(i)._v[1]);
				}
				pRing->closeRings();
				pPolygon->addRing(pRing);

				vector<Triangle> tris;
				GLuint trisVBO;
				DelaunayTriangleation(pPolygon, tris);

				PAINTWINDOWPARA pwpara;
				pwpara.hDC = m_hDC;
				pwpara.nFBOXSize = m_pCurSelectRange->m_nCols;
				pwpara.nFBOYSize = m_pCurSelectRange->m_nRows;
				pwpara.frag = drawfrag;
				pwpara.vert = drawvert;
				PaintWindow pw(pwpara);

				MultiThreadWGLMakeCurrent(pw.GetHdc(), pw.GetGlRC());
				GLuint vao = 0;
				glGenVertexArrays(1, &vao);
				GetTriangleBuffer(tris, trisVBO);

				pw.EraseBlackBoard(0.0, 0.0, 0.0, 0.0);
				pw.SetViewEnvelope(selEnve);
				pw.BeginDraw();
				pw.BeginDrawGeometry();
				glDisable(GL_DEPTH_TEST);
				glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
				DrawTriangles(tris.size(), trisVBO, vao);
				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
				pw.DownloadAlpha(m_pCurSelectRange->m_pHeightMask);
				pw.EndDraw();
				glDeleteBuffers(1, &trisVBO);
				glDeleteVertexArrays(1, &vao);
				MultiThreadWGLMakeCurrent(NULL, NULL);
				SwapData(m_pCurSelectRange->m_pHeightMask, m_pCurSelectRange->m_nCols, m_pCurSelectRange->m_nRows);
			}
		}
	}
	CView::OnRButtonDown(nFlags, point);
}

void CDEMEditorView::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bStartDrage = false;
	CView::OnLButtonUp(nFlags, point);
}

void CDEMEditorView::Add2History()
{
	SelectRange* pHistory = new SelectRange;

	pHistory->m_nStartCol = m_pCurSelectOld->m_nStartCol;
	pHistory->m_nStartRow = m_pCurSelectOld->m_nStartRow;
	pHistory->m_nRows = m_pCurSelectOld->m_nRows;
	pHistory->m_nCols = m_pCurSelectOld->m_nCols;

	memcpy(pHistory->m_pHeightData, m_pCurSelectOld->m_pHeightData, sizeof(float) * MAX_SELECT_RANGE * MAX_SELECT_RANGE);
	memcpy(pHistory->m_pHeightMask, m_pCurSelectOld->m_pHeightMask, MAX_SELECT_RANGE * MAX_SELECT_RANGE);

	m_HistoryOp.push_back(pHistory);

	if (m_HistoryOp.size() > 10)
	{
		delete m_HistoryOp[0];
		m_HistoryOp.erase(m_HistoryOp.begin());
	}
}

void CDEMEditorView::UpdateFlatZ()
{
	float z = m_CurSumHeight / m_nValidSelPointCount;
	CMainFrame* pMrf = (CMainFrame *)theApp.GetMainWnd();
	CMFCRibbonEdit* pHeightEdit = pMrf->GetMakeFlatHeightEdit();
	CString strHeight;
	strHeight.Format("%-5lf", z);
	pHeightEdit->SetEditText(strHeight);
}

void CDEMEditorView::OnEditMakeFlat()
{
	if (m_PointsCount >= 3)
	{
		CMainFrame* pMrf = (CMainFrame *)theApp.GetMainWnd();
		CMFCRibbonEdit* pHeightEdit = pMrf->GetMakeFlatHeightEdit();
		CString strHeight = pHeightEdit->GetEditText();
		float height = 0.0f;
		sscanf(strHeight, "%f", &height);
		GDALDataset* pDataset = m_pOSGView->GetDEM()->GetDataSet();
		if (pDataset)
		{
			int nBandCount = 1;
			int nBandMap = 1;
			pDataset->RasterIO(GF_Read, m_pCurSelectRange->m_nStartCol, m_pCurSelectRange->m_nStartRow,
				m_pCurSelectRange->m_nCols, m_pCurSelectRange->m_nRows, m_pCurSelectRange->m_pHeightData,
				m_pCurSelectRange->m_nCols, m_pCurSelectRange->m_nRows, GDT_Float32, nBandCount, &nBandMap, 0, 0, 0);
		}
		m_pCurSelectOld->m_nStartCol = m_pCurSelectRange->m_nStartCol;
		m_pCurSelectOld->m_nStartRow = m_pCurSelectRange->m_nStartRow;
		m_pCurSelectOld->m_nCols = m_pCurSelectRange->m_nCols;
		m_pCurSelectOld->m_nRows = m_pCurSelectRange->m_nRows;
		memcpy(m_pCurSelectOld->m_pHeightData, m_pCurSelectRange->m_pHeightData, sizeof(float)* MAX_SELECT_RANGE * MAX_SELECT_RANGE);
		HeightProcess_Flat<float>((float *)m_pCurSelectRange->m_pHeightData, m_pCurSelectRange->m_pHeightMask,
			m_pCurSelectRange->m_nCols, m_pCurSelectRange->m_nRows, height);
		UpdateData(m_pCurSelectRange);
		Add2History();
	}
}

void CDEMEditorView::OnEditMakeAdd()
{
	if (m_PointsCount >= 3)
	{
		CMainFrame* pMrf = (CMainFrame *)theApp.GetMainWnd();
		CMFCRibbonEdit* pHeightEditAdd = pMrf->GetMakeAddHeightEdit();
		CString strHeight = pHeightEditAdd->GetEditText();
		float height = 0.0f;
		sscanf(strHeight, "%f", &height);
		GDALDataset* pDataset = m_pOSGView->GetDEM()->GetDataSet();
		if (pDataset)
		{
			int nBandCount = 1;
			int nBandMap = 1;
			pDataset->RasterIO(GF_Read, m_pCurSelectRange->m_nStartCol, m_pCurSelectRange->m_nStartRow,
				m_pCurSelectRange->m_nCols, m_pCurSelectRange->m_nRows, m_pCurSelectRange->m_pHeightData,
				m_pCurSelectRange->m_nCols, m_pCurSelectRange->m_nRows, GDT_Float32, nBandCount, &nBandMap, 0, 0, 0);
		}
		m_pCurSelectOld->m_nStartCol = m_pCurSelectRange->m_nStartCol;
		m_pCurSelectOld->m_nStartRow = m_pCurSelectRange->m_nStartRow;
		m_pCurSelectOld->m_nCols = m_pCurSelectRange->m_nCols;
		m_pCurSelectOld->m_nRows = m_pCurSelectRange->m_nRows;
		memcpy(m_pCurSelectOld->m_pHeightData, m_pCurSelectRange->m_pHeightData, sizeof(float)* MAX_SELECT_RANGE * MAX_SELECT_RANGE);
		HeightProcess_Add<float>((float *)m_pCurSelectRange->m_pHeightData, m_pCurSelectRange->m_pHeightMask,
			m_pCurSelectRange->m_nCols, m_pCurSelectRange->m_nRows, height, m_pOSGView->GetDEM()->GetNoDataValue());
		UpdateData(m_pCurSelectRange);
		Add2History();
	}
}

void CDEMEditorView::OnEditMakeSub()
{
	if (m_PointsCount >= 3)
	{
		CMainFrame* pMrf = (CMainFrame *)theApp.GetMainWnd();
		CMFCRibbonEdit* pHeightEditSub = pMrf->GetMakeSubHeightEdit();
		CString strHeight = pHeightEditSub->GetEditText();
		float height = 0.0f;
		sscanf(strHeight, "%f", &height);
		GDALDataset* pDataset = m_pOSGView->GetDEM()->GetDataSet();
		if (pDataset)
		{
			int nBandCount = 1;
			int nBandMap = 1;
			pDataset->RasterIO(GF_Read, m_pCurSelectRange->m_nStartCol, m_pCurSelectRange->m_nStartRow,
				m_pCurSelectRange->m_nCols, m_pCurSelectRange->m_nRows, m_pCurSelectRange->m_pHeightData,
				m_pCurSelectRange->m_nCols, m_pCurSelectRange->m_nRows, GDT_Float32, nBandCount, &nBandMap, 0, 0, 0);
		}
		m_pCurSelectOld->m_nStartCol = m_pCurSelectRange->m_nStartCol;
		m_pCurSelectOld->m_nStartRow = m_pCurSelectRange->m_nStartRow;
		m_pCurSelectOld->m_nCols = m_pCurSelectRange->m_nCols;
		m_pCurSelectOld->m_nRows = m_pCurSelectRange->m_nRows;
		memcpy(m_pCurSelectOld->m_pHeightData, m_pCurSelectRange->m_pHeightData, sizeof(float)* MAX_SELECT_RANGE * MAX_SELECT_RANGE);
		HeightProcess_Sub<float>((float *)m_pCurSelectRange->m_pHeightData, m_pCurSelectRange->m_pHeightMask,
			m_pCurSelectRange->m_nCols, m_pCurSelectRange->m_nRows, height, m_pOSGView->GetDEM()->GetNoDataValue());
		UpdateData(m_pCurSelectRange);
		Add2History();
	}
}

void CDEMEditorView::OnUndo()
{
	if (m_HistoryOp.size() > 0)
	{
		UpdateData(m_HistoryOp[m_HistoryOp.size() - 1]);
		delete m_HistoryOp[m_HistoryOp.size() - 1];
		m_HistoryOp.pop_back();
	}
}

void CDEMEditorView::OnUpdateUndo(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_HistoryOp.size() > 0);
}

void CDEMEditorView::OnEditMakeFlatHeight()
{

}

void CDEMEditorView::OnEditMakeAddHeight()
{

}

void CDEMEditorView::OnEditMakeSubHeight()
{

}

