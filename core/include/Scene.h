/*************************************************************************
*   CScene                                                               *
*                                                                        *
*       This class is the heart of the raytracer engine. It contains all *
* of the objects within the 3-space that comprise the scene. It also     *
* performs all of the actual ray-tracing. The trace routine is totally   *
* thread-safe and reentrant.                                             *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __SCENE_H__
#define __SCENE_H__

#include "Light.h"
#include "Camera.h"
#include "RaySettings.h"
#include "ObjectsArray.h"
#include "LightArray.h"

#include <string>

#define NO_SER_ERROR    (DWORD)0
#define BAD_FILE        (DWORD)1
#define WRONG_VERSION   (DWORD)2

class CScene
{
    protected:
        CObjectsArray       m_oObjects;
        CLightArray         m_oLights;
        CSettings           m_oSettings;
        CCamera             m_oCamera;
        CColor              m_oBackgroundColor;
        std::string         m_strLoadError;
#ifdef RAY_GUI
        HTREEITEM           m_hCamera;
        HTREEITEM           m_hObjects;
        HTREEITEM           m_hLights;
        CSceneTreeView     *m_poSceneTree;
#endif

    public:
                            CScene( void );
        virtual            ~CScene( void );

#ifdef RAY_GUI
        virtual void        ToTree( CSceneTreeView *oSceneTreeDlg );
#endif

        virtual void        AddLight( CLight *pLight );
        virtual void        AddObject( CObjects *pObject );
#ifdef RAY_GUI
        virtual BOOL        TraceScene( CDC *pDC, CRect oRect, BOOL *pbStop, int nQuarter = 0 );
#endif

            // TO DO: Replace TraceScene calls with multiple calls to TraceRay
        virtual BOOL        TraceRay( CRay &oRay, CColor &oColor, CObjects *pDeny = NULL );
        virtual BOOL        Intersect( CRay &oRay, CObjects *pIgnore = NULL );

            /// Reads or writes the binary format of 1996-1999 (versions 1 and
            /// 2). Scenes are now saved as JSON; this is kept to read old files.
        virtual DWORD       Serialize( CRayStream &oStream, BOOL bLoading );
            /// The scene as JSON text (version 3 of the .ray format): indented
            /// for a file, or on one line for the network.
        std::string         ToJson( BOOL bCompact = FALSE );
            /// Replaces the scene with one read from JSON text. Returns 0,
            /// BAD_FILE (see GetLoadError) or WRONG_VERSION. On failure the
            /// scene is left as it was.
        DWORD               FromJson( const std::string &strText );
            /// Reads a .ray file of any version: JSON, or the old binary
            /// format. Returns 0, BAD_FILE or WRONG_VERSION.
        DWORD               Load( const char *strFileName );
            /// Why the last Load or FromJson failed.
        const char         *GetLoadError( void ) const { return m_strLoadError.c_str(); }
            /// Writes a .ray file as JSON. Returns 0 or BAD_FILE.
        DWORD               Save( const char *strFileName );
        virtual CLight     *GetLight( int nIndex );
        CColor             &GetBackground( void ) { return m_oBackgroundColor; }
        CSettings          &GetSettings( void) { return m_oSettings; }

        CCamera            &GetCamera( void ) { return m_oCamera; }

//	    virtual BOOL        WireFrame( CDC *pDC, FLT nTop, FLT nLeft, FLT nBottom, FLT nRight, FLT nOriginX, FLT nOriginY, FLT nPixelsPerUnit );
};

#endif // __SCENE_H__
