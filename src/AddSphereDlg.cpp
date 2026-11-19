// AddSphereDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "Ray.h"
#include "Objects.h"
#include "AddSphereDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddSphereDlg dialog


CAddSphereDlg::CAddSphereDlg( CVector *poPosition, FLT *pnRadius, CWnd* pParent /*=NULL*/)
	: CDialog(CAddSphereDlg::IDD, pParent)
{
    m_pnRadius = pnRadius;
    m_poPosition = poPosition;
    *m_pnRadius = 1.0;
	//{{AFX_DATA_INIT(CAddSphereDlg)
	m_nRadius = (double)*m_pnRadius;
	m_nX = m_poPosition->GetX();
	m_nY = m_poPosition->GetX();
	m_nZ = m_poPosition->GetX();
	//}}AFX_DATA_INIT
}


void CAddSphereDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddSphereDlg)
	DDX_Text(pDX, IDC_RADIUS, m_nRadius);
	DDX_Text(pDX, IDC_X_COORD, m_nX);
	DDX_Text(pDX, IDC_Y_COORD, m_nY);
	DDX_Text(pDX, IDC_Z_COORD, m_nZ);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddSphereDlg, CDialog)
	//{{AFX_MSG_MAP(CAddSphereDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddSphereDlg message handlers

void CAddSphereDlg::OnOK() 
{
	UpdateData( TRUE );

    *m_pnRadius = m_nRadius;

    m_poPosition->Set( m_nX, m_nY, m_nZ );
	
	CDialog::OnOK();
}
