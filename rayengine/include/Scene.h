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
#ifdef _WINDOWS
        HTREEITEM           m_hCamera;
        HTREEITEM           m_hObjects;
        HTREEITEM           m_hLights;
        CSceneTreeView     *m_poSceneTree;
#endif

    public:
                            CScene( void );
        virtual            ~CScene( void );

#ifdef _WINDOWS
        virtual void        ToTree( CSceneTreeView *oSceneTreeDlg );
#endif

        virtual void        AddLight( CLight *pLight );
        virtual void        AddObject( CObjects *pObject );
#ifdef _WINDOWS
        virtual BOOL        TraceScene( CDC *pDC, CRect oRect, BOOL *pbStop, int nQuarter = 0 );
#endif

            // TO DO: Replace TraceScene calls with multiple calls to TraceRay
        virtual BOOL        TraceRay( CRay &oRay, CColor &oColor, CObjects *pDeny = NULL );
        virtual BOOL        Intersect( CRay &oRay, CObjects *pIgnore = NULL );

        virtual DWORD       Serialize( CStream &oStream, BOOL bLoading );
        virtual CLight     *GetLight( int nIndex );
        CColor             &GetBackground( void ) { return m_oBackgroundColor; }
        CSettings          &GetSettings( void) { return m_oSettings; }

        CCamera            &GetCamera( void ) { return m_oCamera; }

//	    virtual BOOL        WireFrame( CDC *pDC, FLT nTop, FLT nLeft, FLT nBottom, FLT nRight, FLT nOriginX, FLT nOriginY, FLT nPixelsPerUnit );
};

#endif // __SCENE_H__
