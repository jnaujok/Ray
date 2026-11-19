// AddPlaneDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddPlaneDlg dialog

class CAddPlaneDlg : public CDialog
{
// Construction
public:
	CAddPlaneDlg( CVector *poPosition, CVector *poNormal, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddPlaneDlg)
	enum { IDD = IDD_ADD_OBJECT_PLANE };
	double	m_nI;
	double	m_nJ;
	double	m_nK;
	double	m_nX;
	double	m_nY;
	double	m_nZ;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddPlaneDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddPlaneDlg)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP();
    CVector    *m_poPosition;
    CVector    *m_poNormal;
};
