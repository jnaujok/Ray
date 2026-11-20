/*************************************************************************
*   CSceneCanvas                                                         *
*                                                                        *
*   The editor's shaded view of the scene: an OpenGL canvas that draws   *
*   the tessellated objects lit by the scene's lights, with a wireframe  *
*   overlay to turn on, icons for the lights and the camera, and the     *
*   selected object outlined. The mouse orbits (left), pans (right or    *
*   middle) and zooms (wheel); a click picks the object under it with    *
*   the tracer's own hit test. It also shows the Render button's image   *
*   over the view as it comes in.                                        *
*                                                                        *
*   It uses OpenGL 1.x fixed-function calls only (display lists, GL      *
*   lighting, client arrays), which every platform's OpenGL has: Windows'*
*   opengl32, macOS's legacy 2.1 profile and Mesa.                       *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __SCENECANVAS_H__
#define __SCENECANVAS_H__

#include "RenderJob.h"
#include "Tessellator.h"
#include "ViewCamera.h"

#include <wx/glcanvas.h>
#include <wx/timer.h>

#include <functional>
#include <memory>
#include <vector>

class CScene;

class CSceneCanvas : public wxGLCanvas
{
    protected:
        std::unique_ptr<wxGLContext>    m_pContext;
        RayEditor::CViewCamera          m_oView;
        RayEditor::SSceneDrawing        m_oDrawing;
        std::vector<unsigned int>       m_anLists;          // a display list a mesh
        bool                            m_bListsDirty;
        bool                            m_bWireframe;
        bool                            m_bSceneCamera;     // looking through the scene's camera
        int                             m_nSelected;        // the selected object, or -1
        int                             m_nSelectedLight;   // the selected light, or -1
        float                           m_anBackground[ 3 ];
        std::vector<std::vector<float>> m_aanLights;        // position (w last) and color of up to 8 lights

            // The mouse.
        wxPoint                         m_oDragStart;
        wxPoint                         m_oLast;
        bool                            m_bDragging;
        bool                            m_bMoved;

            // The render shown over the view.
        RayEditor::CRenderJob           m_oJob;
        wxTimer                         m_oTimer;
        unsigned                        m_nShownVersion;
        bool                            m_bShowRender;
        unsigned int                    m_nTexture;
        int                             m_nTextureWidth;
        int                             m_nTextureHeight;
        std::vector<unsigned char>      m_abRender;

        CScene                         *m_pScene;

        void            OnPaint( wxPaintEvent &oEvent );
        void            OnSize( wxSizeEvent &oEvent );
        void            OnMouse( wxMouseEvent &oEvent );
        void            OnTimer( wxTimerEvent &oEvent );
        void            BuildLists( void );
        void            DrawMeshes( bool bTranslucent );
        void            DrawRender( void );
        void            ViewChanged( void );
        wxSize          PixelSize( void ) const;

    public:
                        CSceneCanvas( wxWindow *pParent );
                       ~CSceneCanvas( void );

            /// Called when a click picks an object (its number, or -1).
        std::function<void( int )>          m_fnPicked;
            /// Called when the view moves on its own (dragging out of the
            /// scene camera's view, for the window to uncheck it).
        std::function<void( void )>         m_fnLeftSceneCamera;
            /// Called when the render's state changes (status text).
        std::function<void( const wxString & )> m_fnStatus;

            /// Draws this scene from now on (the window keeps it alive);
            /// call again after it changes.
        void            SetScene( CScene &oScene, bool bFrame );
        void            SetSelected( int nObject, int nLight );
        void            SetWireframe( bool bOn ) { m_bWireframe = bOn; Refresh(); }
        bool            GetWireframe( void ) const { return m_bWireframe; }
            /// Looks through the scene's camera (and keeps doing so as it
            /// changes), or goes back to the free view.
        void            SetSceneCamera( bool bOn );
        bool            GetSceneCamera( void ) const { return m_bSceneCamera; }
        void            FrameAll( void );
        const RayEditor::CViewCamera &GetView( void ) const { return m_oView; }

            /// Starts ray-tracing the view; the image comes in over it.
        bool            StartRender( wxString &strError );
            /// Stops the render and goes back to the shaded view.
        void            StopRender( void );
        bool            IsRendering( void ) const { return m_oJob.IsRunning(); }
        bool            HasRender( void ) const { return m_bShowRender; }
            /// The render as an image (empty if there is none).
        bool            GetRender( std::vector<unsigned char> &abPixels, int &nWidth, int &nHeight );
};

#endif // __SCENECANVAS_H__
