// DiffuseMaterialPropDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDiffuseMaterialPropDlg dialog

class CDiffuseMaterialPropDlg : public CDialog
{
// Construction
public:
    CColor  m_oColor;
	CDiffuseMaterialPropDlg( CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDiffuseMaterialPropDlg)
	enum { IDD = IDD_DIFFUSE_DIALOG };
	BOOL	m_bCastDiffuse;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDiffuseMaterialPropDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDiffuseMaterialPropDlg)
	afx_msg void OnSetColor();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
