#ifndef _HEIGHT_PROCESS_H_
#define _HEIGHT_PROCESS_H_

template<typename T>
inline void HeightProcess_Flat(T* pHeight, unsigned char* pMask, int nCols, int nRows, T hei)
{
	int nDataLen = nCols * nRows;
	for (int i = 0; i < nDataLen; ++i)
	{
		if (pMask[i]) pHeight[i] = hei;
	}
}

template<typename T>
inline void HeightProcess_Add(T* pHeight, unsigned char* pMask, int nCols, int nRows, T add, T unValidValue)
{
	int nDataLen = nCols * nRows;
	for (int i = 0; i < nDataLen; ++i)
	{
		if (pMask[i])
		{
			if (pHeight[i] != unValidValue)
				pHeight[i] += add;
		}
	}
}

template<typename T>
inline void HeightProcess_Sub(T* pHeight, unsigned char* pMask, int nCols, int nRows, T sub, T unValidValue)
{
	int nDataLen = nCols * nRows;
	for (int i = 0; i < nDataLen; ++i)
	{
		if (pMask[i])
		{
			if (pHeight[i] != unValidValue)
				pHeight[i] -= sub;
		}
	}
}

#endif
