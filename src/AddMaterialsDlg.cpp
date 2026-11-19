// AddMaterialsDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "Ray.h"
#include "AddMaterialsDlg.h"
#include "Material.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddMaterialsDlg dialog


CAddMaterialsDlg::CAddMaterialsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddMaterialsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddMaterialsDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CAddMaterialsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddMaterialsDlg)
	DDX_Control(pDX, IDC_MATERIAL_LIST, m_oAddList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddMaterialsDlg, CDialog)
	//{{AFX_MSG_MAP(CAddMaterialsDlg)
	ON_BN_CLICKED(IDC_ADD_BUTTON, OnAddButton)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddMaterialsDlg message handlers

void CAddMaterialsDlg::OnAddButton() 
{
    LPADDFUNC    lpAddFunc;
    CMaterial   *pMaterial;
    UINT         nMask;
    int          nCount;

    nCount = 0;
    while ( nCount < m_oAddList.GetItemCount() )
    {
        nMask = m_oAddList.GetItemState( nCount, LVIS_SELECTED );
        if ( nMask & LVIS_SELECTED )
        {
        	lpAddFunc = (LPADDFUNC)m_oAddList.GetItemData( nCount );
            pMaterial = (*lpAddFunc)();
            m_oMaterialList.Add( pMaterial, 0.00 );
        }
        nCount++;
    }
    CDialog::OnOK();
}

BOOL CAddMaterialsDlg::OnInitDialog() 
{
    CDynMaterialList    oMatList;

	CDialog::OnInitDialog();
	
    m_oAddList.InsertColumn( 0, "Material Type", LVCFMT_LEFT, 100, 0 );

    oMatList.LoadAddList( m_oAddList );
	
	return TRUE;
}
