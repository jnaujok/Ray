// SpecMatDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSpecMatDlg dialog

class CSpecMatDlg : public CDialog
{
// Construction
public:
	CSpecMatDlg(CWnd* pParent = NULL);   // standard constructor
    CColor  m_oColor;
// Dialog Data
	//{{AFX_DATA(CSpecMatDlg)
	enum { IDD = IDD_SPECULAR_DIALOG };
	double	m_nIORValue;
	BOOL	m_bMaterialHighlights;
	double	m_nPower;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpecMatDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSpecMatDlg)
	afx_msg void OnSetColor();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
