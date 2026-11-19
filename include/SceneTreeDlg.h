#ifndef __SCENE_TREE_DLG_H__
#define __SCENE_TREE_DLG_H__

#include "resource.h"

class CObjects;
class CMaterial;

// SceneTreeDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSceneTreeDlg dialog

class CSceneTreeDlg : public CDialogBar
{
// Construction
public:
	void UpdateDoc( LPCSTR strName );
	CImageList m_oImageList;
	HTREEITEM AddObject( CObjects *pObject, int nImage, LPCSTR strName, HTREEITEM hUnder = TVI_ROOT );
	HTREEITEM AddMaterial( CMaterial *pObject, int nImage, LPCSTR strName, HTREEITEM hUnder = TVI_ROOT );


	CSceneTreeDlg(CWnd* pParent = NULL);   // standard constructor
	~CSceneTreeDlg( void );   // standard destructor

// Dialog Data
	//{{AFX_DATA(CSceneTreeDlg)
	enum { IDD = IDD_SCENE_TREE_DLG };
	CTreeCtrl	m_oSceneTree;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSceneTreeDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void Empty( void );

	int			m_nLastWidth;
	int			m_nLastHeight;
	int			m_nDockedWidth;
	int			m_nDockedHeight;
	int			m_nFreeWidth;
	int			m_nFreeHeight;

    BOOL        m_bInitialized;

	HTREEITEM	m_hRoot;

	// Generated message map functions
	//{{AFX_MSG(CSceneTreeDlg)
	afx_msg void OnSelchangedSceneTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg int OnCreate( LPCREATESTRUCT pUnused );
	afx_msg void OnRclickSceneTree(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
  

};

#endif // __SCENE_TREE_DLG_H__
