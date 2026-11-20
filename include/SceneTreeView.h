#ifndef __SCENE_TREE_VIEW_H__
#define __SCENE_TREE_VIEW_H__

// SceneTreeView.h : interface of the CSceneTreeView class
//
/////////////////////////////////////////////////////////////////////////////

#include <string>

class CRayDoc;
class CObjects;
class CMaterial;
class CTreeData;

class CSceneTreeView : public CTreeView
{
protected: // create from serialization only
	CSceneTreeView();
	DECLARE_DYNCREATE(CSceneTreeView)

// Attributes
public:
	BOOL m_bInitialized;
	CRayDoc    *GetDocument();
    CImageList  m_oImageList;
    HTREEITEM   m_hRoot;
    CTreeData  *m_pDataItems;

// Operations
public:
    HTREEITEM   AddObject( CObjects *pObject, int nImage, LPCSTR strName, HTREEITEM hUnder = TVI_ROOT );
    HTREEITEM   AddMaterial( CMaterial *pObject, int nImage, LPCSTR strName, HTREEITEM hUnder = TVI_ROOT );
    void        Empty( void );
    void        EmptyUnder( HTREEITEM hUnder );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSceneTreeView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
    virtual void OnActivateFrame( UINT nState, CFrameWnd* pFrameWnd );
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSceneTreeView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CSceneTreeView)
	afx_msg void OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnObjectProperties();
	afx_msg void OnMaterialProperties();
	afx_msg void OnObjectMaterials();
	afx_msg void OnRenderRayTrace();
	afx_msg void OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSettingsProperties();
	//}}AFX_MSG
	afx_msg void OnObjectEditJson();
	afx_msg void OnObjectDelete();
	afx_msg void OnSceneEditSettings();
	afx_msg void OnSceneEditCamera();
	afx_msg void OnSceneEditEnvironment();
	afx_msg void OnSceneAddObject();
	afx_msg void OnSceneAddLight();
	afx_msg void OnSceneImportModel();
	DECLARE_MESSAGE_MAP()

        // Shows a part of the scene ("camera", "objects/2", "lights/+", ...)
        // as JSON to edit, and puts it back; rebuilds the tree after a change.
    void EditPartAsJson( const char *strPart, const char *strTitle, const char *strNote, const char *strStart = NULL );
        // The selected object's part ("objects/N", "lights/N", "camera"),
        // or empty.
    std::string SelectedPart( void );
    void RebuildTree( void );
};

#ifndef _DEBUG  // debug version in RayView.cpp
inline CRayDoc* CSceneTreeView::GetDocument()
   { return (CRayDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

#endif
