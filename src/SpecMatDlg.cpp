// SpecMatDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "Ray.h"
#include "SpecMatDlg.h"
#include "SetColorDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecMatDlg dialog


CSpecMatDlg::CSpecMatDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSpecMatDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSpecMatDlg)
	m_nIORValue = 0.0;
	m_bMaterialHighlights = FALSE;
	m_nPower = 0.0;
	//}}AFX_DATA_INIT
}


void CSpecMatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecMatDlg)
	DDX_Text(pDX, IDC_IOR_VALUE, m_nIORValue);
	DDV_MinMaxDouble(pDX, m_nIORValue, 1.e-003, 50.);
	DDX_Check(pDX, IDC_MATERIAL_HIGHLIGHT, m_bMaterialHighlights);
	DDX_Text(pDX, IDC_POWER_VALUE, m_nPower);
	DDV_MinMaxDouble(pDX, m_nPower, 0., 1.);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSpecMatDlg, CDialog)
	//{{AFX_MSG_MAP(CSpecMatDlg)
	ON_BN_CLICKED(IDC_SET_COLOR, OnSetColor)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecMatDlg message handlers

void CSpecMatDlg::OnSetColor() 
{
    CSetColorDlg    oDlg( &m_oColor, this );	

    oDlg.DoModal();
}
