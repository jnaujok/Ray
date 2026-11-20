// RayDoc.cpp : implementation of the CRayDoc class
//

#include "StdAfx.h"
#include "Ray.h"
#include "PtLight.h"
#include "Sphere.h"
#include "Plane.h"
#include "AddPtLightDlg.h"
#include "AddSphereDlg.h"
#include "AddPlaneDlg.h"
#include "MainFrm.h"
#include "ImageFile.h"
#include "ThreadTaskMaster.h"
#include "Triangle.h"

#include "RayDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRayDoc

IMPLEMENT_DYNCREATE(CRayDoc, CDocument)

BEGIN_MESSAGE_MAP(CRayDoc, CDocument)
	//{{AFX_MSG_MAP(CRayDoc)
	ON_COMMAND(ID_OBJECT_ADD_PLANE, OnObjectAddPlane)
	ON_COMMAND(ID_OBJECT_ADD_SPHERE, OnObjectAddSphere)
	ON_COMMAND(ID_OBJECT_DELETE, OnObjectDelete)
	ON_COMMAND(ID_LIGHT_ADD_POINT_LIGHT, OnLightAddPointLight)
	ON_COMMAND(ID_RENDER_KILL_TRACE, OnRenderKillTrace)
	ON_UPDATE_COMMAND_UI(ID_RENDER_KILL_TRACE, OnUpdateRenderKillTrace)
	ON_COMMAND(ID_OBJECTS_ADD_TRIANGLE, OnObjectsAddTriangle)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRayDoc construction/destruction

CRayDoc::CRayDoc()
{
	m_pTraced = NULL;
    m_bTracing = FALSE;
    m_oData.pDC = NULL;
    m_poTaskMaster = NULL;
}

CRayDoc::~CRayDoc()
{
    delete m_pTraced;
    delete m_poTaskMaster;
}

BOOL CRayDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CRayDoc serialization

void CRayDoc::Serialize(CArchive& ar)
{
}

/////////////////////////////////////////////////////////////////////////////
// CRayDoc diagnostics

#ifdef _DEBUG
void CRayDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CRayDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CRayDoc commands

void CRayDoc::OnObjectAddPlane() 
{
	CPlane         *pPlane;
    CVector         oNormal;
    CVector         oPosition;
    CAddPlaneDlg    oDialog( &oPosition, &oNormal );

    if ( oDialog.DoModal() == IDOK )
    {
        pPlane = new CPlane( oPosition, oNormal );
        m_oScene.AddObject( pPlane );
        UpdateAllViews( NULL );
        SetModifiedFlag( TRUE );
    }
}

void CRayDoc::OnObjectAddSphere() 
{
	CSphere        *pSphere;
    CVector         oPosition;
    FLT             nRadius;
    CAddSphereDlg   oDialog( &oPosition, &nRadius );

    if ( oDialog.DoModal() == IDOK )
    {
        pSphere = new CSphere( oPosition, nRadius );
        m_oScene.AddObject( pSphere );
        UpdateAllViews( NULL );
        SetModifiedFlag( TRUE );
    }
}

void CRayDoc::OnObjectDelete() 
{
	// TODO: Add your command handler code here
    SetModifiedFlag( TRUE );
}

void CRayDoc::OnLightAddPointLight() 
{
	CPointLight    *pLight;
    CVector         oColor;
    CVector         oPosition;
    CAddPtLightDlg  oDialog( &oColor, &oPosition );

    if ( oDialog.DoModal() == IDOK )
    {
        pLight = new CPointLight( oColor, oPosition );
        m_oScene.AddLight( pLight );
        UpdateAllViews( NULL );
        SetModifiedFlag( TRUE );
    }

}

UINT SceneTracer( LPVOID pParam )
{
    TRACE_THREAD_DATA   *pData = (TRACE_THREAD_DATA *)pParam;
    CTaskMaster        *pTaskMaster = pData->poTaskMaster;
    CSettings          &oSettings = pData->pScene->GetSettings();
    CImageFile         oImageFile( oSettings.GetImageFileName(), oSettings.GetImageWidth(), oSettings.GetImageHeight(), FALSE );
 
    pData->nTracing++;

    pData->bChoosing = FALSE;

    pTaskMaster->TraceScene( *(pData->pScene), oImageFile );

    pTaskMaster->EndTasks();

    pData->nTracing--;

    return 0;
}

UINT TraceThread( LPVOID pParam )
{
    TRACE_THREAD_DATA  *pData = (TRACE_THREAD_DATA *)pParam;
    CDC                 oDC;
    CBitmap            *pOldBitmap;
    CSettings          &oSettings = pData->pScene->GetSettings();
    int                 nX, nY;
    CColor              oColor;
    int                 nLastY;
    int                 nPreviousY = 0;

        // Create the device context to draw on.
    oDC.CreateCompatibleDC( NULL );
    pOldBitmap = oDC.SelectObject( pData->pBitmap );

    pData->pDC = &oDC;
  
    pData->nTracing = 0;

    pData->nQuarter = 1;

    pData->bChoosing = TRUE;

    AfxBeginThread( SceneTracer, pData );

    while ( pData->bChoosing )
        Sleep( 10 );

        // The tracer has created the image file by now; this thread only
        // reads it back to draw the progress. (It used to open the file for
        // writing too, which truncated it under the tracer.)
    CImageFile          oImageFile( oSettings.GetImageFileName(), oSettings.GetImageWidth(), oSettings.GetImageHeight(), TRUE );

        // Wait for all the tracer threads to finish
    while ( pData->nTracing )
    {
        Sleep(1000);
        nX = 0;
        nY = nPreviousY;
        nLastY = ( oSettings.GetImageHeight() * pData->poTaskMaster->Status() ) / 10000;

        while ( nY < nLastY )
        {
            while ( nX < oSettings.GetImageWidth() )
            {
                oColor = oImageFile.GetPoint( nX, nY );
                oDC.SetPixel( nX, nY, (COLORREF)oColor );
                nX++;
            }
            nX = 0;
            nY++;
        }
        nPreviousY = nLastY;
    }

    nX = 0;
    nY = 0;
    while ( nY < oSettings.GetImageHeight() )
    {
        while ( nX < oSettings.GetImageWidth() )
        {
            oColor = oImageFile.GetPoint( nX, nY );
            oDC.SetPixel( nX, nY, (COLORREF)oColor );
            nX++;
        }
        nX = 0;
        nY++;
    }

    oDC.SelectObject( pOldBitmap );
    
    pData->pDC = NULL;

    pData->bTracing = FALSE;
    return 0;
}

void CRayDoc::RenderRayTrace() 
{
    static CRect        oRect;
    CDC                 oDC;
    CSettings          &oSettings = m_oScene.GetSettings();
    CBitmap            *pOldBitmap;
    CBrush              oBlackBrush( RGB( 0, 0, 0 ) );
    CRect               oFillRect( 0, 0, oSettings.GetImageWidth(), oSettings.GetImageHeight() );


    if ( m_bTracing )
        m_poTaskMaster->EndTrace();

    if ( m_poTaskMaster )
    {
        delete m_poTaskMaster;
        m_poTaskMaster = NULL;
    }

    if ( m_pTraced )
        delete m_pTraced;

    oDC.CreateCompatibleDC( NULL );
    m_pTraced = new CBitmap;
    m_pTraced->CreateBitmap( oSettings.GetImageWidth(), oSettings.GetImageHeight(), oDC.GetDeviceCaps( PLANES ), oDC.GetDeviceCaps( BITSPIXEL ), NULL );
    pOldBitmap = oDC.SelectObject( m_pTraced );
    oDC.FillRect( oFillRect, &oBlackBrush );
    oDC.SelectObject( pOldBitmap );
    oDC.DeleteDC();

    oRect.top = 0;
    oRect.left = 0;
    oRect.right = oSettings.GetImageWidth() - 1;
    oRect.bottom = oSettings.GetImageHeight() - 1;

    if ( !m_poTaskMaster )
        m_poTaskMaster = new CThreadTaskMaster;

    m_poTaskMaster->SpawnTasks( 4 );

    m_oData.pScene = &m_oScene;
    m_oData.pBitmap = m_pTraced;
    m_oData.pRect = &oRect;
    m_bTracing = TRUE;
    m_oData.bTracing = m_bTracing;
    m_oData.bStop = FALSE;
    m_oData.pDC = NULL;
    m_oData.poTaskMaster = m_poTaskMaster;

    AfxBeginThread( TraceThread, &m_oData );
}

CBitmap *CRayDoc::GetTrace( void )
{
    return m_pTraced;
}

CDC *CRayDoc::GetOngoingTrace( void )
{
    return m_oData.pDC;
}

BOOL CRayDoc::CheckTrace( void )
{
    if ( m_bTracing )
    {
        if ( m_oData.bTracing )
            return TRUE;
    }
    m_oData.bTracing = FALSE;
    m_bTracing = FALSE;
    m_oData.pDC = NULL;

    return FALSE;
}

void CRayDoc::OnRenderKillTrace() 
{
    if ( m_bTracing )
        if ( m_poTaskMaster->EndTrace() == FALSE )
            ::MessageBox( NULL, "The currently running Ray Trace did not respond to being stopped.", "Error", MB_ICONSTOP | MB_OK );
    m_bTracing = FALSE;
}

void CRayDoc::OnUpdateRenderKillTrace(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable( m_bTracing );
}

BOOL CRayDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
    if ( m_oScene.Load( lpszPathName ) != 0 )
    {
        AfxMessageBox( "This file is not a scene this program can read.", MB_ICONSTOP | MB_OK );
        return FALSE;
    }

    SetModifiedFlag( FALSE );

    return TRUE;
}

BOOL CRayDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
    if ( m_oScene.Save( lpszPathName ) != 0 )
    {
        AfxMessageBox( "The scene could not be saved.", MB_ICONSTOP | MB_OK );
        return FALSE;
    }

    SetModifiedFlag( FALSE );

    return TRUE;
}

void CRayDoc::OnObjectsAddTriangle() 
{
	CTriangle      *pTriangle;

    pTriangle = new CTriangle;
    m_oScene.AddObject( pTriangle );
    UpdateAllViews( NULL );
    SetModifiedFlag( TRUE );
}
