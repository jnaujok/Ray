/*************************************************************************
*   Extruder                                                             *
*                                                                        *
*   Closed solids from flat shapes. See Extruder.h.                      *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

    #include "Globals.h"

#include "Extruder.h"
#include "LocalShape.h"

#include <cmath>
#include <string>


namespace Extruder
{
    int SBuilder::Add( double x, double y, double z, double nx, double ny, double nz )
    {
        return AddUV( x, y, z, nx, ny, nz, x * nPerUnit, y * nPerUnit );
    }

    int SBuilder::AddUV( double x, double y, double z, double nx, double ny, double nz, double u, double v )
    {
        double l = std::sqrt( ( nx * nx ) + ( ny * ny ) + ( nz * nz ) );

        aoVertices.push_back( CVector( x, y, z ) );
        aoNormals.push_back( CVector( nx / l, ny / l, nz / l ) );
        anUVs.push_back( u );
        anUVs.push_back( v );
        return (int)aoVertices.size() - 1;
    }

    void SBuilder::Face( int a, int b, int c, double nx, double ny, double nz )
    {
        CVector &A = aoVertices[ (size_t)a ], &B = aoVertices[ (size_t)b ], &C = aoVertices[ (size_t)c ];
        CVector  e1 = B - A, e2 = C - B;
        CVector  n = e1.Cross( e2 );

            // The rule CTriangle uses for which way a face faces.
        if ( ( n.GetX() * nx ) + ( n.GetY() * ny ) + ( n.GetZ() * nz ) < 0.0 )
            std::swap( b, c );
        anFaces.push_back( a );
        anFaces.push_back( b );
        anFaces.push_back( c );
    }

    void SBuilder::Quad( int a, int b, int c, int d, double nx, double ny, double nz )
    {
        Face( a, b, c, nx, ny, nz );
        Face( a, c, d, nx, ny, nz );
    }

    double TriangleArea2( const Outline::SShape &oShape )
    {
        double nArea = 0.0;

        for ( size_t t = 0; t + 2 < oShape.anTriangles.size(); t += 3 )
        {
            const Outline::SPoint &a = oShape.aoPoints[ (size_t)oShape.anTriangles[ t ] ];
            const Outline::SPoint &b = oShape.aoPoints[ (size_t)oShape.anTriangles[ t + 1 ] ];
            const Outline::SPoint &c = oShape.aoPoints[ (size_t)oShape.anTriangles[ t + 2 ] ];

            nArea += ( ( b.x - a.x ) * ( c.y - a.y ) ) - ( ( b.y - a.y ) * ( c.x - a.x ) );
        }
        return nArea;
    }
}

namespace
{
    using Extruder::SBuilder;

        // A flat face of the shape at depth z, facing nFacing (-1 or 1) in z.
    void Cap( SBuilder &oOut, const Outline::SShape &oShape, double z, double nFacing )
    {
        std::vector<int> anIndex( oShape.aoPoints.size() );

        for ( size_t i = 0; i < oShape.aoPoints.size(); i++ )
            anIndex[ i ] = oOut.Add( oShape.aoPoints[ i ].x, oShape.aoPoints[ i ].y, z, 0.0, 0.0, nFacing );
        for ( size_t t = 0; t + 2 < oShape.anTriangles.size(); t += 3 )
            oOut.Face( anIndex[ (size_t)oShape.anTriangles[ t ] ], anIndex[ (size_t)oShape.anTriangles[ t + 1 ] ],
                       anIndex[ (size_t)oShape.anTriangles[ t + 2 ] ], 0.0, 0.0, nFacing );
    }

        // Each loop vertex's outward normal (in the plane) for the edge
        // before it and the edge after it: the same, averaged, where the
        // outline turns gently; each edge's own at a corner.
    void LoopNormals( const std::vector<Outline::SPoint> &aoLoop, std::vector<Outline::SPoint> &aoBefore,
                      std::vector<Outline::SPoint> &aoAfter )
    {
        size_t                       nN = aoLoop.size();
        std::vector<Outline::SPoint> aoEdge( nN );
        const double                 nSmooth = std::cos( 35.0 * PI / 180.0 );

        for ( size_t k = 0; k < nN; k++ )
        {
            const Outline::SPoint &a = aoLoop[ k ], &b = aoLoop[ ( k + 1 ) % nN ];
            double                 l = std::hypot( b.x - a.x, b.y - a.y );
            Outline::SPoint        n = { ( b.y - a.y ) / l, -( b.x - a.x ) / l };

            aoEdge[ k ] = n;
        }
        aoBefore.resize( nN );
        aoAfter.resize( nN );
        for ( size_t k = 0; k < nN; k++ )
        {
            const Outline::SPoint &p = aoEdge[ ( k + nN - 1 ) % nN ], &q = aoEdge[ k ];

            if ( ( p.x * q.x ) + ( p.y * q.y ) >= nSmooth )
            {
                double          l = std::hypot( p.x + q.x, p.y + q.y );
                Outline::SPoint m = { ( p.x + q.x ) / l, ( p.y + q.y ) / l };

                aoBefore[ k ] = aoAfter[ k ] = m;
            }
            else
            {
                aoBefore[ k ] = p;
                aoAfter[ k ] = q;
            }
        }
    }

        // The band between two copies of a loop: aoFrom at depth z0 and aoTo
        // at z1, the normals leaning nLean in z (0 for the straight sides).
    void Band( SBuilder &oOut, const std::vector<Outline::SPoint> &aoFrom, double z0, const std::vector<Outline::SPoint> &aoTo, double z1,
               const std::vector<Outline::SPoint> &aoBefore, const std::vector<Outline::SPoint> &aoAfter, double nLean )
    {
        size_t nN = aoFrom.size();

        for ( size_t k = 0; k < nN; k++ )
        {
            size_t                 k1 = ( k + 1 ) % nN;
            const Outline::SPoint &n0 = aoAfter[ k ], &n1 = aoBefore[ k1 ];
            double                 ex = aoTo[ k1 ].y - aoTo[ k ].y, ey = -( aoTo[ k1 ].x - aoTo[ k ].x );
            int                    a = oOut.Add( aoFrom[ k ].x, aoFrom[ k ].y, z0, n0.x, n0.y, nLean );
            int                    b = oOut.Add( aoFrom[ k1 ].x, aoFrom[ k1 ].y, z0, n1.x, n1.y, nLean );
            int                    c = oOut.Add( aoTo[ k1 ].x, aoTo[ k1 ].y, z1, n1.x, n1.y, nLean );
            int                    d = oOut.Add( aoTo[ k ].x, aoTo[ k ].y, z1, n0.x, n0.y, nLean );
            double                 l = std::hypot( ex, ey );

            if ( l > 0.0 )
                oOut.Quad( a, b, c, d, ex / l, ey / l, nLean );
        }
    }

    std::vector<Outline::SPoint> LoopPoints( const Outline::SShape &oShape, size_t nLoop )
    {
        std::vector<Outline::SPoint> aoOut;

        for ( int i : oShape.aanLoops[ nLoop ] )
            aoOut.push_back( oShape.aoPoints[ (size_t)i ] );
        return aoOut;
    }

        // The outline moved in by nBevel, filled: used only if it fills
        // cleanly and keeps every point.
    bool BevelShape( const Outline::SShape &oShape, double nBevel, Outline::SShape &oInner, std::vector<Outline::Polygon> &aoInset )
    {
        std::string strError;

        if ( ! ( nBevel > 0.0 ) )
            return false;
        aoInset = Outline::Inset( oShape, nBevel );
        if ( ! Outline::Fill( aoInset, oInner, strError ) || oInner.aanLoops.size() != oShape.aanLoops.size() )
            return false;

        double nInner = Extruder::TriangleArea2( oInner ), nOuter = Extruder::TriangleArea2( oShape );
        bool   bGood = nInner > 0.0 && nInner < nOuter;

        for ( size_t n = 0; bGood && n < oShape.aanLoops.size(); n++ )
            bGood = oInner.aanLoops[ n ].size() == oShape.aanLoops[ n ].size();
        return bGood;
    }
}

namespace Extruder
{
    void Extrude( SBuilder &oOut, const Outline::SShape &oShape, double nDepth, double nBevel, double nBackBevel )
    {
        Outline::SShape               oFront, oBack;
        std::vector<Outline::Polygon> aoInset, aoBackInset;
        bool                          bBevel = BevelShape( oShape, nBevel, oFront, aoInset );
        bool                          bBack = BevelShape( oShape, nBackBevel, oBack, aoBackInset );
        double                        nSide = bBevel ? nBevel : 0.0;
        double                        nBackSide = bBack ? nBackBevel : 0.0;

        Cap( oOut, bBevel ? oFront : oShape, 0.0, -1.0 );
        Cap( oOut, bBack ? oBack : oShape, nDepth, 1.0 );
        for ( size_t n = 0; n < oShape.aanLoops.size(); n++ )
        {
            std::vector<Outline::SPoint> aoLoop = LoopPoints( oShape, n );
            std::vector<Outline::SPoint> aoBefore, aoAfter;

            LoopNormals( aoLoop, aoBefore, aoAfter );
            if ( bBevel )
            {
                    // The bevel from the inset outline (point for point
                    // with the loop) at the face back to the loop itself.
                Band( oOut, aoInset[ n ], 0.0, aoLoop, nSide, aoBefore, aoAfter, -1.0 );
            }
            Band( oOut, aoLoop, nSide, aoLoop, nDepth - nBackSide, aoBefore, aoAfter, 0.0 );
            if ( bBack )
                Band( oOut, aoLoop, nDepth - nBackSide, aoBackInset[ n ], nDepth, aoBefore, aoAfter, 1.0 );
        }
    }

    void ExtrudeTapered( SBuilder &oOut, const Outline::SShape &oShape, double nDepth, double nTaper, double nTwist )
    {
        int nSlices = (int)std::ceil( std::fabs( nTwist ) / ( 2.0 * PI / 180.0 ) );

        if ( nSlices < 1 )
            nSlices = 1;

            // Where a point of the front ends up at slice j, and the
            // slope of that along the depth.
        auto Scale = [&]( int j ) { return 1.0 + ( ( nTaper - 1.0 ) * j / nSlices ); };
        auto Angle = [&]( int j ) { return nTwist * j / nSlices; };

        Outline::SShape oEnd = oShape;

        for ( Outline::SPoint &p : oEnd.aoPoints )
        {
            double s = Scale( nSlices ), a = Angle( nSlices );
            double x = s * ( ( p.x * std::cos( a ) ) - ( p.y * std::sin( a ) ) );
            double y = s * ( ( p.x * std::sin( a ) ) + ( p.y * std::cos( a ) ) );

            p.x = x;
            p.y = y;
        }
        Cap( oOut, oShape, 0.0, -1.0 );
        Cap( oOut, oEnd, nDepth, 1.0 );

        double nScaleSlope = ( nTaper - 1.0 ) / nDepth, nTurnSlope = nTwist / nDepth;

        for ( size_t n = 0; n < oShape.aanLoops.size(); n++ )
        {
            std::vector<Outline::SPoint> aoLoop = LoopPoints( oShape, n );
            std::vector<Outline::SPoint> aoBefore, aoAfter;
            size_t                       nN = aoLoop.size();

            LoopNormals( aoLoop, aoBefore, aoAfter );

                // A vertex of slice j for loop point k with the 2D normal m:
                // the normal is the loop's tangent there (carried by the
                // slice's scale and turn) crossed with the slope along z.
            auto Vertex = [&]( int j, size_t k, const Outline::SPoint &m, double *anNormal ) -> int
            {
                double s = Scale( j ), a = Angle( j ), c = std::cos( a ), si = std::sin( a ), z = nDepth * j / nSlices;
                const Outline::SPoint &q = aoLoop[ k ];
                double px = s * ( ( q.x * c ) - ( q.y * si ) ), py = s * ( ( q.x * si ) + ( q.y * c ) );
                double tx = -m.y, ty = m.x;
                double Tx = s * ( ( tx * c ) - ( ty * si ) ), Ty = s * ( ( tx * si ) + ( ty * c ) );
                double Dx = ( nScaleSlope * ( ( q.x * c ) - ( q.y * si ) ) ) + ( s * nTurnSlope * ( ( -q.x * si ) - ( q.y * c ) ) );
                double Dy = ( nScaleSlope * ( ( q.x * si ) + ( q.y * c ) ) ) + ( s * nTurnSlope * ( ( q.x * c ) - ( q.y * si ) ) );
                double Nx = Ty, Ny = -Tx, Nz = ( Tx * Dy ) - ( Ty * Dx );
                double Mx = ( m.x * c ) - ( m.y * si ), My = ( m.x * si ) + ( m.y * c );

                    // T x D with D = ( Dx, Dy, 1 ); it should lean toward
                    // the outline's outside.
                if ( ( Nx * Mx ) + ( Ny * My ) < 0.0 )
                {
                    Nx = -Nx;
                    Ny = -Ny;
                    Nz = -Nz;
                }
                anNormal[ 0 ] = Nx;
                anNormal[ 1 ] = Ny;
                anNormal[ 2 ] = Nz;
                return oOut.Add( px, py, z, Nx, Ny, Nz );
            };

            for ( int j = 0; j < nSlices; j++ )
            {
                for ( size_t k = 0; k < nN; k++ )
                {
                    size_t k1 = ( k + 1 ) % nN;
                    double na[ 3 ], nb[ 3 ], nc[ 3 ], nd[ 3 ];
                    int    a = Vertex( j, k, aoAfter[ k ], na );
                    int    b = Vertex( j, k1, aoBefore[ k1 ], nb );
                    int    c = Vertex( j + 1, k1, aoBefore[ k1 ], nc );
                    int    d = Vertex( j + 1, k, aoAfter[ k ], nd );

                    oOut.Quad( a, b, c, d, na[ 0 ] + nb[ 0 ] + nc[ 0 ] + nd[ 0 ], na[ 1 ] + nb[ 1 ] + nc[ 1 ] + nd[ 1 ],
                               na[ 2 ] + nb[ 2 ] + nc[ 2 ] + nd[ 2 ] );
                }
            }
        }
    }

    void Place( SBuilder &oOut, CVector oRotationDegrees, CVector oMove )
    {
        FLT nToRadians = PI / 180.0;

        for ( size_t i = 0; i < oOut.aoVertices.size(); i++ )
        {
            FLT anPoint[ 3 ] = { oOut.aoVertices[ i ].GetX(), oOut.aoVertices[ i ].GetY(), oOut.aoVertices[ i ].GetZ() };
            FLT anNormal[ 3 ] = { oOut.aoNormals[ i ].GetX(), oOut.aoNormals[ i ].GetY(), oOut.aoNormals[ i ].GetZ() };

            CLocalShape::RotateXYZ( anPoint, oRotationDegrees.GetX() * nToRadians, oRotationDegrees.GetY() * nToRadians,
                                    oRotationDegrees.GetZ() * nToRadians );
            CLocalShape::RotateXYZ( anNormal, oRotationDegrees.GetX() * nToRadians, oRotationDegrees.GetY() * nToRadians,
                                    oRotationDegrees.GetZ() * nToRadians );
            oOut.aoVertices[ i ] = CVector( anPoint[ 0 ], anPoint[ 1 ], anPoint[ 2 ] ) + oMove;
            oOut.aoNormals[ i ] = CVector( anNormal[ 0 ], anNormal[ 1 ], anNormal[ 2 ] );
        }
    }
}
