// PlanePropertiesDlg.h : header file
//

#include "Objects.h"
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CPlanePropertiesDlg dialog

class CPlanePropertiesDlg : public CDialog
{
// Construction
public:
	CPlanePropertiesDlg( CVector *oPosition, CVector *oNormal, CWnd* pParent = NULL);   // standard constructor

    CVector    *m_poPos;
    CVector    *m_poNorm;
    CColor      m_oColor;
    BOOL        m_bColorChanged;

// Dialog Data
	//{{AFX_DATA(CPlanePropertiesDlg)
	enum { IDD = IDD_PLANE_PROPERTIES };
	double	m_nI;
	double	m_nJ;
	double	m_nK;
	double	m_nX;
	double	m_nY;
	double	m_nZ;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPlanePropertiesDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPlanePropertiesDlg)
	virtual void OnOK();
	afx_msg void OnSetColor();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
