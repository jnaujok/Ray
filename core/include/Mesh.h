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
*   The faces are kept in a bounding volume hierarchy, a tree of boxes,  *
*   so a ray tests only the faces in boxes it passes through. Rays       *
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

            // The bounding volume hierarchy: each node a box around its
            // faces, either two child nodes or (a leaf) a run of m_anOrder.
        struct SNode
        {
            FLT                 anMin[ 3 ];
            FLT                 anMax[ 3 ];
            int                 nLeft;              // child nodes, if nCount is 0
            int                 nRight;
            int                 nFirst;             // the leaf's faces in m_anOrder
            int                 nCount;
        };
        std::vector<SNode>      m_aoNodes;          // the root first
        std::vector<int>        m_anOrder;          // face indices, grouped by leaf

        void                    DeleteFaces( void );
        BOOL                    HitsBox( CRay &oRay );
            // Builds the tree, each box nMargin larger than its faces.
        void                    BuildTree( FLT nMargin );
        int                     BuildNode( int nFirst, int nCount, const std::vector<FLT> &anCenters, FLT nMargin );
            // Whether the ray passes through a node's box before nLimit.
        static BOOL             HitsNode( const SNode &oNode, const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLimit );

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
            /// Intersect without the tree: every face, in order. The tree
            /// must find exactly what this does; the tests hold it to that.
        BOOL                    IntersectEachFace( CRay &oRay, CVector &oIntersect );
        virtual CColor          Shade( CScene &oScene, CRay &oRay, CVector &oIntersect );

#ifdef RAY_GUI
        virtual void            ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
#endif

    private:
                                CMesh( const CMesh & );
        CMesh                  &operator =( const CMesh & );
};

#endif // __MESH_H__
