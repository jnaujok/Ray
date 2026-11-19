// SpherePropertiesDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "Ray.h"
#include "SpherePropertiesDlg.h"
#include "SetColorDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpherePropertiesDlg dialog


CSpherePropertiesDlg::CSpherePropertiesDlg(  CVector *oPosition, FLT *nRadius, CWnd* pParent /*=NULL*/)
	: CDialog(CSpherePropertiesDlg::IDD, pParent)
{
    m_poPos = oPosition;
    m_pnRadius = nRadius;
    m_bColorChanged = FALSE;
    //{{AFX_DATA_INIT(CSpherePropertiesDlg)
	m_nX = m_poPos->GetX();
	m_nY = m_poPos->GetY();
	m_nZ = m_poPos->GetZ();
	m_nRadius = *m_pnRadius;
	//}}AFX_DATA_INIT
}


void CSpherePropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpherePropertiesDlg)
	DDX_Text(pDX, IDC_X_COORD, m_nX);
	DDX_Text(pDX, IDC_Y_COORD, m_nY);
	DDX_Text(pDX, IDC_Z_COORD, m_nZ);
	DDX_Text(pDX, IDC_RADIUS, m_nRadius);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSpherePropertiesDlg, CDialog)
	//{{AFX_MSG_MAP(CSpherePropertiesDlg)
	ON_BN_CLICKED(IDC_SET_COLOR, OnSetColor)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpherePropertiesDlg message handlers

void CSpherePropertiesDlg::OnOK() 
{
	UpdateData( TRUE );

    m_poPos->Set( m_nX, m_nY, m_nZ );

    *m_pnRadius = m_nRadius;

    CDialog::OnOK();
}

void CSpherePropertiesDlg::OnSetColor() 
{
    CSetColorDlg    oDlg( &m_oColor, this );

    if ( oDlg.DoModal() == IDOK )
        m_bColorChanged = TRUE;
}
