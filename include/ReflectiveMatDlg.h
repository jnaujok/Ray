// ReflectiveMatDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CReflectiveMatDlg dialog

class CReflectiveMatDlg : public CDialog
{
// Construction
public:
	CReflectiveMatDlg(CWnd* pParent = NULL);   // standard constructor

    CColor  m_oColor;
// Dialog Data
	//{{AFX_DATA(CReflectiveMatDlg)
	enum { IDD = IDD_REFLECTIVE_DIALOG };
	double	m_nDiffuseReflection;
	BOOL	m_bFilterReflection;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReflectiveMatDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CReflectiveMatDlg)
	afx_msg void OnSetColor();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
