// AddPtLightDlg.h : header file
//

#include "Objects.h"

/////////////////////////////////////////////////////////////////////////////
// CAddPtLightDlg dialog

class CAddPtLightDlg : public CDialog
{
// Construction
public:
	CAddPtLightDlg( CVector *pColor, CVector *pPosition, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddPtLightDlg)
	enum { IDD = IDD_ADD_POINT_LIGHT };
	double	m_nBlue;
	double	m_nGreen;
	double	m_nRed;
	double	m_nX;
	double	m_nY;
	double	m_nZ;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddPtLightDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddPtLightDlg)
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP();
    CVector        *m_pPosition;
    CVector        *m_pColor;

};
