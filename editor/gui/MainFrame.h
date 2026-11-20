/*************************************************************************
*   CMainFrame                                                           *
*                                                                        *
*   The scene editor's window: the scene tree on the left, the shaded    *
*   view in the middle and the property panel on the right, with menus   *
*   to open and save scenes, import models, add, duplicate and delete    *
*   objects and lights, undo and redo, and render.                       *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __MAINFRAME_H__
#define __MAINFRAME_H__

#include "EditorDocument.h"

#include <wx/frame.h>
#include <wx/treectrl.h>

#include <map>
#include <string>

class CPropertyPanel;
class CSceneCanvas;

class CMainFrame : public wxFrame
{
    protected:
        RayEditor::CEditorDocument  m_oDocument;
        wxTreeCtrl                 *m_pTree;
        CSceneCanvas               *m_pCanvas;
        CPropertyPanel             *m_pProperties;
        std::map<int, std::string>  m_mapAddTypes;      // menu id: the type it adds
        bool                        m_bFilling;         // filling the tree: ignore its selection events

        void            Rebuild( bool bFrame );         // the tree and the view, from the document
        void            SelectPart( const std::string &strPart, int nMaterial = -1 );
        void            UpdateTitle( void );
        bool            ConfirmDiscard( void );
        bool            SaveTo( const wxString &strPath );
        bool            Open( const wxString &strPath );
        void            ReportError( const wxString &strWhat, const std::string &strWhy );
        std::string     SelectedPart( int *pnMaterial = NULL );

        void            OnNew( wxCommandEvent &oEvent );
        void            OnOpen( wxCommandEvent &oEvent );
        void            OnSave( wxCommandEvent &oEvent );
        void            OnSaveAs( wxCommandEvent &oEvent );
        void            OnImport( wxCommandEvent &oEvent );
        void            OnQuit( wxCommandEvent &oEvent );
        void            OnClose( wxCloseEvent &oEvent );
        void            OnUndo( wxCommandEvent &oEvent );
        void            OnRedo( wxCommandEvent &oEvent );
        void            OnAdd( wxCommandEvent &oEvent );
        void            OnAddEnvironment( wxCommandEvent &oEvent );
        void            OnDuplicate( wxCommandEvent &oEvent );
        void            OnDelete( wxCommandEvent &oEvent );
        void            OnWireframe( wxCommandEvent &oEvent );
        void            OnSceneCamera( wxCommandEvent &oEvent );
        void            OnFrameAll( wxCommandEvent &oEvent );
        void            OnCameraFromView( wxCommandEvent &oEvent );
        void            OnRender( wxCommandEvent &oEvent );
        void            OnStopRender( wxCommandEvent &oEvent );
        void            OnSaveRender( wxCommandEvent &oEvent );
        void            OnAbout( wxCommandEvent &oEvent );
        void            OnTreeSelection( wxTreeEvent &oEvent );
        void            OnUpdateUI( wxUpdateUIEvent &oEvent );

    public:
                        CMainFrame( void );
            /// Opens a scene file given on the command line.
        void            OpenAtStart( const wxString &strPath );
};

#endif // __MAINFRAME_H__
