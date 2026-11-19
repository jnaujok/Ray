// DiffuseMaterialPropDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "Ray.h"
#include "DiffuseMaterialPropDlg.h"
#include "SetColorDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDiffuseMaterialPropDlg dialog


CDiffuseMaterialPropDlg::CDiffuseMaterialPropDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDiffuseMaterialPropDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDiffuseMaterialPropDlg)
	m_bCastDiffuse = FALSE;
	//}}AFX_DATA_INIT
}


void CDiffuseMaterialPropDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDiffuseMaterialPropDlg)
	DDX_Check(pDX, IDC_CAST_DIFFUSE, m_bCastDiffuse);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDiffuseMaterialPropDlg, CDialog)
	//{{AFX_MSG_MAP(CDiffuseMaterialPropDlg)
	ON_BN_CLICKED(IDC_SET_COLOR, OnSetColor)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDiffuseMaterialPropDlg message handlers

void CDiffuseMaterialPropDlg::OnSetColor() 
{
    CSetColorDlg    oDlg( &m_oColor, this );	

    oDlg.DoModal();
}
