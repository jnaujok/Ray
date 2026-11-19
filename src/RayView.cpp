// RayView.cpp : implementation of the CRayView class
//

#include "StdAfx.h"
#include "Ray.h"
#include "SceneTreeView.h"
#include "MainFrm.h"

#include "RayDoc.h"
#include "RayView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define UPDATE_BITMAP   10

/////////////////////////////////////////////////////////////////////////////
// CRayView

IMPLEMENT_DYNCREATE(CRayView, CScrollView)

BEGIN_MESSAGE_MAP(CRayView, CScrollView)
	//{{AFX_MSG_MAP(CRayView)
	ON_WM_TIMER()
	ON_COMMAND(IDC_RENDER_RAY_TRACE, OnRenderRayTrace)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CScrollView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRayView construction/destruction

CRayView::CRayView()
{
	m_hTimer = 0;

}

CRayView::~CRayView()
{
    if ( m_hTimer )
        KillTimer( m_hTimer );
}

BOOL CRayView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CScrollView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CRayView drawing

void CRayView::OnDraw(CDC* pDC)
{
	CRayDoc        *pDoc = GetDocument();
	ASSERT_VALID(pDoc);
    CBitmap        *pTrace;
    CDC            *pTempDC;

    if ( m_hTimer )
    {
        if ( ! pDoc->CheckTrace() )
        {
            KillTimer( m_hTimer );
            m_hTimer = 0;
        }
    }
        // Check if there is an ongoing trace. If so, we need to copy from
        // the currently plotting DC to get an image.
    if ( pTempDC = pDoc->GetOngoingTrace() )
    {
        CScene &oScene = pDoc->GetScene();
        CSettings &oSettings = oScene.GetSettings();

        pDC->BitBlt( 0, 0, oSettings.GetImageWidth(), oSettings.GetImageHeight(), pTempDC, 0, 0, SRCCOPY );
    }
    else
    {
        CScene &oScene = pDoc->GetScene();
        CSettings &oSettings = oScene.GetSettings();

        pTrace = pDoc->GetTrace();
        if ( pTrace )
        {
            CDC         oDCTemp;
            CBitmap    *pOldBitmap;

            oDCTemp.CreateCompatibleDC( pDC );
            pOldBitmap = oDCTemp.SelectObject( pTrace );
            pDC->BitBlt( 0, 0, oSettings.GetImageWidth(), oSettings.GetImageHeight(), &oDCTemp, 0, 0, SRCCOPY );
            oDCTemp.SelectObject( pOldBitmap );
        }
    }
}

void CRayView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();
	CRayDoc        *pDoc = GetDocument();
    ASSERT( pDoc );
    CScene         &oScene = pDoc->GetScene();
    CSettings      &oSettings = oScene.GetSettings();
    CSize           oWholeSize( oSettings.GetImageWidth(), oSettings.GetImageHeight() );
    CSize           oPage( 40, 40 );
    CSize           oLine( 5, 5 );

    SetScrollSizes( MM_TEXT, oWholeSize, oPage, oLine );
}

/////////////////////////////////////////////////////////////////////////////
// CRayView printing

BOOL CRayView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CRayView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CRayView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CRayView diagnostics

#ifdef _DEBUG
void CRayView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CRayView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}

CRayDoc* CRayView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CRayDoc)));
	return (CRayDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CRayView message handlers

void CRayView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
    CScrollView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

void CRayView::OnTimer(UINT_PTR nIDEvent) 
{
    Invalidate( FALSE );
    
	CScrollView::OnTimer(nIDEvent);
}

void CRayView::OnRenderRayTrace() 
{
	CRayDoc      *pDoc = GetDocument();

        // Start the Ray Trace
    pDoc->RenderRayTrace();

    if ( m_hTimer == 0 )
        m_hTimer = SetTimer( UPDATE_BITMAP, 1000, NULL );
}

void CRayView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CRayDoc        *pDoc = GetDocument();
    ASSERT( pDoc );
    CScene         &oScene = pDoc->GetScene();
    CSettings      &oSettings = oScene.GetSettings();
    CSize           oWholeSize( oSettings.GetImageWidth(), oSettings.GetImageHeight() );
    CSize           oPage( 40, 40 );
    CSize           oLine( 5, 5 );

    SetScrollSizes( MM_TEXT, oWholeSize, oPage, oLine );

    if ( lHint == 999 )
    {
        if ( m_hTimer == 0 )
            m_hTimer = SetTimer( UPDATE_BITMAP, 1000, NULL );
    }
}
