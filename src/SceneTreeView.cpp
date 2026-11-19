// SceneTreeView.cpp : implementation of the CSceneTreeView class
//

#include "StdAfx.h"
#include "Ray.h"
#include "SceneTreeView.h"
#include "MainFrm.h"

#include "RayDoc.h"
#include "SceneTreeView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define UPDATE_BITMAP   10

#define TYPE_ROOT       0
#define TYPE_OBJECT     1
#define TYPE_MATERIAL   2

class CTreeData
{
    public:
        int                 m_nType;
        void               *m_pData;
        CTreeData          *m_pNext;

                            CTreeData( CTreeData *pFirst, void *pData, int nType )
                            {
                                m_nType = nType;
                                m_pData = pData;
                                m_pNext = pFirst;
                            }
        static void         KillAll( CTreeData  *pFirst )
                            {
                                CTreeData   *pNext;
                                CTreeData   *pThis;

                                pThis = pFirst;

                                while ( pThis )
                                {
                                    pNext = pThis->m_pNext;
                                    delete pThis;
                                    pThis = pNext;
                                }
                            }
};


/////////////////////////////////////////////////////////////////////////////
// CSceneTreeView

IMPLEMENT_DYNCREATE(CSceneTreeView, CTreeView)

BEGIN_MESSAGE_MAP(CSceneTreeView, CTreeView)
	//{{AFX_MSG_MAP(CSceneTreeView)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnRclick)
	ON_COMMAND(IDM_OBJECT_PROPERTIES, OnObjectProperties)
	ON_COMMAND(IDM_MATERIAL_PROPERTIES, OnMaterialProperties)
	ON_COMMAND(IDM_OBJECT_MATERIALS, OnObjectMaterials)
	ON_COMMAND(IDC_RENDER_RAY_TRACE, OnRenderRayTrace)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemexpanded)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_COMMAND(IDM_SETTINGS_PROPERTIES, OnSettingsProperties)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CTreeView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CTreeView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CTreeView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSceneTreeView construction/destruction

CSceneTreeView::CSceneTreeView()
{
    m_bInitialized = FALSE;
    m_pDataItems = NULL;
}

CSceneTreeView::~CSceneTreeView()
{
    Empty();
}

BOOL CSceneTreeView::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style |= TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_EDITLABELS | TVS_SHOWSELALWAYS;
	return CTreeView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CSceneTreeView drawing

void CSceneTreeView::OnDraw(CDC* pDC)
{
    CTreeView::OnDraw( pDC );

    if ( ! m_bInitialized )
    {
        OnUpdate( NULL, 0, NULL );
    }
}

void CSceneTreeView::OnInitialUpdate()
{
    CRayDoc    *pDoc = GetDocument();
    CTreeData  *pData;
    CString     strTitle; 
    CTreeCtrl  &oTree = GetTreeCtrl();
    
    if ( IsWindow( m_hWnd ) )
    {
        if ( oTree.GetCount() < 2 )
        {
            Empty();
    
	        m_oImageList.Create( IDB_SCENE_TREE_IMAGE_LIST, 24, 16, RGB( 255, 0, 255 ) );

	        GetTreeCtrl().SetImageList( &m_oImageList, TVSIL_NORMAL );

            strTitle = pDoc->GetTitle();

	        m_hRoot = GetTreeCtrl().InsertItem( strTitle, 0, 1, TVI_ROOT, TVI_LAST );

            m_pDataItems = pData = new CTreeData( m_pDataItems, NULL, TYPE_ROOT );

            GetTreeCtrl().SetItemData( m_hRoot, (DWORD)pData );

            GetTreeCtrl().SetIndent( 24 );

            pDoc->GetScene().ToTree( this );
        }
        m_bInitialized = TRUE;
    }

/*
    CTreeData   *pData;

	// CTreeView::OnInitialUpdate();

	m_oImageList.Create( IDB_SCENE_TREE_IMAGE_LIST, 24, 16, RGB( 255, 0, 255 ) );

	GetTreeCtrl().SetImageList( &m_oImageList, TVSIL_NORMAL );

	GetTreeCtrl().DeleteAllItems();
	
	m_hRoot = GetTreeCtrl().InsertItem( "Scene", 0, 1, TVI_ROOT, TVI_LAST );

    m_pDataItems = pData = new CTreeData( m_pDataItems, NULL, TYPE_ROOT );

    GetTreeCtrl().SetItemData( m_hRoot, (DWORD)pData );

    GetTreeCtrl().SetIndent( 24 );
*/
}

/////////////////////////////////////////////////////////////////////////////
// CSceneTreeView printing

BOOL CSceneTreeView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CSceneTreeView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CSceneTreeView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CSceneTreeView diagnostics

#ifdef _DEBUG
void CSceneTreeView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CSceneTreeView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

CRayDoc* CSceneTreeView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CRayDoc)));
	return (CRayDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSceneTreeView message handlers

HTREEITEM CSceneTreeView::AddObject( CObjects *pObject, int nImage, LPCSTR strName, HTREEITEM hUnder )
{
    HTREEITEM       hReturn;
    CTreeData      *pData;

	if ( hUnder == TVI_ROOT )
		hUnder = m_hRoot;

	hReturn = GetTreeCtrl().InsertItem( strName, nImage, nImage + 1, hUnder, TVI_LAST );

    m_pDataItems = pData = new CTreeData( m_pDataItems, pObject, TYPE_OBJECT );

    GetTreeCtrl().SetItemData( hReturn, (DWORD)pData );

    return hReturn;
}

HTREEITEM CSceneTreeView::AddMaterial( CMaterial *pObject, int nImage, LPCSTR strName, HTREEITEM hUnder )
{
    HTREEITEM       hReturn;
    CTreeData      *pData;

	if ( hUnder == TVI_ROOT )
		hUnder = m_hRoot;

	hReturn = GetTreeCtrl().InsertItem( strName, nImage, nImage + 1, hUnder, TVI_LAST );

    m_pDataItems = pData = new CTreeData( m_pDataItems, pObject, TYPE_MATERIAL );

    GetTreeCtrl().SetItemData( hReturn, (DWORD)pData );

    return hReturn;
}

static void EmptyTree( CTreeCtrl &oTree, HTREEITEM hItem )
{
    HTREEITEM   hChild;
    CTreeData  *pData;

    if ( hItem == NULL )
        return;
   
    while ( hChild = oTree.GetChildItem( hItem ) )
    {
        EmptyTree( oTree, hChild );
        oTree.DeleteItem( hChild );
    }

    pData = (CTreeData *)oTree.GetItemData( hItem );

    delete pData;
}

void CSceneTreeView::Empty( void )
{
    if ( IsWindow( m_hWnd ) )
    {
        GetTreeCtrl().DeleteAllItems();
    }
    CTreeData::KillAll( m_pDataItems );
    m_pDataItems = NULL;
}

void CSceneTreeView::EmptyUnder( HTREEITEM hUnder )
{
    CTreeCtrl       &oTree = GetTreeCtrl();
    HTREEITEM       hChild;

    while ( hChild = oTree.GetChildItem( hUnder ) )
    {
        oTree.DeleteItem( hChild );
    }
}

void CSceneTreeView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
    CRayDoc    *pDoc = GetDocument();
    CTreeData  *pData;
    CString     strTitle; 
    CTreeCtrl  &oTree = GetTreeCtrl();
    
    if ( IsWindow( m_hWnd ) )
    {
        if ( oTree.GetCount() < 2 )
        {
            Empty();
    
            strTitle = pDoc->GetTitle();

	        m_hRoot = GetTreeCtrl().InsertItem( strTitle, 0, 1, TVI_ROOT, TVI_LAST );

            m_pDataItems = pData = new CTreeData( m_pDataItems, NULL, TYPE_ROOT );

            GetTreeCtrl().SetItemData( m_hRoot, (DWORD)pData );

            GetTreeCtrl().SetIndent( 24 );

            pDoc->GetScene().ToTree( this );
        }
        m_bInitialized = TRUE;
    }
}

void CSceneTreeView::OnActivateFrame( UINT nState, CFrameWnd* pFrameWnd )
{
    if ( ( nState == WA_ACTIVE ) && ( ! m_bInitialized ) )
    {
        OnUpdate( NULL, NULL, NULL );
    }

    CTreeView::OnActivateFrame( nState, pFrameWnd );
}


void CSceneTreeView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
   if ( ( bActivate ) && ( ! m_bInitialized ) )
    {
        OnUpdate( NULL, NULL, NULL );
    }

    CTreeView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

void CSceneTreeView::OnRclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
    HTREEITEM       hItem;
    CTreeData      *pData;
    CPoint          oPoint;
    UINT            nFlags;
    CMenu           oPopup;

    GetCursorPos( &oPoint );

    GetTreeCtrl().ScreenToClient( &oPoint );
    hItem = GetTreeCtrl().HitTest( oPoint, &nFlags );
    if ( hItem == NULL )
    {
        hItem = GetTreeCtrl().GetSelectedItem();
    }
    else
    {
        GetTreeCtrl().SelectItem( hItem );
    }

    if ( hItem )
    {
        GetTreeCtrl().ClientToScreen( &oPoint );
        pData = (CTreeData *)GetTreeCtrl().GetItemData( hItem );

        if ( pData )
        {
            switch ( pData->m_nType )
            {
                case TYPE_ROOT:
                    if ( oPopup.CreatePopupMenu() )
                    {
                        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_SETTINGS_PROPERTIES, "Properties..." );
                        oPopup.TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, oPoint.x, oPoint.y, AfxGetMainWnd(), NULL );
                        oPopup.DestroyMenu();
                    }
                    break;

                case TYPE_OBJECT:
                    if ( oPopup.CreatePopupMenu() )
                    {
                        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_OBJECT_CUT, "Cut" );
                        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_OBJECT_COPY, "Copy" );
                        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_OBJECT_PASTE, "Paste" );
                        oPopup.AppendMenu( MF_SEPARATOR );
                        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_OBJECT_MATERIALS, "Materials..." );
                        oPopup.AppendMenu( MF_SEPARATOR );
                        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_OBJECT_PROPERTIES, "Properties..." );
                        oPopup.TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, oPoint.x, oPoint.y, AfxGetMainWnd(), NULL );
                        oPopup.DestroyMenu();
                    }
                    break;

                case TYPE_MATERIAL:
                    if ( oPopup.CreatePopupMenu() )
                    {
                        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_MATERIAL_CUT, "Cut" );
                        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_MATERIAL_COPY, "Copy" );
                        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_MATERIAL_PASTE, "Paste" );
                        oPopup.AppendMenu( MF_SEPARATOR );
                        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_MATERIAL_PROPERTIES, "Properties..." );
                        oPopup.AppendMenu( MF_SEPARATOR );
                        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_MATERIAL_DELETE, "Delete" );
                        oPopup.TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, oPoint.x, oPoint.y, AfxGetMainWnd(), NULL );
                        oPopup.DestroyMenu();
                    }
                    break;

                default:
                    break;
            }
        }
    }

    *pResult = 0;
}

void CSceneTreeView::OnObjectProperties() 
{
    HTREEITEM       hItem;
    CTreeData      *pData;
    CObjects       *pObject;
    CRayDoc        *pDoc = GetDocument();

    hItem = GetTreeCtrl().GetSelectedItem();
    pData = (CTreeData *)GetTreeCtrl().GetItemData( hItem );
    if ( pData )
    {
	    pObject = (CObjects *)pData->m_pData;
        if ( pObject )
        {
            pObject->SetProperties();
            pDoc->SetModifiedFlag();
        }
    }
}

void CSceneTreeView::OnMaterialProperties() 
{
    HTREEITEM       hItem;
    CTreeData      *pData;
    CMaterial      *pMaterial;
    CRayDoc        *pDoc = GetDocument();

    hItem = GetTreeCtrl().GetSelectedItem();
    pData = (CTreeData *)GetTreeCtrl().GetItemData( hItem );
    if ( pData )
    {
	    pMaterial = (CMaterial *)pData->m_pData;
        if ( pMaterial )
        {
            pMaterial->Properties();
            pDoc->SetModifiedFlag( TRUE );
        }
    }
}

void CSceneTreeView::OnObjectMaterials() 
{
    HTREEITEM       hItem;
    CTreeData      *pData;
    CObjects       *pObject;
    CRayDoc        *pDoc = GetDocument();

    hItem = GetTreeCtrl().GetSelectedItem();
    pData = (CTreeData *)GetTreeCtrl().GetItemData( hItem );
    if ( pData )
    {
	    pObject = (CObjects *)pData->m_pData;
        if ( pObject )
        {
            pObject->SetMaterials();
            pDoc->SetModifiedFlag( TRUE );
        }
    }
}

void CSceneTreeView::OnRenderRayTrace() 
{
	CRayDoc      *pDoc = GetDocument();

        // Start the Ray Trace
    pDoc->RenderRayTrace();

    pDoc->UpdateAllViews( this, 999, NULL );
}

void CSceneTreeView::OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}

void CSceneTreeView::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}

void CSceneTreeView::OnSettingsProperties() 
{
	CRayDoc      *pDoc = GetDocument();
    CScene      &oScene = pDoc->GetScene();
    CSettings   &oSettings = oScene.GetSettings();

    oSettings.Properties();
}
