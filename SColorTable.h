#ifndef _DOC_SMX_COLOR_TABLE_20151215_2216_
#define _DOC_SMX_COLOR_TABLE_20151215_2216_

#include <vector>

class CColor3f
{
public:
	CColor3f()
	{
		r = 0.0f;
		g = 0.0f;
		b = 0.0f;
	}
	CColor3f(float _r, float _g, float _b)
	{
		r = _r;
		g = _g;
		b = _b;
	}
	~CColor3f(){}
public:
	float r;
	float g;
	float b;
};

class CColor3u
{
public:
	CColor3u()
	{
		r = 0;
		g = 0;
		b = 0;
	}
	CColor3u(unsigned char _r, unsigned char _g, unsigned char _b)
	{
		r = _r;
		g = _g;
		b = _b;
	}
	~CColor3u(){}
public:
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

class CColorTable
{
public:
	 CColorTable();
	~CColorTable();
	void AddColor(float r, float g, float b)
	{
		m_OriginColor.push_back(CColor3f(r,g,b));
	}
	void ClearColor()
	{
		m_OriginColor.clear();
		m_ColorTable.clear();
	}
	const std::vector<CColor3f>& GetColorTable(int iCount)
	{
		if (iCount <= m_OriginColor.size())
		{
			return m_OriginColor;
		}
		else
		{
			int iIntervelCount = m_OriginColor.size() - 1;
			int iIntervel = iCount / iIntervelCount;
			for (int i = 0; i < iIntervelCount; ++i)
			{
				if (i != iIntervelCount - 1)
				{
					float deltr = (m_OriginColor[i + 1].r - m_OriginColor[i].r) / iIntervel;
					float deltg = (m_OriginColor[i + 1].g - m_OriginColor[i].g) / iIntervel;
					float deltb = (m_OriginColor[i + 1].b - m_OriginColor[i].b) / iIntervel;
					for (int j = 0; j < iIntervel; ++j)
					{
						m_ColorTable.push_back(CColor3f(m_OriginColor[i].r + deltr * j, m_OriginColor[i].g + deltg * j, m_OriginColor[i].b + deltb * j));
					}
				}
				else
				{
					iIntervel = iIntervel + iCount % iIntervelCount;
					float deltr = (m_OriginColor[i + 1].r - m_OriginColor[i].r) / iIntervel;
					float deltg = (m_OriginColor[i + 1].g - m_OriginColor[i].g) / iIntervel;
					float deltb = (m_OriginColor[i + 1].b - m_OriginColor[i].b) / iIntervel;
					for (int j = 0; j < iIntervel; ++j)
					{
						m_ColorTable.push_back(CColor3f(m_OriginColor[i].r + deltr * j, m_OriginColor[i].g + deltg * j, m_OriginColor[i].b + deltb * j));
					}
				}
			}
		}
		return m_ColorTable;
	}
private:
	std::vector<CColor3f> m_OriginColor;
	std::vector<CColor3f> m_ColorTable;
};

#endif
