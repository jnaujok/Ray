/*************************************************************************
*   CPropertyPanel                                                       *
*                                                                        *
*   Edits the part of the scene selected in the tree, two ways: a        *
*   property grid of its fields (numbers, switches, text, and x, y, z    *
*   for vectors and colors, with materials, textures, the sampler, the   *
*   lens and so on as sections), and its JSON text. Either way the       *
*   change goes through the scene's own reader, and if it refuses the    *
*   change, the reason shows under the editor and nothing changes.       *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __PROPERTYPANEL_H__
#define __PROPERTYPANEL_H__

#include "EditorDocument.h"

#include <wx/panel.h>

#include <functional>
#include <string>

class wxNotebook;
class wxPropertyGrid;
class wxPropertyGridEvent;
class wxPGProperty;
class wxStaticText;
class wxTextCtrl;

namespace IASLib
{
    class CJsonNode;
}

class CPropertyPanel : public wxPanel
{
    protected:
        RayEditor::CEditorDocument &m_oDocument;
        std::string                 m_strPart;
        wxNotebook                 *m_pBook;
        wxPropertyGrid             *m_pGrid;
        wxTextCtrl                 *m_pText;
        wxStaticText               *m_pError;
        wxStaticText               *m_pTitle;

        void            Fill( void );
        void            AddNode( wxPGProperty *pParent, const std::string &strPath, const std::string &strLabel,
                                 const IASLib::CJsonNode &oNode, int nDepth );
        void            OnGridChanged( wxPropertyGridEvent &oEvent );
        void            OnApply( wxCommandEvent &oEvent );
        void            OnRevert( wxCommandEvent &oEvent );
        void            ShowError( const std::string &strError );
        bool            Apply( const std::string &strText );

    public:
                        CPropertyPanel( wxWindow *pParent, RayEditor::CEditorDocument &oDocument );

            /// Shows a part ("" for none), with material nMaterial (if not
            /// -1) brought into view.
        void            ShowPart( const std::string &strPart, int nMaterial = -1 );
        const std::string &GetPart( void ) const { return m_strPart; }
            /// Shows the part again (after the scene changed some other way).
        void            Reload( void ) { ShowPart( m_strPart ); }

            /// Called after an edit has been made.
        std::function<void( void )>     m_fnChanged;
};

#endif // __PROPERTYPANEL_H__
