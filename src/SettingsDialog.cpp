// SettingsDialog.cpp : implementation file
//

#include "StdAfx.h"
#include "Ray.h"
#include "SettingsDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSettingsDialog dialog


CSettingsDialog::CSettingsDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CSettingsDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSettingsDialog)
	m_strFileName = _T("");
	m_nImageHeight = 0;
	m_nImageWidth = 0;
	//}}AFX_DATA_INIT
}


void CSettingsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSettingsDialog)
	DDX_Text(pDX, IDC_IMAGE_FILE_NAME, m_strFileName);
	DDX_Text(pDX, IDC_IMAGE_HEIGHT, m_nImageHeight);
	DDV_MinMaxInt(pDX, m_nImageHeight, 1, 32768);
	DDX_Text(pDX, IDC_IMAGE_WIDTH, m_nImageWidth);
	DDV_MinMaxInt(pDX, m_nImageWidth, 1, 32768);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSettingsDialog, CDialog)
	//{{AFX_MSG_MAP(CSettingsDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSettingsDialog message handlers
