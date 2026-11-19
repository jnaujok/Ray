// AddMaterialsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddMaterialsDlg dialog

class CAddMaterialsDlg : public CDialog
{
// Construction
public:
	CAddMaterialsDlg(CWnd* pParent = NULL);   // standard constructor
    CMaterialList       m_oMaterialList;

// Dialog Data
	//{{AFX_DATA(CAddMaterialsDlg)
	enum { IDD = IDD_ADD_MATERIALS_DLG };
	CListCtrl	m_oAddList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddMaterialsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddMaterialsDlg)
	afx_msg void OnAddButton();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
