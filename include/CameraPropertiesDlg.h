// CameraPropertiesDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCameraPropertiesDlg dialog

class CCameraPropertiesDlg : public CDialog
{
// Construction
public:
	CCameraPropertiesDlg( CVector *poPositon, CVector *poPointAt, CWnd* pParent = NULL);   // standard constructor

    CVector *m_poPosition;
    CVector *m_poPointAt;
// Dialog Data
	//{{AFX_DATA(CCameraPropertiesDlg)
	enum { IDD = IDD_CAMERA_PROPERTIES };
	double	m_nPointAtX;
	double	m_nPointAtY;
	double	m_nPointAtZ;
	double	m_nX;
	double	m_nY;
	double	m_nZ;
	double	m_nCameraAngle;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCameraPropertiesDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCameraPropertiesDlg)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
