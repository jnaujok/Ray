// TransparentMatDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTransparentMatDlg dialog

class CTransparentMatDlg : public CDialog
{
// Construction
public:
	CTransparentMatDlg(CWnd* pParent = NULL);   // standard constructor
    CColor m_oColor;
// Dialog Data
	//{{AFX_DATA(CTransparentMatDlg)
	enum { IDD = IDD_TRANSPARENT_DIALOG };
	double	m_nIndexOfRefraction;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTransparentMatDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTransparentMatDlg)
	afx_msg void OnSetColor();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
