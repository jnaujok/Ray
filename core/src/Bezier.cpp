/*************************************************************************
*   CBezier                                                              *
*                                                                        *
*   A surface made of Bezier patches, traced exactly. See Bezier.h.      *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif
#include "Bezier.h"
#include "SceneJson.h"

#include <algorithm>
#include <math.h>
#include <sstream>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DYNAMIC_IMPLEMENTS( CBezier, "bezier" );

namespace
{
    const int s_nMaxDegree = 15;

        // The Bernstein polynomials of degree n at t, and their slopes.
    void Basis( int n, double t, double *anB, double *anDB )
    {
        double anLower[ s_nMaxDegree + 1 ];
        double s = 1.0 - t;

            // Built up degree by degree; anLower ends as degree n - 1.
        anLower[ 0 ] = 1.0;
        for ( int nDegree = 1; nDegree < n; nDegree++ )
        {
            double nCarry = 0.0;

            for ( int i = 0; i < nDegree; i++ )
            {
                double nOld = anLower[ i ];

                anLower[ i ] = nCarry + ( s * nOld );
                nCarry = t * nOld;
            }
            anLower[ nDegree ] = nCarry;
        }
        if ( n == 0 )
        {
            anB[ 0 ] = 1.0;
            anDB[ 0 ] = 0.0;
            return;
        }
        for ( int i = 0; i <= n; i++ )
        {
            double nLeft = ( i > 0 ) ? anLower[ i - 1 ] : 0.0;
            double nRight = ( i < n ) ? anLower[ i ] : 0.0;

            anB[ i ] = ( t * nLeft ) + ( s * nRight );
            anDB[ i ] = n * ( nLeft - nRight );
        }
    }

    inline double Dot3( const double a[ 3 ], const double b[ 3 ] )
    {
        return ( a[ 0 ] * b[ 0 ] ) + ( a[ 1 ] * b[ 1 ] ) + ( a[ 2 ] * b[ 2 ] );
    }

    inline void Cross3( const double a[ 3 ], const double b[ 3 ], double c[ 3 ] )
    {
        c[ 0 ] = ( a[ 1 ] * b[ 2 ] ) - ( a[ 2 ] * b[ 1 ] );
        c[ 1 ] = ( a[ 2 ] * b[ 0 ] ) - ( a[ 0 ] * b[ 2 ] );
        c[ 2 ] = ( a[ 0 ] * b[ 1 ] ) - ( a[ 1 ] * b[ 0 ] );
    }

        // A control point of a net (n + 1 by m + 1, homogeneous) divided out.
    inline void PointOf( const std::vector<double> &anNet, int nIndex, double anOut[ 3 ] )
    {
        const double *p = &anNet[ (size_t)nIndex * 4 ];

        anOut[ 0 ] = p[ 0 ] / p[ 3 ];
        anOut[ 1 ] = p[ 1 ] / p[ 3 ];
        anOut[ 2 ] = p[ 2 ] / p[ 3 ];
    }

        // Splits the curve of n + 1 homogeneous points (nStride numbers
        // apart) at the middle, by de Casteljau's algorithm.
    void SplitCurve( const double *anIn, int n, int nStride, double *anLeft, double *anRight )
    {
        double anWork[ ( s_nMaxDegree + 1 ) * 4 ];

        for ( int i = 0; i <= n; i++ )
        {
            for ( int c = 0; c < 4; c++ )
                anWork[ i * 4 + c ] = anIn[ i * nStride + c ];
        }
        for ( int c = 0; c < 4; c++ )
        {
            anLeft[ c ] = anWork[ c ];
            anRight[ n * nStride + c ] = anWork[ n * 4 + c ];
        }
        for ( int nLevel = 1; nLevel <= n; nLevel++ )
        {
            for ( int i = 0; i <= n - nLevel; i++ )
            {
                for ( int c = 0; c < 4; c++ )
                    anWork[ i * 4 + c ] = 0.5 * ( anWork[ i * 4 + c ] + anWork[ ( i + 1 ) * 4 + c ] );
            }
            for ( int c = 0; c < 4; c++ )
            {
                anLeft[ nLevel * nStride + c ] = anWork[ c ];
                anRight[ ( n - nLevel ) * nStride + c ] = anWork[ ( n - nLevel ) * 4 + c ];
            }
        }
    }

        // A ray against the slab box: the stretch of t inside it.
    inline BOOL BoxStretch( const double anLow[ 3 ], const double anHigh[ 3 ], const double anOrigin[ 3 ], const double anInverse[ 3 ],
                            double &nIn, double &nOut )
    {
        for ( int nAxis = 0; nAxis < 3; nAxis++ )
        {
            double a = ( anLow[ nAxis ] - anOrigin[ nAxis ] ) * anInverse[ nAxis ];
            double b = ( anHigh[ nAxis ] - anOrigin[ nAxis ] ) * anInverse[ nAxis ];

            if ( a > b )
                std::swap( a, b );
            if ( a != a || b != b )
            {
                    // A direction along the slab: inside only if the origin is.
                if ( anOrigin[ nAxis ] < anLow[ nAxis ] || anOrigin[ nAxis ] > anHigh[ nAxis ] )
                    return FALSE;
                continue;
            }
            nIn = ( a > nIn ) ? a : nIn;
            nOut = ( b < nOut ) ? b : nOut;
            if ( nIn > nOut )
                return FALSE;
        }
        return TRUE;
    }

        // Solves the 3x3 system with columns a, b, c for x; FALSE if it is
        // singular.
    BOOL Solve3( const double a[ 3 ], const double b[ 3 ], const double c[ 3 ], const double r[ 3 ], double x[ 3 ] )
    {
        double anBC[ 3 ], anCA[ 3 ], anAB[ 3 ];

        Cross3( b, c, anBC );
        double nDet = Dot3( a, anBC );
        double nScale = sqrt( Dot3( a, a ) * Dot3( b, b ) * Dot3( c, c ) );

        if ( ! ( ABS( nDet ) > 1e-14 * nScale ) )
            return FALSE;
        Cross3( c, a, anCA );
        Cross3( a, b, anAB );
        x[ 0 ] = Dot3( r, anBC ) / nDet;
        x[ 1 ] = Dot3( r, anCA ) / nDet;
        x[ 2 ] = Dot3( r, anAB ) / nDet;
        return TRUE;
    }

        // Reads a point as [x, y, z] or [x, y, z, w] into four homogeneous
        // numbers.
    BOOL ReadPoint( const IASLib::CJsonNode &oNode, double anOut[ 4 ] )
    {
        double anIn[ 4 ] = { 0.0, 0.0, 0.0, 1.0 };

        if ( ! oNode.isArray() || ( oNode.size() != 3 && oNode.size() != 4 ) )
            return RayJson::Fail( "must be [x, y, z] or [x, y, z, weight]" );
        for ( int c = 0; c < oNode.size(); c++ )
        {
            const IASLib::CJsonNode *pValue = oNode.get( c );

            if ( ! pValue || ! pValue->isNumber() )
                return RayJson::Fail( "must be [x, y, z] or [x, y, z, weight]" );
            anIn[ c ] = pValue->doubleValue();
        }
        if ( ! ( anIn[ 3 ] > 0.0 ) )
            return RayJson::Fail( "a weight must be more than 0" );
        anOut[ 0 ] = anIn[ 0 ] * anIn[ 3 ];
        anOut[ 1 ] = anIn[ 1 ] * anIn[ 3 ];
        anOut[ 2 ] = anIn[ 2 ] * anIn[ 3 ];
        anOut[ 3 ] = anIn[ 3 ];
        return TRUE;
    }

    void WritePoint( IASLib::CArrayNode &oArray, const double anPoint[ 4 ] )
    {
        IASLib::CArrayNode *pPoint = oArray.addArray();

        pPoint->add( RayJson::Rounded( anPoint[ 0 ] / anPoint[ 3 ] ) )
              ->add( RayJson::Rounded( anPoint[ 1 ] / anPoint[ 3 ] ) )
              ->add( RayJson::Rounded( anPoint[ 2 ] / anPoint[ 3 ] ) );
        if ( anPoint[ 3 ] != 1.0 )
            pPoint->add( RayJson::Rounded( anPoint[ 3 ] ) );
    }
}

CBezier::CBezier( void ) :
    m_bShared( FALSE ),
    m_oScale( 1.0, 1.0, 1.0 ),
    m_bClosed( FALSE ),
    m_nSize( 1.0 ),
    m_nRecent( 0 ),
    m_bAnyHit( FALSE )
{
    for ( int i = 0; i < 3; i++ )
    {
        m_anLow[ i ] = -1.0;
        m_anHigh[ i ] = 1.0;
    }
    for ( int i = 0; i < 4; i++ )
        m_aoRecent[ i ].nPatch = -1;
}

CBezier::~CBezier( void )
{
}

void CBezier::Evaluate( const SPatch &oPatch, double u, double v, double anPoint[ 3 ], double anDu[ 3 ], double anDv[ 3 ] )
{
    double anBU[ s_nMaxDegree + 1 ], anDBU[ s_nMaxDegree + 1 ];
    double anBV[ s_nMaxDegree + 1 ], anDBV[ s_nMaxDegree + 1 ];
    double anH[ 4 ] = { 0.0, 0.0, 0.0, 0.0 };
    double anHU[ 4 ] = { 0.0, 0.0, 0.0, 0.0 };
    double anHV[ 4 ] = { 0.0, 0.0, 0.0, 0.0 };
    int    nRow = oPatch.nDegreeU + 1;

    Basis( oPatch.nDegreeU, u, anBU, anDBU );
    Basis( oPatch.nDegreeV, v, anBV, anDBV );
    for ( int j = 0; j <= oPatch.nDegreeV; j++ )
    {
        for ( int i = 0; i < nRow; i++ )
        {
            const double *p = &oPatch.anPoints[ (size_t)( ( j * nRow ) + i ) * 4 ];
            double        nB = anBU[ i ] * anBV[ j ];
            double        nBU = anDBU[ i ] * anBV[ j ];
            double        nBV = anBU[ i ] * anDBV[ j ];

            for ( int c = 0; c < 4; c++ )
            {
                anH[ c ] += nB * p[ c ];
                anHU[ c ] += nBU * p[ c ];
                anHV[ c ] += nBV * p[ c ];
            }
        }
    }
        // Divided out: S = H / w, S' = (H' - S w') / w.
    for ( int c = 0; c < 3; c++ )
    {
        anPoint[ c ] = anH[ c ] / anH[ 3 ];
        anDu[ c ] = ( anHU[ c ] - ( anPoint[ c ] * anHU[ 3 ] ) ) / anH[ 3 ];
        anDv[ c ] = ( anHV[ c ] - ( anPoint[ c ] * anHV[ 3 ] ) ) / anH[ 3 ];
    }
}

void CBezier::Normal( const SPatch &oPatch, double u, double v, double anNormal[ 3 ] )
{
    double anPoint[ 3 ], anDu[ 3 ], anDv[ 3 ];
    double nStep = 0.0;

    for ( int nTry = 0; nTry < 8; nTry++ )
    {
        double uu = u + ( ( 0.5 - u ) * nStep );
        double vv = v + ( ( 0.5 - v ) * nStep );

        Evaluate( oPatch, uu, vv, anPoint, anDu, anDv );
        Cross3( anDu, anDv, anNormal );

        double nLength = sqrt( Dot3( anNormal, anNormal ) );
        double nScale = sqrt( Dot3( anDu, anDu ) * Dot3( anDv, anDv ) );

            // Where an edge shrinks to a point the slope along it vanishes:
            // look a little further inside the patch each time.
        if ( nLength > 1e-10 * nScale && nLength > 0.0 )
        {
            anNormal[ 0 ] /= nLength;
            anNormal[ 1 ] /= nLength;
            anNormal[ 2 ] /= nLength;
            return;
        }
        nStep = ( nStep == 0.0 ) ? 1e-6 : nStep * 10.0;
    }
    anNormal[ 0 ] = 0.0;
    anNormal[ 1 ] = 1.0;
    anNormal[ 2 ] = 0.0;
}

BOOL CBezier::CheckPatch( const SPatch &oPatch, std::string &strError )
{
    if ( oPatch.nDegreeU < 1 || oPatch.nDegreeU > s_nMaxDegree || oPatch.nDegreeV < 1 || oPatch.nDegreeV > s_nMaxDegree )
    {
        strError = "degrees must be whole numbers from 1 to " + std::to_string( s_nMaxDegree );
        return FALSE;
    }
    size_t nCount = (size_t)( oPatch.nDegreeU + 1 ) * (size_t)( oPatch.nDegreeV + 1 );

    if ( oPatch.anPoints.size() != nCount * 4 )
    {
        strError = "a patch of degree " + std::to_string( oPatch.nDegreeU ) + " by " + std::to_string( oPatch.nDegreeV ) + " needs " +
                   std::to_string( (long)nCount ) + " points";
        return FALSE;
    }
    for ( size_t n = 0; n < nCount; n++ )
    {
        if ( ! ( oPatch.anPoints[ n * 4 + 3 ] > 0.0 ) )
        {
            strError = "a weight must be more than 0";
            return FALSE;
        }
        for ( int c = 0; c < 4; c++ )
        {
            double x = oPatch.anPoints[ n * 4 + c ];

            if ( x != x || x - x != 0.0 )
            {
                strError = "points must be finite numbers";
                return FALSE;
            }
        }
    }
    return TRUE;
}

BOOL CBezier::ReadBpt( const std::string &strText, std::vector<SPatch> &aoPatches, std::string &strError )
{
    std::istringstream oIn( strText );
    long               nPatches = 0;

    aoPatches.clear();
    if ( ! ( oIn >> nPatches ) || nPatches < 1 || nPatches > 1000000 )
    {
        strError = "doesn't start with a number of patches";
        return FALSE;
    }
    for ( long nPatch = 0; nPatch < nPatches; nPatch++ )
    {
        SPatch oPatch;

        if ( ! ( oIn >> oPatch.nDegreeU >> oPatch.nDegreeV ) )
        {
            strError = "ends before patch " + std::to_string( nPatch + 1 );
            return FALSE;
        }
        if ( oPatch.nDegreeU < 1 || oPatch.nDegreeU > s_nMaxDegree || oPatch.nDegreeV < 1 || oPatch.nDegreeV > s_nMaxDegree )
        {
            strError = "patch " + std::to_string( nPatch + 1 ) + " has degrees outside 1 to " + std::to_string( s_nMaxDegree );
            return FALSE;
        }
        size_t nCount = (size_t)( oPatch.nDegreeU + 1 ) * (size_t)( oPatch.nDegreeV + 1 );

        oPatch.anPoints.resize( nCount * 4 );
        for ( size_t n = 0; n < nCount; n++ )
        {
            if ( ! ( oIn >> oPatch.anPoints[ n * 4 ] >> oPatch.anPoints[ n * 4 + 1 ] >> oPatch.anPoints[ n * 4 + 2 ] ) )
            {
                strError = "ends inside patch " + std::to_string( nPatch + 1 );
                return FALSE;
            }
            oPatch.anPoints[ n * 4 + 3 ] = 1.0;
        }
        if ( ! CheckPatch( oPatch, strError ) )
        {
            strError = "patch " + std::to_string( nPatch + 1 ) + ": " + strError;
            return FALSE;
        }
        aoPatches.push_back( oPatch );
    }
    std::string strRest;

    if ( oIn >> strRest )
    {
        strError = "has more after its last patch";
        return FALSE;
    }
    return TRUE;
}

BOOL CBezier::SetPatches( const std::vector<SPatch> &aoPatches )
{
    std::string strError;

    if ( aoPatches.empty() )
        return RayJson::Fail( "needs at least one patch" );
    for ( size_t n = 0; n < aoPatches.size(); n++ )
    {
        if ( ! CheckPatch( aoPatches[ n ], strError ) )
            return RayJson::Fail( "patch " + std::to_string( (long)n ) + ": " + strError );
    }
    m_aoPatches = aoPatches;
    m_strFile.clear();
    m_pFileData.reset();
    m_bShared = FALSE;
    m_anShared.clear();
    for ( size_t n = 0; n < m_aoPatches.size(); n++ )
        m_aoPatches[ n ].anIndices.clear();
    return Prepare();
}

BOOL CBezier::PrepareShape( void )
{
    if ( m_aoPatches.empty() )
        return RayJson::Fail( "needs at least one patch" );
    if ( ! ( m_oScale.GetX() > 0.0 && m_oScale.GetY() > 0.0 && m_oScale.GetZ() > 0.0 ) )
        return RayJson::Fail( "scale must be more than 0" );

        // The patches as traced: scaled in their own frame.
    double anScale[ 3 ] = { m_oScale.GetX(), m_oScale.GetY(), m_oScale.GetZ() };

    m_aoScaled = m_aoPatches;
    for ( size_t nPatch = 0; nPatch < m_aoScaled.size(); nPatch++ )
    {
        std::vector<double> &anPoints = m_aoScaled[ nPatch ].anPoints;

        for ( size_t n = 0; n + 3 < anPoints.size(); n += 4 )
        {
            for ( int c = 0; c < 3; c++ )
                anPoints[ n + c ] *= anScale[ c ];
        }
    }

        // The box of every control point (each patch lies inside its own).
    for ( int c = 0; c < 3; c++ )
    {
        m_anLow[ c ] = 1e300;
        m_anHigh[ c ] = -1e300;
    }
    for ( size_t nPatch = 0; nPatch < m_aoScaled.size(); nPatch++ )
    {
        const std::vector<double> &anPoints = m_aoScaled[ nPatch ].anPoints;

        for ( size_t n = 0; n * 4 < anPoints.size(); n++ )
        {
            double anPoint[ 3 ];

            PointOf( anPoints, (int)n, anPoint );
            for ( int c = 0; c < 3; c++ )
            {
                m_anLow[ c ] = ( anPoint[ c ] < m_anLow[ c ] ) ? anPoint[ c ] : m_anLow[ c ];
                m_anHigh[ c ] = ( anPoint[ c ] > m_anHigh[ c ] ) ? anPoint[ c ] : m_anHigh[ c ];
            }
        }
    }
    m_nSize = ( m_anHigh[ 0 ] - m_anLow[ 0 ] ) + ( m_anHigh[ 1 ] - m_anLow[ 1 ] ) + ( m_anHigh[ 2 ] - m_anLow[ 2 ] );
    if ( ! ( m_nSize > 0.0 ) )
        return RayJson::Fail( "the patches have no size" );

        // Each patch split until every piece is within a small fraction of
        // the whole of flat, so the triangles of its corners start Newton's
        // method close to the surface.
    double nFlat = 2e-4 * m_nSize;

    m_aoPieces.clear();
    for ( size_t nPatch = 0; nPatch < m_aoScaled.size(); nPatch++ )
    {
        struct SWork
        {
            std::vector<double> anNet;
            double              anRange[ 4 ];
            int                 nDepth;
        };
        const SPatch      &oPatch = m_aoScaled[ nPatch ];
        int                n = oPatch.nDegreeU, m = oPatch.nDegreeV;
        std::vector<SWork> aoStack;
        SWork              oFirst;

        oFirst.anNet = oPatch.anPoints;
        oFirst.anRange[ 0 ] = 0.0;
        oFirst.anRange[ 1 ] = 0.0;
        oFirst.anRange[ 2 ] = 1.0;
        oFirst.anRange[ 3 ] = 1.0;
        oFirst.nDepth = 0;
        aoStack.push_back( oFirst );
        while ( ! aoStack.empty() )
        {
            SWork oWork = aoStack.back();

            aoStack.pop_back();

            double anC[ 4 ][ 3 ];
            int    nRow = n + 1;

            PointOf( oWork.anNet, 0, anC[ 0 ] );
            PointOf( oWork.anNet, n, anC[ 1 ] );
            PointOf( oWork.anNet, ( m * nRow ) + n, anC[ 2 ] );
            PointOf( oWork.anNet, m * nRow, anC[ 3 ] );

                // How far the net strays from the bilinear sheet of its
                // corners, and how long it is each way.
            double nError = 0.0, nLengthU = 0.0, nLengthV = 0.0;

            for ( int j = 0; j <= m; j++ )
            {
                for ( int i = 0; i <= n; i++ )
                {
                    double s = (double)i / n, t = (double)j / m;
                    double anPoint[ 3 ], nDistance = 0.0;

                    PointOf( oWork.anNet, ( j * nRow ) + i, anPoint );
                    for ( int c = 0; c < 3; c++ )
                    {
                        double nSheet = ( ( 1.0 - s ) * ( 1.0 - t ) * anC[ 0 ][ c ] ) + ( s * ( 1.0 - t ) * anC[ 1 ][ c ] ) +
                                        ( s * t * anC[ 2 ][ c ] ) + ( ( 1.0 - s ) * t * anC[ 3 ][ c ] );

                        nDistance += ( anPoint[ c ] - nSheet ) * ( anPoint[ c ] - nSheet );
                    }
                    nError = ( nDistance > nError ) ? nDistance : nError;
                    if ( i > 0 )
                    {
                        double anPrevious[ 3 ];

                        PointOf( oWork.anNet, ( j * nRow ) + i - 1, anPrevious );
                        nLengthU += sqrt( ( anPoint[ 0 ] - anPrevious[ 0 ] ) * ( anPoint[ 0 ] - anPrevious[ 0 ] ) +
                                          ( anPoint[ 1 ] - anPrevious[ 1 ] ) * ( anPoint[ 1 ] - anPrevious[ 1 ] ) +
                                          ( anPoint[ 2 ] - anPrevious[ 2 ] ) * ( anPoint[ 2 ] - anPrevious[ 2 ] ) );
                    }
                    if ( j > 0 )
                    {
                        double anPrevious[ 3 ];

                        PointOf( oWork.anNet, ( ( j - 1 ) * nRow ) + i, anPrevious );
                        nLengthV += sqrt( ( anPoint[ 0 ] - anPrevious[ 0 ] ) * ( anPoint[ 0 ] - anPrevious[ 0 ] ) +
                                          ( anPoint[ 1 ] - anPrevious[ 1 ] ) * ( anPoint[ 1 ] - anPrevious[ 1 ] ) +
                                          ( anPoint[ 2 ] - anPrevious[ 2 ] ) * ( anPoint[ 2 ] - anPrevious[ 2 ] ) );
                    }
                }
            }
            if ( sqrt( nError ) <= nFlat || oWork.nDepth >= 14 )
            {
                SPiece oPiece;

                oPiece.nPatch = (int)nPatch;
                for ( int k = 0; k < 4; k++ )
                    oPiece.anRange[ k ] = oWork.anRange[ k ];
                for ( int k = 0; k < 4; k++ )
                {
                    for ( int c = 0; c < 3; c++ )
                        oPiece.anCorners[ k ][ c ] = anC[ k ][ c ];
                }
                for ( int c = 0; c < 3; c++ )
                {
                    oPiece.anLow[ c ] = 1e300;
                    oPiece.anHigh[ c ] = -1e300;
                }
                for ( int k = 0; k < ( n + 1 ) * ( m + 1 ); k++ )
                {
                    double anPoint[ 3 ];

                    PointOf( oWork.anNet, k, anPoint );
                    for ( int c = 0; c < 3; c++ )
                    {
                        oPiece.anLow[ c ] = ( anPoint[ c ] < oPiece.anLow[ c ] ) ? anPoint[ c ] : oPiece.anLow[ c ];
                        oPiece.anHigh[ c ] = ( anPoint[ c ] > oPiece.anHigh[ c ] ) ? anPoint[ c ] : oPiece.anHigh[ c ];
                    }
                }
                    // A little room, so a hit found by rounding just outside
                    // the hull is still inside the box.
                for ( int c = 0; c < 3; c++ )
                {
                    oPiece.anLow[ c ] -= 1e-7 * m_nSize;
                    oPiece.anHigh[ c ] += 1e-7 * m_nSize;
                }
                m_aoPieces.push_back( oPiece );
                continue;
            }

                // Split the longer way, by de Casteljau's algorithm.
            SWork oA, oB;

            oA.anNet.resize( oWork.anNet.size() );
            oB.anNet.resize( oWork.anNet.size() );
            oA.nDepth = oB.nDepth = oWork.nDepth + 1;
            for ( int k = 0; k < 4; k++ )
                oA.anRange[ k ] = oB.anRange[ k ] = oWork.anRange[ k ];
            if ( nLengthU * m >= nLengthV * n )
            {
                for ( int j = 0; j <= m; j++ )
                    SplitCurve( &oWork.anNet[ (size_t)( j * nRow ) * 4 ], n, 4, &oA.anNet[ (size_t)( j * nRow ) * 4 ],
                                &oB.anNet[ (size_t)( j * nRow ) * 4 ] );
                oA.anRange[ 2 ] = oB.anRange[ 0 ] = 0.5 * ( oWork.anRange[ 0 ] + oWork.anRange[ 2 ] );
            }
            else
            {
                for ( int i = 0; i <= n; i++ )
                    SplitCurve( &oWork.anNet[ (size_t)i * 4 ], m, nRow * 4, &oA.anNet[ (size_t)i * 4 ], &oB.anNet[ (size_t)i * 4 ] );
                oA.anRange[ 3 ] = oB.anRange[ 1 ] = 0.5 * ( oWork.anRange[ 1 ] + oWork.anRange[ 3 ] );
            }
            aoStack.push_back( oB );
            aoStack.push_back( oA );
        }
    }
    BuildTree();
    m_nRecent = 0;
    for ( int i = 0; i < 4; i++ )
        m_aoRecent[ i ].nPatch = -1;
    return TRUE;
}

void CBezier::BuildTree( void )
{
    m_aoNodes.clear();
    m_anOrder.resize( m_aoPieces.size() );
    for ( size_t n = 0; n < m_aoPieces.size(); n++ )
        m_anOrder[ n ] = (int)n;
    m_aoNodes.reserve( m_aoPieces.size() * 2 );
    if ( ! m_aoPieces.empty() )
        BuildNode( 0, (int)m_aoPieces.size() );
}

int CBezier::BuildNode( int nFirst, int nCount )
{
    SNode oNode;
    int   nIndex = (int)m_aoNodes.size();

    for ( int c = 0; c < 3; c++ )
    {
        oNode.anLow[ c ] = 1e300;
        oNode.anHigh[ c ] = -1e300;
    }
    for ( int n = nFirst; n < nFirst + nCount; n++ )
    {
        const SPiece &oPiece = m_aoPieces[ (size_t)m_anOrder[ (size_t)n ] ];

        for ( int c = 0; c < 3; c++ )
        {
            oNode.anLow[ c ] = ( oPiece.anLow[ c ] < oNode.anLow[ c ] ) ? oPiece.anLow[ c ] : oNode.anLow[ c ];
            oNode.anHigh[ c ] = ( oPiece.anHigh[ c ] > oNode.anHigh[ c ] ) ? oPiece.anHigh[ c ] : oNode.anHigh[ c ];
        }
    }
    oNode.nLeft = oNode.nRight = -1;
    oNode.nFirst = nFirst;
    oNode.nCount = nCount;
    m_aoNodes.push_back( oNode );
    if ( nCount <= 4 )
        return nIndex;

        // Split at the median of the pieces' middles along the widest axis.
    int nAxis = 0;

    for ( int c = 1; c < 3; c++ )
    {
        if ( ( oNode.anHigh[ c ] - oNode.anLow[ c ] ) > ( oNode.anHigh[ nAxis ] - oNode.anLow[ nAxis ] ) )
            nAxis = c;
    }
    std::vector<int>::iterator itFirst = m_anOrder.begin() + nFirst;

    std::nth_element( itFirst, itFirst + ( nCount / 2 ), itFirst + nCount, [this, nAxis]( int a, int b )
    {
        const SPiece &oA = m_aoPieces[ (size_t)a ];
        const SPiece &oB = m_aoPieces[ (size_t)b ];
        double        nA = oA.anLow[ nAxis ] + oA.anHigh[ nAxis ];
        double        nB = oB.anLow[ nAxis ] + oB.anHigh[ nAxis ];

        return ( nA < nB ) || ( nA == nB && a < b );
    } );

    int nLeft = BuildNode( nFirst, nCount / 2 );
    int nRight = BuildNode( nFirst + ( nCount / 2 ), nCount - ( nCount / 2 ) );

    m_aoNodes[ (size_t)nIndex ].nLeft = nLeft;
    m_aoNodes[ (size_t)nIndex ].nRight = nRight;
    return nIndex;
}

BOOL CBezier::PieceHit( const SPiece &oPiece, const double anOrigin[ 3 ], const double anDirection[ 3 ], double nLow, double nHigh,
                        double &nT, double &u, double &v )
{
    const SPatch &oPatch = m_aoScaled[ (size_t)oPiece.nPatch ];
    double        anStarts[ 2 ][ 3 ];
    int           nStarts = 0;

        // A start from the triangles of the piece's corners: (u0,v0), (u1,v0),
        // (u1,v1) and (u0,v0), (u1,v1), (u0,v1). A near miss still starts it.
    static const int s_anTriangles[ 2 ][ 3 ] = { { 0, 1, 2 }, { 0, 2, 3 } };
    static const double s_anU[ 4 ] = { 0.0, 1.0, 1.0, 0.0 };
    static const double s_anV[ 4 ] = { 0.0, 0.0, 1.0, 1.0 };

    for ( int nTriangle = 0; nTriangle < 2 && nStarts == 0; nTriangle++ )
    {
        const int *anK = s_anTriangles[ nTriangle ];
        double     e1[ 3 ], e2[ 3 ], p[ 3 ], q[ 3 ], s[ 3 ];

        for ( int c = 0; c < 3; c++ )
        {
            e1[ c ] = oPiece.anCorners[ anK[ 1 ] ][ c ] - oPiece.anCorners[ anK[ 0 ] ][ c ];
            e2[ c ] = oPiece.anCorners[ anK[ 2 ] ][ c ] - oPiece.anCorners[ anK[ 0 ] ][ c ];
            s[ c ] = anOrigin[ c ] - oPiece.anCorners[ anK[ 0 ] ][ c ];
        }
        Cross3( anDirection, e2, p );

        double nDet = Dot3( e1, p );

        if ( ! ( ABS( nDet ) > 1e-30 ) )
            continue;

        double b = Dot3( s, p ) / nDet;

        Cross3( s, e1, q );

        double c = Dot3( anDirection, q ) / nDet;
        double a = 1.0 - b - c;

        if ( a < -0.25 || b < -0.25 || c < -0.25 )
            continue;
            // Barycentric weights to the corners' (u, v), then to the patch.
        double su = ( a * s_anU[ anK[ 0 ] ] ) + ( b * s_anU[ anK[ 1 ] ] ) + ( c * s_anU[ anK[ 2 ] ] );
        double sv = ( a * s_anV[ anK[ 0 ] ] ) + ( b * s_anV[ anK[ 1 ] ] ) + ( c * s_anV[ anK[ 2 ] ] );

        anStarts[ nStarts ][ 0 ] = oPiece.anRange[ 0 ] + ( su * ( oPiece.anRange[ 2 ] - oPiece.anRange[ 0 ] ) );
        anStarts[ nStarts ][ 1 ] = oPiece.anRange[ 1 ] + ( sv * ( oPiece.anRange[ 3 ] - oPiece.anRange[ 1 ] ) );
        anStarts[ nStarts ][ 2 ] = Dot3( e2, q ) / nDet;
        nStarts++;
    }
        // A ray that doesn't pass near the corners' triangles misses the
        // piece: it is within a small fraction of flat. Otherwise the
        // piece's middle is a second start, if the first doesn't converge.
    if ( nStarts == 0 )
        return FALSE;
    {
        double anMiddle[ 3 ];

        for ( int c = 0; c < 3; c++ )
            anMiddle[ c ] = 0.25 * ( oPiece.anCorners[ 0 ][ c ] + oPiece.anCorners[ 1 ][ c ] + oPiece.anCorners[ 2 ][ c ] +
                                     oPiece.anCorners[ 3 ][ c ] ) - anOrigin[ c ];
        anStarts[ nStarts ][ 0 ] = 0.5 * ( oPiece.anRange[ 0 ] + oPiece.anRange[ 2 ] );
        anStarts[ nStarts ][ 1 ] = 0.5 * ( oPiece.anRange[ 1 ] + oPiece.anRange[ 3 ] );
        anStarts[ nStarts ][ 2 ] = Dot3( anMiddle, anDirection );
        nStarts++;
    }

    double nTolerance = 1e-11 * ( m_nSize + ABS( anOrigin[ 0 ] ) + ABS( anOrigin[ 1 ] ) + ABS( anOrigin[ 2 ] ) );

    for ( int nStart = 0; nStart < nStarts; nStart++ )
    {
        double uu = anStarts[ nStart ][ 0 ], vv = anStarts[ nStart ][ 1 ], tt = anStarts[ nStart ][ 2 ];
        BOOL   bConverged = FALSE;

        double nLast = 1e300;
        int    nWorse = 0;

        for ( int nStep = 0; nStep < 16; nStep++ )
        {
            double anPoint[ 3 ], anDu[ 3 ], anDv[ 3 ], anF[ 3 ], anBack[ 3 ], anX[ 3 ];

            Evaluate( oPatch, uu, vv, anPoint, anDu, anDv );
            for ( int c = 0; c < 3; c++ )
            {
                anF[ c ] = anPoint[ c ] - ( anOrigin[ c ] + ( tt * anDirection[ c ] ) );
                anBack[ c ] = -anDirection[ c ];
                anF[ c ] = -anF[ c ];
            }
            double nResidual = sqrt( Dot3( anF, anF ) );

            if ( nResidual <= nTolerance )
            {
                bConverged = TRUE;
                break;
            }
                // Moving away from the surface three steps running: no root
                // near this start.
            nWorse = ( nResidual >= nLast ) ? nWorse + 1 : 0;
            if ( nWorse >= 3 )
                break;
            nLast = nResidual;
            if ( ! Solve3( anDu, anDv, anBack, anF, anX ) )
                break;
            uu += anX[ 0 ];
            vv += anX[ 1 ];
            tt += anX[ 2 ];
                // Kept near the patch, so a wild step can come back.
            uu = ( uu < -0.5 ) ? -0.5 : ( uu > 1.5 ) ? 1.5 : uu;
            vv = ( vv < -0.5 ) ? -0.5 : ( vv > 1.5 ) ? 1.5 : vv;
        }
        if ( ! bConverged )
            continue;
            // Anywhere on the patch counts; past its edges is a surface
            // the patch doesn't have.
        if ( uu < -1e-9 || uu > 1.0 + 1e-9 || vv < -1e-9 || vv > 1.0 + 1e-9 )
            continue;
        if ( ! ( tt > nLow && tt < nHigh ) )
            continue;
        nT = tt;
        u = ( uu < 0.0 ) ? 0.0 : ( uu > 1.0 ) ? 1.0 : uu;
        v = ( vv < 0.0 ) ? 0.0 : ( vv > 1.0 ) ? 1.0 : vv;
        return TRUE;
    }
    return FALSE;
}

BOOL CBezier::LocalHit( const FLT anOriginIn[ 3 ], const FLT anDirectionIn[ 3 ], FLT nLow, FLT nHigh, FLT &nT )
{
    double anOrigin[ 3 ], anDirection[ 3 ], anInverse[ 3 ];
    double nBest = nHigh;
    int    nBestPatch = -1;
    double nBestU = 0.0, nBestV = 0.0;

    if ( m_aoNodes.empty() )
        return FALSE;
    for ( int c = 0; c < 3; c++ )
    {
        anOrigin[ c ] = anOriginIn[ c ];
        anDirection[ c ] = anDirectionIn[ c ];
        anInverse[ c ] = 1.0 / anDirection[ c ];
    }

    int anStack[ 128 ];
    int nTop = 0;

    anStack[ nTop++ ] = 0;
    while ( nTop > 0 )
    {
        const SNode &oNode = m_aoNodes[ (size_t)anStack[ --nTop ] ];
        double       nIn = nLow, nOut = nBest;

        if ( ! BoxStretch( oNode.anLow, oNode.anHigh, anOrigin, anInverse, nIn, nOut ) )
            continue;
        if ( oNode.nLeft >= 0 )
        {
                // The nearer child first, so a hit there cuts the farther
                // one short.
            const SNode &oLeft = m_aoNodes[ (size_t)oNode.nLeft ];
            const SNode &oRight = m_aoNodes[ (size_t)oNode.nRight ];
            double       nLeftIn = nLow, nLeftOut = nBest, nRightIn = nLow, nRightOut = nBest;
            BOOL         bLeft = BoxStretch( oLeft.anLow, oLeft.anHigh, anOrigin, anInverse, nLeftIn, nLeftOut );
            BOOL         bRight = BoxStretch( oRight.anLow, oRight.anHigh, anOrigin, anInverse, nRightIn, nRightOut );

            if ( nTop + 2 > 128 )
                continue;
            if ( bLeft && bRight )
            {
                anStack[ nTop++ ] = ( nLeftIn <= nRightIn ) ? oNode.nRight : oNode.nLeft;
                anStack[ nTop++ ] = ( nLeftIn <= nRightIn ) ? oNode.nLeft : oNode.nRight;
            }
            else if ( bLeft )
                anStack[ nTop++ ] = oNode.nLeft;
            else if ( bRight )
                anStack[ nTop++ ] = oNode.nRight;
            continue;
        }
        for ( int n = oNode.nFirst; n < oNode.nFirst + oNode.nCount; n++ )
        {
            const SPiece &oPiece = m_aoPieces[ (size_t)m_anOrder[ (size_t)n ] ];
            double        nPieceIn = nLow, nPieceOut = nBest;
            double        t, u, v;

            if ( ! BoxStretch( oPiece.anLow, oPiece.anHigh, anOrigin, anInverse, nPieceIn, nPieceOut ) )
                continue;
            if ( PieceHit( oPiece, anOrigin, anDirection, nLow, nBest, t, u, v ) )
            {
                nBest = t;
                nBestPatch = oPiece.nPatch;
                nBestU = u;
                nBestV = v;
                if ( m_bAnyHit )
                    nTop = 0;
            }
        }
    }
    if ( nBestPatch < 0 )
        return FALSE;

    double anPoint[ 3 ];

    for ( int c = 0; c < 3; c++ )
        anPoint[ c ] = anOrigin[ c ] + ( nBest * anDirection[ c ] );
    Remember( anPoint, nBestPatch, nBestU, nBestV );
    nT = nBest;
    return TRUE;
}

BOOL CBezier::Occludes( CRay &oRay, CObjects *pIgnore )
{
    CVector oHit;
    BOOL    bHit;

    m_bAnyHit = TRUE;
    bHit = Intersect( oRay, oHit, pIgnore );
    m_bAnyHit = FALSE;
    return bHit;
}

void CBezier::Remember( const double anPoint[ 3 ], int nPatch, double u, double v )
{
    SHit &oHit = m_aoRecent[ m_nRecent ];

    for ( int c = 0; c < 3; c++ )
        oHit.anPoint[ c ] = anPoint[ c ];
    oHit.nPatch = nPatch;
    oHit.u = u;
    oHit.v = v;
    m_nRecent = ( m_nRecent + 1 ) % 4;
}

BOOL CBezier::Locate( const double anPoint[ 3 ], int &nPatch, double &u, double &v )
{
        // A point a recent hit found, met again through the scene's frame.
    double nNear = 1e-7 * m_nSize;

    for ( int i = 0; i < 4; i++ )
    {
        const SHit &oHit = m_aoRecent[ i ];

        if ( oHit.nPatch >= 0 && ABS( oHit.anPoint[ 0 ] - anPoint[ 0 ] ) <= nNear && ABS( oHit.anPoint[ 1 ] - anPoint[ 1 ] ) <= nNear &&
             ABS( oHit.anPoint[ 2 ] - anPoint[ 2 ] ) <= nNear )
        {
            nPatch = oHit.nPatch;
            u = oHit.u;
            v = oHit.v;
            return TRUE;
        }
    }

        // Otherwise the nearest point of the pieces whose boxes hold it.
    double nBest = 1e-3 * m_nSize;
    BOOL   bFound = FALSE;
    double nRoom = 1e-4 * m_nSize;
    int    anStack[ 128 ];
    int    nTop = 0;

    if ( m_aoNodes.empty() )
        return FALSE;
    anStack[ nTop++ ] = 0;
    while ( nTop > 0 )
    {
        const SNode &oNode = m_aoNodes[ (size_t)anStack[ --nTop ] ];
        BOOL         bInside = TRUE;

        for ( int c = 0; c < 3; c++ )
        {
            if ( anPoint[ c ] < oNode.anLow[ c ] - nRoom || anPoint[ c ] > oNode.anHigh[ c ] + nRoom )
                bInside = FALSE;
        }
        if ( ! bInside )
            continue;
        if ( oNode.nLeft >= 0 )
        {
            if ( nTop + 2 <= 128 )
            {
                anStack[ nTop++ ] = oNode.nRight;
                anStack[ nTop++ ] = oNode.nLeft;
            }
            continue;
        }
        for ( int n = oNode.nFirst; n < oNode.nFirst + oNode.nCount; n++ )
        {
            const SPiece &oPiece = m_aoPieces[ (size_t)m_anOrder[ (size_t)n ] ];
            const SPatch &oPatch = m_aoScaled[ (size_t)oPiece.nPatch ];
            BOOL          bInPiece = TRUE;

            for ( int c = 0; c < 3; c++ )
            {
                if ( anPoint[ c ] < oPiece.anLow[ c ] - nRoom || anPoint[ c ] > oPiece.anHigh[ c ] + nRoom )
                    bInPiece = FALSE;
            }
            if ( ! bInPiece )
                continue;

                // Gauss-Newton toward the nearest point of the patch.
            double uu = 0.5 * ( oPiece.anRange[ 0 ] + oPiece.anRange[ 2 ] );
            double vv = 0.5 * ( oPiece.anRange[ 1 ] + oPiece.anRange[ 3 ] );
            double nDistance = 1e300;

            for ( int nStep = 0; nStep < 25; nStep++ )
            {
                double anS[ 3 ], anDu[ 3 ], anDv[ 3 ], anF[ 3 ];

                Evaluate( oPatch, uu, vv, anS, anDu, anDv );
                for ( int c = 0; c < 3; c++ )
                    anF[ c ] = anS[ c ] - anPoint[ c ];
                nDistance = sqrt( Dot3( anF, anF ) );
                if ( nDistance <= 1e-12 * m_nSize )
                    break;

                double a = Dot3( anDu, anDu ), b = Dot3( anDu, anDv ), d = Dot3( anDv, anDv );
                double ru = -Dot3( anDu, anF ), rv = -Dot3( anDv, anF );
                double nDet = ( a * d ) - ( b * b );

                if ( ! ( ABS( nDet ) > 1e-30 ) )
                    break;
                uu += ( ( d * ru ) - ( b * rv ) ) / nDet;
                vv += ( ( a * rv ) - ( b * ru ) ) / nDet;
                uu = ( uu < 0.0 ) ? 0.0 : ( uu > 1.0 ) ? 1.0 : uu;
                vv = ( vv < 0.0 ) ? 0.0 : ( vv > 1.0 ) ? 1.0 : vv;
            }
            if ( nDistance < nBest )
            {
                nBest = nDistance;
                nPatch = oPiece.nPatch;
                u = uu;
                v = vv;
                bFound = TRUE;
            }
        }
    }
    return bFound;
}

void CBezier::LocalNormal( const FLT anPointIn[ 3 ], FLT anNormal[ 3 ] )
{
    double anPoint[ 3 ] = { anPointIn[ 0 ], anPointIn[ 1 ], anPointIn[ 2 ] };
    int    nPatch;
    double u, v, anOut[ 3 ];

    if ( ! Locate( anPoint, nPatch, u, v ) )
    {
        anNormal[ 0 ] = 0.0;
        anNormal[ 1 ] = 1.0;
        anNormal[ 2 ] = 0.0;
        return;
    }
    Normal( m_aoScaled[ (size_t)nPatch ], u, v, anOut );
    for ( int c = 0; c < 3; c++ )
        anNormal[ c ] = anOut[ c ];
}

void CBezier::LocalCoords( const FLT anPointIn[ 3 ], FLT &nU, FLT &nV )
{
    double anPoint[ 3 ] = { anPointIn[ 0 ], anPointIn[ 1 ], anPointIn[ 2 ] };
    int    nPatch;
    double u, v;

    if ( ! Locate( anPoint, nPatch, u, v ) )
    {
        nU = nV = 0.0;
        return;
    }

    const double *anUV = m_aoScaled[ (size_t)nPatch ].anUV;

    nU = anUV[ 0 ] + ( u * ( anUV[ 2 ] - anUV[ 0 ] ) );
    nV = anUV[ 1 ] + ( v * ( anUV[ 3 ] - anUV[ 1 ] ) );
}

void CBezier::LocalBounds( FLT anLow[ 3 ], FLT anHigh[ 3 ] )
{
    for ( int c = 0; c < 3; c++ )
    {
        anLow[ c ] = m_anLow[ c ];
        anHigh[ c ] = m_anHigh[ c ];
    }
}

void CBezier::LocalTriangles( int nSegments, std::vector<FLT> &anPositions, std::vector<FLT> &anNormals )
{
    int nSteps = ( nSegments < 2 ) ? 2 : ( nSegments > 64 ) ? 64 : nSegments;

    anPositions.clear();
    anNormals.clear();
    for ( size_t nPatch = 0; nPatch < m_aoScaled.size(); nPatch++ )
    {
        const SPatch       &oPatch = m_aoScaled[ nPatch ];
        std::vector<double> anGrid( (size_t)( nSteps + 1 ) * ( nSteps + 1 ) * 6 );

        for ( int j = 0; j <= nSteps; j++ )
        {
            for ( int i = 0; i <= nSteps; i++ )
            {
                double *p = &anGrid[ (size_t)( ( j * ( nSteps + 1 ) ) + i ) * 6 ];
                double  anDu[ 3 ], anDv[ 3 ];

                Evaluate( oPatch, (double)i / nSteps, (double)j / nSteps, p, anDu, anDv );
                Normal( oPatch, (double)i / nSteps, (double)j / nSteps, p + 3 );
            }
        }
        for ( int j = 0; j < nSteps; j++ )
        {
            for ( int i = 0; i < nSteps; i++ )
            {
                const double *a = &anGrid[ (size_t)( ( j * ( nSteps + 1 ) ) + i ) * 6 ];
                const double *b = &anGrid[ (size_t)( ( j * ( nSteps + 1 ) ) + i + 1 ) * 6 ];
                const double *c = &anGrid[ (size_t)( ( ( j + 1 ) * ( nSteps + 1 ) ) + i + 1 ) * 6 ];
                const double *d = &anGrid[ (size_t)( ( ( j + 1 ) * ( nSteps + 1 ) ) + i ) * 6 ];
                FLT           fa[ 3 ], fb[ 3 ], fc[ 3 ], fd[ 3 ], na[ 3 ], nb[ 3 ], nc[ 3 ], nd[ 3 ];

                for ( int k = 0; k < 3; k++ )
                {
                    fa[ k ] = a[ k ];
                    fb[ k ] = b[ k ];
                    fc[ k ] = c[ k ];
                    fd[ k ] = d[ k ];
                    na[ k ] = a[ k + 3 ];
                    nb[ k ] = b[ k + 3 ];
                    nc[ k ] = c[ k + 3 ];
                    nd[ k ] = d[ k + 3 ];
                }
                LocalMesh::Add( anPositions, anNormals, fa, fb, fc, na, nb, nc );
                LocalMesh::Add( anPositions, anNormals, fa, fc, fd, na, nc, nd );
            }
        }
    }
}

void CBezier::CommonToJson( IASLib::CObjectNode &oNode )
{
    if ( m_oScale.GetX() == m_oScale.GetY() && m_oScale.GetY() == m_oScale.GetZ() )
    {
        if ( m_oScale.GetX() != 1.0 )
            RayJson::PutNumber( oNode, "scale", m_oScale.GetX() );
    }
    else
        RayJson::PutVector( oNode, "scale", m_oScale );
    if ( m_bClosed )
        RayJson::PutBool( oNode, "closed", m_bClosed );
}

BOOL CBezier::CommonFromJson( const IASLib::CJsonNode &oNode )
{
    const IASLib::CJsonNode *pScale = oNode.get( "scale" );

    if ( pScale && pScale->isNumber() )
    {
        double nScale = pScale->doubleValue();

        m_oScale.Set( nScale, nScale, nScale );
    }
    else if ( pScale && ! RayJson::GetVector( oNode, "scale", m_oScale, FALSE ) )
        return FALSE;
    if ( ! ( m_oScale.GetX() > 0.0 && m_oScale.GetY() > 0.0 && m_oScale.GetZ() > 0.0 ) )
    {
        RayJson::CWhere oWhere( "scale" );

        return RayJson::Fail( "must be more than 0 (a number, or [x, y, z])" );
    }
    return RayJson::GetBool( oNode, "closed", m_bClosed, FALSE );
}

const char *const *CBezier::ShapeFields( void )
{
    static const char *const astrFields[] = { "file", "data", "points", "patches", "scale", "closed", NULL };

    return astrFields;
}

void CBezier::ShapeToJson( IASLib::CObjectNode &oNode )
{
    if ( ! m_strFile.empty() )
    {
        oNode.put( "file", RayJson::SceneFileName( m_strFile, m_strFilePath, m_pFileData ) );
        if ( m_pFileData && RayJson::EmbeddingData() )
        {
            std::vector<unsigned char> anData( m_pFileData->begin(), m_pFileData->end() );

            oNode.putBinary( "data", anData );
        }
    }
    else
    {
        if ( m_bShared )
        {
            IASLib::CArrayNode *pPoints = oNode.putArray( "points" );

            for ( size_t n = 0; n + 3 < m_anShared.size(); n += 4 )
                WritePoint( *pPoints, &m_anShared[ n ] );
        }

        IASLib::CArrayNode *pPatches = oNode.putArray( "patches" );

        for ( size_t nPatch = 0; nPatch < m_aoPatches.size(); nPatch++ )
        {
            const SPatch       &oPatch = m_aoPatches[ nPatch ];
            IASLib::CObjectNode *pPatch = pPatches->addObject();

            if ( oPatch.nDegreeU != oPatch.nDegreeV )
                pPatch->putArray( "degree" )->add( oPatch.nDegreeU )->add( oPatch.nDegreeV );
            if ( ! oPatch.anIndices.empty() )
            {
                IASLib::CArrayNode *pIndices = pPatch->putArray( "indices" );

                for ( size_t n = 0; n < oPatch.anIndices.size(); n++ )
                    pIndices->add( oPatch.anIndices[ n ] );
            }
            else
            {
                IASLib::CArrayNode *pPoints = pPatch->putArray( "points" );

                for ( size_t n = 0; n + 3 < oPatch.anPoints.size(); n += 4 )
                    WritePoint( *pPoints, &oPatch.anPoints[ n ] );
            }
            if ( oPatch.bOwnUV )
                pPatch->putArray( "uv" )->add( RayJson::Rounded( oPatch.anUV[ 0 ] ) )->add( RayJson::Rounded( oPatch.anUV[ 1 ] ) )
                      ->add( RayJson::Rounded( oPatch.anUV[ 2 ] ) )->add( RayJson::Rounded( oPatch.anUV[ 3 ] ) );
        }
    }
    CommonToJson( oNode );
}

BOOL CBezier::ShapeFromJson( const IASLib::CJsonNode &oNode )
{
    const IASLib::CJsonNode *pFile = oNode.get( "file" );
    const IASLib::CJsonNode *pPatches = oNode.get( "patches" );
    const IASLib::CJsonNode *pPoints = oNode.get( "points" );

    if ( ! CommonFromJson( oNode ) )
        return FALSE;
    m_aoPatches.clear();
    m_anShared.clear();
    m_bShared = FALSE;
    m_strFile.clear();
    m_strFilePath.clear();
    m_pFileData.reset();

    if ( pFile )
    {
        std::string strData, strError;
        const IASLib::CJsonNode *pData = oNode.get( "data" );

        if ( pPatches || pPoints )
            return RayJson::Fail( "give either a file or patches, not both" );
        if ( ! RayJson::GetString( oNode, "file", m_strFile, TRUE ) )
            return FALSE;
        if ( m_strFile.empty() )
            return RayJson::Fail( "file can't be empty" );
        if ( pData )
        {
                // The file's bytes, carried in a copy of the scene sent to a task.
            std::vector<unsigned char> anData;

            if ( ! RayJson::BinaryValue( *pData, anData ) )
                return RayJson::Fail( "data must be the file's bytes, in base64" );
            strData.assign( anData.begin(), anData.end() );
        }
        else if ( ! RayJson::ReadSceneFile( m_strFile, strData, m_strFilePath, strError ) )
            return RayJson::Fail( "can't open the patch file \"" + m_strFile + "\"" );
        m_pFileData.reset( new std::string( strData ) );
        if ( ! ReadBpt( strData, m_aoPatches, strError ) )
            return RayJson::Fail( "the patch file \"" + m_strFile + "\" " + strError );
        return TRUE;
    }
    if ( oNode.get( "data" ) )
        return RayJson::Fail( "data goes with a file" );
    if ( ! pPatches )
        return RayJson::Fail( "needs patches (or a file of them)" );

    if ( pPoints )
    {
        RayJson::CWhere oWhere( "points" );

        if ( ! pPoints->isArray() || pPoints->size() < 1 )
            return RayJson::Fail( "must be an array of points" );
        m_anShared.resize( (size_t)pPoints->size() * 4 );
        for ( int n = 0; n < pPoints->size(); n++ )
        {
            RayJson::CWhere oElement( "", n );

            if ( ! ReadPoint( *pPoints->get( n ), &m_anShared[ (size_t)n * 4 ] ) )
                return FALSE;
        }
        m_bShared = TRUE;
    }

    RayJson::CWhere oWhere( "patches" );

    if ( ! pPatches->isArray() || pPatches->size() < 1 )
        return RayJson::Fail( "must be an array of patches" );
    for ( int nPatch = 0; nPatch < pPatches->size(); nPatch++ )
    {
        static const char *const astrPatchFields[] = { "degree", "points", "indices", "uv", NULL };
        RayJson::CWhere          oElement( "", nPatch );
        const IASLib::CJsonNode &oPatchNode = *pPatches->get( nPatch );
        SPatch                   oPatch;
        std::string              strError;

        if ( ! oPatchNode.isObject() )
            return RayJson::Fail( "must be an object with points or indices" );
        if ( ! RayJson::OnlyFields( oPatchNode, astrPatchFields ) )
            return FALSE;

        const IASLib::CJsonNode *pOwn = oPatchNode.get( "points" );
        const IASLib::CJsonNode *pIndices = oPatchNode.get( "indices" );
        const IASLib::CJsonNode *pDegree = oPatchNode.get( "degree" );
        int                      nCount;

        if ( ( pOwn != NULL ) == ( pIndices != NULL ) )
            return RayJson::Fail( "needs points or indices, one of them" );
        if ( pIndices && ! m_bShared )
            return RayJson::Fail( "indices need the shape's own points list" );
        {
            const IASLib::CJsonNode *pList = pOwn ? pOwn : pIndices;

            if ( ! pList->isArray() || pList->size() < 4 )
                return RayJson::Fail( pOwn ? "points must be an array of at least four points" :
                                             "indices must be an array of at least four point numbers" );
            nCount = pList->size();
        }
        if ( pDegree )
        {
            RayJson::CWhere oDegree( "degree" );

            if ( ! pDegree->isArray() || pDegree->size() != 2 || ! pDegree->get( 0 )->isNumber() || ! pDegree->get( 1 )->isNumber() )
                return RayJson::Fail( "must be [u degree, v degree]" );

            double nU = pDegree->get( 0 )->doubleValue(), nV = pDegree->get( 1 )->doubleValue();

            if ( nU != floor( nU ) || nV != floor( nV ) || nU < 1 || nV < 1 || nU > s_nMaxDegree || nV > s_nMaxDegree )
                return RayJson::Fail( "degrees must be whole numbers from 1 to " + std::to_string( s_nMaxDegree ) );
            oPatch.nDegreeU = (int)nU;
            oPatch.nDegreeV = (int)nV;
        }
        else
        {
                // Square nets need no degree: 16 points are bicubic.
            int nSide = (int)floor( sqrt( (double)nCount ) + 0.5 );

            if ( nSide * nSide != nCount || nSide < 2 )
                return RayJson::Fail( "has " + std::to_string( nCount ) + " points, not a square number: give its degree" );
            oPatch.nDegreeU = oPatch.nDegreeV = nSide - 1;
        }
        if ( nCount != ( oPatch.nDegreeU + 1 ) * ( oPatch.nDegreeV + 1 ) )
            return RayJson::Fail( "a patch of degree " + std::to_string( oPatch.nDegreeU ) + " by " + std::to_string( oPatch.nDegreeV ) +
                                  " needs " + std::to_string( ( oPatch.nDegreeU + 1 ) * ( oPatch.nDegreeV + 1 ) ) + " points, not " +
                                  std::to_string( nCount ) );
        oPatch.anPoints.resize( (size_t)nCount * 4 );
        for ( int n = 0; n < nCount; n++ )
        {
            if ( pOwn )
            {
                RayJson::CWhere oPoint( "points", n );

                if ( ! ReadPoint( *pOwn->get( n ), &oPatch.anPoints[ (size_t)n * 4 ] ) )
                    return FALSE;
                continue;
            }

            const IASLib::CJsonNode *pIndex = pIndices->get( n );
            double                   dIndex = ( pIndex && pIndex->isNumber() ) ? pIndex->doubleValue() : -1.0;
            size_t                   nShared = m_anShared.size() / 4;

            if ( dIndex != floor( dIndex ) || dIndex < 0 || dIndex >= (double)nShared )
                return RayJson::Fail( "indices must be whole numbers from 0 to " + std::to_string( (long)nShared - 1 ) );
            oPatch.anIndices.push_back( (int)dIndex );
            for ( int c = 0; c < 4; c++ )
                oPatch.anPoints[ (size_t)n * 4 + c ] = m_anShared[ (size_t)dIndex * 4 + c ];
        }

        const IASLib::CJsonNode *pUV = oPatchNode.get( "uv" );

        if ( pUV )
        {
            RayJson::CWhere oUV( "uv" );

            if ( ! pUV->isArray() || pUV->size() != 4 )
                return RayJson::Fail( "must be [u0, v0, u1, v1]" );
            for ( int k = 0; k < 4; k++ )
            {
                if ( ! pUV->get( k )->isNumber() )
                    return RayJson::Fail( "must be [u0, v0, u1, v1]" );
                oPatch.anUV[ k ] = pUV->get( k )->doubleValue();
            }
            oPatch.bOwnUV = TRUE;
        }
        if ( ! CheckPatch( oPatch, strError ) )
            return RayJson::Fail( strError );
        m_aoPatches.push_back( oPatch );
    }
    return TRUE;
}

void CBezier::ToJson( IASLib::CObjectNode &oNode )
{
    CLocalShape::ToJson( oNode );
}

BOOL CBezier::FromJson( const IASLib::CJsonNode &oNode )
{
    return CLocalShape::FromJson( oNode );
}
