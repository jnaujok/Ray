// AddPtLightDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "Ray.h"
#include "AddPtLightDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddPtLightDlg dialog


CAddPtLightDlg::CAddPtLightDlg(CVector *pColor, CVector *pPosition, CWnd* pParent /*=NULL*/)
	: CDialog(CAddPtLightDlg::IDD, pParent)
{
    m_pColor = pColor;
    m_pPosition = pPosition;
	//{{AFX_DATA_INIT(CAddPtLightDlg)
	m_nBlue = m_pColor->GetZ();
	m_nGreen = m_pColor->GetY();
	m_nRed = m_pColor->GetX();
	m_nX = m_pPosition->GetX();
	m_nY = m_pPosition->GetY();
	m_nZ = m_pPosition->GetZ();
	//}}AFX_DATA_INIT
}


void CAddPtLightDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddPtLightDlg)
	DDX_Text(pDX, IDC_BLUE_COMPONENT, m_nBlue);
	DDX_Text(pDX, IDC_GREEN_COMPONENT, m_nGreen);
	DDX_Text(pDX, IDC_RED_COMPONENT, m_nRed);
	DDX_Text(pDX, IDC_X_COORD, m_nX);
	DDX_Text(pDX, IDC_Y_COORD, m_nY);
	DDX_Text(pDX, IDC_Z_COORD, m_nZ);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddPtLightDlg, CDialog)
	//{{AFX_MSG_MAP(CAddPtLightDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddPtLightDlg message handlers

void CAddPtLightDlg::OnOK() 
{
    if ( UpdateData( TRUE ) )
    {
        m_pColor->Set( m_nRed, m_nGreen, m_nBlue, 1.0 );
        m_pPosition->Set( m_nX, m_nY, m_nZ, 1.0 );
    	CDialog::OnOK();
    }
}

void CAddPtLightDlg::OnCancel() 
{
    MessageBox( "Canceling this will cause the point source to not be added. Are you sure you want to cancel this operation?", "Confirmation", MB_ICONEXCLAMATION | MB_OK );
	CDialog::OnCancel();
}
