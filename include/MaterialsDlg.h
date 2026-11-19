// MaterialsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMaterialsDlg dialog

class CMaterialsDlg : public CDialog
{
// Construction
public:
	CMaterialsDlg( CWnd* pParent = NULL);   // standard constructor

    CMaterialList   m_oMaterials;
    CImageList      m_oBigIcons;
    CImageList      m_oSmallIcons;

// Dialog Data
	//{{AFX_DATA(CMaterialsDlg)
	enum { IDD = IDD_MATERIALS_DLG };
	CListCtrl	m_oMaterialList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMaterialsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMaterialsDlg)
	afx_msg void OnPropertyButton();
	virtual void OnCancel();
	afx_msg void OnDblclkMaterialList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRclickMaterialList(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	afx_msg void OnMaterialAdd();
	afx_msg void OnMaterialInfluence();
	afx_msg void OnMaterialListDelete();
	afx_msg void OnMaterialProperty();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
