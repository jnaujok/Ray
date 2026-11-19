// PlanePropertiesDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "Ray.h"
#include "PlanePropertiesDlg.h"
#include "SetColorDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPlanePropertiesDlg dialog


CPlanePropertiesDlg::CPlanePropertiesDlg( CVector *oPosition, CVector *oNormal, CWnd* pParent /*=NULL*/)
	: CDialog(CPlanePropertiesDlg::IDD, pParent)
{

    m_poPos = oPosition;
    m_poNorm = oNormal;
    m_bColorChanged = FALSE;
	//{{AFX_DATA_INIT(CPlanePropertiesDlg)
	m_nI = m_poNorm->GetX();
	m_nJ = m_poNorm->GetY();
	m_nK = m_poNorm->GetZ();
	m_nX = m_poPos->GetX();
	m_nY = m_poPos->GetY();
	m_nZ = m_poPos->GetZ();
	//}}AFX_DATA_INIT
}


void CPlanePropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPlanePropertiesDlg)
	DDX_Text(pDX, IDC_NORMAL_I, m_nI);
	DDX_Text(pDX, IDC_NORMAL_J, m_nJ);
	DDX_Text(pDX, IDC_NORMAL_K, m_nK);
	DDX_Text(pDX, IDC_X_COORD, m_nX);
	DDX_Text(pDX, IDC_Y_COORD, m_nY);
	DDX_Text(pDX, IDC_Z_COORD, m_nZ);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPlanePropertiesDlg, CDialog)
	//{{AFX_MSG_MAP(CPlanePropertiesDlg)
	ON_BN_CLICKED(IDC_SET_COLOR, OnSetColor)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPlanePropertiesDlg message handlers

void CPlanePropertiesDlg::OnOK() 
{
	UpdateData( TRUE );

    m_poNorm->Set( m_nI, m_nJ, m_nK, SQRT( ( m_nI * m_nI ) + ( m_nJ * m_nJ ) + ( m_nK * m_nK ) ) );
    m_poPos->Set( m_nX, m_nY, m_nZ );

    CDialog::OnOK();
}

void CPlanePropertiesDlg::OnSetColor() 
{
    CSetColorDlg    oDlg( &m_oColor, this );

    if ( oDlg.DoModal() == IDOK )
        m_bColorChanged = TRUE;
}
