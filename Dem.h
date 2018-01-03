#ifndef _DEM_H_20151210_
#define _DEM_H_20151210_
#include <string>
#include <vector>
#include <list>
#include "SColorTable.h"
#include <GDAL/gdal_priv.h>
#include <GDAL/cpl_conv.h>
#include "Geometry/GeometryOP.h"
#pragma comment(lib, "gdal_i.lib")

#include "osgInclude.h"
#include "osgTerrain/Terrain"
#include "osgTerrain/GeometryTechnique"

const int	G_TileWidth = 256;
const int   G_BufWidth = 0;
const int   G_BufTileWidth = G_TileWidth + 2 * G_BufWidth;
const float G_MAXHeight = 9999.0;


class CDEMTile;
class CDem    ;
class CTilePool;

class CTerrainTile : public osgTerrain::TerrainTile
{
public:
	CTerrainTile()
	{
		m_pColorLayerData = new BYTE[G_BufTileWidth * G_BufTileWidth * 3];
		m_pHeighLayerData = new BYTE[G_BufTileWidth * G_BufTileWidth * 4];
		m_TerrainTechnique = new osgTerrain::GeometryTechnique;
	}

	~CTerrainTile()
	{
		delete []m_pColorLayerData; m_pColorLayerData = NULL;
		delete []m_pHeighLayerData; m_pHeighLayerData = NULL;
	}

	BYTE* m_pColorLayerData;
	BYTE* m_pHeighLayerData;
	osg::ref_ptr<osgTerrain::GeometryTechnique> m_TerrainTechnique;
};

class CTilePool
{
public:
	CTilePool();

   ~CTilePool();

   static osg::ref_ptr<CTerrainTile> CreateTile();

   void  NewPool(int iTileWith, int iInitialSize, int iExpandSize);

   osg::ref_ptr<CTerrainTile> GetBlk();

   void  ReturnBlk(osg::ref_ptr<CTerrainTile> tile);

	int   GetResidualCount();

	int   GetPoolSize();

	void  FreePool();

private:
	int   m_OriCount;
	int   m_BlkLen;
	int   m_ExpandSize;
	std::vector<osg::ref_ptr<CTerrainTile>> m_TerrainTiles;
};

class CDEMTile
{
public:
	CDEMTile();

   ~CDEMTile();

public:
	int				  iLevel;
	OGREnvelope		  indem;
	bool			  bOK;
	bool			  bForceReload;
	osg::ref_ptr<CTerrainTile> tile;

	CDem*		pDEM;
	void LoadData();
	void FreeData();
	static CTilePool  Pool;
};

class CDEMNode
{
public:
	CDEMNode()
	{
		memset(children, 0, sizeof(void*)* 4);
	}
	~CDEMNode()
	{

	}
public:
	CDEMTile   tile;
	CDEMNode*   children[4];
};

class CDem
{
public:
	CDem();

   ~CDem();

    void Clear();

	void LoadDEM(const std::string& filename);

	void Search(const OGREnvelope& Env, int iLevel, std::vector<CDEMTile*>& tiles);

	void SetColorTable(const std::vector<CColor3f>& table);

	const std::vector<CColor3u>& GetColorTable3u();
	const std::vector<CColor3f>& GetColorTable3f();

	float    GetNoDataValue();

	float    GetMinX() { return m_Trans[0]; }

	float    GetMinY() { return m_Trans[3] + m_Rows * m_Trans[5]; }

	void     SetNoDataValue(float v);

	float	 GetMinZ()    const ;

	float	 GetMaxZ()    const ;

	float    GetZRange()  const ;

	int      GetCols()    const ;

	int      GetRows()    const ;

	int      GetOverviewCount() const;

	double*  GetGeoTrans();

	const OGREnvelope& GetEnvelope() const;

	const OGREnvelope3D& GetEnvelope3D() const;

	const std::string& GetFileName() const;

	GDALDataset* GetDataSet();

	float GetZ(double x, double y);

	void PrintfQuadTree();


private:

	void InitialQuadTree();

	void CreateQuadBranch(int iDepth, const OGREnvelope& img, CDEMNode** ppNode);

	void SplitImgEnvelope(const OGREnvelope& Env, OGREnvelope* pChildEnv);

	void Destroy();

	void DestroyNode(CDEMNode* pNode);

	void SearchQuadTree(CDEMNode* pStartNode, const OGREnvelope& Env, int iLevel, std::vector<CDEMTile*>& ResultItem);

	void PrintfNode(CDEMNode* pNode, std::ofstream* pOfs);

private:
	int					  m_Cols;
	int					  m_Rows;
	int					  m_OverviewCount;
	int					  m_Depth;
	double				  m_Trans[6];
	float                 m_NoDataValue;
	float				  m_MaxZ;
	float				  m_MinZ;
	CDEMNode			  m_Root;
	GDALDataType		  m_DataType;
	std::string			  m_FileName;

	GDALDataset*         m_pDataSet;

private:
	std::vector<CColor3f>  m_Colors3f;
	std::vector<CColor3u>  m_Colors3u;
};

#endif