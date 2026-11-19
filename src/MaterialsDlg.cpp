// MaterialsDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "Ray.h"
#include "MaterialsDlg.h"
#include "InfluenceDlg.h"
#include "AddMaterialsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMaterialsDlg dialog


CMaterialsDlg::CMaterialsDlg( CWnd* pParent /*=NULL*/)
	: CDialog(CMaterialsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMaterialsDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CMaterialsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMaterialsDlg)
	DDX_Control(pDX, IDC_MATERIAL_LIST, m_oMaterialList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMaterialsDlg, CDialog)
	//{{AFX_MSG_MAP(CMaterialsDlg)
	ON_BN_CLICKED(IDC_PROPERTY_BUTTON, OnPropertyButton)
	ON_NOTIFY(NM_DBLCLK, IDC_MATERIAL_LIST, OnDblclkMaterialList)
	ON_NOTIFY(NM_RCLICK, IDC_MATERIAL_LIST, OnRclickMaterialList)
	ON_COMMAND(IDM_MATERIAL_ADD, OnMaterialAdd)
	ON_COMMAND(IDM_MATERIAL_INFLUENCE, OnMaterialInfluence)
	ON_COMMAND(IDM_MATERIAL_LIST_DELETE, OnMaterialListDelete)
	ON_COMMAND(IDM_MATERIAL_PROPERTY, OnMaterialProperty)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMaterialsDlg message handlers

void CMaterialsDlg::OnPropertyButton() 
{
    CMaterial   *pMaterial;
    UINT         nMask;
    int          nCount;

    nCount = 0;
    while ( nCount < m_oMaterialList.GetItemCount() )
    {
        nMask = m_oMaterialList.GetItemState( nCount, LVIS_SELECTED );
        if ( nMask & LVIS_SELECTED )
        {
        	pMaterial = (CMaterial *)m_oMaterialList.GetItemData( nCount );
            pMaterial->Properties();
        }
        nCount++;
    }
}

void CMaterialsDlg::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

void CMaterialsDlg::OnDblclkMaterialList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    OnPropertyButton();	

	*pResult = 0;
}

void CMaterialsDlg::OnRclickMaterialList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    CPoint          oPoint;
    CMenu           oPopup;
    UINT            nFlags;
    int             nIndex;
    int             nCount;

    GetCursorPos( &oPoint );

    m_oMaterialList.ScreenToClient( &oPoint );
    nIndex = m_oMaterialList.HitTest( oPoint, &nFlags );
    if ( nIndex != -1 )
    {
        nCount = 0;
        while ( nCount < m_oMaterialList.GetItemCount() )
        {
            if ( nCount != nIndex )
            {
                m_oMaterialList.SetItemState( nCount, 0, LVIF_STATE );
            }
            else
            {
                m_oMaterialList.SetItemState( nCount, LVIF_STATE, LVIF_STATE );
            }
            nCount++;
        }
    }

    if ( oPopup.CreatePopupMenu() )
    {
        m_oMaterialList.ClientToScreen( &oPoint );
        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_MATERIAL_PROPERTY, "Properties..." );
        oPopup.AppendMenu( MF_SEPARATOR );
        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_MATERIAL_INFLUENCE, "Influence..." );
        oPopup.AppendMenu( MF_SEPARATOR );
        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_MATERIAL_ADD, "Add" );
        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_MATERIAL_LIST_DELETE, "Delete" );
        oPopup.TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, oPoint.x, oPoint.y, this, NULL );
        oPopup.DestroyMenu();
    }
    *pResult = 0;
}

BOOL CMaterialsDlg::OnInitDialog() 
{
    int             nCount;
    CMaterial      *pMaterial;

	CDialog::OnInitDialog();
	
	m_oBigIcons.Create( IDB_LARGE_ICONS, 32, 4, RGB( 255, 0, 255 ) );
    m_oSmallIcons.Create( IDB_SMALL_ICONS, 16, 4, RGB( 255, 0, 255 ) );

    m_oMaterialList.SetImageList( &m_oBigIcons, LVSIL_NORMAL );
    m_oMaterialList.SetImageList( &m_oSmallIcons, LVSIL_SMALL );

    m_oMaterialList.InsertColumn( 0, "Material Type", LVCFMT_LEFT, 100, 0 );
    m_oMaterialList.InsertColumn( 1, "Influence", LVCFMT_LEFT, 75, 0 );

    nCount = 0;
    while ( nCount < m_oMaterials.GetSize() )
    {
        pMaterial = m_oMaterials[ nCount ];
        pMaterial->ToList( m_oMaterialList, nCount, m_oMaterials.GetInfluence( nCount ) );
        nCount++;
    }
  
	return TRUE;
}

void CMaterialsDlg::OnMaterialAdd() 
{
    int         nCount;
    CMaterial  *pMaterial;

    CAddMaterialsDlg    oDlg;

    oDlg.m_oMaterialList = m_oMaterials;
    oDlg.DoModal();
    m_oMaterials = oDlg.m_oMaterialList;

    m_oMaterialList.DeleteAllItems();

    nCount = 0;
    while ( nCount < m_oMaterials.GetSize() )
    {
        pMaterial = m_oMaterials[ nCount ];
        pMaterial->ToList( m_oMaterialList, nCount, m_oMaterials.GetInfluence( nCount ) );
        nCount++;
    }
}

void CMaterialsDlg::OnMaterialInfluence() 
{
    FLT             nInfluence;
    UINT            nMask;
    int             nCount;
    CInfluenceDlg   oDlg;
    CMaterial      *pMaterial;

    nCount = 0;
    while ( nCount < m_oMaterialList.GetItemCount() )
    {
        nMask = m_oMaterialList.GetItemState( nCount, LVIS_SELECTED );
        if ( nMask & LVIS_SELECTED )
        {
            nInfluence = m_oMaterials.GetInfluence( nCount );
            oDlg.m_nInfluence = nInfluence;
            if ( oDlg.DoModal() == IDOK )
                m_oMaterials.SetInfluence( nCount, oDlg.m_nInfluence );
        }
        nCount++;
    }

    m_oMaterialList.DeleteAllItems();

    nCount = 0;
    while ( nCount < m_oMaterials.GetSize() )
    {
        pMaterial = m_oMaterials[ nCount ];
        pMaterial->ToList( m_oMaterialList, nCount, m_oMaterials.GetInfluence( nCount ) );
        nCount++;
    }
}

void CMaterialsDlg::OnMaterialListDelete() 
{
    if ( MessageBox( "Delete this/these material(s)?", "Delete A Material", MB_YESNO | MB_ICONQUESTION ) == IDYES )
    {
        CMaterial   *pMaterial;
        UINT         nMask;
        int          nCount;

        nCount = 0;
        while ( nCount < m_oMaterialList.GetItemCount() )
        {
            nMask = m_oMaterialList.GetItemState( nCount, LVIS_SELECTED );
            if ( nMask & LVIS_SELECTED )
            {
        	    pMaterial = (CMaterial *)m_oMaterialList.GetItemData( nCount );
                m_oMaterials.Delete( nCount );
                delete pMaterial;
            }
            nCount++;
        }

        m_oMaterialList.DeleteAllItems();

        nCount = 0;
        while ( nCount < m_oMaterials.GetSize() )
        {
            pMaterial = m_oMaterials[ nCount ];
            pMaterial->ToList( m_oMaterialList, nCount, m_oMaterials.GetInfluence( nCount ) );
            nCount++;
        }
    }
}

void CMaterialsDlg::OnMaterialProperty() 
{
	OnPropertyButton();
}
