#include "StdAfx.h"
#include "Dem.h"
#include "osgDB/WriteFile"
#include <fstream>
#include <omp.h>
CTilePool CDEMTile::Pool;

inline void SwapData(void* pData, int nRowLen, int nRows)
{
	BYTE* pBData = (BYTE *)pData;
	unsigned char t = 0;
	for (int i = 0; i < nRows / 2; ++i)
	{
		for (int j = 0; j < nRowLen; ++j)
		{
			t = pBData[i * nRowLen + j];
			pBData[i * nRowLen + j] = pBData[(nRows - i - 1) * nRowLen + j];
			pBData[(nRows - i - 1) * nRowLen + j] = t;
		}
	}
}


CTilePool::CTilePool()
{
	m_OriCount = 0;
	m_BlkLen = 0;
	m_ExpandSize = 0;
}

CTilePool::~CTilePool()
{
	FreePool();
}

osg::ref_ptr<CTerrainTile> CTilePool::CreateTile()
{
	osg::ref_ptr<CTerrainTile> tile = new CTerrainTile;
	osg::Texture::FilterMode filter = osg::Texture::LINEAR;
	osg::ref_ptr<osgTerrain::HeightFieldLayer> heightlayer = new osgTerrain::HeightFieldLayer;
	osg::ref_ptr<osg::HeightField> hf = new osg::HeightField;
	hf->allocate(G_BufTileWidth, G_BufTileWidth);
	heightlayer->setHeightField(hf);
	heightlayer->setMagFilter(filter);
	heightlayer->setMinFilter(filter);
	osg::ref_ptr<osgTerrain::ImageLayer> imagelayer = new osgTerrain::ImageLayer;
	osg::ref_ptr<osg::Image> image = new osg::Image;
	image->allocateImage(G_BufTileWidth, G_BufTileWidth, 1, GL_RGB, GL_UNSIGNED_BYTE);
	imagelayer->setImage(image);
	imagelayer->setMagFilter(filter);
	imagelayer->setMinFilter(filter);
	tile->setElevationLayer(heightlayer);
	tile->setColorLayer(0, imagelayer);
	tile->setBlendingPolicy(osgTerrain::TerrainTile::ENABLE_BLENDING);
	tile->setTerrainTechnique(tile->m_TerrainTechnique);
	tile->setTreatBoundariesToValidDataAsDefaultValue(true);
	return tile;
}

void  CTilePool::NewPool(int iTileWith, int iInitialSize, int iExpandSize)
{
	FreePool();
	m_OriCount = iInitialSize;
	for (int i = 0; i < iInitialSize; ++i)
	{
		m_TerrainTiles.push_back(CreateTile());
	}
	m_ExpandSize = iExpandSize;
}

osg::ref_ptr<CTerrainTile> CTilePool::GetBlk()
{
	osg::ref_ptr<CTerrainTile> pReturn;
	if (m_TerrainTiles.size() <= 0)
	{
		for (int i = 0; i < m_ExpandSize; ++i)
		{
			m_TerrainTiles.push_back(CreateTile());
		}
		if (m_TerrainTiles.size())
		{
			pReturn = m_TerrainTiles.front();
			m_TerrainTiles.erase(m_TerrainTiles.begin());
		}
	}
	else
	{
		pReturn = m_TerrainTiles.front();
		m_TerrainTiles.erase(m_TerrainTiles.begin());
	}
	return pReturn;
}

void  CTilePool::ReturnBlk(osg::ref_ptr<CTerrainTile> tile)
{
	if (tile)
	{
		tile->releaseGLObjects();
		m_TerrainTiles.push_back(tile);
	}
}

int   CTilePool::GetResidualCount()
{
	return m_TerrainTiles.size();
}

void  CTilePool::FreePool()
{
	for (auto it = m_TerrainTiles.begin(); it != m_TerrainTiles.end(); ++it)
	{
		it->release();
	}
	m_TerrainTiles.clear();
}

int   CTilePool::GetPoolSize()
{
	return m_OriCount;
}

CDEMTile:: CDEMTile()
{
	iLevel = -1;
	pDEM   = NULL;
	bOK    = false;
	tile   = NULL;
	bForceReload = false;
}

CDEMTile::~CDEMTile()
{

}

void CDEMTile::LoadData()
{
	if (!tile)
	{
		tile = Pool.GetBlk();
	}

	int iBandMap  = 1;
	int iPixLen   = 0;
	GDALDataset* pDataSet = pDEM->GetDataSet();
	if (!pDataSet)
	{
		return;
	}
	double GeoTrans[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	pDataSet->GetGeoTransform(GeoTrans);
	if (pDataSet == NULL)
	{
		return;
	}
	GDALRasterBand* pBand = pDataSet->GetRasterBand(1);
	GDALDataType type     = pBand->GetRasterDataType();
	if (type == GDT_Byte)
	{
		iPixLen = 1;
	}
	else if (type == GDT_Int16)
	{
		iPixLen = 2;
	}
	else if (type == GDT_UInt16)
	{
		iPixLen = 2;
	}
	else if (type == GDT_Int32)
	{
		iPixLen = 4;
	}
	else if (type == GDT_UInt32)
	{
		iPixLen = 4;
	}
	else if (type == GDT_Float32)
	{
		iPixLen = 4;
	}
	int iRealCol, iRealRow;
	int iDEMCol = pDEM->GetCols();
	int iDEMRow = pDEM->GetRows();

	OGREnvelope indembuf;
	indembuf.MinX = indem.MinX - G_BufWidth;
	indembuf.MaxX = indem.MaxX + G_BufWidth;
	indembuf.MinY = indem.MinY - G_BufWidth;
	indembuf.MaxY = indem.MaxY + G_BufWidth;

	if (indembuf.MinX < 0)
	{
		indembuf.MinX = 0;
	}
	if (indembuf.MaxX > iDEMCol)
	{
		iRealCol = iDEMCol    - indembuf.MinX;
	}
	else
	{
		iRealCol = indembuf.MaxX - indembuf.MinX;
	}

	if (indembuf.MinY < 0)
	{
		indembuf.MinY = 0;
	}
	if (indembuf.MaxY > iDEMRow)
	{
		iRealRow = iDEMRow    - indembuf.MinY;
	}
	else
	{
		iRealRow = indembuf.MaxY - indembuf.MinY;
	}
	int iZoom = 1 << iLevel;
	int iZoomCol = iRealCol / iZoom;
	int iZoomRow = iRealRow / iZoom;
	OGREnvelope CurEnve;
	OGREnvelope enve = pDEM->GetEnvelope();
	CurEnve.MinX = GeoTrans[0] + GeoTrans[1] * indembuf.MinX - enve.MinX;
	CurEnve.MaxX = GeoTrans[0] + GeoTrans[1] * indembuf.MaxX - enve.MinX;
	CurEnve.MaxY = GeoTrans[3] - indembuf.MinY * GeoTrans[1] - enve.MinY;
	CurEnve.MinY = GeoTrans[3] - indembuf.MaxY * GeoTrans[1] - enve.MinY;

	BYTE *pHeight = tile->m_pHeighLayerData;
	BYTE *pColor  = tile->m_pColorLayerData;
	int nBandLen = G_BufTileWidth * G_BufTileWidth;
	int nBandLen3 = nBandLen * 3;
	for (int i = 0; i < nBandLen3; ++i)
	{
		pColor[i] = 200;
	}
	if (type == GDT_Float32)
	{
		for (int i = 0; i < nBandLen; ++i)
		{
			((float*)pHeight)[i] = -99999.0;
		}
	}
	pDataSet->RasterIO(GF_Read, indembuf.MinX, indembuf.MinY, iRealCol, iRealRow, pHeight, iZoomCol, iZoomRow, type, 1, &iBandMap, iPixLen, G_BufTileWidth * iPixLen, 0);
	SwapData(pHeight, G_BufTileWidth * sizeof(float), G_BufTileWidth);
	/************************************************************************/
	/*						          设置高程                              */
	/************************************************************************/
	osgTerrain::HeightFieldLayer* heighlayer = (osgTerrain::HeightFieldLayer*)tile->getElevationLayer();
	osg::HeightField* heightfield = heighlayer->getHeightField();
	heightfield->setXInterval(GeoTrans[1] * iZoom);
	heightfield->setYInterval(GeoTrans[1] * iZoom);
	heightfield->setSkirtHeight(0.1);
	heightfield->setBorderWidth(G_BufWidth * 2);
	
	heightfield->setOrigin(osg::Vec3(CurEnve.MinX, CurEnve.MinY, pDEM->GetMinZ()));
	if (type == GDT_Byte)
	{
		for (int i = 0; i < G_BufTileWidth; ++i)
		{
			for (int j = 0; j < G_BufTileWidth; ++j)
			{
				heightfield->setHeight(j, i, pHeight[i * G_BufTileWidth + j]);
			}
		}
	}
	else if (type == GDT_CInt16)
	{
		for (int i = 0; i < G_BufTileWidth; ++i)
		{
			for (int j = 0; j < G_BufTileWidth; ++j)
			{
				heightfield->setHeight(j, i, ((short*)pHeight)[i * G_BufTileWidth + j]);
			}
		}
	}
	else if (type == GDT_CInt32)
	{
		for (int i = 0; i < G_BufTileWidth; ++i)
		{
			for (int j = 0; j < G_BufTileWidth; ++j)
			{
				heightfield->setHeight(j, i, ((int*)pHeight)[i * G_BufTileWidth + j]);
			}
		}
	}
	else if (type == GDT_Float32)
	{
		for (int i = 0; i < G_BufTileWidth; ++i)
		{
			for (int j = 0; j < G_BufTileWidth; ++j)
			{
				heightfield->setHeight(j, i, ((float*)pHeight)[i * G_BufTileWidth + j]);
			}
		}
	}
	
	osg::ref_ptr<osgTerrain::ValidDataOperator> ValidValueOperator = new osgTerrain::NoDataValue(pDEM->GetNoDataValue());
	osg::ref_ptr<osgTerrain::Locator> CurLocater = new osgTerrain::Locator;
	CurLocater->setCoordinateSystemType(osgTerrain::Locator::PROJECTED); //航拍及无人机测图
	CurLocater->setTransformAsExtents(CurEnve.MinX, CurEnve.MinY, CurEnve.MaxX, CurEnve.MaxY);
	heighlayer->setLocator(CurLocater.get());
	heighlayer->setValidDataOperator(ValidValueOperator);
	/************************************************************************/
	/*						         设置色彩                               */
	/************************************************************************/
	osgTerrain::ImageLayer *colorlayer = (osgTerrain::ImageLayer*)tile->getColorLayer(0);
	colorlayer->setLocator(CurLocater.get());
	osg::Image* image = colorlayer->getImage();
	const std::vector<CColor3u>& colors = pDEM->GetColorTable3u();
	const int iColorLevel = colors.size() - 1;
	const float COLORINTERVEL = pDEM->GetZRange() / iColorLevel;
	const float minZ = pDEM->GetMinZ();
	const float maxZ = pDEM->GetMaxZ();
	
	if (type == GDT_Byte)
	{
		unsigned char* pTemp = (unsigned char*)pHeight;
		for (int j = 0; j < nBandLen; ++j)
		{
			int idx = ((pTemp[j] - minZ) / COLORINTERVEL);
			if (idx < 0) idx = 0;
			if (idx > iColorLevel) idx = iColorLevel;
			int ot = 3 * j;
			pColor[ot] = colors[idx].r;
			pColor[ot + 1] = colors[idx].g;
			pColor[ot + 2] = colors[idx].b;
		}
	}
	else if (type == GDT_Int16)
	{
		short* pTemp = (short*)pHeight;
		for (int j = 0; j < nBandLen; ++j)
		{
			int idx = ((pTemp[j] - minZ) / COLORINTERVEL);
			if (idx < 0) idx = 0;
			if (idx > iColorLevel) idx = iColorLevel;
			int ot = 3 * j;
			pColor[ot] = colors[idx].r;
			pColor[ot + 1] = colors[idx].g;
			pColor[ot + 2] = colors[idx].b;
		}
	}
	else if (type == GDT_UInt16)
	{
		unsigned short* pTemp = (unsigned short*)pHeight;
		for (int j = 0; j < nBandLen; ++j)
		{
			int idx = ((pTemp[j] - minZ) / COLORINTERVEL);
			if (idx < 0) idx = 0;
			if (idx > iColorLevel) idx = iColorLevel;
			int ot = 3 * j;
			pColor[ot] = colors[idx].r;
			pColor[ot + 1] = colors[idx].g;
			pColor[ot + 2] = colors[idx].b;
		}
	}
	else if (type == GDT_Int32)
	{
		int* pTemp = (int*)pHeight;
		for (int j = 0; j < nBandLen; ++j)
		{
			int idx = ((pTemp[j] - minZ) / COLORINTERVEL);
			if (idx < 0) idx = 0;
			if (idx > iColorLevel) idx = iColorLevel;
			int ot = 3 * j;
			pColor[ot] = colors[idx].r;
			pColor[ot + 1] = colors[idx].g;
			pColor[ot + 2] = colors[idx].b;
		}
	}
	else if (type == GDT_UInt32)
	{
		unsigned int* pTemp = (unsigned int*)pHeight;
		for (int j = 0; j < nBandLen; ++j)
		{
			int idx = ((pTemp[j] - minZ) / COLORINTERVEL);
			if (idx < 0) idx = 0;
			if (idx > iColorLevel) idx = iColorLevel;
			int ot = 3 * j;
			pColor[ot] = colors[idx].r;
			pColor[ot + 1] = colors[idx].g;
			pColor[ot + 2] = colors[idx].b;
		}
	}
	else if (type == GDT_Float32)
	{
		float* pTemp = (float*)pHeight;
		float noData = pDEM->GetNoDataValue();
		for (int j = 0; j < nBandLen; ++j)
		{
			if (pTemp[j] == noData)
			{
				continue;
			}
			int idx = ((pTemp[j] - minZ) / COLORINTERVEL);
			if (idx < 0) idx = 0;
			else if (idx > iColorLevel) idx = iColorLevel;
			int ot = 3 * j;
			pColor[ot] = colors[idx].r;
			pColor[ot + 1] = colors[idx].g;
			pColor[ot + 2] = colors[idx].b;
		}
	}
	image->setImage(G_BufTileWidth, G_BufTileWidth, 1, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, pColor, osg::Image::NO_DELETE);

	//十分关键的一句！！ 告知Tile内容已经被修改,需要重新生成渲染对象
	tile->dirtyBound();
	tile->setDirtyMask(osgTerrain::TerrainTile::ALL_DIRTY);
	/************************************************************************/
	/*				为Tile设置ID,x y从左下角排列起,用于边界统一               */
	/************************************************************************/
	osgTerrain::TileID tileID;
	tileID.level = iLevel;
	tileID.x = indem.MinX / iZoom / 256 + 1;
	tileID.y = 1000 - (indem.MinY / iZoom / 256 + 1);
	tile->setTileID(tileID);

	bForceReload = false;
	bOK = true;
}

void CDEMTile::FreeData()
{
	if (tile)
	{
		Pool.ReturnBlk(tile);
		tile = NULL;
		bOK = false;
	}
}

CDem::CDem()
{
	m_MaxZ  = -FLT_MAX;
	m_MinZ  =  FLT_MAX;
	m_Depth = 0;
	m_OverviewCount = -1;
	m_Cols = 0;
	m_Rows = 0;
	m_NoDataValue = -99999.0;
	m_pDataSet = NULL;
	CColorTable cb;
	cb.AddColor(0.4375, 0.5976, 0.349);
	cb.AddColor(0.9453, 0.9296, 0.6328);
	cb.AddColor(0.9453, 0.8046, 0.519);
	cb.AddColor(0.7578, 0.5468, 0.4843);
	cb.AddColor(1.0, 0.9453, 1.0);
	SetColorTable(cb.GetColorTable(2048));
}

CDem::~CDem()
{

	Clear();
}

void  CDem::Clear()
{
	Destroy();
	m_Cols  =  0;
	m_Rows  =  0;
	m_Depth = 0;
	m_MinZ  =  FLT_MAX;
	m_MaxZ  = -FLT_MAX;
	memset(m_Trans, 0, sizeof(double) * 6);
	m_FileName = _T("");
	GDALClose(m_pDataSet);
	m_pDataSet = NULL;
}

void  CDem::LoadDEM(const std::string& filename)
{
	Clear();
	m_FileName = filename;
	GDALAllRegister();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
	if (m_pDataSet)
	{
		GDALClose(m_pDataSet);
	}
	m_pDataSet = (GDALDataset *)GDALOpen(filename.c_str(), GA_Update);
	if (m_pDataSet == NULL)
	{
		GDALClose(m_pDataSet);
		return;
	}
	if (m_pDataSet->GetRasterCount() > 1)
	{
		AfxMessageBox(_T("DEM不能为多波段Tiff."));
		GDALClose(m_pDataSet);
		m_pDataSet = NULL;
		return;
	}
	m_Cols = m_pDataSet->GetRasterXSize();
	m_Rows = m_pDataSet->GetRasterYSize();

	m_pDataSet->GetGeoTransform(m_Trans);
	GDALRasterBand* pBand = m_pDataSet->GetRasterBand(1);
	m_DataType = pBand->GetRasterDataType();
	m_OverviewCount = pBand->GetOverviewCount();
	int iOverviewCount	   = pBand->GetOverviewCount();
	int iNeedOverviewCount = ceil(log(max(m_Cols, m_Rows) / 256.0) / log(2.0));
	bool bNeedRebuildPyramid = false;
	if (iNeedOverviewCount > iOverviewCount)
	{
		bNeedRebuildPyramid = true;
	}
	if (bNeedRebuildPyramid)
	{
		int iOverViewList[12] = { 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };
		m_pDataSet->BuildOverviews("NEAREST", iNeedOverviewCount, iOverViewList, 0, NULL, NULL, NULL);
	}
	m_pDataSet->FlushCache();
	/*-----从最接近4096的层级中,寻找最大高程值和最小高程值-----*/
	int iBestLevel = ceil(log(max(m_Cols, m_Rows) / 4096.0) / log(2.0));
	if (iBestLevel < 0)
	{
		iBestLevel = 0;
	}
	int iZoom = 1 << iBestLevel;
	int iZoomCols = ceil(m_Cols / (double)iZoom);
	int iZoomRows = ceil(m_Rows / (double)iZoom);
	int iDataLen = iZoomCols * iZoomRows;
	void* pBestData = NULL;
	int iPixLen = 0;
	if (m_DataType == GDT_Byte)
	{
		iPixLen = 1;
	}
	else if (m_DataType == GDT_Int16)
	{
		iPixLen = 2;
	}
	else if (m_DataType == GDT_UInt16)
	{
		iPixLen = 2;
	}
	else if (m_DataType == GDT_Int32)
	{
		iPixLen = 4;
	}
	else if (m_DataType == GDT_UInt32)
	{
		iPixLen = 4;
	}
	else if (m_DataType == GDT_Float32)
	{
		iPixLen = 4;
	}
	pBestData = new BYTE[iDataLen * iPixLen];
	memset(pBestData, 0, iDataLen * iPixLen);
	int iBandMap = 1;
	m_pDataSet->RasterIO(GF_Read, 0, 0, m_Cols, m_Rows, pBestData, iZoomCols, iZoomRows, m_DataType, 1, &iBandMap, iPixLen, iZoomCols * iPixLen, 0);
	if (m_DataType == GDT_Byte)
	{
		BYTE* pTemp = (BYTE*)pBestData;
		for (int i = 0; i < iDataLen; ++i)
		{
			if (pTemp[i] < m_MinZ) m_MinZ = pTemp[i];
			if (pTemp[i] > m_MaxZ) m_MaxZ = pTemp[i];
		}
	}
	else if (m_DataType == GDT_Int16)
	{
		short* pTemp = (short*)pBestData;
		for (int i = 0; i < iDataLen; ++i)
		{
			if (pTemp[i] < m_MinZ) m_MinZ = pTemp[i];
			if (pTemp[i] > m_MaxZ) m_MaxZ = pTemp[i];
		}
	}
	else if (m_DataType == GDT_UInt16)
	{
		unsigned short* pTemp = (unsigned short*)pBestData;
		for (int i = 0; i < iDataLen; ++i)
		{
			if (pTemp[i] < m_MinZ) m_MinZ = pTemp[i];
			if (pTemp[i] > m_MaxZ) m_MaxZ = pTemp[i];
		}
	}
	else if (m_DataType == GDT_Int32)
	{
		int* pTemp = (int*)pBestData;
		for (int i = 0; i < iDataLen; ++i)
		{
			if (pTemp[i] < -10000) continue;
			if (pTemp[i] < m_MinZ) m_MinZ = pTemp[i];
			if (pTemp[i] > m_MaxZ) m_MaxZ = pTemp[i];
		}
	}
	else if (m_DataType == GDT_UInt32)
	{
		unsigned int* pTemp = (unsigned int*)pBestData;
		for (int i = 0; i < iDataLen; ++i)
		{
			if (pTemp[i] < m_MinZ) m_MinZ = pTemp[i];
			if (pTemp[i] > m_MaxZ) m_MaxZ = pTemp[i];
		}
	}
	else if (m_DataType == GDT_Float32)
	{
		float* pTemp = (float*)pBestData;
		for (int i = 0; i < iDataLen; ++i)
		{
			if (pTemp[i] < -10000) continue;
			if (pTemp[i] < m_MinZ) m_MinZ = pTemp[i];
			if (pTemp[i] > m_MaxZ) m_MaxZ = pTemp[i];
		}
	}
	delete pBestData; pBestData = NULL;
	InitialQuadTree();
}

void  CDem::Search(const OGREnvelope& Env, int iLevel, std::vector<CDEMTile*>& tiles)
{
	/*-----虚拟影像坐标转换为影像坐标系-----*/
	OGREnvelope localenv;
	localenv.MinX = floor((Env.MinX - m_Trans[0]) / m_Trans[1]);
	localenv.MaxX = ceil((Env.MaxX - m_Trans[0]) / m_Trans[1]);
	localenv.MinY = floor((m_Trans[3] - Env.MaxY) / m_Trans[1]);
	localenv.MaxY = ceil((m_Trans[3] - Env.MinY) / m_Trans[1]);

	OGREnvelope imgenve;
	imgenve.MinX = 0;
	imgenve.MaxX = m_Cols;
	imgenve.MinY = 0;
	imgenve.MaxY = m_Rows;

	Intersect(localenv, imgenve, localenv);
	
	SearchQuadTree(&m_Root, localenv, iLevel, tiles);
}

GDALDataset* CDem::GetDataSet()
{
	return m_pDataSet;
}

float CDem::GetNoDataValue()
{
	return m_NoDataValue;
}

void CDem::SetNoDataValue(float v)
{
	m_NoDataValue = v;
}

float CDem::GetMinZ() const
{
	return m_MinZ;
}

float CDem::GetMaxZ() const
{
	return m_MaxZ;
}

float CDem::GetZRange() const
{
	return m_MaxZ - m_MinZ;
}

float CDem::GetZ(double x, double y)
{
	int nx = (x - m_Trans[0]) / m_Trans[1];
	int ny = (m_Trans[3] - y) / m_Trans[1];
	int nband = 1;
	if (m_DataType == GDT_Float32)
	{
		float hei = 0.0f;
		//m_pDataSet->RasterIO(GF_Read, nx, ny, 1, 1, &hei, 1, 1, GDT_Float32, 1, &nband, 0, 0, 0);
		return hei;
	}
}


const std::string& CDem::GetFileName() const
{
	return m_FileName;
}

double* CDem::GetGeoTrans()
{
	return m_Trans;
}

const OGREnvelope& CDem::GetEnvelope() const
{
	static OGREnvelope enve;
	enve.MinX = m_Trans[0];
	enve.MaxX = m_Trans[0] + m_Cols * m_Trans[1];
	enve.MaxY = m_Trans[3];
	enve.MinY = m_Trans[3] - m_Rows * m_Trans[1];
	return enve;
}

const OGREnvelope3D& CDem::GetEnvelope3D() const
{
	static OGREnvelope3D enve;
	enve.MinX = m_Trans[0];
	enve.MaxX = m_Trans[0] + m_Cols * m_Trans[1];
	enve.MaxY = m_Trans[3];
	enve.MinY = m_Trans[3] - m_Rows * m_Trans[1];
	enve.MinZ = m_MinZ;
	enve.MaxZ = m_MaxZ;
	return enve;
}

void CDem::SetColorTable(const std::vector<CColor3f>& table)
{
	m_Colors3f = table;
	for each (auto var in m_Colors3f)
	{
		m_Colors3u.push_back(CColor3u(var.r * 255, var.g * 255, var.b * 255));
	}
}

const std::vector<CColor3u>& CDem::GetColorTable3u()
{
	return m_Colors3u;
}

const std::vector<CColor3f>& CDem::GetColorTable3f()
{
	return m_Colors3f;
}

void CDem::InitialQuadTree()
{
	int iPyrLevel = ceil(log(double(max(m_Cols, m_Rows)) / G_TileWidth) / log(2.0));
	m_Depth = iPyrLevel + 1;
	m_Root.tile.indem.MinX = 0;
	m_Root.tile.indem.MinY = 0;
	m_Root.tile.indem.MaxX = G_TileWidth << iPyrLevel;
	m_Root.tile.indem.MaxY = G_TileWidth << iPyrLevel;
	m_Root.tile.iLevel = iPyrLevel;
	m_Root.tile.pDEM   = this;
	CreateQuadBranch(iPyrLevel - 1, m_Root.tile.indem, m_Root.children);
};

void CDem::CreateQuadBranch(int iDepth, const OGREnvelope& img, CDEMNode** ppNode)
{
	if (iDepth >= 0)
	{
		OGREnvelope childimg[4];
		SplitImgEnvelope(img, childimg);
		for (int i = 0; i < 4; ++i)
		{
			if (childimg[i].MinX < m_Cols || childimg[i].MinY < m_Rows)
			{
				ppNode[i] = new CDEMNode;
				ppNode[i]->tile.indem  = childimg[i];
				ppNode[i]->tile.iLevel = iDepth;
				ppNode[i]->tile.pDEM   = this;
				CreateQuadBranch(iDepth - 1, childimg[i], ppNode[i]->children);
			}
		}
	}
}

void CDem::SplitImgEnvelope(const OGREnvelope& Env, OGREnvelope* pChildEnv)
{
	pChildEnv[0].MinX = Env.MinX;
	pChildEnv[0].MinY = Env.MinY;
	pChildEnv[0].MaxX = (Env.MinX + Env.MaxX) / 2;
	pChildEnv[0].MaxY = (Env.MinY + Env.MaxY) / 2;
	pChildEnv[1].MinX = (Env.MinX + Env.MaxX) / 2;
	pChildEnv[1].MinY = Env.MinY;
	pChildEnv[1].MaxX = Env.MaxX;
	pChildEnv[1].MaxY = (Env.MinY + Env.MaxY) / 2;
	pChildEnv[2].MinX = Env.MinX;
	pChildEnv[2].MinY = (Env.MinY + Env.MaxY) / 2;
	pChildEnv[2].MaxX = (Env.MinX + Env.MaxX) / 2;
	pChildEnv[2].MaxY = Env.MaxY;
	pChildEnv[3].MinX = (Env.MinX + Env.MaxX) / 2;
	pChildEnv[3].MinY = (Env.MinY + Env.MaxY) / 2;
	pChildEnv[3].MaxX = Env.MaxX;
	pChildEnv[3].MaxY = Env.MaxY;
}

void CDem::Destroy()
{
	for (int i = 0; i < 4; ++i)
	{
		if (m_Root.children[i])
		{
			m_Root.tile.FreeData();
			DestroyNode(m_Root.children[i]);
			m_Root.children[i] = NULL;
		}
	}
}

void CDem::DestroyNode(CDEMNode* pNode)
{
	if (pNode == NULL)
	{
		return;
	}
	else
	{
		if (pNode->tile.tile)
		{
			pNode->tile.FreeData();
		}
		for (int i = 0; i < 4; ++i)
		{
			DestroyNode(pNode->children[i]);
		}
		delete pNode; pNode = NULL;
	}
}

void CDem::SearchQuadTree(CDEMNode* pStartNode, const OGREnvelope& Env, int iLevel, std::vector<CDEMTile*>& ResultItem)
{
	if (pStartNode == NULL)
	{
		return;
	}
	if (Intersect(pStartNode->tile.indem, Env) && pStartNode->tile.iLevel == iLevel)
	{
		ResultItem.push_back(&pStartNode->tile);
		return;
	}
	else if (pStartNode->tile.iLevel <= iLevel)
	{
		return;
	}
	else
	{
		for (int i = 0; i < 4; ++i)
		{
			SearchQuadTree(pStartNode->children[i], Env, iLevel, ResultItem);
		}
	}
}

int  CDem::GetCols() const
{
	return m_Cols;
}

int  CDem::GetRows() const
{
	return m_Rows;
}

int  CDem::GetOverviewCount() const
{
	return m_OverviewCount;
}

void CDem::PrintfNode(CDEMNode* pNode, std::ofstream* pOfs)
{
	if (pNode == NULL) return;
	else
	{
		(*pOfs) << pNode << std::endl;
		for (int i = 0; i < 4; ++i) PrintfNode(pNode->children[i], pOfs);
	}
}

void CDem::PrintfQuadTree()
{
	std::ofstream ofs;
	ofs.open("C:\\1.txt");
	PrintfNode(&m_Root, &ofs);
	ofs.close();
}