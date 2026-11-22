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

#include "RayCounts.h"
#include "Light.h"
#include "Camera.h"
#include "RaySettings.h"
#include "Environment.h"
#include "Haze.h"
#include "ObjectsArray.h"
#include "LightArray.h"

#include <memory>
#include <string>
#include <vector>

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
            // What rays that leave the scene see, instead of the background
            // color; NULL for none.
        std::shared_ptr<const CEnvironment> m_pEnvironment;
            // The atmosphere the scene sits in, or NULL for clear air.
        std::shared_ptr<const CHaze>        m_pHaze;
        std::string         m_strLoadError;
        int                 m_nOnlyObject;      // while saving one object: its number, else -1

            // A bounding volume hierarchy over the objects, built when a ray
            // first needs it, so a ray tests only the objects near its path.
            // Objects without bounds (planes) are tested every time.
        struct STreeNode
        {
            FLT             anMin[ 3 ];
            FLT             anMax[ 3 ];
            int             nLeft;              // child nodes, if nCount is 0
            int             nRight;
            int             nFirst;             // the leaf's objects in m_anTreeObjects
            int             nCount;
        };
        std::vector<STreeNode>  m_aoTree;
        std::vector<int>        m_anTreeObjects;    // object numbers, grouped by leaf
        std::vector<int>        m_anUnbounded;      // object numbers
        std::vector<FLT>        m_anObjectBounds;   // min x, y, z, max x, y, z for each object
        BOOL                    m_bTreeBuilt;
        CObjects               *m_pLastOccluder;    // what last blocked a shadow ray
        CRayCounts              m_oRayCounts;       // the rays this copy of the scene has cast

        void                ForgetTree( void );
        void                BuildTree( void );
        int                 BuildTreeNode( int nFirst, int nCount );
        static BOOL         RayHitsBox( const FLT anMin[ 3 ], const FLT anMax[ 3 ], const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ],
                                        FLT nLimit );

    public:
                            CScene( void );
        virtual            ~CScene( void );


        virtual void        AddLight( CLight *pLight );
        virtual void        AddObject( CObjects *pObject );

            // TO DO: Replace TraceScene calls with multiple calls to TraceRay
            /// nKind is what the ray is, for the ray counts (CRayCounts):
            /// counted if the ray is traced, not if the depth limit or
            /// threshold stops it first.
        virtual BOOL        TraceRay( CRay &oRay, CColor &oColor, CObjects *pDeny = NULL, int nKind = CRayCounts::OTHER );
            /// Counts a ray of kind nKind (a shadow or bounce ray the
            /// caller casts with Intersect or ClosestHit).
        void                CountRay( int nKind ) { m_oRayCounts.Count( nKind ); }
        const CRayCounts   &GetRayCounts( void ) const { return m_oRayCounts; }
        void                ClearRayCounts( void ) { m_oRayCounts.Clear(); }
        virtual BOOL        Intersect( CRay &oRay, CObjects *pIgnore = NULL );
            /// The object the ray hits first (and sets the ray's t and the
            /// point), or NULL, using the tree; pDeny is skipped as in
            /// TraceRay.
        CObjects           *ClosestHit( CRay &oRay, CVector &oIntersect, CObjects *pDeny );
            /// The same, and Intersect, without the tree: every object, in
            /// order. The tree must find exactly what these do; the tests
            /// hold it to that.
        CObjects           *ClosestHitByEachObject( CRay &oRay, CVector &oIntersect, CObjects *pDeny );
            /// The numbers of the objects that may come within nDistance of
            /// oPoint (their bounding boxes do, by the tree), and every
            /// unbounded object (planes), in scene order.
        void                Nearby( const CVector &oPoint, FLT nDistance, std::vector<int> &anObjects );
        BOOL                IntersectEachObject( CRay &oRay, CObjects *pIgnore );

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
            /// Reads a .ray file: a scene archive or plain JSON. Returns 0,
            /// BAD_FILE or WRONG_VERSION. (The binary files of 1996-1999 are
            /// no longer read.)
        DWORD               Load( const char *strFileName );
            /// Why the last Load or FromJson failed.
        const char         *GetLoadError( void ) const { return m_strLoadError.c_str(); }
            /// Writes a .ray file: an IASLib resource archive holding the scene
            /// as JSON ("scene.json") and every file it uses (texture images),
            /// under the names the scene gives them. Returns 0 or BAD_FILE
            /// (see GetLoadError). A failed save leaves any old file as it was.
        DWORD               Save( const char *strFileName );
            /// Writes the scene as a plain JSON file, for editing by hand, and
            /// the files it uses beside it, under the names it gives them.
        DWORD               SaveJson( const char *strFileName );
            /// Writes a scene holding only object nIndex (from 0) and its
            /// files, with the scene's settings, camera and lights: a prefab
            /// for an include. bJson writes plain JSON with the files beside
            /// it, else an archive. Returns 0 or BAD_FILE (see GetLoadError).
        DWORD               SaveObject( int nIndex, const char *strFileName, BOOL bJson );
            /// The object named strName, or numbered strName (from 1, as the
            /// editor's tree numbers them); -1 if there is none.
        int                 FindObject( const std::string &strName );

            /// One part of the scene as JSON text, for editing by hand (the
            /// scene editor's "Edit as JSON"): "settings", "camera",
            /// "environment" (empty if there is none), "objects/N" or
            /// "lights/N" (from 0). Empty if there is no such part.
        std::string         PartToJson( const std::string &strPart );
            /// Replaces a part of the scene with one read from strText: a part
            /// named as above, or "objects/+" or "lights/+" to add one at the
            /// end. Empty text removes an object, a light or the environment.
            /// Files the text names are read relative to strDirectory, or
            /// from the archive strArchive (if not empty) when it has them.
            /// Returns 0 or BAD_FILE (see GetLoadError); on failure the scene
            /// is as it was.
        DWORD               EditPart( const std::string &strPart, const std::string &strText,
                                      const std::string &strDirectory, const std::string &strArchive = std::string() );
            /// Where an object or light is in its list, or -1.
        int                 IndexOfObject( CObjects *pObject );
        int                 IndexOfLight( CObjects *pLight );
        int                 GetLightCount( void ) const { return m_oLights.GetSize(); }
        int                 GetObjectCount( void ) const { return m_oObjects.GetSize(); }
        virtual CLight     *GetLight( int nIndex );
            /// The scene's nIndex'th object (not light), or NULL.
        CObjects           *GetObjectAt( int nIndex ) { return ( nIndex >= 0 && nIndex < m_oObjects.GetSize() ) ? m_oObjects[ nIndex ] : NULL; }
        CColor             &GetBackground( void ) { return m_oBackgroundColor; }
            /// The environment, or NULL if the scene has none.
        const CEnvironment *GetEnvironment( void ) const { return m_pEnvironment.get(); }
            /// What a ray leaving the scene along oDirection sees: the
            /// environment, or the background color.
        CColor              Escaped( CVector &oDirection );
            /// The haze, or NULL if the air is clear.
        const CHaze        *GetHaze( void ) const { return m_pHaze.get(); }
            /// How much of a light's light reaches oPoint along oToLight (the
            /// vector from the point to the light, as CLight::ToLight gives
            /// it) through the haze: 1 in clear air.
        FLT                 LightThrough( const CVector &oPoint, const CVector &oToLight );
        CSettings          &GetSettings( void) { return m_oSettings; }

        CCamera            &GetCamera( void ) { return m_oCamera; }

//	    virtual BOOL        WireFrame( CDC *pDC, FLT nTop, FLT nLeft, FLT nBottom, FLT nRight, FLT nOriginX, FLT nOriginY, FLT nPixelsPerUnit );
};

#endif // __SCENE_H__
