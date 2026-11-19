// ChildFrm.cpp : implementation of the CChildFrame class
//

#include "StdAfx.h"
#include "Ray.h"
#include "RayView.h"
#include "SceneTreeView.h"

#include "ChildFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CChildFrame)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildFrame construction/destruction

CChildFrame::CChildFrame()
{
    m_bInitialized = FALSE;
}

CChildFrame::~CChildFrame()
{
}

BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	return CMDIChildWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CChildFrame diagnostics

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
	CMDIChildWnd::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
	CMDIChildWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CChildFrame message handlers

BOOL CChildFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
    CSize   oSize;
    CRect   oRect;

    GetClientRect( oRect );
    oSize.cx = oRect.right - oRect.left;
    oSize.cy = oRect.bottom - oRect.top;

    oSize.cx /= 4;
    oSize.cx *= 3;

    m_oClientPane.CreateStatic( this, 1, 2 );

    m_oClientPane.CreateView( 0, 0, RUNTIME_CLASS( CRayView ), oSize, pContext );
  
    oSize.cx /= 3;

    m_oClientPane.CreateView( 0, 1, RUNTIME_CLASS( CSceneTreeView ), oSize, pContext );

    m_bInitialized = TRUE;

    if ( pContext )
    {
        CView   *pView;

        pView = (CView *)m_oClientPane.GetPane( 0, 0 );
        if ( ! pView->GetDocument() )
        {
            pContext->m_pCurrentDoc->AddView( pView );
        }
        pView = (CView *)m_oClientPane.GetPane( 0, 1 );
        if ( ! pView->GetDocument() )
        {
            pContext->m_pCurrentDoc->AddView( pView );
        }
    }
    return TRUE;
}

void CChildFrame::OnSize(UINT nType, int cx, int cy) 
{
    int     nColWidth;
    int     nDummy;
    int     n1, n2, n3, n4;

    if ( m_bInitialized )
        m_oClientPane.GetColumnInfo( 1, nColWidth, nDummy );

	CMDIChildWnd::OnSize(nType, cx, cy);
	
    if ( m_bInitialized )
    {
        m_oClientPane.RecalcLayout();
        m_oClientPane.GetColumnInfo( 0, n1, n2 );
        m_oClientPane.GetColumnInfo( 1, n3, n4 );
        n1 = n1 + n3;
        n1 = n1 - nColWidth;
        m_oClientPane.SetColumnInfo( 0, n1, n2 );
        m_oClientPane.SetColumnInfo( 1, nColWidth, nDummy );
        m_oClientPane.RecalcLayout();
    }
}
