#ifndef _OSGVIEW_H_20170816_
#define _OSGVIEW_H_20170816_

#include "osgInclude.h"
#include "osgGA/TrackballManipulator"
#include "osgGA/KeySwitchMatrixManipulator"
#include "DEMManipulator.h"
#include "Dem.h"
#include "DemReader.h"
#include "osgTerrain/GeometryTechnique"
#include "osgShadow/ShadowMap"
#include "osgShadow/ShadowTechnique"
#include "osgShadow/SoftShadowMap"
#include "osgShadow/ShadowedScene"

class CDemReader;
struct CameraDrawCallback;
struct RootUpdateCallback;
class OSGView
{
public:
	 OSGView(HWND hWnd);
	~OSGView();

	void InitOSG();
	void InitWithDEM(const std::string& filename);
	void InitSceneGraph(void);
	void InitCameraConfig(void);
	void SetupWindow(void);
	void SetupCamera(void);
	void PreFrameUpdate(void);
	void PostFrameUpdate(void);

	float GetZ(float x, float y);
	osg::ref_ptr<osg::MatrixTransform>& GetRootNode();
	osg::ref_ptr<osg::Group>& GetDEMTiles();
	osg::ref_ptr<osg::Geometry>&  GetSelectNode();
	osg::ref_ptr<osgTerrain::Terrain>& GetTerrain() { return m_Terrain; }
	osg::ref_ptr<osgTerrain::GeometryTechnique>& GetTerrainTechnique() { return m_TerrainTechnique; }

	void UpdateViewRange();
	CDemReader* GetReader() { return m_pDemReader; }
	osgViewer::Viewer* getViewer() { return mViewer; }
	DEManipulator* GetManipulator() { return m_pManipulator; }
	CDem* GetDEM() { return m_pDem; }

	CRITICAL_SECTION& GetRenderMutex() { return m_RenderMutex; }
	CRITICAL_SECTION& GetReaderMutex() { return m_ReaderMutex; }

private:
	HWND m_hWnd;
	osgViewer::Viewer* mViewer;
	osg::ref_ptr<osg::MatrixTransform> m_Root;
	osg::ref_ptr<osg::Geometry>  m_SelectNode;
	osg::ref_ptr<osg::Group> m_DEMTiles;
	osg::ref_ptr<osgTerrain::Terrain> m_Terrain;

	/************************************************************************/
	/*						         光照效果                               */
	/************************************************************************/
	osg::ref_ptr<osg::LightSource> m_LightSource;
	osg::ref_ptr<osg::Light> m_Light1;

	/************************************************************************/
	/*								 阴影效果                                */
	/************************************************************************/
	osg::ref_ptr<osgShadow::SoftShadowMap> m_SoftShadowMap;
	osg::ref_ptr<osgShadow::ShadowedScene> m_ShadowScene;


	osg::ref_ptr<osgTerrain::GeometryTechnique> m_TerrainTechnique;
	CRITICAL_SECTION m_ReaderMutex;
	CRITICAL_SECTION m_RenderMutex;
	std::string m_strDemPath;
	CDem* m_pDem;
	CDemReader* m_pDemReader;
	DEManipulator* m_pManipulator;


	/************************************************************************/
	/*						 主窗口相机移动回调函数                           */
	/************************************************************************/
	osg::ref_ptr<CameraDrawCallback> m_pCameraCallback;

	/************************************************************************/
	/*						   根节点更新回调函数                            */
	/************************************************************************/
	osg::ref_ptr<RootUpdateCallback> m_pRootUpdateCallback;
};

class CRenderingThread : public OpenThreads::Thread
{
public:
	CRenderingThread(OSGView* ptr);
	virtual ~CRenderingThread();

	virtual void run();

protected:
	OSGView* _ptr;
	bool _done;
};

struct CameraDrawCallback : public osg::Camera::DrawCallback
{
	CameraDrawCallback(OSGView* pView)
	{
		m_pView = pView;
	}
	CameraDrawCallback(){}
	CameraDrawCallback(const DrawCallback&, const CopyOp&) {}

	META_Object(DrawCallback, CameraDrawCallback);

	virtual void operator () (osg::RenderInfo& renderInfo) const
	{
		if (m_pView)
		{
			m_pView->UpdateViewRange();
		}
	}
	virtual void operator () (const osg::Camera& /*camera*/) const {}

private:
	OSGView* m_pView;
};

struct RootUpdateCallback : public osg::NodeCallback
{
public:
	virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
	{
		traverse(node, nv);
	}
};

#endif
