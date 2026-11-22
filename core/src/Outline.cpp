/*************************************************************************
*   Outline                                                              *
*                                                                        *
*   Flattening, filling and cutting flat outlines into triangles. See    *
*   Outline.h.                                                           *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/


#include "Outline.h"

#include <algorithm>
#include <cmath>

namespace Outline
{
    namespace
    {
        SPoint Mid( const SPoint &a, const SPoint &b )
        {
            SPoint m = { ( a.x + b.x ) / 2.0, ( a.y + b.y ) / 2.0 };

            return m;
        }

        double Cross( const SPoint &o, const SPoint &a, const SPoint &b )
        {
            return ( ( a.x - o.x ) * ( b.y - o.y ) ) - ( ( a.y - o.y ) * ( b.x - o.x ) );
        }

        bool Same( const SPoint &a, const SPoint &b )
        {
            return a.x == b.x && a.y == b.y;
        }

            // Cuts the quadratic Bezier a, c, b into pieces no further than
            // nTolerance from it, adding each piece's end.
        void Quadratic( Polygon &oOut, const SPoint &a, const SPoint &c, const SPoint &b, double nTolerance, int nDepth )
        {
            double nX = a.x - ( 2.0 * c.x ) + b.x, nY = a.y - ( 2.0 * c.y ) + b.y;

            if ( nDepth >= 16 || std::sqrt( ( nX * nX ) + ( nY * nY ) ) / 4.0 <= nTolerance )
            {
                oOut.push_back( b );
                return;
            }

            SPoint ac = Mid( a, c ), cb = Mid( c, b ), m = Mid( ac, cb );

            Quadratic( oOut, a, ac, m, nTolerance, nDepth + 1 );
            Quadratic( oOut, m, cb, b, nTolerance, nDepth + 1 );
        }

            // Drops repeated points and points in line with their
            // neighbours.
        Polygon Clean( const Polygon &oIn )
        {
            Polygon oOut;

            for ( const SPoint &p : oIn )
            {
                if ( oOut.empty() || ! Same( oOut.back(), p ) )
                    oOut.push_back( p );
            }
            while ( oOut.size() > 1 && Same( oOut.front(), oOut.back() ) )
                oOut.pop_back();

            bool bChanged = true;

            while ( bChanged && oOut.size() >= 3 )
            {
                bChanged = false;
                for ( size_t i = 0; i < oOut.size() && oOut.size() >= 3; i++ )
                {
                    const SPoint &p = oOut[ ( i + oOut.size() - 1 ) % oOut.size() ];
                    const SPoint &n = oOut[ ( i + 1 ) % oOut.size() ];
                    double        nLength = std::fabs( n.x - p.x ) + std::fabs( n.y - p.y );

                        // In line, and not a turn back on itself.
                    if ( std::fabs( Cross( p, oOut[ i ], n ) ) <= 1e-12 * nLength * nLength &&
                         ( ( oOut[ i ].x - p.x ) * ( n.x - oOut[ i ].x ) ) + ( ( oOut[ i ].y - p.y ) * ( n.y - oOut[ i ].y ) ) >= 0.0 )
                    {
                        oOut.erase( oOut.begin() + (long)i );
                        bChanged = true;
                        i--;
                    }
                }
            }
            return oOut;
        }

        bool Inside( const SPoint &q, const Polygon &oPolygon )
        {
            bool bInside = false;

            for ( size_t i = 0, j = oPolygon.size() - 1; i < oPolygon.size(); j = i++ )
            {
                const SPoint &a = oPolygon[ i ], &b = oPolygon[ j ];

                if ( ( ( a.y > q.y ) != ( b.y > q.y ) ) && ( q.x < ( ( b.x - a.x ) * ( q.y - a.y ) / ( b.y - a.y ) ) + a.x ) )
                    bInside = ! bInside;
            }
            return bInside;
        }

            // q inside or on the edge of the counterclockwise triangle a b c.
        bool InTriangle( const SPoint &q, const SPoint &a, const SPoint &b, const SPoint &c )
        {
            return Cross( a, b, q ) >= 0.0 && Cross( b, c, q ) >= 0.0 && Cross( c, a, q ) >= 0.0;
        }

            // Joins each hole into the outline, by a cut from the hole's
            // rightmost vertex to a vertex of the outline it can see.
        bool Bridge( const std::vector<SPoint> &aoPoints, std::vector<int> &anPolygon, std::vector< std::vector<int> > aanHoles,
                     std::string &strError )
        {
            std::sort( aanHoles.begin(), aanHoles.end(), [ &aoPoints ]( const std::vector<int> &a, const std::vector<int> &b )
            {
                double nA = -1e300, nB = -1e300;

                for ( int i : a )
                    nA = ( std::max )( nA, aoPoints[ (size_t)i ].x );
                for ( int i : b )
                    nB = ( std::max )( nB, aoPoints[ (size_t)i ].x );
                return nA > nB;
            } );

            for ( const std::vector<int> &anHole : aanHoles )
            {
                size_t nM = 0;

                for ( size_t i = 1; i < anHole.size(); i++ )
                {
                    const SPoint &p = aoPoints[ (size_t)anHole[ i ] ], &m = aoPoints[ (size_t)anHole[ nM ] ];

                    if ( p.x > m.x || ( p.x == m.x && p.y > m.y ) )
                        nM = i;
                }

                const SPoint &M = aoPoints[ (size_t)anHole[ nM ] ];
                size_t        nN = anPolygon.size();
                double        nBestX = 1e300;
                long          nEdge = -1;

                    // The nearest outline edge the ray to the right meets.
                for ( size_t k = 0; k < nN; k++ )
                {
                    const SPoint &a = aoPoints[ (size_t)anPolygon[ k ] ];
                    const SPoint &b = aoPoints[ (size_t)anPolygon[ ( k + 1 ) % nN ] ];

                    if ( a.y == b.y || ( a.y > M.y && b.y > M.y ) || ( a.y < M.y && b.y < M.y ) )
                        continue;

                    double x = a.x + ( ( M.y - a.y ) * ( b.x - a.x ) / ( b.y - a.y ) );

                    if ( x >= M.x && x < nBestX )
                    {
                        nBestX = x;
                        nEdge = (long)k;
                    }
                }
                if ( nEdge < 0 )
                {
                    strError = "has a hole that isn't inside its outline";
                    return false;
                }

                size_t        k0 = (size_t)nEdge, k1 = ( k0 + 1 ) % nN;
                const SPoint &a = aoPoints[ (size_t)anPolygon[ k0 ] ];
                const SPoint &b = aoPoints[ (size_t)anPolygon[ k1 ] ];
                SPoint        I = { nBestX, M.y };
                size_t        nP;

                if ( Same( a, I ) )
                    nP = k0;
                else if ( Same( b, I ) )
                    nP = k1;
                else
                {
                    nP = ( a.x > b.x ) ? k0 : k1;

                        // A reflex vertex inside M, I, P would block the
                        // cut: take the one at the smallest angle instead.
                    SPoint P = aoPoints[ (size_t)anPolygon[ nP ] ];
                    SPoint t1 = M, t2 = I, t3 = P;

                    if ( Cross( t1, t2, t3 ) < 0.0 )
                        std::swap( t2, t3 );

                    double nBestAngle = 1e300, nBestDistance = 1e300;

                    for ( size_t k = 0; k < nN; k++ )
                    {
                        if ( k == nP )
                            continue;

                        const SPoint &r = aoPoints[ (size_t)anPolygon[ k ] ];
                        const SPoint &rp = aoPoints[ (size_t)anPolygon[ ( k + nN - 1 ) % nN ] ];
                        const SPoint &rn = aoPoints[ (size_t)anPolygon[ ( k + 1 ) % nN ] ];

                        if ( Cross( rp, r, rn ) >= 0.0 || ! InTriangle( r, t1, t2, t3 ) || Same( r, P ) )
                            continue;

                        double dx = r.x - M.x, dy = r.y - M.y;
                        double nDistance = std::sqrt( ( dx * dx ) + ( dy * dy ) );
                        double nAngle = ( nDistance > 0.0 ) ? std::fabs( dy ) / nDistance : 0.0;

                        if ( nAngle < nBestAngle || ( nAngle == nBestAngle && nDistance < nBestDistance ) )
                        {
                            nBestAngle = nAngle;
                            nBestDistance = nDistance;
                            nP = k;
                        }
                    }
                }

                    // P, around the hole from M back to M, then P again.
                std::vector<int> anJoined( anPolygon.begin(), anPolygon.begin() + (long)nP + 1 );

                for ( size_t i = 0; i <= anHole.size(); i++ )
                    anJoined.push_back( anHole[ ( nM + i ) % anHole.size() ] );
                anJoined.push_back( anPolygon[ nP ] );
                anJoined.insert( anJoined.end(), anPolygon.begin() + (long)nP + 1, anPolygon.end() );
                anPolygon.swap( anJoined );
            }
            return true;
        }

            // Cuts a counterclockwise polygon (which may touch itself along
            // the cuts to its holes) into triangles by clipping ears.
        void EarClip( const std::vector<SPoint> &aoPoints, std::vector<int> anPolygon, std::vector<int> &anTriangles )
        {
            while ( anPolygon.size() > 3 )
            {
                size_t nN = anPolygon.size();
                bool   bClipped = false;
                long   nFlattest = -1;
                double nFlattestCross = 1e300;
                long   nBestConvex = -1;
                double nBestConvexCross = -1.0;

                for ( size_t i = 0; i < nN && ! bClipped; i++ )
                {
                    int           ip = anPolygon[ ( i + nN - 1 ) % nN ], iv = anPolygon[ i ], in = anPolygon[ ( i + 1 ) % nN ];
                    const SPoint &p = aoPoints[ (size_t)ip ], &v = aoPoints[ (size_t)iv ], &n = aoPoints[ (size_t)in ];
                    double        c = Cross( p, v, n );

                    if ( std::fabs( c ) < nFlattestCross )
                    {
                        nFlattestCross = std::fabs( c );
                        nFlattest = (long)i;
                    }
                    if ( c <= 0.0 )
                        continue;
                    if ( c > nBestConvexCross )
                    {
                        nBestConvexCross = c;
                        nBestConvex = (long)i;
                    }

                    bool bEar = true;

                    for ( size_t k = 0; k < nN && bEar; k++ )
                    {
                        const SPoint &q = aoPoints[ (size_t)anPolygon[ k ] ];

                        if ( Same( q, p ) || Same( q, v ) || Same( q, n ) )
                            continue;
                        if ( InTriangle( q, p, v, n ) )
                            bEar = false;
                    }
                    if ( bEar )
                    {
                        anTriangles.push_back( ip );
                        anTriangles.push_back( iv );
                        anTriangles.push_back( in );
                        anPolygon.erase( anPolygon.begin() + (long)i );
                        bClipped = true;
                    }
                }
                if ( bClipped )
                    continue;

                    // No ear: drop a vertex in line with its neighbours, or
                    // (an outline that crosses itself) clip the most convex
                    // corner anyway, so the loop always ends.
                double nScale = 0.0;

                for ( int i : anPolygon )
                    nScale = ( std::max )( nScale, std::fabs( aoPoints[ (size_t)i ].x ) + std::fabs( aoPoints[ (size_t)i ].y ) );
                if ( nFlattest >= 0 && nFlattestCross <= 1e-12 * nScale * nScale )
                    anPolygon.erase( anPolygon.begin() + nFlattest );
                else if ( nBestConvex >= 0 )
                {
                    size_t i = (size_t)nBestConvex;

                    anTriangles.push_back( anPolygon[ ( i + nN - 1 ) % nN ] );
                    anTriangles.push_back( anPolygon[ i ] );
                    anTriangles.push_back( anPolygon[ ( i + 1 ) % nN ] );
                    anPolygon.erase( anPolygon.begin() + (long)i );
                }
                else
                    return;
            }
            if ( anPolygon.size() == 3 &&
                 Cross( aoPoints[ (size_t)anPolygon[ 0 ] ], aoPoints[ (size_t)anPolygon[ 1 ] ], aoPoints[ (size_t)anPolygon[ 2 ] ] ) > 0.0 )
                anTriangles.insert( anTriangles.end(), anPolygon.begin(), anPolygon.end() );
        }
    }

    Polygon Flatten( const CFontFile::Contour &oContour, double nTolerance )
    {
        Polygon oOut;

        if ( oContour.size() < 2 )
            return oOut;

            // Between two control points is an implied point on the curve.
        std::vector<CFontFile::SPoint> aoPoints;

        for ( size_t i = 0; i < oContour.size(); i++ )
        {
            const CFontFile::SPoint &p = oContour[ i ], &n = oContour[ ( i + 1 ) % oContour.size() ];

            aoPoints.push_back( p );
            if ( ! p.bOnCurve && ! n.bOnCurve )
            {
                CFontFile::SPoint m = { ( p.x + n.x ) / 2.0, ( p.y + n.y ) / 2.0, true };

                aoPoints.push_back( m );
            }
        }

        size_t nStart = 0;

        while ( nStart < aoPoints.size() && ! aoPoints[ nStart ].bOnCurve )
            nStart++;
        if ( nStart == aoPoints.size() )
            return oOut;

        size_t nCount = aoPoints.size();
        SPoint oCurrent = { aoPoints[ nStart ].x, aoPoints[ nStart ].y };

        oOut.push_back( oCurrent );
        for ( size_t j = 1; j <= nCount; )
        {
            const CFontFile::SPoint &q = aoPoints[ ( nStart + j ) % nCount ];
            SPoint                   oQ = { q.x, q.y };

            if ( q.bOnCurve )
            {
                oOut.push_back( oQ );
                oCurrent = oQ;
                j++;
                continue;
            }

            const CFontFile::SPoint &r = aoPoints[ ( nStart + j + 1 ) % nCount ];
            SPoint                   oR = { r.x, r.y };

            Quadratic( oOut, oCurrent, oQ, oR, nTolerance, 0 );
            oCurrent = oR;
            j += 2;
        }
        return Clean( oOut );
    }

    double SignedArea2( const Polygon &oPolygon )
    {
        double nArea = 0.0;

        for ( size_t i = 0, j = oPolygon.size() - 1; i < oPolygon.size(); j = i++ )
            nArea += ( oPolygon[ j ].x * oPolygon[ i ].y ) - ( oPolygon[ i ].x * oPolygon[ j ].y );
        return nArea;
    }

    bool Fill( const std::vector<Polygon> &aoOutlines, SShape &oShape, std::string &strError )
    {
        std::vector<Polygon> aoKept;
        std::vector<double>  anArea;

        oShape = SShape();
        for ( const Polygon &oOutline : aoOutlines )
        {
            Polygon oClean = Clean( oOutline );
            double  nArea = ( oClean.size() >= 3 ) ? SignedArea2( oClean ) : 0.0;

            if ( std::fabs( nArea ) > 0.0 )
            {
                aoKept.push_back( oClean );
                anArea.push_back( nArea );
            }
        }

            // How deep each outline sits, and the smallest one around it.
        size_t           nCount = aoKept.size();
        std::vector<int> anDepth( nCount, 0 ), anParent( nCount, -1 );

        for ( size_t i = 0; i < nCount; i++ )
        {
            for ( size_t j = 0; j < nCount; j++ )
            {
                if ( i == j || std::fabs( anArea[ j ] ) <= std::fabs( anArea[ i ] ) || ! Inside( aoKept[ i ][ 0 ], aoKept[ j ] ) )
                    continue;
                anDepth[ i ]++;
                if ( anParent[ i ] < 0 || std::fabs( anArea[ j ] ) < std::fabs( anArea[ (size_t)anParent[ i ] ] ) )
                    anParent[ i ] = (int)j;
            }
        }

            // Every outline goes in as a loop, filled side on its left.
        std::vector< std::vector<int> > aanLoop( nCount );

        for ( size_t i = 0; i < nCount; i++ )
        {
            bool    bHole = ( anDepth[ i ] % 2 ) == 1;
            Polygon oRun = aoKept[ i ];

            if ( ( anArea[ i ] > 0.0 ) == bHole )
                std::reverse( oRun.begin(), oRun.end() );
            for ( const SPoint &p : oRun )
            {
                aanLoop[ i ].push_back( (int)oShape.aoPoints.size() );
                oShape.aoPoints.push_back( p );
            }
            oShape.aanLoops.push_back( aanLoop[ i ] );
        }

        for ( size_t i = 0; i < nCount; i++ )
        {
            if ( anDepth[ i ] % 2 == 1 )
                continue;

            std::vector<int>                anPolygon = aanLoop[ i ];
            std::vector< std::vector<int> > aanHoles;

            for ( size_t h = 0; h < nCount; h++ )
            {
                if ( anDepth[ h ] % 2 == 1 && anParent[ h ] == (int)i )
                    aanHoles.push_back( aanLoop[ h ] );
            }
            if ( ! Bridge( oShape.aoPoints, anPolygon, aanHoles, strError ) )
                return false;
            EarClip( oShape.aoPoints, anPolygon, oShape.anTriangles );
        }
        return true;
    }

    std::vector<Polygon> Inset( const SShape &oShape, double nInset )
    {
        std::vector<Polygon> aoOut;

        for ( const std::vector<int> &anLoop : oShape.aanLoops )
        {
            Polygon oPolygon;
            size_t  nN = anLoop.size();

            for ( size_t i = 0; i < nN; i++ )
            {
                const SPoint &p = oShape.aoPoints[ (size_t)anLoop[ ( i + nN - 1 ) % nN ] ];
                const SPoint &v = oShape.aoPoints[ (size_t)anLoop[ i ] ];
                const SPoint &n = oShape.aoPoints[ (size_t)anLoop[ ( i + 1 ) % nN ] ];
                double        l1 = std::hypot( v.x - p.x, v.y - p.y ), l2 = std::hypot( n.x - v.x, n.y - v.y );
                double        n1x = -( v.y - p.y ) / l1, n1y = ( v.x - p.x ) / l1;
                double        n2x = -( n.y - v.y ) / l2, n2y = ( n.x - v.x ) / l2;
                double        mx = n1x + n2x, my = n1y + n2y, ml = std::hypot( mx, my );
                double        nDistance = nInset;

                if ( ml < 1e-9 )
                {
                    mx = n1x;
                    my = n1y;
                }
                else
                {
                    mx /= ml;
                    my /= ml;

                    double nCos = ( mx * n1x ) + ( my * n1y );

                    nDistance = ( nCos > 1.0 / 3.0 ) ? nInset / nCos : 3.0 * nInset;
                }

                SPoint q = { v.x + ( mx * nDistance ), v.y + ( my * nDistance ) };

                oPolygon.push_back( q );
            }
            aoOut.push_back( oPolygon );
        }
        return aoOut;
    }
}
