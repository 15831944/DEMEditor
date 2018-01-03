#include "StdAfx.h"
#include "DemReader.h"


CDemReader::CDemReader(OSGView * pView)
{
	m_hThread = NULL;
	m_DEM = NULL;
	m_pView = pView;
	m_ReaderThread = new ReadThread(this);
	m_ReaderThread->setSchedulePriority(OpenThreads::Thread::THREAD_PRIORITY_MAX);
}

CDemReader::~CDemReader()
{
	Clear();
	delete m_ReaderThread;
	m_ReaderThread = NULL;
}

void CDemReader::AttachDEM(CDem* pDem)
{
	m_DEM = pDem;
	m_ReaderThread->start();
}

bool CDemReader::ResetWnd(const OGREnvelope& enve, int iLevel)
{
	if (enve.MinX == m_Envelope.MinX &&enve.MinY == m_Envelope.MinY &&enve.MaxX == m_Envelope.MaxX &&enve.MaxY == m_Envelope.MaxY)
	{
		return false;
	}
	if (m_DEM == NULL) return false;
	m_Envelope = enve;
	CRITICAL_SECTION& ReaderMutex = m_pView->GetReaderMutex();
	CRITICAL_SECTION& RenderMutex = m_pView->GetRenderMutex();
	/************************************************************************/
	/*						   将当前tile推入缓存tile                        */
	/************************************************************************/
	std::vector<CDEMTile*> vTempTileList = m_BufTile;
	if (m_CurTile.size())
	{
		int iCurTileCount = m_CurTile.size();
		int iBufTileCount = m_BufTile.size();
		if (iBufTileCount == 0)
		{
			for (int i = 0; i < iCurTileCount; ++i)
			{
				if (m_CurTile[i]->bOK) vTempTileList.push_back(m_CurTile[i]);
			}
		}
		else
		{
			int i = 0, j = 0;
			for (i = 0; i < iCurTileCount; ++i)
			{
				if (m_CurTile[i]->tile == NULL)
				{
					continue;
				}
				for (j = 0; j < iBufTileCount; ++j)
				{
					if (m_CurTile[i] == m_BufTile[j]) break;
				}
				if (j == iBufTileCount)
				{
					vTempTileList.push_back(m_CurTile[i]);
				}
			}
		}
	}
	m_BufTile = vTempTileList;

	EnterCriticalSection(&ReaderMutex);
	m_CurTile.clear();
	m_DEM->Search(m_Envelope, iLevel, m_CurTile);
	int iBufTileCount = m_BufTile.size();
	int iCurTileCount = m_CurTile.size();
	int iUnloadCount = 0;
	for (int i = 0; i < iCurTileCount; ++i)
	{
		if (m_CurTile[i]->tile == NULL)
		{
			iUnloadCount++;
		}
		else
		{
			for (auto it = m_BufTile.begin(); it != m_BufTile.end(); ++it)
			{
				if (m_CurTile[i] == *it)
				{
					m_BufTile.erase(it);
					break;
				}
			}
		}
	}
	int iNeedDelCount = iUnloadCount - CDEMTile::Pool.GetResidualCount();
    if (iNeedDelCount > 0)
	{
		CDEMTile* pTile = NULL;
		while (iNeedDelCount > 0 && m_BufTile.size() > 0)
		{
			pTile = m_BufTile[0];
			if (pTile->tile) --iNeedDelCount;
			pTile->FreeData();
			m_BufTile.erase(m_BufTile.begin());
		}
	}
	LeaveCriticalSection(&ReaderMutex);

	//删除掉根节点中与m_CurTiles不同的
	EnterCriticalSection(&RenderMutex);
	osg::Group* pDEMTiles = m_pView->GetDEMTiles();
	
	int i = 0, j = 0;
	for (i = 0; i < pDEMTiles->getNumChildren(); ++i)
	{
		for (j = 0; j < m_CurTile.size(); ++j)
		{
			if (pDEMTiles->getChild(i) == m_CurTile[j]->tile)
			{
				break;
			}
		}
		if (j == m_CurTile.size())
		{
			pDEMTiles->removeChild(i);
			pDEMTiles->dirtyBound();
		}
	}
	LeaveCriticalSection(&RenderMutex);
	return true;
}

void CDemReader::ReLoadTiles(const OGREnvelope& enve)
{
	CRITICAL_SECTION& RenderMutex = m_pView->GetRenderMutex();
	CRITICAL_SECTION& ReaderMutex = m_pView->GetReaderMutex();
	EnterCriticalSection(&RenderMutex);
	EnterCriticalSection(&ReaderMutex);
	for (int i = 0; i < m_CurTile.size(); ++i)
	{
		if (Intersect(enve, m_CurTile[i]->indem) && m_CurTile[i]->bOK)
		{
			m_CurTile[i]->bForceReload = true;
		}
	}
	for (int i = 0; i < m_BufTile.size(); ++i)
	{
		if (Intersect(enve, m_BufTile[i]->indem) && m_BufTile[i]->bOK)
		{
			m_BufTile[i]->bForceReload = true;
		}
	}
	LeaveCriticalSection(&ReaderMutex);
	LeaveCriticalSection(&RenderMutex);
}

void CDemReader::Clear()
{
	CRITICAL_SECTION& ReaderMutex = m_pView->GetReaderMutex();
	CRITICAL_SECTION& RenderMutex = m_pView->GetRenderMutex();
	EnterCriticalSection(&ReaderMutex);
	for each (auto var in m_CurTile)
	{
		var->FreeData();
		var->bOK = false;
	}
	for each(auto var in m_BufTile)
	{
		var->FreeData();
		var->bOK = false;
	}
	m_CurTile.clear();
	m_BufTile.clear();
	Sleep(100);
	LeaveCriticalSection(&ReaderMutex);
	m_ReaderThread->stop();
	m_Envelope.MinX = 0.0; m_Envelope.MaxX = 0.0;
	m_Envelope.MinY = 0.0; m_Envelope.MaxY = 0.0;
}

int  CDemReader::GetCurTexSum() const
{
	int iCurTileCount = m_CurTile.size();
	int iCurTexCount = 0;
	for (int i = 0; i < iCurTileCount; ++i)
	{
		if (m_CurTile[i]->tile)
		{
			iCurTexCount++;
		}
	}
	return iCurTexCount;
}

CDEMTile* CDemReader::GetCurTile(int idx) const
{
	return m_CurTile[idx];
}

const std::vector<CDEMTile*>& CDemReader::GetCurTiles() const
{
	return m_CurTile;
}

const std::vector<CDEMTile*>& CDemReader::GetBufTiles() const
{
	return m_BufTile;
}

int	 CDemReader::GetCurTileSum() const
{
	return m_CurTile.size();
}

void CDemReader::Work()
{
	CRITICAL_SECTION& ReaderMutex = m_pView->GetReaderMutex();
	CRITICAL_SECTION& RenderMutex = m_pView->GetRenderMutex();
	EnterCriticalSection(&ReaderMutex);
	vector<CDEMTile*> tiles(m_CurTile);
	LeaveCriticalSection(&ReaderMutex);

	for (auto it = tiles.begin(); it != tiles.end(); ++it)
	{
		EnterCriticalSection(&ReaderMutex);
		if (tiles.size() != m_CurTile.size())
		{
			LeaveCriticalSection(&ReaderMutex);
			return;
		}
		else
		{
			for (int i = 0; i < tiles.size(); ++i)
			{
				if (tiles[i] != m_CurTile[i])
				{
					LeaveCriticalSection(&ReaderMutex);
					return;
				}
			}
		}
		if ((*it)->tile == NULL)  //如果是空,需要载入Tile
		{
			(*it)->LoadData();
		}
		else if ((*it)->bForceReload && (*it)->bOK)
		{
			(*it)->LoadData();
			(*it)->tile->setTerrain(m_pView->GetTerrain());
			(*it)->tile->getTerrainTechnique()->init(1023, false);
			osgTerrain::TileID tileID1 = (*it)->tile->getTileID(); tileID1.x -= 1; //left
			osgTerrain::TileID tileID2 = (*it)->tile->getTileID(); tileID2.x += 1; //right
			osgTerrain::TileID tileID3 = (*it)->tile->getTileID(); tileID3.y -= 1; //bottom
			osgTerrain::TileID tileID4 = (*it)->tile->getTileID(); tileID4.y += 1; //top
			osgTerrain::TerrainTile* tile1 = (*it)->tile->getTerrain()->getTile(tileID1);
			if (tile1) tile1->getTerrainTechnique()->init(1023, false);
			osgTerrain::TerrainTile* tile2 = (*it)->tile->getTerrain()->getTile(tileID2);
			if (tile2) tile2->getTerrainTechnique()->init(1023, false);
			osgTerrain::TerrainTile* tile3 = (*it)->tile->getTerrain()->getTile(tileID3);
			if (tile3) tile3->getTerrainTechnique()->init(1023, false);
			osgTerrain::TerrainTile* tile4 = (*it)->tile->getTerrain()->getTile(tileID4);
			if (tile4) tile4->getTerrainTechnique()->init(1023, false);
			
		}
		LeaveCriticalSection(&ReaderMutex);


		EnterCriticalSection(&RenderMutex);
		if ((*it)->tile)
		{
			osg::Group* pDEMTiles = m_pView->GetDEMTiles();
			(*it)->tile->setTerrain(m_pView->GetTerrain());
			if (!pDEMTiles->containsNode((*it)->tile))
			{
				pDEMTiles->addChild((*it)->tile);
			}
			pDEMTiles->dirtyBound();
		}
		LeaveCriticalSection(&RenderMutex);
	}
}