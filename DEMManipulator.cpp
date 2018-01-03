#include "stdafx.h"
#include "DEMManipulator.h"
#include "osg/ComputeBoundsVisitor"
#include "osg/BoundingBox"
#include "osg/Matrixd"
#include "osg/ShapeDrawable"
#include "osgGA/CameraManipulator"
#include "osgGA/TerrainManipulator"
#include "osgGA/GUIEventAdapter"
#include "osgGA/TrackballManipulator"
#include "osgUtil/RayIntersector"
#include "osg/ShapeDrawable"

using namespace osg;
using namespace osgGA;
using namespace osgViewer;
using namespace osgUtil;


/************************************************************************/
/*							    2D操作器                                */
/************************************************************************/
DEManipulator::DEManipulator()
{
}

DEManipulator::~DEManipulator()
{
}

DEManipulator::DEManipulator(osgViewer::Viewer* pViewer)
{
	m_pViewer = pViewer;
	m_bMidButtonPush = false;
	m_bLftButtonPush = false;
	m_bRhtButtonPush = false;
	if (pViewer)
	{
		m_pCamera = m_pViewer->getCamera();
	}
	m_InitiScreenX = m_InitiScreenY = 0.0;
}

void   DEManipulator::InitialWithEnvelope3D(const OGREnvelope3D& enve)
{
	m_InitEnve = enve;
	computeHomePosition();
	setHomePosition(m_HomeEye, m_HomeCenter, m_HomeUp);
	setTransformation(m_HomeEye, m_HomeCenter, m_HomeUp);
}

bool   DEManipulator::Client2World(Vec2d cPos, Vec2d& wPos)
{
	if (m_pViewer == NULL)
	{
		return false;
	}
	osg::Viewport* pPort = m_pCamera->getViewport();

	int screenx = pPort->width();
	int screeny = pPort->height();

	double l, r, b, t, n, f;
	m_pCamera->getProjectionMatrixAsOrtho(l, r, b, t, n, f);

	osg::Vec3d cen = getCenter();

	double minx = cen[0] + l;
	double maxx = cen[0] + r;
	double miny = cen[1] + b;
	double maxy = cen[1] + t;

	wPos._v[0] = minx + (cPos[0] / (double)screenx) * (r - l);
	wPos._v[1] = miny + (cPos[1] / (double)screeny) * (t - b);
	
	return true;
}

bool   DEManipulator::World2Client(Vec2d wPos, Vec2d& cPos)
{
	if (m_pViewer == NULL)
	{
		return false;
	}
	osg::Viewport* pPort = m_pCamera->getViewport();

	int screenx = pPort->width();
	int screeny = pPort->height();

	double l, r, b, t, n, f;
	m_pCamera->getProjectionMatrixAsOrtho(l, r, b, t, n, f);

	osg::Vec3d cen = getCenter();

	double minx = cen[0] + l;
	double maxx = cen[0] + r;
	double miny = cen[1] + b;
	double maxy = cen[1] + b;

	cPos._v[0] = (cen[0] - minx) * screenx / (r - l);
	wPos._v[1] = (maxy - cen[1]) * screeny / (t - b);

	return true;
}

void   DEManipulator::computeHomePosition()
{
	m_HomeEye    = osg::Vec3((m_InitEnve.MinX + m_InitEnve.MaxX) / 2.0, (m_InitEnve.MinY + m_InitEnve.MaxY) / 2.0, 10000.0);
	m_HomeCenter = osg::Vec3((m_InitEnve.MinX + m_InitEnve.MaxX) / 2.0, (m_InitEnve.MinY + m_InitEnve.MaxY) / 2.0, 0.0);
	m_HomeUp     = Vec3d(0.0, 1.0, 0.0);
	if (m_pCamera)
	{
		osg::Viewport* pPort = m_pCamera->getViewport();
		if (!pPort) return;

		double screenx = pPort->width();
		double screeny = pPort->height();
		double groundx = m_InitEnve.MaxX - m_InitEnve.MinX;
		double groundy = m_InitEnve.MaxY - m_InitEnve.MinY;
		double zoomx = groundx / screenx;
		double zoomy = groundy / screeny;
		if (zoomx > zoomy)
		{
			groundy = zoomx * screeny;
			m_ZoomRate = zoomx;
		}
		else
		{
			groundx = zoomy * screenx;
			m_ZoomRate = zoomy;
		}
		m_pCamera->setProjectionMatrixAsOrtho2D(-groundx / 1.5, groundx / 1.5, -groundy / 1.5, groundy / 1.5);
		m_InitiScreenX = screenx;
		m_InitiScreenY = screeny;
	}
}

void   DEManipulator::zoomModel(const float dy)
{
	osg::Vec2d ptWorldBefore, ptWorldAfter;  //修改投影矩阵前后鼠标位置对应的世界坐标点  
	float x1 = m_StartPos.x(), y1 = m_StartPos.y();
	osg::Vec2d vecWindow1(x1, y1);
	{
		Client2World(vecWindow1, ptWorldBefore);
	}
	double dFactor = 1;
	if (dy > 0)
	{
		dFactor = 1.2;
	}
	else
	{
		dFactor = 0.8;
	}
	double dL, dR, dT, dB, dZ, dF;
	m_pCamera->getProjectionMatrixAsOrtho(dL, dR, dB, dT, dZ, dF);

	double width, height;

	width = dR - dL;
	height = dT - dB;

	dR = width * dFactor;
	dT = height * dFactor;
	m_pCamera->setProjectionMatrixAsOrtho(-dR * 0.5, dR * 0.5, -dT * 0.5, dT * 0.5, dZ, dF);
	{
		Client2World(vecWindow1, ptWorldAfter);
	}
	osg::Vec2d vecMove = ptWorldBefore - ptWorldAfter;
	panModel(vecMove.x(), vecMove.y(), 0.0);
}

bool   DEManipulator::handleMouseWheel(const osgGA::GUIEventAdapter & ea, osgGA::GUIActionAdapter & us)
{
	osgGA::GUIEventAdapter::ScrollingMotion sm = ea.getScrollingMotion();

	m_StartPos.set(ea.getX(), ea.getY());
	Client2World(m_StartPos, m_StartPos2D);

	if (_flags & SET_CENTER_ON_WHEEL_FORWARD_MOVEMENT)
	{
		if (((sm == GUIEventAdapter::SCROLL_DOWN && _wheelZoomFactor > 0.)) || ((sm == GUIEventAdapter::SCROLL_UP   && _wheelZoomFactor < 0.)))
		{
			if (getAnimationTime() <= 0.)
			{
				setCenterByMousePointerIntersection(ea, us);
			}
			else
			{
				if (!isAnimating()) startAnimationByMousePointerIntersection(ea, us);
			}
		}
	}

	switch (sm)
	{
	case GUIEventAdapter::SCROLL_UP:
	{
		zoomModel(-_wheelZoomFactor);

		break;
	}
	case GUIEventAdapter::SCROLL_DOWN:
	{
		zoomModel(_wheelZoomFactor);
		break;
	}
	}

	us.requestRedraw();
	us.requestContinuousUpdate(isAnimating() || _thrown);
	return true;
}

bool   DEManipulator::performMovementRightMouseButton(const double eventTimeDelta, const double dx, const double dy)
{
	return true;
}

bool   DEManipulator::performMovementLeftMouseButton(const double eventTimeDelta, const double dx, const double dy)
{
	return true;
}

bool   DEManipulator::performMovementMiddleMouseButton(const double eventTimeDelta, const double dx, const double dy)
{
	if (m_pCamera)
	{
		double l = 0.0, r = 0.0, t = 0.0, b = 0.0, n = 0.0, f = 0.0;
		m_pCamera->getProjectionMatrixAsOrtho(l, r, b, t, n, f);
		Viewport* pViewport = m_pCamera->getViewport();
		double zoom = (r - l) / pViewport->width();
		_allowThrow = false;
		float scale = -zoom;
		m_pCamera->setProjectionMatrixAsOrtho2D(l + scale * dx, r + scale * dx, b + scale * dy, t + scale * dy);
	}
	return true;
}

bool   DEManipulator::performMovement()
{
	// return if less then two events have been added
	if (_ga_t0.get() == NULL || _ga_t1.get() == NULL)
		return false;

	// get delta time
	double eventTimeDelta = _ga_t0->getTime() - _ga_t1->getTime();
	if (eventTimeDelta < 0.)
	{
		OSG_WARN << "Manipulator warning: eventTimeDelta = " << eventTimeDelta << std::endl;
		eventTimeDelta = 0.;
	}

	float dx = _ga_t0->getXnormalized() - _ga_t1->getXnormalized();
	float dy = _ga_t0->getYnormalized() - _ga_t1->getYnormalized();

	if (dx == 0. && dy == 0.)
		return false;


	// call appropriate methods
	unsigned int buttonMask = _ga_t1->getButtonMask();
	if (buttonMask == GUIEventAdapter::LEFT_MOUSE_BUTTON)
	{
		return performMovementLeftMouseButton(eventTimeDelta, dx, dy);
	}
	else if (buttonMask == GUIEventAdapter::MIDDLE_MOUSE_BUTTON ||
		buttonMask == (GUIEventAdapter::LEFT_MOUSE_BUTTON | GUIEventAdapter::RIGHT_MOUSE_BUTTON))
	{
		return performMovementMiddleMouseButton(eventTimeDelta, _ga_t0->getX() - _ga_t1->getX(), _ga_t0->getY() - _ga_t1->getY());
	}
	else if (buttonMask == GUIEventAdapter::RIGHT_MOUSE_BUTTON)
	{
		return performMovementRightMouseButton(eventTimeDelta, dx, dy);
	}

	return false;
}
