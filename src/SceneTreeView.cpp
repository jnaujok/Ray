// SceneTreeView.cpp : implementation of the CSceneTreeView class
//

#include "StdAfx.h"
#include "Ray.h"
#include "SceneTreeView.h"
#include "MainFrm.h"

#include "RayDoc.h"
#include "SceneTreeView.h"
#include "JsonEditDlg.h"

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
	ON_COMMAND(IDM_OBJECT_EDIT_JSON, OnObjectEditJson)
	ON_COMMAND(IDM_OBJECT_DELETE, OnObjectDelete)
	ON_COMMAND(IDM_SCENE_EDIT_SETTINGS, OnSceneEditSettings)
	ON_COMMAND(IDM_SCENE_EDIT_CAMERA, OnSceneEditCamera)
	ON_COMMAND(IDM_SCENE_EDIT_ENVIRONMENT, OnSceneEditEnvironment)
	ON_COMMAND(IDM_SCENE_ADD_OBJECT, OnSceneAddObject)
	ON_COMMAND(IDM_SCENE_ADD_LIGHT, OnSceneAddLight)
	ON_COMMAND(IDM_SCENE_IMPORT_MODEL, OnSceneImportModel)
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
                        oPopup.AppendMenu( MF_SEPARATOR );
                        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_SCENE_EDIT_SETTINGS, "Edit Settings as JSON..." );
                        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_SCENE_EDIT_CAMERA, "Edit Camera as JSON..." );
                        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_SCENE_EDIT_ENVIRONMENT, "Edit Environment as JSON..." );
                        oPopup.AppendMenu( MF_SEPARATOR );
                        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_SCENE_ADD_OBJECT, "Add Object as JSON..." );
                        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_SCENE_ADD_LIGHT, "Add Light as JSON..." );
                        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_SCENE_IMPORT_MODEL, "Import Model (STL, OBJ)..." );
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
                        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_OBJECT_EDIT_JSON, "Edit as JSON..." );
                        oPopup.AppendMenu( MF_SEPARATOR );
                        oPopup.AppendMenu( MF_STRING | MF_ENABLED, IDM_OBJECT_DELETE, "Delete" );
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

/////////////////////////////////////////////////////////////////////////////
// Editing parts of the scene as JSON

void CSceneTreeView::RebuildTree( void )
{
    CRayDoc *pDoc = GetDocument();

    pDoc->GetScene().ForgetTreeItems();
    Empty();
    OnUpdate( NULL, 0, NULL );
    pDoc->SetModifiedFlag( TRUE );
    pDoc->UpdateAllViews( this );
}

std::string CSceneTreeView::SelectedPart( void )
{
    HTREEITEM   hItem = GetTreeCtrl().GetSelectedItem();
    CTreeData  *pData = hItem ? (CTreeData *)GetTreeCtrl().GetItemData( hItem ) : NULL;
    CScene     &oScene = GetDocument()->GetScene();

    if ( pData == NULL || pData->m_nType != TYPE_OBJECT || pData->m_pData == NULL )
        return std::string();

    CObjects   *pObject = (CObjects *)pData->m_pData;
    int         nIndex;

    if ( pObject->IsCamera() )
        return "camera";
    if ( ( nIndex = oScene.IndexOfObject( pObject ) ) >= 0 )
        return "objects/" + std::to_string( nIndex );
    if ( ( nIndex = oScene.IndexOfLight( pObject ) ) >= 0 )
        return "lights/" + std::to_string( nIndex );
    return std::string();
}

void CSceneTreeView::EditPartAsJson( const char *strPart, const char *strTitle, const char *strNote, const char *strStart )
{
    CRayDoc        *pDoc = GetDocument();
    CScene         &oScene = pDoc->GetScene();
    std::string     strPath = (LPCSTR)pDoc->GetPathName();
    size_t          nSlash = strPath.find_last_of( "/\\" );
    std::string     strDirectory = ( nSlash == std::string::npos ) ? std::string() : strPath.substr( 0, nSlash );
    std::string     strPartName( strPart );
    std::string     strText = strStart ? std::string( strStart ) : oScene.PartToJson( strPartName );
    BOOL            bChanged = FALSE;

    CJsonEditDlg oDialog( strTitle, strNote, strText,
                          [ & ]( const std::string &strNew, std::string &strError ) -> BOOL
                          {
                                  // The archive the scene came from may hold
                                  // the files the text names.
                              if ( oScene.EditPart( strPartName, strNew, strDirectory, strPath ) != 0 )
                              {
                                  strError = oScene.GetLoadError();
                                  return FALSE;
                              }
                              bChanged = TRUE;
                              return TRUE;
                          }, this );

    oDialog.DoModal();
    if ( bChanged )
        RebuildTree();
}

void CSceneTreeView::OnObjectEditJson()
{
    std::string strPart = SelectedPart();

    if ( strPart.empty() )
        return;
    EditPartAsJson( strPart.c_str(), "Edit as JSON",
                    "Its fields are as in a scene file (scenes/README.md). Leave it empty to delete it (not the camera)." );
}

void CSceneTreeView::OnObjectDelete()
{
    std::string strPart = SelectedPart();
    CRayDoc    *pDoc = GetDocument();

    if ( strPart.empty() || strPart == "camera" )
        return;
    if ( AfxMessageBox( "Delete it from the scene?", MB_YESNO | MB_ICONQUESTION ) != IDYES )
        return;
    if ( pDoc->GetScene().EditPart( strPart, "", "" ) == 0 )
        RebuildTree();
}

void CSceneTreeView::OnSceneEditSettings()
{
    EditPartAsJson( "settings", "Edit Settings as JSON",
                    "imageWidth, imageHeight, imageFile, colorDepth, maximumDepth, depthThreshold." );
}

void CSceneTreeView::OnSceneEditCamera()
{
    EditPartAsJson( "camera", "Edit Camera as JSON",
                    "Position and aim, and its sampler (anti-aliasing), colorModel, exposure, clamp and lens: see scenes/README.md." );
}

void CSceneTreeView::OnSceneEditEnvironment()
{
    CScene     &oScene = GetDocument()->GetScene();
    const char *strStart = NULL;

    if ( oScene.GetEnvironment() == NULL )
        strStart = "{\n    \"type\": \"sky\",\n    \"zenith\": [0.25, 0.45, 0.85],\n    \"horizon\": [0.85, 0.9, 1.0],\n"
                   "    \"ground\": [0.35, 0.3, 0.25]\n}\n";
    EditPartAsJson( "environment", "Edit Environment as JSON",
                    "A \"sky\" gradient or an \"image\" (an HDR, PNG or PPM file). Leave it empty for the plain background color.",
                    strStart );
}

void CSceneTreeView::OnSceneAddObject()
{
    EditPartAsJson( "objects/+", "Add Object as JSON",
                    "sphere, plane, box, cylinder, cone, triangle, mesh (vertices and faces, or a model \"file\") or csg: see scenes/README.md.",
                    "{\n    \"type\": \"box\",\n    \"center\": [0, 0.5, 0],\n    \"size\": 1,\n    \"rotation\": [0, 30, 0],\n"
                    "    \"materials\": [\n        { \"type\": \"ambient\", \"color\": [0.08, 0.08, 0.08] },\n"
                    "        { \"type\": \"diffuse\", \"influence\": 0.85, \"color\": [0.8, 0.8, 0.8] }\n    ]\n}\n" );
}

void CSceneTreeView::OnSceneAddLight()
{
    EditPartAsJson( "lights/+", "Add Light as JSON",
                    "point, sphereLight, rectangleLight, spot or directional: see scenes/README.md.",
                    "{\n    \"type\": \"spot\",\n    \"position\": [0, 8, -6],\n    \"direction\": [0, -0.8, 0.6],\n"
                    "    \"color\": [1, 1, 1],\n    \"angle\": 30,\n    \"falloff\": 5\n}\n" );
}

void CSceneTreeView::OnSceneImportModel()
{
    CFileDialog oOpen( TRUE, NULL, NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
                       "Models (*.stl;*.obj)|*.stl;*.obj|All Files (*.*)|*.*||", this );

    if ( oOpen.DoModal() != IDOK )
        return;

        // Forward slashes, and quotes escaped, so the path is good JSON.
    std::string strFile = (LPCSTR)oOpen.GetPathName();
    std::string strJson;

    for ( size_t nIndex = 0; nIndex < strFile.size(); nIndex++ )
    {
        if ( strFile[ nIndex ] == '\\' )
            strJson += '/';
        else if ( strFile[ nIndex ] == '"' )
            strJson += "\\\"";
        else
            strJson += strFile[ nIndex ];
    }

    std::string strStart = "{\n    \"type\": \"mesh\",\n    \"file\": \"" + strJson + "\",\n    \"fitSize\": 2,\n"
                           "    \"rotation\": [-90, 0, 0],\n    \"align\": \"bottom\",\n"
                           "    \"materials\": [\n        { \"type\": \"ambient\", \"color\": [0.08, 0.08, 0.08] },\n"
                           "        { \"type\": \"diffuse\", \"influence\": 0.85, \"color\": [0.8, 0.8, 0.8] },\n"
                           "        { \"type\": \"specular\", \"influence\": 0.3, \"color\": [1, 1, 1], \"power\": 0.25 }\n    ]\n}\n";

    EditPartAsJson( "objects/+", "Import Model",
                    "Placed 2 units across, turned from z up (most STL files) to y up, standing on y = 0. The file goes into the scene when it is saved.",
                    strStart.c_str() );
}

void CSceneTreeView::OnSettingsProperties()
{
	CRayDoc      *pDoc = GetDocument();
    CScene      &oScene = pDoc->GetScene();
    CSettings   &oSettings = oScene.GetSettings();

    oSettings.Properties();
}
