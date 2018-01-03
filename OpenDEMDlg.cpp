// OpenDEMDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "DEMEditor.h"
#include "OpenDEMDlg.h"
#include "afxdialogex.h"
#include <GDAL/gdal_priv.h>
#include <GDAL/cpl_conv.h>

// COpenDEMDlg 对话框

IMPLEMENT_DYNAMIC(COpenDEMDlg, CDialogEx)

COpenDEMDlg::COpenDEMDlg(CWnd* pParent, const CString& filename)
	: CDialogEx(COpenDEMDlg::IDD, pParent)
	, m_NoDataValue(_T(""))
	, m_DataType(_T(""))
{
	m_NoDataValue = "-99999.0";
	GDALAllRegister();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
	GDALDataset* pDataset = (GDALDataset *)GDALOpen(filename, GA_ReadOnly);

	if (!pDataset)
	{
		return;
	}

	nBandCount = pDataset->GetRasterCount();

	GDALDataType t = pDataset->GetRasterBand(1)->GetRasterDataType();
	if (t == GDT_Float32)
	{
		m_DataType = "FLOAT32";
	}
	else if (t == GDT_Float64)
	{
		m_DataType = "FLOAT64";
	}
	else if (t == GDT_UInt16)
	{
		m_DataType = "UINT16";
	}
	else if (t == GDT_Byte)
	{
		m_DataType = "UINT8";
	}

	GDALClose(pDataset);
}

COpenDEMDlg::~COpenDEMDlg()
{
}

void COpenDEMDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_NODATA_VALUE, m_NoDataValue);
	DDX_Text(pDX, IDC_EDIT_DATA_TYPE, m_DataType);
}


BEGIN_MESSAGE_MAP(COpenDEMDlg, CDialogEx)
END_MESSAGE_MAP()
