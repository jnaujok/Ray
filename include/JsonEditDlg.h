// JsonEditDlg.h : header file
//
// A dialog for editing any part of a scene as JSON: the scene editor's way
// of reaching everything the scene format can hold (samplers, color models,
// lenses, every shape and light, model files, textures, bump maps, the
// environment) without a dialog of its own for each. The text is checked
// when OK is pressed, by the function the caller gives, and the dialog stays
// open with the reason if it is refused.

#pragma once

#include "resource.h"

#include <functional>
#include <string>

class CJsonEditDlg : public CDialog
{
public:
        // Checks and applies the text; FALSE, with the reason in strError,
        // leaves the dialog open.
    typedef std::function<BOOL( const std::string &strText, std::string &strError )> ApplyFunction;

    CJsonEditDlg( LPCSTR strTitle, LPCSTR strNote, const std::string &strText, ApplyFunction fnApply, CWnd *pParent = NULL );

    enum { IDD = IDD_JSON_EDIT };

protected:
    std::string     m_strTitle;
    std::string     m_strNote;
    std::string     m_strText;
    ApplyFunction   m_fnApply;
    CFont           m_oFont;

    virtual BOOL    OnInitDialog();
    virtual void    OnOK();

    DECLARE_MESSAGE_MAP()
};
