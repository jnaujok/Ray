/*************************************************************************
*   CMesh                                                                *
*                                                                        *
*   A surface made of triangles that share their vertices: a list of     *
*   vertices, and faces given as three vertex indices each. The faces    *
*   stay indices (a face is shaded through a stand-in CTriangle made     *
*   when it is hit, sharing the mesh's materials), and the shape is      *
*   built once and shared by every mesh given the same geometry, as      *
*   each thread's copy of a scene is. Each vertex gets a normal, the     *
*   sum of the normals of                                                *
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

#include <memory>
#include <string>
#include <vector>

#include "ModelFile.h"

class CMesh : public CObjects
{
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

            // How a model file is placed: scaled (by m_oScale, or uniformly
            // to m_nFitSize across), turned (degrees about x, then y, then
            // z), aligned, then moved by m_oTranslation.
        enum EAlign { ALIGN_NONE, ALIGN_CENTER, ALIGN_BOTTOM };

    protected:
        SUVMap                  m_oUVMap;

            // A model file the geometry came from, or empty.
        std::string             m_strFile;          // its name in the scene
        std::string             m_strFilePath;      // where on the disk it came from (empty: the archive or the scene)
        std::shared_ptr<const std::string> m_pFileData;
        std::string             m_strNode;          // a glTF model's node to read, or empty for its scene
            // The other files a .gltf names, by its URI, with where each came from.
        ModelFile::SGltfFiles   m_oGltfFiles;
        std::vector<std::string> m_astrGltfPaths;
            // The file as read, shared by every copy of the scene that names
            // it, so it is read once while any of them lives.
        std::shared_ptr<const ModelFile::SModel> m_pModel;
        CVector                 m_oScale;
        FLT                     m_nFitSize;         // 0: use m_oScale
        CVector                 m_oRotation;
        int                     m_nAlign;           // EAlign
        CVector                 m_oTranslation;

            // The "uvMap" JSON field, read and written.
        BOOL                    ReadUVMap( const IASLib::CJsonNode &oNode, BOOL bUnused );
        void                    UVMapToJson( IASLib::CObjectNode &oNode ) const;
            // Reads a mesh that names a model "file", and loads it.
        BOOL                    ModelFromJson( const IASLib::CJsonNode &oNode, BOOL bSmooth, BOOL bDoubleSided );
            // The folder of m_strFile, with its slash ("" for none), where a
            // .gltf's other files are.
        std::string             GltfFolder( void ) const;
        BOOL                    m_bDoubleSided;
        BOOL                    m_bSmooth;
        CColor                  m_oColor;

    public:
            // The bounding volume hierarchy: each node a box around its
            // faces, either two child nodes or (a leaf) a run of anOrder.
        struct SNode
        {
            FLT                 anMin[ 3 ];
            FLT                 anMax[ 3 ];
            int                 nLeft;              // child nodes, if nCount is 0
            int                 nRight;
            int                 nFirst;             // the leaf's faces in anOrder
            int                 nCount;
        };

            // Everything about a mesh's shape, built once and never changed
            // after: what SetGeometry was given, and the faces, normals,
            // surface coordinates and tree worked out from it. Faces are
            // three vertex indices each, not objects; a face is shaded
            // through a stand-in CTriangle made for it when it is hit. Every
            // mesh given the same geometry (each thread's copy of a scene)
            // shares one, so it is built and held once.
        struct SGeometry
        {
                // What SetGeometry was given.
            std::vector<CVector>        aoVertices;
            std::vector<int>            anFaces;            // three vertex indices a face, as given
            std::vector<CVector>        aoGivenNormals;     // one a vertex, or empty to sum the faces' normals
            std::vector<FLT>            anUVs;              // u and v for each vertex, or empty
            std::vector<CColor>         aoGivenColors;      // one a face as given, or empty
            std::vector<unsigned char>  abGivenHasColor;
            BOOL                        bSmooth;
            BOOL                        bDoubleSided;
            SUVMap                      oUVMap;

                // The faces kept (those with a surface), in order.
            std::vector<int>            anKept;             // three vertex indices each
            std::vector<CVector>        aoFaceNormals;      // each face's unit normal
            std::vector<CVector>        aoVertexNormals;    // what a smooth face blends, one a vertex (empty if not smooth)
            std::vector<FLT>            anCornerUVs;        // six a face (u, v at each corner), or empty
            std::vector<CColor>         aoFaceColors;       // one a face, or empty
            std::vector<unsigned char>  abFaceHasColor;
            CVector                     oMinimum;           // the bounding box
            CVector                     oMaximum;
            std::vector<SNode>          aoNodes;            // the root first
            std::vector<int>            anOrder;            // face indices, grouped by leaf

            int                         FaceCount( void ) const { return (int)aoFaceNormals.size(); }
        };

            /// How many geometries are alive and shared (for tests).
        static int              SharedGeometries( void );
            /// The geometry this mesh uses, which every mesh given the same
            /// shape shares (for tests).
        const SGeometry        *GetGeometry( void ) const { return m_pGeometry.get(); }

    protected:
        std::shared_ptr<const SGeometry> m_pGeometry;
        int                     m_nHitFace;         // the face Intersect hit last, or -1
            // Stand-ins for faces a csg asked for with HitPart, which it
            // keeps pointers to while it works out its own hit.
        std::vector<std::unique_ptr<CTriangle>> m_apLeafFaces;
        std::vector<int>        m_anLeafFace;       // which face each of those is

            // A stand-in triangle for face nFace (for shading and the like).
        CTriangle              *MakeFace( int nFace );

        BOOL                    HitsBox( CRay &oRay );
            // Builds a geometry's faces, normals, coordinates and tree.
        static void             Build( SGeometry &oGeometry );
        static void             MapUVs( SGeometry &oGeometry );
            // Builds the tree, each box nMargin larger than its faces.
        static void             BuildTree( SGeometry &oGeometry, FLT nMargin );
        static int              BuildNode( SGeometry &oGeometry, int nFirst, int nCount, const std::vector<FLT> &anCenters, FLT nMargin,
                                           int nDepth );
            // Whether the ray passes through a node's box before nLimit.
        static BOOL             HitsNode( const SNode &oNode, const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLimit );
            // HitsNode, and where the ray enters the box (0 if it starts inside).
        static BOOL             NodeEntry( const SNode &oNode, const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLimit, FLT &nEntry );

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
                                             const std::vector<FLT> &anUVs = std::vector<FLT>(),
                                             const std::vector<CColor> &aoFaceColors = std::vector<CColor>(),
                                             const std::vector<unsigned char> &abHasColor = std::vector<unsigned char>() );
            /// Loads the geometry from a model file's contents (STL or OBJ;
            /// strName picks the format) and places it with the model
            /// settings below. FALSE, with a reason in strError, if the file
            /// can't be read.
        BOOL                    LoadModel( const std::string &strData, const std::string &strName, std::string &strError );
            /// Whether the mesh came from a model file.
        BOOL                    IsFromFile( void ) const { return ! m_strFile.empty(); }
        int                     GetFaceCount( void ) const { return m_pGeometry ? m_pGeometry->FaceCount() : 0; }
            /// The vertices, where they are in the scene.
        const std::vector<CVector> &GetVertices( void ) const;
            /// Face nFace's color from a colored model file, if it has one.
        BOOL                    FaceColor( int nFace, CColor &oColor );
            /// Whether the faces have surface coordinates (from "uvs", a
            /// model file's, or a UV map).
        BOOL                    HasUVs( void ) const { return m_pGeometry && ! m_pGeometry->anUVs.empty(); }
            /// Sets the UV map, used when there are no uvs; it takes effect
            /// at the next SetGeometry.
        void                    SetUVMap( const SUVMap &oMap ) { m_oUVMap = oMap; }
            /// Face nFace's surface coordinates at a point on it.
        BOOL                    FaceCoordsAt( int nFace, CVector oPoint, FLT &nU, FLT &nV );
            /// The normal face nFace shades with at a point on it.
        CVector                 FaceNormalAt( int nFace, CVector oPoint );
            /// Whether face nFace has the mesh's bump map (for tests).
        BOOL                    FaceHasBump( int nFace );

        virtual BOOL            Intersect( CRay &oRay, CVector &oIntersect );
        virtual BOOL            Intersect( CRay &oRay, CVector &oIntersect, CObjects *pIgnore );
            /// Stops at the first face the ray hits.
        virtual BOOL            Occludes( CRay &oRay, CObjects *pIgnore );
        virtual BOOL            GetBounds( CVector &oMinimum, CVector &oMaximum );
            // The face its last Intersect hit. A mesh is taken to be closed
            // (a csg needs it to be; a mesh with holes gives wrong results).
        virtual CObjects       *HitPart( BOOL &bFlip );
        virtual BOOL            IsClosed( void ) { return TRUE; }
            /// Intersect without the tree: every face, in order. The tree
            /// must find exactly what this does; the tests hold it to that.
        BOOL                    IntersectEachFace( CRay &oRay, CVector &oIntersect );
        virtual CColor          Shade( CScene &oScene, CRay &oRay, CVector &oIntersect );
            /// The faces share the mesh's bump map, as they share its materials.
        virtual void            SetBump( const std::shared_ptr<const CBumpMap> &pBump );
        virtual void            SetTextureFrame( const std::shared_ptr<const STextureFrame> &pFrame );

#ifdef RAY_GUI
        virtual void            ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
#endif

    private:
                                CMesh( const CMesh & );
        CMesh                  &operator =( const CMesh & );
};

#endif // __MESH_H__
