// SpherePropertiesDlg.h : header file
//

#include "Objects.h"
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CSpherePropertiesDlg dialog

class CSpherePropertiesDlg : public CDialog
{
// Construction
public:
	CSpherePropertiesDlg( CVector *oPosition, FLT *nRadius, CWnd* pParent = NULL);   // standard constructor

    CVector    *m_poPos;
    FLT        *m_pnRadius;
    CColor      m_oColor;
    BOOL        m_bColorChanged;

// Dialog Data
	//{{AFX_DATA(CSpherePropertiesDlg)
	enum { IDD = IDD_SPHERE_PROPERTIES };
	double	m_nX;
	double	m_nY;
	double	m_nZ;
	double	m_nRadius;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpherePropertiesDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSpherePropertiesDlg)
	virtual void OnOK();
	afx_msg void OnSetColor();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
