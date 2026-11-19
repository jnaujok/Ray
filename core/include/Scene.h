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

namespace IASLib
{
    class CJsonNode;
    class CObjectNode;
}

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

            /// The scene as JSON text (version 3 of the .ray format): indented
            /// for a file, or on one line for the network.
        std::string         ToJson( BOOL bCompact = FALSE );
            /// The scene as a JSON tree (not rounded), which the caller owns.
        IASLib::CObjectNode *ToJsonTree( void );
            /// Replaces the scene with one read from JSON text. Returns 0,
            /// BAD_FILE (see GetLoadError) or WRONG_VERSION. On failure the
            /// scene is left as it was.
        DWORD               FromJson( const std::string &strText );
            /// The same, from a JSON tree.
        DWORD               FromJsonTree( const IASLib::CJsonNode &oRoot );
            /// Reads a JSON .ray file. Returns 0, BAD_FILE or WRONG_VERSION.
            /// (The binary files of 1996-1999 are no longer read.)
        DWORD               Load( const char *strFileName );
            /// Why the last Load or FromJson failed.
        const char         *GetLoadError( void ) const { return m_strLoadError.c_str(); }
            /// Writes a .ray file as JSON. Returns 0 or BAD_FILE.
        DWORD               Save( const char *strFileName );
        virtual CLight     *GetLight( int nIndex );
            /// The scene's nIndex'th object (not light), or NULL.
        CObjects           *GetObjectAt( int nIndex ) { return ( nIndex >= 0 && nIndex < m_oObjects.GetSize() ) ? m_oObjects[ nIndex ] : NULL; }
        CColor             &GetBackground( void ) { return m_oBackgroundColor; }
        CSettings          &GetSettings( void) { return m_oSettings; }

        CCamera            &GetCamera( void ) { return m_oCamera; }

//	    virtual BOOL        WireFrame( CDC *pDC, FLT nTop, FLT nLeft, FLT nBottom, FLT nRight, FLT nOriginX, FLT nOriginY, FLT nPixelsPerUnit );
};

#endif // __SCENE_H__
