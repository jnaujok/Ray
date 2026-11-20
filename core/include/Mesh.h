/*************************************************************************
*   CMesh                                                                *
*                                                                        *
*   A surface made of triangles that share their vertices: a list of     *
*   vertices, and faces given as three vertex indices each. The faces    *
*   are ordinary CTriangles, owned by the mesh and sharing its           *
*   materials. Each vertex gets a normal, the sum of the normals of      *
*   every face it belongs to, so a smooth mesh shades as one curved      *
*   surface; the scene file can give the vertex normals instead. The corner order of each face decides its front, as for     *
*   CTriangle.                                                           *
*                                                                        *
*   For textures, a mesh takes surface coordinates for its vertices      *
*   ("uvs"), or works them out for each face with a UV map: planar,      *
*   cylindrical, spherical or box projection about an axis.              *
*                                                                        *
*   A ray that misses the mesh's bounding box skips all its faces. Rays  *
*   leaving one face (shadows, reflections) skip only that face, so a    *
*   mesh shadows and reflects itself.                                    *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __MESH_H__
#define __MESH_H__

#include "Objects.h"
#include "Triangle.h"

#include <vector>

class CMesh : public CObjects
{
    protected:
        std::vector<CVector>    m_aoVertices;
        std::vector<int>        m_anFaces;          // three vertex indices a face
        std::vector<CVector>    m_aoGivenNormals;   // one a vertex, or empty to sum the faces' normals
        std::vector<FLT>        m_anUVs;            // u and v for each vertex, or empty

    public:
            // How a mesh without uvs works out surface coordinates.
        enum EUVMap { UVMAP_NONE, UVMAP_PLANAR, UVMAP_CYLINDRICAL, UVMAP_SPHERICAL, UVMAP_BOX };
        struct SUVMap
        {
            int                 nType;              // EUVMap
            int                 nAxis;              // 0, 1, 2: x, y, z
            CVector             oCenter;
            BOOL                bCenterGiven;       // else the bounding box's center
            FLT                 anScale[ 2 ];       // repeats in u and v
        };

    protected:
        SUVMap                  m_oUVMap;

            // Works out each face's corner coordinates from the UV map.
        void                    MapUVs( void );
        BOOL                    m_bDoubleSided;
        BOOL                    m_bSmooth;
        CColor                  m_oColor;

        std::vector<CTriangle *> m_apFaces;         // built from the above
        CVector                 m_oMinimum;         // the bounding box
        CVector                 m_oMaximum;
        CTriangle              *m_pHitFace;         // the face Intersect hit last

        void                    DeleteFaces( void );
        BOOL                    HitsBox( CRay &oRay );

    public:
                                DYNAMIC_OBJECTS( CMesh );

                                CMesh( void );
        virtual                ~CMesh( void );

            /// Replaces the geometry and rebuilds the faces and normals.
            /// Faces whose corners are in a line are left out. aoNormals, if
            /// not empty, gives each vertex's normal (unit vectors) instead
            /// of the sum of its faces' normals; anUVs, if not empty, gives
            /// each vertex's surface coordinates, u then v. Returns FALSE if
            /// an index is out of range or a list is the wrong length.
        BOOL                    SetGeometry( const std::vector<CVector> &aoVertices, const std::vector<int> &anFaces,
                                             BOOL bSmooth, BOOL bDoubleSided,
                                             const std::vector<CVector> &aoNormals = std::vector<CVector>(),
                                             const std::vector<FLT> &anUVs = std::vector<FLT>() );
        int                     GetFaceCount( void ) const { return (int)m_apFaces.size(); }
            /// Sets the UV map, used when there are no uvs; it takes effect
            /// at the next SetGeometry.
        void                    SetUVMap( const SUVMap &oMap ) { m_oUVMap = oMap; }
            /// Face nFace's surface coordinates at a point on it.
        BOOL                    FaceCoordsAt( int nFace, CVector oPoint, FLT &nU, FLT &nV ) { return m_apFaces[ (size_t)nFace ]->SurfaceCoords( oPoint, nU, nV ); }
            /// The normal face nFace shades with at a point on it.
        CVector                 FaceNormalAt( int nFace, CVector oPoint ) { return m_apFaces[ (size_t)nFace ]->NormalAt( oPoint ); }

        virtual BOOL            Intersect( CRay &oRay, CVector &oIntersect );
        virtual BOOL            Intersect( CRay &oRay, CVector &oIntersect, CObjects *pIgnore );
        virtual CColor          Shade( CScene &oScene, CRay &oRay, CVector &oIntersect );

#ifdef RAY_GUI
        virtual void            ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
#endif

    private:
                                CMesh( const CMesh & );
        CMesh                  &operator =( const CMesh & );
};

#endif // __MESH_H__
