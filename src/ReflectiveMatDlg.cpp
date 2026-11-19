// ReflectiveMatDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "Ray.h"
#include "ReflectiveMatDlg.h"
#include "SetColorDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CReflectiveMatDlg dialog


CReflectiveMatDlg::CReflectiveMatDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CReflectiveMatDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CReflectiveMatDlg)
	m_nDiffuseReflection = 0.0;
	m_bFilterReflection = FALSE;
	//}}AFX_DATA_INIT
}


void CReflectiveMatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReflectiveMatDlg)
	DDX_Text(pDX, IDC_DIFFUSE_REFLECTION, m_nDiffuseReflection);
	DDX_Check(pDX, IDC_FILTER_REFLECTION, m_bFilterReflection);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CReflectiveMatDlg, CDialog)
	//{{AFX_MSG_MAP(CReflectiveMatDlg)
	ON_BN_CLICKED(IDC_SET_COLOR, OnSetColor)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReflectiveMatDlg message handlers

void CReflectiveMatDlg::OnSetColor() 
{
    CSetColorDlg    oDlg( &m_oColor, this );
    
    oDlg.DoModal();
}
