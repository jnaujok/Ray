#ifndef __RAYDOC_H__
#define __RAYDOC_H__

// RayDoc.h : interface of the CRayDoc class
//
/////////////////////////////////////////////////////////////////////////////
#include "Scene.h"

class CTaskMaster;

#include <atomic>

    // Shared by the document and its tracing threads; the flags are atomic
    // because several threads read and write them.
typedef struct
{
    CScene             *pScene;
    CBitmap            *pBitmap;
    CRect              *pRect;
    std::atomic<int>    bStop;
    std::atomic<int>    nTracing;
    std::atomic<int>    bTracing;
    int                 nQuarter;
    CDC * volatile      pDC;
    std::atomic<int>    bChoosing;
    CTaskMaster        *poTaskMaster;
} TRACE_THREAD_DATA;

class CRayDoc : public CDocument
{
    protected: // create from serialization only
	    CRayDoc();
	    DECLARE_DYNCREATE(CRayDoc)

        CTaskMaster            *m_poTaskMaster;
        // Attributes
    public:
	    BOOL                    m_bTracing;
        TRACE_THREAD_DATA       m_oData;
        CBitmap                *GetTrace( void );
        CDC                    *GetOngoingTrace( void );
        void                    RenderRayTrace();
        BOOL                    CheckTrace( void );


        // Operations
    public:

        // Overrides
	    // ClassWizard generated virtual function overrides
	    //{{AFX_VIRTUAL(CRayDoc)
	    public:
	        virtual BOOL        OnNewDocument();
	        virtual void        Serialize(CArchive& ar);
	        virtual BOOL        OnOpenDocument(LPCTSTR lpszPathName);
	        virtual BOOL        OnSaveDocument(LPCTSTR lpszPathName);
	    //}}AFX_VIRTUAL

        // Implementation
    public:
	    virtual                ~CRayDoc();

#ifdef _DEBUG
    	virtual void            AssertValid() const;
	    virtual void            Dump(CDumpContext& dc) const;
#endif

    protected:
    	CScene                  m_oScene;
    public:
        CScene                 &GetScene( void ) { return m_oScene; }
    protected:
        CBitmap                *m_pTraced;

        // Generated message map functions
    protected:
	    //{{AFX_MSG(CRayDoc)
	    afx_msg void OnObjectAddPlane();
	    afx_msg void OnObjectAddSphere();
	    afx_msg void OnObjectDelete();
	    afx_msg void OnLightAddPointLight();
	    afx_msg void OnRenderKillTrace();
	    afx_msg void OnUpdateRenderKillTrace(CCmdUI* pCmdUI);
	afx_msg void OnObjectsAddTriangle();
	//}}AFX_MSG
	    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif