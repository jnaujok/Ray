/*************************************************************************
*   ray-editor                                                           *
*                                                                        *
*   The cross-platform scene editor: wxWidgets on Windows, Linux (GTK)   *
*   and macOS. ray-editor [scene.ray] opens a scene at start.            *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#include "Globals.h"
#include "MainFrame.h"

#include <wx/app.h>
#include <wx/image.h>

class CEditorApp : public wxApp
{
    public:
        virtual bool OnInit( void ) override
        {
            if ( ! wxApp::OnInit() )
                return false;
            wxInitAllImageHandlers();

            CMainFrame *pFrame = new CMainFrame;

            pFrame->Show();
            if ( argc > 1 )
                pFrame->OpenAtStart( argv[ 1 ] );
            return true;
        }

            // The scene to open is the only argument; wxApp's own parser
            // would refuse it.
        virtual void OnInitCmdLine( wxCmdLineParser & ) override {}
        virtual bool OnCmdLineParsed( wxCmdLineParser & ) override { return true; }
};

wxIMPLEMENT_APP( CEditorApp );
