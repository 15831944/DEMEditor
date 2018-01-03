#ifndef _DEMMANIPULATOR_H_20170816_
#define _DEMMANIPULATOR_H_20170816_

#include "osgInclude.h"
#include "osgGA/TrackballManipulator"
#include <GDAL/gdal_priv.h>
#include <GDAL/cpl_conv.h>

class DEManipulator : public osgGA::TrackballManipulator
{
public:
	 DEManipulator();
	~DEManipulator();

	DEManipulator(osgViewer::Viewer* pViewer);

	void InitialWithEnvelope3D(const OGREnvelope3D& enve);

	virtual void computeHomePosition();

	virtual void zoomModel(const float dy);

	virtual bool Client2World(Vec2d cPos, Vec2d& wPos);

	virtual bool World2Client(Vec2d wPos, Vec2d& cPos);

	virtual void fitView(){};

	bool performMovement();

private:

	osgViewer::Viewer* m_pViewer;

	Camera* m_pCamera;

	OGREnvelope3D m_InitEnve;

	double m_InitiScreenX, m_InitiScreenY;

	Vec2d m_StartPos, m_EndPos;
	Vec2d m_StartPos2D, m_EndPos2D;

	bool m_bMidButtonPush, m_bLftButtonPush, m_bRhtButtonPush;
	double m_ZoomRate;

	Vec3d m_HomeEye, m_HomeCenter, m_HomeUp;

private:

	virtual bool handleMouseWheel(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us);
	virtual bool performMovementRightMouseButton(const double eventTimeDelta, const double dx, const double dy);
	virtual bool performMovementLeftMouseButton(const double eventTimeDelta, const double dx, const double dy);
	virtual bool performMovementMiddleMouseButton(const double eventTimeDelta, const double dx, const double dy);

};


#endif