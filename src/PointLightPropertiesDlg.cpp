// PointLightPropertiesDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "Ray.h"
#include "PointLightPropertiesDlg.h"
#include "SetColorDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPointLightPropertiesDlg dialog


CPointLightPropertiesDlg::CPointLightPropertiesDlg( CVector *poVector, CColor *poColor, CWnd* pParent /*=NULL*/)
	: CDialog(CPointLightPropertiesDlg::IDD, pParent)
{
    m_poPosition = poVector;
    m_poColor = poColor;

	//{{AFX_DATA_INIT(CPointLightPropertiesDlg)
	m_nX = poVector->GetX();
	m_nY = poVector->GetY();
	m_nZ = poVector->GetZ();
	//}}AFX_DATA_INIT
}


void CPointLightPropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPointLightPropertiesDlg)
	DDX_Text(pDX, IDC_X_COORD, m_nX);
	DDX_Text(pDX, IDC_Y_COORD, m_nY);
	DDX_Text(pDX, IDC_Z_COORD, m_nZ);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPointLightPropertiesDlg, CDialog)
	//{{AFX_MSG_MAP(CPointLightPropertiesDlg)
	ON_BN_CLICKED(IDC_SET_COLOR, OnSetColor)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPointLightPropertiesDlg message handlers

void CPointLightPropertiesDlg::OnOK() 
{
	UpdateData( TRUE );

    m_poPosition->Set( m_nX, m_nY, m_nZ );
	
	CDialog::OnOK();
}

void CPointLightPropertiesDlg::OnSetColor() 
{
    CSetColorDlg    oDlg( m_poColor, this );

    oDlg.DoModal();
}
