/*************************************************************************
*   CLathe                                                               *
*                                                                        *
*   A solid of revolution: a profile in the r-y plane (r the distance    *
*   from the axis, y the height) spun around its own y axis, as a lathe  *
*   turns a vase, a goblet, a chess piece or a table leg. The profile is *
*   a path of lines and curves (see Profile.h), or with smooth a curve   *
*   through its points; r must not go below 0.                           *
*                                                                        *
*   The lathe is traced as it is, not cut into triangles: each piece of  *
*   the profile spun around the axis meets a ray where a polynomial in   *
*   the piece's own parameter is zero (degree 2 for a line, which makes  *
*   a cone, up to 6 for a cubic), found by bisection between its turning *
*   points and polished by Newton's method.                              *
*                                                                        *
*   Which side is outside: a profile that starts and ends on the axis    *
*   (r = 0) encloses a solid, and so does one whose open ends capped     *
*   closes with flat discs from each end in to the axis; its normals     *
*   point out of the solid whichever way it runs. An open profile is a   *
*   shell seen from both sides. sweep (degrees, default 360) spins it    *
*   only part way round, from +x toward +z; a solid is then closed by    *
*   the flat faces at each end of the sweep.                             *
*                                                                        *
*   Surface coordinates: u is the angle around (0 to 1 over the sweep),  *
*   v the distance along the profile (0 to 1, including the caps).       *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __LATHE_H__
#define __LATHE_H__

#include "LocalShape.h"
#include "Profile.h"

class CLathe : public CLocalShape
{
    protected:
            // A piece of the closed (or open) loop in r and y, as
            // polynomials in its parameter s from 0 to 1.
        struct SPiece
        {
            int     nDegree;            // 1 to 3
            double  anR[ 4 ];
            double  anY[ 4 ];
            double  nRMax;              // the most r and the y range of its control points
            double  nYMin;
            double  nYMax;
            double  nV0;                // v at s = 0 and s = 1
            double  nV1;
        };

        Profile::SPath      m_oProfile;
        BOOL                m_bSmooth;
        BOOL                m_bCapped;
        FLT                 m_nSweep;           // radians, more than 0 up to 2 pi

            // Worked out by PrepareShape.
        std::vector<SPiece> m_aoPieces;
        Outline::Polygon    m_oRegion;          // the area the profile closes in r-y, for the end faces
        BOOL                m_bClosed;
        BOOL                m_bPartial;
        double              m_nOrient;          // 1, or -1 if the closed loop runs clockwise in r-y
        double              m_nRMax;
        double              m_nYMin;
        double              m_nYMax;
        double              m_anEndNormal[ 2 ][ 3 ];    // the end faces' outward normals (partial sweep)
        double              m_anEndAlong[ 2 ][ 3 ];     // the direction in each end face away from the axis

        BOOL                PieceHit( const SPiece &oPiece, const FLT anO[ 3 ], const FLT anD[ 3 ], double nLow, double nHigh, double &nBest );
        BOOL                InSweep( double x, double z ) const;
        BOOL                InRegion( double r, double y ) const;
            // The nearest point of the loop to (r, y): which piece and its s.
        double              Nearest( double r, double y, int &nPiece, double &s ) const;

        virtual BOOL        PrepareShape( void );
        virtual BOOL        LocalHit( const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLow, FLT nHigh, FLT &nT );
        virtual void        LocalNormal( const FLT anPoint[ 3 ], FLT anNormal[ 3 ] );
        virtual void        LocalCoords( const FLT anPoint[ 3 ], FLT &nU, FLT &nV );
        virtual void        LocalBounds( FLT anLow[ 3 ], FLT anHigh[ 3 ] );
        virtual void        ShapeToJson( IASLib::CObjectNode &oNode );
        virtual BOOL        ShapeFromJson( const IASLib::CJsonNode &oNode );
        virtual const char *const *ShapeFields( void );
        virtual void        LocalTriangles( int nSegments, std::vector<FLT> &anPositions, std::vector<FLT> &anNormals );

    public:
                            DYNAMIC_OBJECTS( CLathe );

                            CLathe( void );
        virtual            ~CLathe( void );

        virtual BOOL        IsClosed( void ) { return m_bClosed; }
};

#endif // __LATHE_H__
