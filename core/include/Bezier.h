/*************************************************************************
*   CBezier                                                              *
*                                                                        *
*   A surface made of Bezier patches, traced exactly rather than cut     *
*   into triangles. Each patch has a degree in u and in v (bicubic, 3 by *
*   3, is the usual) and (u degree + 1) * (v degree + 1) control points, *
*   row by row with u varying fastest; a point may carry a weight, which *
*   makes the patch rational (a B-spline surface with weights, a NURBS,  *
*   becomes rational patches, see BSpline.h).                            *
*                                                                        *
*   The patches can be given in the scene ("patches": each with its own  *
*   "points", or "indices" into a shared "points" list, as the Utah      *
*   teapot is usually listed), or read from a .bpt file ("file"), which  *
*   goes into a saved scene's archive like any file a scene uses. They   *
*   are in the shape's own frame: scaled by "scale", turned by           *
*   "rotation" and moved to "center" (default the origin).               *
*                                                                        *
*   How a ray finds it: each patch is split in halves, by de Casteljau's *
*   algorithm, until every piece is nearly flat; a tree of the pieces'   *
*   bounding boxes (a patch lies inside the box of its control points)   *
*   finds the pieces a ray passes near. For each, the ray is first       *
*   crossed with the two triangles of the piece's corners, which gives a *
*   starting (u, v, t), and Newton's method then solves exactly where    *
*   the ray meets the surface (from the piece's middle too, if that      *
*   fails). A solution anywhere on the patch counts, not only inside the *
*   piece that found it, so there are no cracks between pieces or along  *
*   the seams between patches.                                           *
*                                                                        *
*   The normal is the cross product of the surface's slopes in u and v.  *
*   Where it vanishes (a patch whose edge shrinks to a point, like the   *
*   teapot's knob and base) it is taken a hair inside the patch. The     *
*   surface is open and shows the side a ray sees, unless "closed" says  *
*   the patches enclose a volume, with their normals outward, so a csg   *
*   can use it.                                                          *
*                                                                        *
*   Surface coordinates: (u, v) on each patch, from 0 to 1, or across    *
*   the rectangle a patch's "uv" gives ([u0, v0, u1, v1]), so textures   *
*   can run across several patches.                                      *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __BEZIER_H__
#define __BEZIER_H__

#include "LocalShape.h"

#include <memory>
#include <string>
#include <vector>

class CBezier : public CLocalShape
{
    public:
            // One patch: its degrees and control points (x w, y w, z w, w:
            // homogeneous, four numbers a point, u varying fastest), and the
            // rectangle of surface coordinates it covers.
        struct SPatch
        {
            int                 nDegreeU;
            int                 nDegreeV;
            std::vector<double> anPoints;
            double              anUV[ 4 ];
            BOOL                bOwnUV;         // the scene gave "uv"
            std::vector<int>    anIndices;      // into the shared points, if given so

                                SPatch( void ) : nDegreeU( 3 ), nDegreeV( 3 ), bOwnUV( FALSE )
                                { anUV[ 0 ] = 0.0; anUV[ 1 ] = 0.0; anUV[ 2 ] = 1.0; anUV[ 3 ] = 1.0; }
        };

    protected:
            // A nearly flat piece of a patch, for the tree.
        struct SPiece
        {
            int                 nPatch;
            double              anRange[ 4 ];   // u0, v0, u1, v1 on the patch
            double              anLow[ 3 ];
            double              anHigh[ 3 ];
            double              anCorners[ 4 ][ 3 ];    // the surface at (u0,v0), (u1,v0), (u1,v1), (u0,v1)
        };
        struct SNode
        {
            double              anLow[ 3 ];
            double              anHigh[ 3 ];
            int                 nLeft;          // a child, or -1 for a leaf
            int                 nRight;
            int                 nFirst;         // a leaf's pieces, in m_anOrder
            int                 nCount;
        };
            // A recent hit, so the normal and coordinates at it need no search.
        struct SHit
        {
            double              anPoint[ 3 ];
            int                 nPatch;
            double              u;
            double              v;
        };

            // What the scene gave, kept to write back as it came.
        std::string             m_strFile;
        std::string             m_strFilePath;
        std::shared_ptr<const std::string> m_pFileData;
        std::vector<double>     m_anShared;     // shared points, four numbers each (homogeneous)
        BOOL                    m_bShared;
        CVector                 m_oScale;
        BOOL                    m_bClosed;

            // Worked out by PrepareShape, in the shape's own frame.
        std::vector<SPatch>     m_aoPatches;    // as given
        std::vector<SPatch>     m_aoScaled;     // scaled, the ones traced
        std::vector<SPiece>     m_aoPieces;
        std::vector<SNode>      m_aoNodes;
        std::vector<int>        m_anOrder;
        double                  m_nSize;        // the sum of the box's sides
        double                  m_anLow[ 3 ];
        double                  m_anHigh[ 3 ];
        SHit                    m_aoRecent[ 4 ];
        int                     m_nRecent;
        BOOL                    m_bAnyHit;      // a shadow ray: the first hit found will do

        virtual BOOL        PrepareShape( void );
        virtual BOOL        LocalHit( const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLow, FLT nHigh, FLT &nT );
        virtual void        LocalNormal( const FLT anPoint[ 3 ], FLT anNormal[ 3 ] );
        virtual void        LocalCoords( const FLT anPoint[ 3 ], FLT &nU, FLT &nV );
        virtual void        LocalBounds( FLT anLow[ 3 ], FLT anHigh[ 3 ] );
        virtual void        ShapeToJson( IASLib::CObjectNode &oNode );
        virtual BOOL        ShapeFromJson( const IASLib::CJsonNode &oNode );
        virtual const char *const *ShapeFields( void );
        virtual void        LocalTriangles( int nSegments, std::vector<FLT> &anPositions, std::vector<FLT> &anNormals );
        virtual BOOL        CenterRequired( void ) { return FALSE; }

            // The fields CBezier and CBSpline share ("scale", "closed").
        void                CommonToJson( IASLib::CObjectNode &oNode );
        BOOL                CommonFromJson( const IASLib::CJsonNode &oNode );
            // Where a point is on the surface: the patch and (u, v). FALSE if
            // it isn't near the surface.
        BOOL                Locate( const double anPoint[ 3 ], int &nPatch, double &u, double &v );
        void                Remember( const double anPoint[ 3 ], int nPatch, double u, double v );
        void                BuildTree( void );
        int                 BuildNode( int nFirst, int nCount );
        BOOL                PieceHit( const SPiece &oPiece, const double anOrigin[ 3 ], const double anDirection[ 3 ], double nLow,
                                      double nHigh, double &nT, double &u, double &v );

    public:
                            DYNAMIC_OBJECTS( CBezier );

                            CBezier( void );
        virtual            ~CBezier( void );

        virtual BOOL        IsClosed( void ) { return m_bClosed; }
            /// Whether a shadow ray meets the patches: stops at the first hit
            /// found rather than the nearest.
        virtual BOOL        Occludes( CRay &oRay, CObjects *pIgnore );

            /// Sets the patches directly (in the shape's own frame) and
            /// prepares them; FALSE (with RayJson::Fail) if one is bad.
        BOOL                SetPatches( const std::vector<SPatch> &aoPatches );
        const std::vector<SPatch> &GetPatches( void ) const { return m_aoPatches; }
            /// The number of flat pieces the patches were split into.
        int                 PieceCount( void ) const { return (int)m_aoPieces.size(); }

            /// A patch's point and its slopes at (u, v), in its own
            /// coordinates (rational patches divided out).
        static void         Evaluate( const SPatch &oPatch, double u, double v, double anPoint[ 3 ], double anDu[ 3 ], double anDv[ 3 ] );
            /// The unit normal of a patch at (u, v), taken a hair inside the
            /// patch where the slopes' cross product vanishes.
        static void         Normal( const SPatch &oPatch, double u, double v, double anNormal[ 3 ] );
            /// Reads the patches of a .bpt file's text; FALSE with a reason.
        static BOOL         ReadBpt( const std::string &strText, std::vector<SPatch> &aoPatches, std::string &strError );
            /// Checks a patch: degrees, points and positive weights. FALSE
            /// with a reason.
        static BOOL         CheckPatch( const SPatch &oPatch, std::string &strError );
};

#endif // __BEZIER_H__
