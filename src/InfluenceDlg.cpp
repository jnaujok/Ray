// InfluenceDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "Ray.h"
#include "InfluenceDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInfluenceDlg dialog


CInfluenceDlg::CInfluenceDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CInfluenceDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CInfluenceDlg)
	m_nInfluence = 0.0;
	//}}AFX_DATA_INIT
}


void CInfluenceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInfluenceDlg)
	DDX_Text(pDX, IDC_INFLUENCE_EDIT, m_nInfluence);
	DDV_MinMaxDouble(pDX, m_nInfluence, 0., 1.);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInfluenceDlg, CDialog)
	//{{AFX_MSG_MAP(CInfluenceDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInfluenceDlg message handlers
