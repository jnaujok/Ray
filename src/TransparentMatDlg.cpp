// TransparentMatDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "Ray.h"
#include "TransparentMatDlg.h"
#include "SetColorDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTransparentMatDlg dialog


CTransparentMatDlg::CTransparentMatDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTransparentMatDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTransparentMatDlg)
	m_nIndexOfRefraction = 0.0;
	//}}AFX_DATA_INIT
}


void CTransparentMatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTransparentMatDlg)
	DDX_Text(pDX, IDC_DIFFUSE_REFLECTION, m_nIndexOfRefraction);
	DDV_MinMaxDouble(pDX, m_nIndexOfRefraction, 0., 10000.);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTransparentMatDlg, CDialog)
	//{{AFX_MSG_MAP(CTransparentMatDlg)
	ON_BN_CLICKED(IDC_SET_COLOR, OnSetColor)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTransparentMatDlg message handlers

void CTransparentMatDlg::OnSetColor() 
{
    CSetColorDlg    oDlg( &m_oColor, this );
    
    oDlg.DoModal();
}
