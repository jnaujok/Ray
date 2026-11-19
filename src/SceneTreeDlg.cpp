// SceneTreeDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "Ray.h"
#include "SceneTreeDlg.h"
#include "Objects.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MINIMUM_SIZE	100

#define SCENE_TREE_SECTION			"Scene Tree"
#define SCENE_TREE_DOCK_WIDTH_KEY	"Docked Width"
#define SCENE_TREE_DOCK_HEIGHT_KEY	"Docked Height"
#define SCENE_TREE_FREE_WIDTH_KEY	"Free Width"
#define SCENE_TREE_FREE_HEIGHT_KEY	"Free Height"
/////////////////////////////////////////////////////////////////////////////
// CSceneTreeDlg dialog

#define TYPE_ROOT       0
#define TYPE_OBJECT     1
#define TYPE_MATERIAL   2

class CTreeData
{
    public:
        int         m_nType;
        void       *m_pData;

                    CTreeData( void *pData, int nType )
                    {
                        m_nType = nType;
                        m_pData = pData;
                    }
};

CSceneTreeDlg::CSceneTreeDlg(CWnd* pParent /*=NULL*/)
{
	//{{AFX_DATA_INIT(CSceneTreeDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	CRayApp		*pRayApp = (CRayApp *)AfxGetApp();

	m_nDockedWidth = pRayApp->GetProfileInt( SCENE_TREE_SECTION, SCENE_TREE_DOCK_WIDTH_KEY, 100 );
	m_nDockedHeight = pRayApp->GetProfileInt( SCENE_TREE_SECTION, SCENE_TREE_DOCK_HEIGHT_KEY, 100 );
	m_nFreeWidth = pRayApp->GetProfileInt( SCENE_TREE_SECTION, SCENE_TREE_FREE_WIDTH_KEY, 100 );
	m_nFreeHeight = pRayApp->GetProfileInt( SCENE_TREE_SECTION, SCENE_TREE_FREE_HEIGHT_KEY, 100 );

	m_nLastHeight = m_nDockedHeight;
	m_nLastWidth = m_nDockedWidth;

    m_bInitialized = FALSE;
}

CSceneTreeDlg::~CSceneTreeDlg( void )
{
	CRayApp		*pRayApp = (CRayApp *)AfxGetApp();

	pRayApp->WriteProfileInt( SCENE_TREE_SECTION, SCENE_TREE_DOCK_WIDTH_KEY, m_nDockedWidth );
	pRayApp->WriteProfileInt( SCENE_TREE_SECTION, SCENE_TREE_DOCK_HEIGHT_KEY, m_nDockedHeight );
	pRayApp->GetProfileInt( SCENE_TREE_SECTION, SCENE_TREE_FREE_WIDTH_KEY, m_nFreeWidth );
	pRayApp->GetProfileInt( SCENE_TREE_SECTION, SCENE_TREE_FREE_HEIGHT_KEY, m_nFreeHeight );
}


void CSceneTreeDlg::DoDataExchange(CDataExchange* pDX)
{
	CWnd::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSceneTreeDlg)
	DDX_Control(pDX, IDC_SCENE_TREE, m_oSceneTree);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSceneTreeDlg, CDialogBar)
	//{{AFX_MSG_MAP(CSceneTreeDlg)
	ON_NOTIFY(TVN_SELCHANGED, IDC_SCENE_TREE, OnSelchangedSceneTree)
	ON_NOTIFY(NM_RCLICK, IDC_SCENE_TREE, OnRclickSceneTree)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CSceneTreeDlg::OnSelchangedSceneTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}


void CSceneTreeDlg::OnSize( UINT nType, int cx, int cy )
{
	CWnd	*pTree;

	CWnd::OnSize( nType, cx, cy );

	pTree = GetDlgItem( IDC_SCENE_TREE );

	if ( ( pTree ) && ( pTree->m_hWnd ) )
	{
		CRect	oRect;

		GetClientRect( oRect );
		pTree->MoveWindow( oRect.left + 2, oRect.top + 2, oRect.left + cx - 4, oRect.top + cy - 4, TRUE );
		Invalidate();
		UpdateWindow();
	}
}

int CSceneTreeDlg::OnCreate( LPCREATESTRUCT pUnused )
{
	CTreeCtrl	*pTree	= (CTreeCtrl *)GetDlgItem( IDC_SCENE_TREE );
    CTreeData   *pData;

	ASSERT( pTree );

	m_oImageList.Create( IDB_SCENE_TREE_IMAGE_LIST, 24, 16, RGB( 255, 0, 255 ) );

	pTree->SetImageList( &m_oImageList, TVSIL_NORMAL );

	pTree->DeleteAllItems();
	
	m_hRoot = pTree->InsertItem( "Scene", 0, 1, TVI_ROOT, TVI_LAST );

    pData = new CTreeData( NULL, TYPE_ROOT );

    pTree->SetItemData( m_hRoot, (DWORD)pData );

    pTree->SetIndent( 24 );

	return 0;
}

HTREEITEM CSceneTreeDlg::AddObject( CObjects *pObject, int nImage, LPCSTR strName, HTREEITEM hUnder )
{
	CTreeCtrl	   *pTree = (CTreeCtrl *)GetDlgItem( IDC_SCENE_TREE );
    HTREEITEM       hReturn;
    CTreeData      *pData;

	if ( hUnder == TVI_ROOT )
		hUnder = m_hRoot;

	hReturn = pTree->InsertItem( strName, nImage, nImage + 1, hUnder, TVI_LAST );

    pData = new CTreeData( pObject, TYPE_OBJECT );

    pTree->SetItemData( hReturn, (DWORD)pData );

    return hReturn;
}

void CSceneTreeDlg::UpdateDoc( LPCSTR strName )
{
   	CTreeCtrl	*pTree	= (CTreeCtrl *)GetDlgItem( IDC_SCENE_TREE );
    CTreeData   *pData;

	ASSERT( pTree );

    if ( ! m_bInitialized )
    {
        m_bInitialized = TRUE;
        m_oImageList.Create( IDB_SCENE_TREE_IMAGE_LIST, 24, 16, RGB( 255, 0, 255 ) );
    	pTree->SetImageList( &m_oImageList, TVSIL_NORMAL );
    }

	Empty();
	
    pData = new CTreeData( NULL, TYPE_ROOT );

    m_hRoot = pTree->InsertItem( strName, 0, 1, TVI_ROOT, TVI_LAST );

    pTree->SetItemData( m_hRoot, (DWORD)pData );
}

void CSceneTreeDlg::OnRclickSceneTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CTreeCtrl      *pTree = (CTreeCtrl *)GetDlgItem( IDC_SCENE_TREE );
    HTREEITEM       hItem;
    CObjects       *pObject;
    CMaterial      *pMaterial;
    CTreeData      *pData;
    POINT           stPoint;
    CPoint          oPoint;
    UINT            nFlags;

    GetCursorPos( &stPoint );
    oPoint = stPoint;

    pTree->ScreenToClient( &oPoint );
    hItem = pTree->HitTest( oPoint, &nFlags );
    if ( hItem == NULL )
    {
        hItem = pTree->GetSelectedItem();
    }

    if ( hItem )
    {
        pData = (CTreeData *)pTree->GetItemData( hItem );

        if ( pData )
        {
            switch ( pData->m_nType )
            {
                case TYPE_ROOT:
                    // TO DO: Settings
                    break;

                case TYPE_OBJECT:
                    pObject = (CObjects *)pData->m_pData;
                    if ( pObject )
                        pObject->SetProperties();
                    break;

                case TYPE_MATERIAL:
                    pMaterial = (CMaterial *)pData->m_pData;
                    if ( pMaterial )
                        pMaterial->Properties();
                    break;

                default:
                    break;
            }
        }
    }

    *pResult = 0;
}

HTREEITEM CSceneTreeDlg::AddMaterial( CMaterial *pObject, int nImage, LPCSTR strName, HTREEITEM hUnder )
{
	CTreeCtrl	   *pTree = (CTreeCtrl *)GetDlgItem( IDC_SCENE_TREE );
    HTREEITEM       hReturn;
    CTreeData      *pData;

	if ( hUnder == TVI_ROOT )
		hUnder = m_hRoot;

	hReturn = pTree->InsertItem( strName, nImage, nImage + 1, hUnder, TVI_LAST );

    pData = new CTreeData( pObject, TYPE_MATERIAL );

    pTree->SetItemData( hReturn, (DWORD)pData );

    return hReturn;
}

static void EmptyTree( CTreeCtrl *pTree, HTREEITEM hItem )
{
    HTREEITEM   hChild;
    CTreeData  *pData;

    if ( hItem == NULL )
        return;
   
    while ( hChild = pTree->GetChildItem( hItem ) )
    {
        EmptyTree( pTree, hChild );
        pTree->DeleteItem( hChild );
    }

    pData = (CTreeData *)pTree->GetItemData( hItem );

    delete pData;
}

void CSceneTreeDlg::Empty( void )
{
	CTreeCtrl	   *pTree = (CTreeCtrl *)GetDlgItem( IDC_SCENE_TREE );

    EmptyTree( pTree, pTree->GetRootItem() );

    pTree->DeleteAllItems();
}
