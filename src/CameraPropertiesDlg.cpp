// CameraPropertiesDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "Ray.h"
#include "CameraPropertiesDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCameraPropertiesDlg dialog


CCameraPropertiesDlg::CCameraPropertiesDlg( CVector *poPosition, CVector *poPointAt, CWnd* pParent /*=NULL*/)
	: CDialog(CCameraPropertiesDlg::IDD, pParent)
{
    m_poPosition = poPosition;
    m_poPointAt = poPointAt;
    //{{AFX_DATA_INIT(CCameraPropertiesDlg)
	m_nPointAtX = poPointAt->GetX();
	m_nPointAtY = poPointAt->GetY();
	m_nPointAtZ = poPointAt->GetZ();
	m_nX = poPosition->GetX();
	m_nY = poPosition->GetY();
	m_nZ = poPosition->GetZ();
	m_nCameraAngle = 0.0;
	//}}AFX_DATA_INIT
}


void CCameraPropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCameraPropertiesDlg)
	DDX_Text(pDX, IDC_POINT_AT_X, m_nPointAtX);
	DDX_Text(pDX, IDC_POINT_AT_Y, m_nPointAtY);
	DDX_Text(pDX, IDC_POINT_AT_Z, m_nPointAtZ);
	DDX_Text(pDX, IDC_X_COORD, m_nX);
	DDX_Text(pDX, IDC_Y_COORD, m_nY);
	DDX_Text(pDX, IDC_Z_COORD, m_nZ);
	DDX_Text(pDX, IDC_CAMERA_ANGLE, m_nCameraAngle);
	DDV_MinMaxDouble(pDX, m_nCameraAngle, 1.e-002, 179.99);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCameraPropertiesDlg, CDialog)
	//{{AFX_MSG_MAP(CCameraPropertiesDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCameraPropertiesDlg message handlers

void CCameraPropertiesDlg::OnOK() 
{
    UpdateData( TRUE );

    m_poPosition->Set( m_nX, m_nY, m_nZ );
    m_poPointAt->Set( m_nPointAtX, m_nPointAtY, m_nPointAtZ );
	
	CDialog::OnOK();
}
