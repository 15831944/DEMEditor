#ifndef _DEMreader_H_20151212_ 
#define _DEMreader_H_20151212_
#include "Dem.h"
#include "OSGView.h"

class OSGView;
class ReadThread;

class CDemReader
{
	friend void DEMREADPROC(LPVOID para);
public:
	CDemReader(OSGView* pView);
   ~CDemReader();
public:
	void Clear();
	void ReLoadTiles(const OGREnvelope& enve);
	bool ResetWnd(const OGREnvelope& enve, int iLevel);
	void AttachDEM(CDem* pDem);
	int  GetCurTexSum()  const;
	int	 GetCurTileSum() const;
	CDEMTile* GetCurTile(int idx) const;
	const std::vector<CDEMTile*>& GetCurTiles() const;
	const std::vector<CDEMTile*>& GetBufTiles() const;


public:
	void Work();
private:
	CDem*				   m_DEM     ;
	OSGView*			   m_pView;
	std::vector<CDEMTile*> m_CurTile ;
	std::vector<CDEMTile*> m_BufTile ;
	OGREnvelope            m_Envelope;
	HANDLE				   m_hThread ;
	DWORD				   m_ThreadID;
	bool				   m_bStop   ;
	FILE*                  m_pLog;

	ReadThread*  m_ReaderThread;

};

class ReadThread : public OpenThreads::Thread
{
public:
	ReadThread(CDemReader* ptr)
	{
		_done = false;
		_ptr = ptr;
	}
	virtual ~ReadThread()
	{
		stop();
	}

	void stop()
	{
		_done = true;
		if (isRunning())
		{
			join();
		}
	}

	virtual void run()
	{
		if (!_ptr)
		{
			_done = true;
			return;
		}
		_done = false;
		do 
		{
			_ptr->Work();
		} while (!_done);
	}

protected:
	CDemReader* _ptr;
	bool _done;
};

#endif