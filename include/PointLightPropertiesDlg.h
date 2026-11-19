// PointLightPropertiesDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPointLightPropertiesDlg dialog

class CPointLightPropertiesDlg : public CDialog
{
// Construction
public:
	CPointLightPropertiesDlg( CVector *, CColor *, CWnd* pParent = NULL);   // standard constructor

    CVector *m_poPosition;
    CColor  *m_poColor;
// Dialog Data
	//{{AFX_DATA(CPointLightPropertiesDlg)
	enum { IDD = IDD_POINT_LIGHT_PROPERTIES };
	double	m_nX;
	double	m_nY;
	double	m_nZ;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPointLightPropertiesDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPointLightPropertiesDlg)
	virtual void OnOK();
	afx_msg void OnSetColor();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
