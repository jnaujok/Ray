// SetColorDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "Ray.h"
#include "SetColorDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSetColorDlg dialog


CSetColorDlg::CSetColorDlg( CColor *pColor, CWnd* pParent /*=NULL*/)
	: CDialog(CSetColorDlg::IDD, pParent)
{
    m_poColor = pColor;
	//{{AFX_DATA_INIT(CSetColorDlg)
    m_strRed = _T("");
    m_strGreen = _T("");
    m_strBlue = _T("");
	//}}AFX_DATA_INIT
    m_strRed.Format( "%#3.2f", pColor->GetR() );
    m_strGreen.Format( "%#3.2f", pColor->GetG() );
    m_strBlue.Format( "%#3.2f", pColor->GetB() );
}


void CSetColorDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CSetColorDlg)
	DDX_Control(pDX, IDC_SLIDER_RED, m_oRedSlider);
	DDX_Control(pDX, IDC_SLIDER_GREEN, m_oGreenSlider);
	DDX_Control(pDX, IDC_SLIDER_BLUE, m_oBlueSlider);
	DDX_Text(pDX, IDC_BLUE_VALUE, m_strBlue);
	DDX_Text(pDX, IDC_GREEN_VALUE, m_strGreen);
	DDX_Text(pDX, IDC_RED_VALUE, m_strRed);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSetColorDlg, CDialog)
	//{{AFX_MSG_MAP(CSetColorDlg)
	ON_WM_HSCROLL()
	//}}AFX_MSG_MAP
    ON_MESSAGE( WM_USER+1, OnInitPaint )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSetColorDlg message handlers

void CSetColorDlg::OnOK() 
{
    m_poColor->Set( (FLT)((FLT)m_oRedSlider.GetPos() / 512.0),
                    (FLT)((FLT)m_oGreenSlider.GetPos() / 512.0 ),
                    (FLT)((FLT)m_oBlueSlider.GetPos() / 512 ) );
	CDialog::OnOK();
}



BOOL CSetColorDlg::OnInitDialog() 
{
    CWnd       *pSample;
    CRect       oRect;
    CDC        *pDC;
    CBrush      oBrush( (COLORREF)(*m_poColor) );

    CDialog::OnInitDialog();
// CSliderCtrl

    m_oRedSlider.SetRange( 0, 510 );
    m_oRedSlider.SetTicFreq( 51 );
    m_oRedSlider.SetLineSize( 2 );
    m_oRedSlider.SetPageSize( 32 );
    m_oRedSlider.SetPos( (int)( m_poColor->GetR() * 512.0 ) );

    m_oGreenSlider.SetRange( 0, 510 );
    m_oGreenSlider.SetTicFreq( 51 );
    m_oGreenSlider.SetLineSize( 2 );
    m_oGreenSlider.SetPageSize( 32 );
    m_oGreenSlider.SetPos( (int)( m_poColor->GetG() * 512.0 ) );

    m_oBlueSlider.SetRange( 0, 510 );
    m_oBlueSlider.SetTicFreq( 51 );
    m_oBlueSlider.SetLineSize( 2 );
    m_oBlueSlider.SetPageSize( 32 );
    m_oBlueSlider.SetPos( (int)( m_poColor->GetB() * 512.0 ) );

    PostMessage( WM_USER + 1, 0, 0 );
    pSample = GetDlgItem( IDC_SAMPLE_FRAME );
    pSample->GetClientRect( oRect );
    pDC = pSample->GetDC();
    pDC->FillRect( oRect, &oBrush );
    pSample->ReleaseDC( pDC );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSetColorDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
    CWnd       *pSample;
    CRect       oRect;
    CDC        *pDC;
    CBrush      oBrush( RGB( m_oRedSlider.GetPos() / 2, m_oGreenSlider.GetPos() / 2, m_oBlueSlider.GetPos() / 2 ) );
    FLT         nRed, nGreen, nBlue;


    switch ( nSBCode )
    {
        case TB_BOTTOM:
        case TB_ENDTRACK:
        case TB_LINEDOWN:
        case TB_LINEUP:
        case TB_PAGEDOWN:
        case TB_PAGEUP:
        case TB_THUMBPOSITION:
        case TB_THUMBTRACK:
        case TB_TOP:

            if ( m_oRedSlider.m_hWnd )
            {
                nRed   = ((FLT)m_oRedSlider.GetPos()) / 512.0;
                nGreen = ((FLT)m_oGreenSlider.GetPos()) / 512.0;
                nBlue  = ((FLT)m_oBlueSlider.GetPos()) / 512.0;

                m_strRed.Format( "%#3.2f", nRed );
                m_strGreen.Format( "%#3.2f", nGreen );
                m_strBlue.Format( "%#3.2f", nBlue );
                UpdateData( FALSE );
            }
            pSample = GetDlgItem( IDC_SAMPLE_FRAME );
            pSample->GetClientRect( oRect );
            pDC = pSample->GetDC();
            pDC->FillRect( oRect, &oBrush );
            pSample->ReleaseDC( pDC );
            break;

        default:
            break;
    }

    CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

LONG CSetColorDlg::OnInitPaint( UINT wParam, LONG lParam )
{
    CWnd       *pSample;
    CRect       oRect;
    CDC        *pDC;
    CBrush      oBrush( RGB( m_oRedSlider.GetPos() / 2, m_oGreenSlider.GetPos() / 2, m_oBlueSlider.GetPos() / 2 ) );

    pSample = GetDlgItem( IDC_SAMPLE_FRAME );
    pSample->GetClientRect( oRect );
    pDC = pSample->GetDC();
    pDC->FillRect( oRect, &oBrush );
    pSample->ReleaseDC( pDC );

    return 0;
}

