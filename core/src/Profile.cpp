/*************************************************************************
*   Profile                                                              *
*                                                                        *
*   Flat paths of lines and curves. See Profile.h.                       *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Profile.h"
#include "SceneJson.h"

#include <cmath>
#include <string>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace
{
    BOOL ReadXY( const IASLib::CJsonNode *pNode, double anOut[ 2 ] )
    {
        if ( ! pNode || ! pNode->isArray() || pNode->size() != 2 )
            return RayJson::Fail( "a point must be [x, y]" );
        for ( int c = 0; c < 2; c++ )
        {
            const IASLib::CJsonNode *pValue = pNode->get( c );

            if ( ! pValue || ! pValue->isNumber() )
                return RayJson::Fail( "a point must be [x, y]" );
            anOut[ c ] = pValue->doubleValue();
            if ( ! std::isfinite( anOut[ c ] ) )
                return RayJson::Fail( "a point must be [x, y]" );
        }
        return TRUE;
    }

    void AddXY( IASLib::CArrayNode &oArray, const double anPoint[ 2 ] )
    {
        oArray.addArray()->add( RayJson::Rounded( anPoint[ 0 ] ) )->add( RayJson::Rounded( anPoint[ 1 ] ) );
    }

    void Line( std::vector<Profile::SPiece> &aoPieces, const double a[ 2 ], const double b[ 2 ] )
    {
        Profile::SPiece oPiece;

        oPiece.nDegree = 1;
        for ( int c = 0; c < 2; c++ )
        {
            oPiece.p[ 0 ][ c ] = a[ c ];
            oPiece.p[ 1 ][ c ] = a[ c ] + ( ( b[ c ] - a[ c ] ) / 3.0 );
            oPiece.p[ 2 ][ c ] = a[ c ] + ( 2.0 * ( b[ c ] - a[ c ] ) / 3.0 );
            oPiece.p[ 3 ][ c ] = b[ c ];
        }
        aoPieces.push_back( oPiece );
    }

        // How far a piece's control points stray from the line between its
        // ends: the most the curve can.
    double Bulge( const Profile::SPiece &oPiece )
    {
        double dx = oPiece.p[ 3 ][ 0 ] - oPiece.p[ 0 ][ 0 ], dy = oPiece.p[ 3 ][ 1 ] - oPiece.p[ 0 ][ 1 ];
        double l = std::hypot( dx, dy ), nMost = 0.0;

        for ( int k = 1; k < 3; k++ )
        {
            double ex = oPiece.p[ k ][ 0 ] - oPiece.p[ 0 ][ 0 ], ey = oPiece.p[ k ][ 1 ] - oPiece.p[ 0 ][ 1 ];
            double d = ( l > 0.0 ) ? std::fabs( ( ex * dy ) - ( ey * dx ) ) / l : std::hypot( ex, ey );

            if ( d > nMost )
                nMost = d;
        }
        return nMost;
    }

    void Push( Outline::Polygon &oOut, double x, double y )
    {
        if ( ! oOut.empty() && oOut.back().x == x && oOut.back().y == y )
            return;

        Outline::SPoint p = { x, y };

        oOut.push_back( p );
    }
}

namespace Profile
{
    void MakePieces( SPath &oPath )
    {
        oPath.aoPieces.clear();
        if ( oPath.aoEntries.empty() )
            return;
        if ( oPath.bSmooth )
        {
                // Catmull-Rom through the points: a closed path wraps
                // around, an open one repeats its end points.
            size_t nN = oPath.aoEntries.size();
            size_t nSpans = oPath.bClosed ? nN : nN - 1;

            for ( size_t i = 0; i < nSpans; i++ )
            {
                auto P = [&]( long k ) -> const double *
                {
                    if ( oPath.bClosed )
                        k = ( ( k % (long)nN ) + (long)nN ) % (long)nN;
                    else if ( k < 0 )
                        k = 0;
                    else if ( k >= (long)nN )
                        k = (long)nN - 1;
                    return oPath.aoEntries[ (size_t)k ].anPoints[ 0 ];
                };
                const double *p0 = P( (long)i - 1 ), *p1 = P( (long)i ), *p2 = P( (long)i + 1 ), *p3 = P( (long)i + 2 );
                SPiece        oPiece;

                oPiece.nDegree = 3;
                for ( int c = 0; c < 2; c++ )
                {
                    oPiece.p[ 0 ][ c ] = p1[ c ];
                    oPiece.p[ 1 ][ c ] = p1[ c ] + ( ( p2[ c ] - p0[ c ] ) / 6.0 );
                    oPiece.p[ 2 ][ c ] = p2[ c ] - ( ( p3[ c ] - p1[ c ] ) / 6.0 );
                    oPiece.p[ 3 ][ c ] = p2[ c ];
                }
                oPath.aoPieces.push_back( oPiece );
            }
            return;
        }

        const double *pAt = oPath.aoEntries[ 0 ].anPoints[ 0 ];

        for ( size_t i = 1; i < oPath.aoEntries.size(); i++ )
        {
            const SEntry &oEntry = oPath.aoEntries[ i ];
            SPiece        oPiece;

            if ( oEntry.nKind == 1 )
                Line( oPath.aoPieces, pAt, oEntry.anPoints[ 0 ] );
            else
            {
                oPiece.nDegree = oEntry.nKind;
                for ( int c = 0; c < 2; c++ )
                {
                    oPiece.p[ 0 ][ c ] = pAt[ c ];
                    if ( oEntry.nKind == 2 )
                    {
                            // Raised to a cubic.
                        oPiece.p[ 1 ][ c ] = pAt[ c ] + ( 2.0 * ( oEntry.anPoints[ 0 ][ c ] - pAt[ c ] ) / 3.0 );
                        oPiece.p[ 2 ][ c ] = oEntry.anPoints[ 1 ][ c ] + ( 2.0 * ( oEntry.anPoints[ 0 ][ c ] - oEntry.anPoints[ 1 ][ c ] ) / 3.0 );
                        oPiece.p[ 3 ][ c ] = oEntry.anPoints[ 1 ][ c ];
                    }
                    else
                    {
                        oPiece.p[ 1 ][ c ] = oEntry.anPoints[ 0 ][ c ];
                        oPiece.p[ 2 ][ c ] = oEntry.anPoints[ 1 ][ c ];
                        oPiece.p[ 3 ][ c ] = oEntry.anPoints[ 2 ][ c ];
                    }
                }
                oPath.aoPieces.push_back( oPiece );
            }
            pAt = oEntry.anPoints[ oEntry.nKind - 1 ];
        }

        const double *pStart = oPath.aoEntries[ 0 ].anPoints[ 0 ];

        if ( oPath.bClosed && ( pAt[ 0 ] != pStart[ 0 ] || pAt[ 1 ] != pStart[ 1 ] ) )
            Line( oPath.aoPieces, pAt, pStart );
    }

    BOOL Read( const IASLib::CJsonNode &oNode, bool bClosed, bool bSmooth, int nMinimumPieces, SPath &oPath )
    {
        oPath = SPath();
        oPath.bClosed = bClosed;
        oPath.bSmooth = bSmooth;
        if ( ! oNode.isArray() || oNode.size() < 1 )
            return RayJson::Fail( "must be an array of points and curves" );
        for ( int n = 0; n < oNode.size(); n++ )
        {
            RayJson::CWhere          oWhere( "", n );
            const IASLib::CJsonNode *pEntry = oNode.get( n );
            SEntry                   oEntry;

            if ( ! pEntry )
                return RayJson::Fail( "is missing" );
            if ( pEntry->isArray() )
            {
                oEntry.nKind = 1;
                if ( ! ReadXY( pEntry, oEntry.anPoints[ 0 ] ) )
                    return FALSE;
            }
            else if ( pEntry->isObject() )
            {
                const IASLib::CJsonNode *pQuadratic = pEntry->get( "quadratic" );
                const IASLib::CJsonNode *pCubic = pEntry->get( "cubic" );
                const IASLib::CJsonNode *pPoints = pQuadratic ? pQuadratic : pCubic;
                int                      nCount = pQuadratic ? 2 : 3;

                if ( pEntry->size() != 1 || ! pPoints )
                    return RayJson::Fail( "a curve must be { \"quadratic\": [ control, end ] } or "
                                          "{ \"cubic\": [ control, control, end ] }" );
                if ( n == 0 )
                    return RayJson::Fail( "the path must start with a point" );
                if ( bSmooth )
                    return RayJson::Fail( "a smooth path takes only points" );
                if ( ! pPoints->isArray() || pPoints->size() != nCount )
                    return RayJson::Fail( pQuadratic ? "quadratic takes two points: [ control, end ]"
                                                     : "cubic takes three points: [ control, control, end ]" );
                oEntry.nKind = nCount;
                for ( int k = 0; k < nCount; k++ )
                {
                    if ( ! ReadXY( pPoints->get( k ), oEntry.anPoints[ k ] ) )
                        return FALSE;
                }
            }
            else
                return RayJson::Fail( "must be a point [x, y] or a curve" );
            oPath.aoEntries.push_back( oEntry );
        }
        MakePieces( oPath );
        if ( (int)oPath.aoPieces.size() < nMinimumPieces )
            return RayJson::Fail( bClosed ? "an outline needs at least three points" : "needs at least two points" );
        for ( const SPiece &oPiece : oPath.aoPieces )
        {
            if ( oPiece.p[ 0 ][ 0 ] == oPiece.p[ 3 ][ 0 ] && oPiece.p[ 0 ][ 1 ] == oPiece.p[ 3 ][ 1 ] &&
                 oPiece.nDegree == 1 )
                return RayJson::Fail( "has the same point twice in a row" );
        }
        return TRUE;
    }

    void Write( IASLib::CArrayNode &oArray, const SPath &oPath )
    {
        for ( const SEntry &oEntry : oPath.aoEntries )
        {
            if ( oEntry.nKind == 1 )
                AddXY( oArray, oEntry.anPoints[ 0 ] );
            else
            {
                IASLib::CObjectNode *pCurve = oArray.addObject();
                IASLib::CArrayNode  *pPoints = pCurve->putArray( ( oEntry.nKind == 2 ) ? "quadratic" : "cubic" );

                for ( int k = 0; k < oEntry.nKind; k++ )
                    AddXY( *pPoints, oEntry.anPoints[ k ] );
            }
        }
    }

    void Evaluate( const SPiece &oPiece, double s, double anPoint[ 2 ] )
    {
        double u = 1.0 - s;
        double b0 = u * u * u, b1 = 3.0 * u * u * s, b2 = 3.0 * u * s * s, b3 = s * s * s;

        for ( int c = 0; c < 2; c++ )
            anPoint[ c ] = ( b0 * oPiece.p[ 0 ][ c ] ) + ( b1 * oPiece.p[ 1 ][ c ] ) + ( b2 * oPiece.p[ 2 ][ c ] ) + ( b3 * oPiece.p[ 3 ][ c ] );
    }

    void Slope( const SPiece &oPiece, double s, double anSlope[ 2 ] )
    {
        double u = 1.0 - s;

        for ( int c = 0; c < 2; c++ )
            anSlope[ c ] = 3.0 * ( ( u * u * ( oPiece.p[ 1 ][ c ] - oPiece.p[ 0 ][ c ] ) ) +
                                   ( 2.0 * u * s * ( oPiece.p[ 2 ][ c ] - oPiece.p[ 1 ][ c ] ) ) +
                                   ( s * s * ( oPiece.p[ 3 ][ c ] - oPiece.p[ 2 ][ c ] ) ) );
    }

    void PowerBasis( const SPiece &oPiece, double anX[ 4 ], double anY[ 4 ] )
    {
        double *anOut[ 2 ] = { anX, anY };

        for ( int c = 0; c < 2; c++ )
        {
            double p0 = oPiece.p[ 0 ][ c ], p1 = oPiece.p[ 1 ][ c ], p2 = oPiece.p[ 2 ][ c ], p3 = oPiece.p[ 3 ][ c ];

            anOut[ c ][ 0 ] = p0;
            anOut[ c ][ 1 ] = 3.0 * ( p1 - p0 );
            anOut[ c ][ 2 ] = 3.0 * ( p0 - ( 2.0 * p1 ) + p2 );
            anOut[ c ][ 3 ] = p3 - p0 + ( 3.0 * ( p1 - p2 ) );
            if ( oPiece.nDegree == 1 )
            {
                    // A line is exactly linear: drop the rounding a raised
                    // line leaves in the higher terms.
                anOut[ c ][ 1 ] = p3 - p0;
                anOut[ c ][ 2 ] = 0.0;
                anOut[ c ][ 3 ] = 0.0;
            }
        }
    }

    Outline::Polygon Flatten( const SPath &oPath, double nTolerance )
    {
        Outline::Polygon oOut;

        for ( const SPiece &oPiece : oPath.aoPieces )
        {
            int nSteps = 1;

            if ( oPiece.nDegree > 1 )
            {
                    // A cubic's distance from its chords falls with the
                    // square of the number of steps.
                double nBulge = Bulge( oPiece );

                nSteps = (int)std::ceil( std::sqrt( nBulge / nTolerance ) * 0.75 );
                if ( nSteps < 2 )
                    nSteps = 2;
                if ( nSteps > 256 )
                    nSteps = 256;
            }
            for ( int k = 0; k < nSteps; k++ )
            {
                double p[ 2 ];

                Evaluate( oPiece, (double)k / nSteps, p );
                Push( oOut, p[ 0 ], p[ 1 ] );
            }
        }
        if ( ! oPath.aoPieces.empty() )
        {
            const SPiece &oLast = oPath.aoPieces.back();

            Push( oOut, oLast.p[ 3 ][ 0 ], oLast.p[ 3 ][ 1 ] );
        }
        if ( oPath.bClosed && oOut.size() > 1 && oOut.front().x == oOut.back().x && oOut.front().y == oOut.back().y )
            oOut.pop_back();
        return oOut;
    }

    double Extent( const SPath &oPath )
    {
        double anLow[ 2 ] = { 1e300, 1e300 }, anHigh[ 2 ] = { -1e300, -1e300 };

        for ( const SPiece &oPiece : oPath.aoPieces )
        {
            for ( int k = 0; k < 4; k++ )
            {
                for ( int c = 0; c < 2; c++ )
                {
                    anLow[ c ] = ( oPiece.p[ k ][ c ] < anLow[ c ] ) ? oPiece.p[ k ][ c ] : anLow[ c ];
                    anHigh[ c ] = ( oPiece.p[ k ][ c ] > anHigh[ c ] ) ? oPiece.p[ k ][ c ] : anHigh[ c ];
                }
            }
        }

        double nX = anHigh[ 0 ] - anLow[ 0 ], nY = anHigh[ 1 ] - anLow[ 1 ];

        return ( nX > nY ) ? nX : nY;
    }
}
