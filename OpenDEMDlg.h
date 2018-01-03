#pragma once
class COpenDEMDlg : public CDialogEx
{
	DECLARE_DYNAMIC(COpenDEMDlg)

public:
	COpenDEMDlg(CWnd* pParent, const CString& filename);   // 标准构造函数
	virtual ~COpenDEMDlg();

	enum { IDD = IDD_DIALOG_OPEN_DEM };

	int GetBandCount(){ return nBandCount; }
protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
public:
	CString m_NoDataValue;
	CString m_DataType;
	int nBandCount;
};
