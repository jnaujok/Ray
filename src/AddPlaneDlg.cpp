// AddPlaneDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "Ray.h"
#include "Objects.h"
#include "AddPlaneDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddPlaneDlg dialog


CAddPlaneDlg::CAddPlaneDlg( CVector * poPosition, CVector *poNormal, CWnd* pParent /*=NULL*/)
	: CDialog(CAddPlaneDlg::IDD, pParent)
{
    m_poPosition = poPosition;
    m_poNormal = poNormal;
	//{{AFX_DATA_INIT(CAddPlaneDlg)
	m_nI = m_poNormal->GetX();
	m_nJ = m_poNormal->GetY();
	m_nK = m_poNormal->GetZ();
	m_nX = m_poPosition->GetX();
	m_nY = m_poPosition->GetY();
	m_nZ = m_poPosition->GetZ();
	//}}AFX_DATA_INIT
}


void CAddPlaneDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddPlaneDlg)
	DDX_Text(pDX, IDC_NORMAL_I, m_nI);
	DDX_Text(pDX, IDC_NORMAL_J, m_nJ);
	DDX_Text(pDX, IDC_NORMAL_K, m_nK);
	DDX_Text(pDX, IDC_X_COORD, m_nX);
	DDX_Text(pDX, IDC_Y_COORD, m_nY);
	DDX_Text(pDX, IDC_Z_COORD, m_nZ);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddPlaneDlg, CDialog)
	//{{AFX_MSG_MAP(CAddPlaneDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddPlaneDlg message handlers

void CAddPlaneDlg::OnOK() 
{
    UpdateData( TRUE );
	
    m_poPosition->Set( m_nX, m_nY, m_nZ );
        // Store normal and normalize it.
    m_poNormal->Set(m_nI, m_nJ, m_nK, SQRT( ( m_nI * m_nI ) + ( m_nJ * m_nJ ) + (m_nK * m_nK ) ) );
	
	CDialog::OnOK();
}
