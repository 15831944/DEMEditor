#include "stdafx.h"
#include "OSGView.h"
#include <string>
#include "osgDB/ReadFile"
#include "osgUtil/Optimizer"
#include "osgViewer/ViewerEventHandlers"
#include "osgViewer/api/Win32/GraphicsWindowWin32"
#include "osgDB/WriteFile"
#include "osg/LineWidth"
OSGView::OSGView(HWND hWnd) : m_hWnd(hWnd)
{
	mViewer = new osgViewer::Viewer;
	m_pManipulator = NULL;
	m_Root = NULL;
	m_pDem = new CDem;
	m_pDemReader = new CDemReader(this);
	m_pCameraCallback = new CameraDrawCallback(this);
	m_pRootUpdateCallback = new RootUpdateCallback;
	InitializeCriticalSection(&m_ReaderMutex);
	InitializeCriticalSection(&m_RenderMutex);
}

OSGView::~OSGView()
{
	if (m_pDemReader)
	{
		delete m_pDemReader; m_pDemReader = NULL;
	}
	mViewer->setDone(true);
	Sleep(1000);
	mViewer->stopThreading();
	delete mViewer;
}

osg::ref_ptr<osg::MatrixTransform>& OSGView::GetRootNode()
{
	return m_Root;
}

osg::ref_ptr<osg::Group>& OSGView::GetDEMTiles()
{
	return m_DEMTiles;
}

osg::ref_ptr<osg::Geometry>&  OSGView::GetSelectNode()
{
	return m_SelectNode;
}

void OSGView::InitOSG()
{
	InitSceneGraph();
	InitCameraConfig();
}

void OSGView::InitWithDEM(const std::string& filename)
{
	m_pDemReader->Clear();
	m_pDem->LoadDEM(filename);
	m_pDemReader->AttachDEM(m_pDem);
	OGREnvelope enve = m_pDem->GetEnvelope();
	m_Root->setMatrix(osg::Matrix::translate(enve.MinX, enve.MinY, 0.0));
	//m_Light1->setPosition(osg::Vec4((enve.MaxX + enve.MinX) / 2.0, (enve.MaxY + enve.MinY) / 2.0, 9999.0, 0.0));
	//m_Light1->setAmbient(osg::Vec4(0.5, 0.5, 0.5, 1.0));
	//m_Light1->setDiffuse(osg::Vec4(1.0, 1.0, 1.0, 1.0));
	m_pManipulator->InitialWithEnvelope3D(m_pDem->GetEnvelope3D());
}

void OSGView::InitSceneGraph(void)
{
	m_Root = new osg::MatrixTransform;
	m_DEMTiles = new osg::Group;
	m_SelectNode = new osg::Geometry;
	m_Terrain = new osgTerrain::Terrain;
	m_TerrainTechnique = new osgTerrain::GeometryTechnique;

	osg::ref_ptr<osg::Vec3Array> vertices  = new osg::Vec3Array(100);
	osg::ref_ptr<osg::Vec4Array> colors    = new osg::Vec4Array(1);
	osg::ref_ptr<osg::Vec3Array> normals   = new osg::Vec3Array(1);
	osg::ref_ptr<osg::LineWidth> linewidth = new osg::LineWidth(2.0f);

	colors->at(0)._v[0] = 1.0;
	colors->at(0)._v[1] = 0.0;
	colors->at(0)._v[2] = 0.0;
	colors->at(0)._v[3] = 0.0;

	normals->at(0)._v[0] = 0.0;
	normals->at(0)._v[1] = 0.0;
	normals->at(0)._v[2] = 1.0;

	m_SelectNode->setVertexArray(vertices);
	m_SelectNode->setColorArray(colors);
	m_SelectNode->setColorBinding(osg::Geometry::BIND_OVERALL);
	m_SelectNode->setNormalArray(normals);
	m_SelectNode->setNormalBinding(osg::Geometry::BIND_OVERALL);
	m_SelectNode->getOrCreateStateSet()->setAttributeAndModes(linewidth.get(), StateAttribute::ON);

	m_Root->addChild(m_DEMTiles);
	m_Root->addChild(m_SelectNode);
	m_TerrainTechnique->setFilterBias(0.5);
	m_Terrain->setTerrainTechniquePrototype(m_TerrainTechnique);
	m_Terrain->setVerticalScale(2.0);
	m_Terrain->setSampleRatio(1.0);
	m_Terrain->setBlendingPolicy(osgTerrain::TerrainTile::BlendingPolicy::ENABLE_BLENDING);
	m_Terrain->setEqualizeBoundaries(true);
}

void OSGView::UpdateViewRange()
{
	osg::Camera* pCamera = mViewer->getCamera();
	
	osg::Vec3d cen = m_pManipulator->getCenter();
	double l = 0.0, r = 0.0, b = 0.0, t = 0.0, n = 0.0, f = 0.0;
	pCamera->getProjectionMatrixAsOrtho(l, r, b, t, n, f);

	OGREnvelope enve;
	enve.MinX = cen[0] + l;
	enve.MaxX = cen[0] + r;
	enve.MinY = cen[1] + b;
	enve.MaxY = cen[1] + t;
	osg::Viewport* pViewPort = pCamera->getViewport();
	double iScreenX = pViewPort->width();
	double iScreenY = pViewPort->height();
	double zr = ((enve.MaxX - enve.MinX) / m_pDem->GetGeoTrans()[1]) / iScreenX;
	int iSerachLevel = int(floor(log(zr) / log(2.0)));
	if (iSerachLevel <= 0)
	{
		iSerachLevel = 0;
	}
	m_pDemReader->ResetWnd(enve, iSerachLevel);
}

void OSGView::InitCameraConfig(void)
{
	CRect rect; GetClientRect(m_hWnd, &rect);
	osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
	osg::ref_ptr<osg::Referenced> windata = new osgViewer::GraphicsWindowWin32::WindowData(m_hWnd);
	traits->x = 0;
	traits->y = 0;
	traits->width = rect.right - rect.left;
	traits->height = rect.bottom - rect.top;
	traits->windowDecoration = false;
	traits->doubleBuffer = true;
	traits->setInheritedWindowPixelFormat = true;
	traits->inheritedWindowData = windata;
	osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());

	osg::Camera* pCamera = mViewer->getCamera();
	pCamera->setGraphicsContext(gc.get());
	pCamera->setViewport(new Viewport(0, 0, traits->width, traits->height));
	pCamera->setPostDrawCallback(m_pCameraCallback);
	m_pManipulator = new DEManipulator(mViewer);
	mViewer->setCameraManipulator(m_pManipulator);
	pCamera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	pCamera->setClearColor(Vec4(200.0 / 255.0, 200.0 / 255.0, 200.0 / 255.0, 1));
	mViewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);
	mViewer->realize();
	mViewer->setReleaseContextAtEndOfFrameHint(false);
	mViewer->setSceneData(m_Root);
	mViewer->setKeyEventSetsDone(-99999);
	mViewer->setQuitEventSetsDone(true);
}

void OSGView::PreFrameUpdate()
{
}

void OSGView::PostFrameUpdate()
{
}

float OSGView::GetZ(float x, float y)
{
	int nTileCount = m_DEMTiles->getNumChildren();
	for (int i= 0; i < nTileCount; ++i)
	{
		osgTerrain::TerrainTile* pTile = (osgTerrain::TerrainTile*)m_DEMTiles->getChild(i);
		osg::HeightField* pHeightField = ((osgTerrain::HeightFieldLayer*)(pTile->getElevationLayer()))->getHeightField();
		osg::Vec3 origin = pHeightField->getOrigin();
		float gsd = pHeightField->getXInterval();
		int dx = (x - origin._v[0]) / gsd;
		int dy = (y - origin._v[1]) / gsd;
		if (dx >= 0 && dy >= 0 && dx < G_TileWidth && dy < G_TileWidth)
		{
			return pHeightField->getHeight(dx, dy);
		}
	}
	return -99999.0;
}


CRenderingThread::CRenderingThread(OSGView* ptr) : OpenThreads::Thread(), _ptr(ptr), _done(false)
{
}

CRenderingThread::~CRenderingThread()
{
	_done = true;
	if (isRunning())
	{
		cancel();
		join();
	}
}

void CRenderingThread::run()
{
	if (!_ptr)
	{
		_done = true;
		return;
	}

	osgViewer::Viewer* viewer = _ptr->getViewer();
	do
	{
		EnterCriticalSection(&_ptr->GetRenderMutex());
		viewer->frame();
		LeaveCriticalSection(&_ptr->GetRenderMutex());
		Sleep(1);
	} while (!testCancel() && !viewer->done() && !_done);
}

