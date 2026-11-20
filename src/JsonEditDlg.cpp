// JsonEditDlg.cpp : implementation file
//
// See JsonEditDlg.h.

#include "StdAfx.h"
#include "Ray.h"
#include "JsonEditDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace
{
        // An edit control wants "\r\n" between lines; JSON text has "\n".
    std::string ToControl( const std::string &strText )
    {
        std::string strOut;

        strOut.reserve( strText.size() + strText.size() / 16 );
        for ( size_t nIndex = 0; nIndex < strText.size(); nIndex++ )
        {
            if ( strText[ nIndex ] == '\n' && ( nIndex == 0 || strText[ nIndex - 1 ] != '\r' ) )
                strOut += '\r';
            strOut += strText[ nIndex ];
        }
        return strOut;
    }

    std::string FromControl( const std::string &strText )
    {
        std::string strOut;

        strOut.reserve( strText.size() );
        for ( size_t nIndex = 0; nIndex < strText.size(); nIndex++ )
        {
            if ( strText[ nIndex ] != '\r' )
                strOut += strText[ nIndex ];
        }
        return strOut;
    }
}

CJsonEditDlg::CJsonEditDlg( LPCSTR strTitle, LPCSTR strNote, const std::string &strText, ApplyFunction fnApply, CWnd *pParent ) :
    CDialog( CJsonEditDlg::IDD, pParent ),
    m_strTitle( strTitle ),
    m_strNote( strNote ),
    m_strText( strText ),
    m_fnApply( fnApply )
{
}

BEGIN_MESSAGE_MAP(CJsonEditDlg, CDialog)
END_MESSAGE_MAP()

BOOL CJsonEditDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    CEdit *pEdit = (CEdit *)GetDlgItem( IDC_JSON_TEXT );

    SetWindowText( m_strTitle.c_str() );
    SetDlgItemText( IDC_JSON_NOTE, m_strNote.c_str() );
    if ( pEdit )
    {
            // A fixed-width font lines the JSON up, and no limit on its length.
        m_oFont.CreateStockObject( ANSI_FIXED_FONT );
        pEdit->SetFont( &m_oFont );
        pEdit->SetLimitText( 0 );
        pEdit->SetWindowText( ToControl( m_strText ).c_str() );
        pEdit->SetFocus();
        return FALSE;       // the focus is set
    }
    return TRUE;
}

void CJsonEditDlg::OnOK()
{
    CEdit *pEdit = (CEdit *)GetDlgItem( IDC_JSON_TEXT );

    if ( pEdit == NULL )
        return;

    int         nLength = pEdit->GetWindowTextLength();
    std::string strText( (size_t)nLength + 1, '\0' );
    std::string strError;

    pEdit->GetWindowText( &strText[ 0 ], nLength + 1 );
    strText.resize( (size_t)nLength );
    strText = FromControl( strText );
    if ( m_fnApply && ! m_fnApply( strText, strError ) )
    {
        SetDlgItemText( IDC_JSON_ERROR, strError.c_str() );
        MessageBeep( MB_ICONEXCLAMATION );
        return;
    }
    m_strText = strText;
    CDialog::OnOK();
}
