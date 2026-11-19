// InfluenceDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CInfluenceDlg dialog

class CInfluenceDlg : public CDialog
{
// Construction
public:
	CInfluenceDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CInfluenceDlg)
	enum { IDD = IDD_INFLUENCE_DLG };
	double	m_nInfluence;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInfluenceDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CInfluenceDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
