// SettingsDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSettingsDialog dialog

class CSettingsDialog : public CDialog
{
// Construction
public:
	CSettingsDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSettingsDialog)
	enum { IDD = IDD_SETTINGS_DIALOG };
	CString	m_strFileName;
	int		m_nImageHeight;
	int		m_nImageWidth;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSettingsDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSettingsDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
