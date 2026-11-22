/*************************************************************************
*   CBSpline                                                             *
*                                                                        *
*   A B-spline (or NURBS) surface, traced as Bezier patches. See         *
*   BSpline.h.                                                           *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

    #include "Globals.h"
#include "BSpline.h"
#include "SceneJson.h"

#include <algorithm>
#include <math.h>


DYNAMIC_IMPLEMENTS( CBSpline, "bspline" );

namespace
{
    const int s_nMaxDegree = 15;

        // How many times t appears in the knots.
    int Multiplicity( const std::vector<double> &anKnots, double t )
    {
        return (int)std::count( anKnots.begin(), anKnots.end(), t );
    }

        // A point of a curve (homogeneous, four numbers a point) at t, by de
        // Boor's algorithm.
    void CurvePoint( const double *anPoints, int nCount, const std::vector<double> &anKnots, int p, double t, double anOut[ 4 ] )
    {
        double a = anKnots[ (size_t)p ], b = anKnots[ (size_t)nCount ];
        int    k = p;

        t = ( t < a ) ? a : ( t > b ) ? b : t;
            // The span holding t; at the end, the last span that isn't empty.
        for ( int i = p; i < nCount; i++ )
        {
            if ( anKnots[ (size_t)i ] <= t && anKnots[ (size_t)i ] < anKnots[ (size_t)i + 1 ] )
                k = i;
        }

        double anD[ ( s_nMaxDegree + 1 ) * 4 ];

        for ( int i = 0; i <= p; i++ )
        {
            for ( int c = 0; c < 4; c++ )
                anD[ i * 4 + c ] = anPoints[ (size_t)( k - p + i ) * 4 + c ];
        }
        for ( int r = 1; r <= p; r++ )
        {
            for ( int i = p; i >= r; i-- )
            {
                double nLow = anKnots[ (size_t)( k - p + i ) ];
                double nHigh = anKnots[ (size_t)( k + 1 + i - r ) ];
                double nAlpha = ( nHigh > nLow ) ? ( t - nLow ) / ( nHigh - nLow ) : 0.0;

                for ( int c = 0; c < 4; c++ )
                    anD[ i * 4 + c ] = ( ( 1.0 - nAlpha ) * anD[ ( i - 1 ) * 4 + c ] ) + ( nAlpha * anD[ i * 4 + c ] );
            }
        }
        for ( int c = 0; c < 4; c++ )
            anOut[ c ] = anD[ p * 4 + c ];
    }

    BOOL ReadKnots( const IASLib::CJsonNode &oNode, const char *strKey, std::vector<double> &anKnots, BOOL &bGiven )
    {
        const IASLib::CJsonNode *pKnots = oNode.get( strKey );

        bGiven = FALSE;
        anKnots.clear();
        if ( ! pKnots )
            return TRUE;

        RayJson::CWhere oWhere( strKey );

        if ( ! pKnots->isArray() )
            return RayJson::Fail( "must be an array of numbers" );
        for ( int n = 0; n < pKnots->size(); n++ )
        {
            const IASLib::CJsonNode *pValue = pKnots->get( n );

            if ( ! pValue || ! pValue->isNumber() )
                return RayJson::Fail( "must be an array of numbers" );
            anKnots.push_back( pValue->doubleValue() );
        }
        bGiven = TRUE;
        return TRUE;
    }
}

CBSpline::CBSpline( void ) :
    m_nDegreeU( 3 ),
    m_nDegreeV( 3 ),
    m_nCountU( 0 ),
    m_nCountV( 0 ),
    m_bOwnKnotsU( FALSE ),
    m_bOwnKnotsV( FALSE )
{
}

CBSpline::~CBSpline( void )
{
}

std::vector<double> CBSpline::ClampedKnots( int nCount, int nDegree )
{
    std::vector<double> anKnots;
    int                 nInterior = nCount - nDegree - 1;

    for ( int i = 0; i <= nDegree; i++ )
        anKnots.push_back( 0.0 );
    for ( int i = 1; i <= nInterior; i++ )
        anKnots.push_back( (double)i );
    for ( int i = 0; i <= nDegree; i++ )
        anKnots.push_back( (double)( nInterior + 1 ) );
    return anKnots;
}

BOOL CBSpline::ToBezier( std::vector<double> &anCurve, std::vector<double> &anKnots, int p )
{
    int nCount = (int)( anCurve.size() / 4 );

    if ( p < 1 || nCount < p + 1 || (int)anKnots.size() != nCount + p + 1 )
        return FALSE;

    double              a = anKnots[ (size_t)p ], b = anKnots[ (size_t)nCount ];
    std::vector<double> anValues;

    for ( size_t n = 0; n < anKnots.size(); n++ )
    {
        double t = anKnots[ n ];

        if ( t >= a && t <= b && ( anValues.empty() || anValues.back() != t ) )
            anValues.push_back( t );
    }
    for ( size_t nValue = 0; nValue < anValues.size(); nValue++ )
    {
        double t = anValues[ nValue ];

        while ( Multiplicity( anKnots, t ) < p )
        {
                // Boehm: the span holding t, the last knot at or below it; at
                // the end of the range, the last span with control points
                // on both sides (the same insertion, as t is a knot there).
            int nNow = (int)( anCurve.size() / 4 );
            int k = (int)( std::upper_bound( anKnots.begin(), anKnots.end(), t ) - anKnots.begin() ) - 1;

            k = ( k > nNow - 1 ) ? nNow - 1 : k;

            if ( k < p || k + p >= (int)anKnots.size() )
                return FALSE;

            std::vector<double> anNew( (size_t)( nNow + 1 ) * 4 );

            for ( int i = 0; i <= nNow; i++ )
            {
                for ( int c = 0; c < 4; c++ )
                {
                    double nValue;

                    if ( i <= k - p )
                        nValue = anCurve[ (size_t)i * 4 + c ];
                    else if ( i <= k )
                    {
                        double nLow = anKnots[ (size_t)i ], nHigh = anKnots[ (size_t)( i + p ) ];
                        double nAlpha = ( nHigh > nLow ) ? ( t - nLow ) / ( nHigh - nLow ) : 0.0;

                        nValue = ( ( 1.0 - nAlpha ) * anCurve[ (size_t)( i - 1 ) * 4 + c ] ) + ( nAlpha * anCurve[ (size_t)i * 4 + c ] );
                    }
                    else
                        nValue = anCurve[ (size_t)( i - 1 ) * 4 + c ];
                    anNew[ (size_t)i * 4 + c ] = nValue;
                }
            }
            anCurve.swap( anNew );
            anKnots.insert( anKnots.begin() + k + 1, t );
        }
    }
    return TRUE;
}

void CBSpline::Range( double anRange[ 4 ] ) const
{
    anRange[ 0 ] = m_anKnotsU[ (size_t)m_nDegreeU ];
    anRange[ 1 ] = m_anKnotsU[ (size_t)m_nCountU ];
    anRange[ 2 ] = m_anKnotsV[ (size_t)m_nDegreeV ];
    anRange[ 3 ] = m_anKnotsV[ (size_t)m_nCountV ];
}

void CBSpline::SurfacePoint( double u, double v, double anPoint[ 3 ] ) const
{
    std::vector<double> anColumn( (size_t)m_nCountV * 4 );

    for ( int j = 0; j < m_nCountV; j++ )
        CurvePoint( &m_anNet[ (size_t)( j * m_nCountU ) * 4 ], m_nCountU, m_anKnotsU, m_nDegreeU, u, &anColumn[ (size_t)j * 4 ] );

    double anH[ 4 ];

    CurvePoint( &anColumn[ 0 ], m_nCountV, m_anKnotsV, m_nDegreeV, v, anH );
    for ( int c = 0; c < 3; c++ )
        anPoint[ c ] = anH[ c ] / anH[ 3 ];
}

BOOL CBSpline::BuildPatches( void )
{
    int p = m_nDegreeU, q = m_nDegreeV;

    if ( p < 1 || p > s_nMaxDegree || q < 1 || q > s_nMaxDegree )
        return RayJson::Fail( "degrees must be whole numbers from 1 to " + std::to_string( s_nMaxDegree ) );
    if ( m_nCountU < p + 1 || m_nCountV < q + 1 )
        return RayJson::Fail( "needs at least degree + 1 points each way: " + std::to_string( p + 1 ) + " in a row and " +
                              std::to_string( q + 1 ) + " rows" );
    if ( m_anNet.size() != (size_t)m_nCountU * (size_t)m_nCountV * 4 )
        return RayJson::Fail( "every row needs the same number of points" );
    if ( ! m_bOwnKnotsU )
        m_anKnotsU = ClampedKnots( m_nCountU, p );
    if ( ! m_bOwnKnotsV )
        m_anKnotsV = ClampedKnots( m_nCountV, q );
    if ( (int)m_anKnotsU.size() != m_nCountU + p + 1 )
        return RayJson::Fail( "knotsU needs " + std::to_string( m_nCountU + p + 1 ) + " knots (points in a row + degree + 1)" );
    if ( (int)m_anKnotsV.size() != m_nCountV + q + 1 )
        return RayJson::Fail( "knotsV needs " + std::to_string( m_nCountV + q + 1 ) + " knots (rows + degree + 1)" );
    for ( size_t n = 1; n < m_anKnotsU.size(); n++ )
    {
        if ( m_anKnotsU[ n ] < m_anKnotsU[ n - 1 ] )
            return RayJson::Fail( "knotsU must never decrease" );
    }
    for ( size_t n = 1; n < m_anKnotsV.size(); n++ )
    {
        if ( m_anKnotsV[ n ] < m_anKnotsV[ n - 1 ] )
            return RayJson::Fail( "knotsV must never decrease" );
    }

    double anRange[ 4 ];

    Range( anRange );
    if ( ! ( anRange[ 1 ] > anRange[ 0 ] ) || ! ( anRange[ 3 ] > anRange[ 2 ] ) )
        return RayJson::Fail( "the knots leave the surface no range" );

        // Each row cut to Bezier pieces along u, then each column of the
        // result along v.
    std::vector<double> anKnotsU, anKnotsV;
    std::vector<double> anRows;
    int                 nNewU = 0;

    for ( int j = 0; j < m_nCountV; j++ )
    {
        std::vector<double> anRow( m_anNet.begin() + (long)( j * m_nCountU ) * 4, m_anNet.begin() + (long)( ( j + 1 ) * m_nCountU ) * 4 );

        anKnotsU = m_anKnotsU;
        if ( ! ToBezier( anRow, anKnotsU, p ) )
            return RayJson::Fail( "knotsU can't be cut into Bezier pieces" );
        nNewU = (int)( anRow.size() / 4 );
        anRows.insert( anRows.end(), anRow.begin(), anRow.end() );
    }

    std::vector<double> anGrid;
    int                 nNewV = 0;

    for ( int i = 0; i < nNewU; i++ )
    {
        std::vector<double> anColumn;

        for ( int j = 0; j < m_nCountV; j++ )
            anColumn.insert( anColumn.end(), anRows.begin() + (long)( j * nNewU + i ) * 4, anRows.begin() + (long)( j * nNewU + i + 1 ) * 4 );
        anKnotsV = m_anKnotsV;
        if ( ! ToBezier( anColumn, anKnotsV, q ) )
            return RayJson::Fail( "knotsV can't be cut into Bezier pieces" );
        nNewV = (int)( anColumn.size() / 4 );
        if ( anGrid.empty() )
            anGrid.resize( (size_t)nNewU * (size_t)nNewV * 4 );
        for ( int j = 0; j < nNewV; j++ )
        {
            for ( int c = 0; c < 4; c++ )
                anGrid[ (size_t)( j * nNewU + i ) * 4 + c ] = anColumn[ (size_t)j * 4 + c ];
        }
    }

        // The spans of the knots that aren't empty: each starts a patch.
    std::vector<int> anStartsU, anStartsV;

    for ( int i = p; i + 1 < (int)anKnotsU.size() - p; i++ )
    {
        if ( anKnotsU[ (size_t)i ] < anKnotsU[ (size_t)i + 1 ] && anKnotsU[ (size_t)i ] >= anRange[ 0 ] && anKnotsU[ (size_t)i + 1 ] <= anRange[ 1 ] )
            anStartsU.push_back( i );
    }
    for ( int k = q; k + 1 < (int)anKnotsV.size() - q; k++ )
    {
        if ( anKnotsV[ (size_t)k ] < anKnotsV[ (size_t)k + 1 ] && anKnotsV[ (size_t)k ] >= anRange[ 2 ] && anKnotsV[ (size_t)k + 1 ] <= anRange[ 3 ] )
            anStartsV.push_back( k );
    }

    std::vector<SPatch> aoPatches;

    for ( size_t nV = 0; nV < anStartsV.size(); nV++ )
    {
        for ( size_t nU = 0; nU < anStartsU.size(); nU++ )
        {
            int    i = anStartsU[ nU ], k = anStartsV[ nV ];
            SPatch oPatch;

            oPatch.nDegreeU = p;
            oPatch.nDegreeV = q;
            oPatch.anPoints.resize( (size_t)( p + 1 ) * (size_t)( q + 1 ) * 4 );
            for ( int jv = 0; jv <= q; jv++ )
            {
                for ( int ju = 0; ju <= p; ju++ )
                {
                    int nFrom = ( ( k - q + jv ) * nNewU ) + ( i - p + ju );

                    for ( int c = 0; c < 4; c++ )
                        oPatch.anPoints[ (size_t)( jv * ( p + 1 ) + ju ) * 4 + c ] = anGrid[ (size_t)nFrom * 4 + c ];
                }
            }
            oPatch.anUV[ 0 ] = ( anKnotsU[ (size_t)i ] - anRange[ 0 ] ) / ( anRange[ 1 ] - anRange[ 0 ] );
            oPatch.anUV[ 1 ] = ( anKnotsV[ (size_t)k ] - anRange[ 2 ] ) / ( anRange[ 3 ] - anRange[ 2 ] );
            oPatch.anUV[ 2 ] = ( anKnotsU[ (size_t)i + 1 ] - anRange[ 0 ] ) / ( anRange[ 1 ] - anRange[ 0 ] );
            oPatch.anUV[ 3 ] = ( anKnotsV[ (size_t)k + 1 ] - anRange[ 2 ] ) / ( anRange[ 3 ] - anRange[ 2 ] );

            std::string strError;

            if ( ! CheckPatch( oPatch, strError ) )
                return RayJson::Fail( strError );
            aoPatches.push_back( oPatch );
        }
    }
    if ( aoPatches.empty() )
        return RayJson::Fail( "the knots leave the surface no patches" );
    m_aoPatches = aoPatches;
    return TRUE;
}

BOOL CBSpline::SetSurface( int nDegreeU, int nDegreeV, int nCountU, int nCountV, const std::vector<double> &anNet,
                           const std::vector<double> &anKnotsU, const std::vector<double> &anKnotsV )
{
    m_nDegreeU = nDegreeU;
    m_nDegreeV = nDegreeV;
    m_nCountU = nCountU;
    m_nCountV = nCountV;
    m_anNet = anNet;
    m_anKnotsU = anKnotsU;
    m_anKnotsV = anKnotsV;
    m_bOwnKnotsU = anKnotsU.empty() ? FALSE : TRUE;
    m_bOwnKnotsV = anKnotsV.empty() ? FALSE : TRUE;
    for ( size_t n = 3; n < m_anNet.size(); n += 4 )
    {
        if ( ! ( m_anNet[ n ] > 0.0 ) )
            return RayJson::Fail( "a weight must be more than 0" );
    }
    return BuildPatches() && Prepare();
}

const char *const *CBSpline::ShapeFields( void )
{
    static const char *const astrFields[] = { "degree", "points", "knotsU", "knotsV", "scale", "closed", NULL };

    return astrFields;
}

void CBSpline::ShapeToJson( IASLib::CObjectNode &oNode )
{
    if ( m_nDegreeU != 3 || m_nDegreeV != 3 )
        oNode.putArray( "degree" )->add( m_nDegreeU )->add( m_nDegreeV );

    IASLib::CArrayNode *pRows = oNode.putArray( "points" );

    for ( int j = 0; j < m_nCountV; j++ )
    {
        IASLib::CArrayNode *pRow = pRows->addArray();

        for ( int i = 0; i < m_nCountU; i++ )
        {
            const double       *p = &m_anNet[ (size_t)( j * m_nCountU + i ) * 4 ];
            IASLib::CArrayNode *pPoint = pRow->addArray();

            pPoint->add( RayJson::Rounded( p[ 0 ] / p[ 3 ] ) )->add( RayJson::Rounded( p[ 1 ] / p[ 3 ] ) )->add( RayJson::Rounded( p[ 2 ] / p[ 3 ] ) );
            if ( p[ 3 ] != 1.0 )
                pPoint->add( RayJson::Rounded( p[ 3 ] ) );
        }
    }
    if ( m_bOwnKnotsU )
    {
        IASLib::CArrayNode *pKnots = oNode.putArray( "knotsU" );

        for ( size_t n = 0; n < m_anKnotsU.size(); n++ )
            pKnots->add( RayJson::Rounded( m_anKnotsU[ n ] ) );
    }
    if ( m_bOwnKnotsV )
    {
        IASLib::CArrayNode *pKnots = oNode.putArray( "knotsV" );

        for ( size_t n = 0; n < m_anKnotsV.size(); n++ )
            pKnots->add( RayJson::Rounded( m_anKnotsV[ n ] ) );
    }
    CommonToJson( oNode );
}

BOOL CBSpline::ShapeFromJson( const IASLib::CJsonNode &oNode )
{
    const IASLib::CJsonNode *pDegree = oNode.get( "degree" );
    const IASLib::CJsonNode *pRows = RayJson::Field( oNode, "points", TRUE );

    if ( ! CommonFromJson( oNode ) || pRows == NULL )
        return FALSE;
    m_nDegreeU = m_nDegreeV = 3;
    if ( pDegree )
    {
        RayJson::CWhere oWhere( "degree" );
        double          nU, nV;

        if ( pDegree->isNumber() )
            nU = nV = pDegree->doubleValue();
        else if ( pDegree->isArray() && pDegree->size() == 2 && pDegree->get( 0 )->isNumber() && pDegree->get( 1 )->isNumber() )
        {
            nU = pDegree->get( 0 )->doubleValue();
            nV = pDegree->get( 1 )->doubleValue();
        }
        else
            return RayJson::Fail( "must be a degree, or [u degree, v degree]" );
        if ( nU != floor( nU ) || nV != floor( nV ) || nU < 1 || nV < 1 || nU > s_nMaxDegree || nV > s_nMaxDegree )
            return RayJson::Fail( "degrees must be whole numbers from 1 to " + std::to_string( s_nMaxDegree ) );
        m_nDegreeU = (int)nU;
        m_nDegreeV = (int)nV;
    }

    {
        RayJson::CWhere oWhere( "points" );

        if ( ! pRows->isArray() || pRows->size() < 2 )
            return RayJson::Fail( "must be an array of rows, each an array of points" );
        m_nCountV = pRows->size();
        m_nCountU = 0;
        m_anNet.clear();
        for ( int j = 0; j < m_nCountV; j++ )
        {
            RayJson::CWhere          oRow( "", j );
            const IASLib::CJsonNode &oRowNode = *pRows->get( j );

            if ( ! oRowNode.isArray() || oRowNode.size() < 2 )
                return RayJson::Fail( "must be a row: an array of points" );
            if ( j == 0 )
                m_nCountU = oRowNode.size();
            else if ( oRowNode.size() != m_nCountU )
                return RayJson::Fail( "every row needs the same number of points (" + std::to_string( m_nCountU ) + ")" );
            for ( int i = 0; i < m_nCountU; i++ )
            {
                RayJson::CWhere          oPoint( "", i );
                const IASLib::CJsonNode &oPointNode = *oRowNode.get( i );
                double                   anIn[ 4 ] = { 0.0, 0.0, 0.0, 1.0 };

                if ( ! oPointNode.isArray() || ( oPointNode.size() != 3 && oPointNode.size() != 4 ) )
                    return RayJson::Fail( "must be [x, y, z] or [x, y, z, weight]" );
                for ( int c = 0; c < oPointNode.size(); c++ )
                {
                    if ( ! oPointNode.get( c )->isNumber() )
                        return RayJson::Fail( "must be [x, y, z] or [x, y, z, weight]" );
                    anIn[ c ] = oPointNode.get( c )->doubleValue();
                }
                if ( ! ( anIn[ 3 ] > 0.0 ) )
                    return RayJson::Fail( "a weight must be more than 0" );
                m_anNet.push_back( anIn[ 0 ] * anIn[ 3 ] );
                m_anNet.push_back( anIn[ 1 ] * anIn[ 3 ] );
                m_anNet.push_back( anIn[ 2 ] * anIn[ 3 ] );
                m_anNet.push_back( anIn[ 3 ] );
            }
        }
    }
    if ( ! ReadKnots( oNode, "knotsU", m_anKnotsU, m_bOwnKnotsU ) || ! ReadKnots( oNode, "knotsV", m_anKnotsV, m_bOwnKnotsV ) )
        return FALSE;
    return BuildPatches();
}

void CBSpline::ToJson( IASLib::CObjectNode &oNode )
{
    CLocalShape::ToJson( oNode );
}

BOOL CBSpline::FromJson( const IASLib::CJsonNode &oNode )
{
    return CLocalShape::FromJson( oNode );
}
