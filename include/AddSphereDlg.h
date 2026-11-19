// AddSphereDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddSphereDlg dialog

class CAddSphereDlg : public CDialog
{
// Construction
public:
	CAddSphereDlg( CVector *poPosition, FLT *pnRadius, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddSphereDlg)
	enum { IDD = IDD_ADD_OBJECT_SPHERE };
	double	m_nRadius;
	double	m_nX;
	double	m_nY;
	double	m_nZ;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddSphereDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddSphereDlg)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP();
    CVector    *m_poPosition;
    FLT        *m_pnRadius;
};
