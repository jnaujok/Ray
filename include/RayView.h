// RayView.h : interface of the CRayView class
//
/////////////////////////////////////////////////////////////////////////////

class CRayView : public CScrollView
{
protected: // create from serialization only
	CRayView();
	DECLARE_DYNCREATE(CRayView)

// Attributes
public:
	UINT m_hTimer;
	CRayDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRayView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CRayView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CRayView)
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnRenderRayTrace();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in RayView.cpp
inline CRayDoc* CRayView::GetDocument()
   { return (CRayDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
