// SetColorDlg.h : header file
//

#include "Objects.h"

/////////////////////////////////////////////////////////////////////////////
// CSetColorDlg dialog

class CSetColorDlg : public CDialog
{
// Construction
public:
	CSetColorDlg( CColor *pColor, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSetColorDlg)
	enum { IDD = IDD_SET_COLOR_DIALOG };
	CSliderCtrl	m_oRedSlider;
	CSliderCtrl	m_oGreenSlider;
	CSliderCtrl	m_oBlueSlider;
	CString	m_strBlue;
	CString	m_strGreen;
	CString	m_strRed;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSetColorDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    CColor  *m_poColor;

	// Generated message map functions
	//{{AFX_MSG(CSetColorDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	afx_msg LONG OnInitPaint( UINT, LONG );
    DECLARE_MESSAGE_MAP()
};
