/*************************************************************************
*   CSweep                                                               *
*                                                                        *
*   A tube along a path. See Sweep.h.                                    *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Sweep.h"
#include "Extruder.h"
#include "SceneJson.h"

#include <cmath>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DYNAMIC_IMPLEMENTS( CSweep, "sweep" );

namespace
{
    struct SV
    {
        double x, y, z;

        SV( void ) : x( 0.0 ), y( 0.0 ), z( 0.0 ) {}
        SV( double a, double b, double c ) : x( a ), y( b ), z( c ) {}
        SV operator +( const SV &o ) const { return SV( x + o.x, y + o.y, z + o.z ); }
        SV operator -( const SV &o ) const { return SV( x - o.x, y - o.y, z - o.z ); }
        SV operator *( double s ) const { return SV( x * s, y * s, z * s ); }
        double Dot( const SV &o ) const { return ( x * o.x ) + ( y * o.y ) + ( z * o.z ); }
        SV Cross( const SV &o ) const { return SV( ( y * o.z ) - ( z * o.y ), ( z * o.x ) - ( x * o.z ), ( x * o.y ) - ( y * o.x ) ); }
        double Length( void ) const { return std::sqrt( Dot( *this ) ); }
        SV Unit( void ) const { double l = Length(); return ( l > 0.0 ) ? *this * ( 1.0 / l ) : SV( 0.0, 0.0, 1.0 ); }
    };

        // v turned by nAngle about the unit axis a (Rodrigues).
    SV Turn( const SV &v, const SV &a, double nAngle )
    {
        double c = std::cos( nAngle ), s = std::sin( nAngle );

        return ( v * c ) + ( a.Cross( v ) * s ) + ( a * ( a.Dot( v ) * ( 1.0 - c ) ) );
    }

        // Any unit vector square to t.
    SV Square( const SV &t )
    {
        SV a = ( std::fabs( t.x ) < 0.9 ) ? SV( 1.0, 0.0, 0.0 ) : SV( 0.0, 1.0, 0.0 );

        return ( a - ( t * a.Dot( t ) ) ).Unit();
    }
}

CSweep::CSweep( void ) :
    m_bSmooth( TRUE ),
    m_bClosedPath( FALSE ),
    m_nRadius( 0.1 ),
    m_bOutline( FALSE ),
    m_nSegments( 24 ),
    m_nSteps( 12 ),
    m_bCapped( TRUE )
{
}

CSweep::~CSweep( void )
{
}

#ifdef RAY_GUI
void CSweep::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    HTREEITEM   hItem;

    hItem = pTree->AddObject( this, 24, "Sweep", hUnder );
    CObjects::MatToTree( pTree, hItem );
}
#endif

BOOL CSweep::Build( std::string &strError )
{
    size_t nN = m_aoPath.size();

    if ( nN < 2 )
    {
        strError = "needs at least two points on its path";
        return FALSE;
    }

    std::vector<SV> aoPoints( nN );

    for ( size_t i = 0; i < nN; i++ )
        aoPoints[ i ] = SV( m_aoPath[ i ].GetX(), m_aoPath[ i ].GetY(), m_aoPath[ i ].GetZ() );

        // Places and directions along the path.
    std::vector<SV> aoAt, aoTangent;
    size_t          nSpans = m_bClosedPath ? nN : nN - 1;

    if ( m_bSmooth )
    {
        auto P = [&]( long k ) -> const SV &
        {
            if ( m_bClosedPath )
                k = ( ( k % (long)nN ) + (long)nN ) % (long)nN;
            else if ( k < 0 )
                k = 0;
            else if ( k >= (long)nN )
                k = (long)nN - 1;
            return aoPoints[ (size_t)k ];
        };

        for ( size_t i = 0; i < nSpans; i++ )
        {
            const SV &p0 = P( (long)i - 1 ), &p1 = P( (long)i ), &p2 = P( (long)i + 1 ), &p3 = P( (long)i + 2 );
            SV        b0 = p1, b1 = p1 + ( ( p2 - p0 ) * ( 1.0 / 6.0 ) ), b2 = p2 - ( ( p3 - p1 ) * ( 1.0 / 6.0 ) ), b3 = p2;
            int       nLast = ( ! m_bClosedPath && i + 1 == nSpans ) ? m_nSteps : m_nSteps - 1;

            for ( int k = 0; k <= nLast; k++ )
            {
                double s = (double)k / m_nSteps, u = 1.0 - s;
                SV     oPoint = ( b0 * ( u * u * u ) ) + ( b1 * ( 3.0 * u * u * s ) ) + ( b2 * ( 3.0 * u * s * s ) ) + ( b3 * ( s * s * s ) );
                SV     oSlope = ( ( b1 - b0 ) * ( 3.0 * u * u ) ) + ( ( b2 - b1 ) * ( 6.0 * u * s ) ) + ( ( b3 - b2 ) * ( 3.0 * s * s ) );

                if ( oSlope.Length() < 1e-12 )
                    oSlope = p2 - p1;
                aoAt.push_back( oPoint );
                aoTangent.push_back( oSlope.Unit() );
            }
        }
    }
    else
    {
        for ( size_t i = 0; i < nN; i++ )
        {
            SV oIn, oOut;

            if ( i > 0 || m_bClosedPath )
                oIn = ( aoPoints[ i ] - aoPoints[ ( i + nN - 1 ) % nN ] ).Unit();
            if ( i + 1 < nN || m_bClosedPath )
                oOut = ( aoPoints[ ( i + 1 ) % nN ] - aoPoints[ i ] ).Unit();
            aoAt.push_back( aoPoints[ i ] );
            aoTangent.push_back( ( oIn + oOut ).Length() > 1e-12 ? ( oIn + oOut ).Unit() : oOut.Unit() );
        }
    }

    size_t nRings = aoAt.size();

    for ( size_t i = 1; i < nRings; i++ )
    {
        if ( ( aoAt[ i ] - aoAt[ i - 1 ] ).Length() <= 0.0 )
        {
            strError = "has the same point twice in a row on its path";
            return FALSE;
        }
    }

        // The frame carried along by double reflection (Wang et al.).
    std::vector<SV> aoNormal( nRings );

    aoNormal[ 0 ] = Square( aoTangent[ 0 ] );

    auto Carry = [&]( const SV &x0, const SV &t0, const SV &r0, const SV &x1, const SV &t1 ) -> SV
    {
        SV     v1 = x1 - x0;
        double c1 = v1.Dot( v1 );

        if ( c1 <= 0.0 )
            return r0;

        SV     rL = r0 - ( v1 * ( 2.0 / c1 * v1.Dot( r0 ) ) );
        SV     tL = t0 - ( v1 * ( 2.0 / c1 * v1.Dot( t0 ) ) );
        SV     v2 = t1 - tL;
        double c2 = v2.Dot( v2 );
        SV     r1 = ( c2 > 1e-300 ) ? rL - ( v2 * ( 2.0 / c2 * v2.Dot( rL ) ) ) : rL;

        return ( r1 - ( t1 * r1.Dot( t1 ) ) ).Unit();
    };

    for ( size_t i = 1; i < nRings; i++ )
        aoNormal[ i ] = Carry( aoAt[ i - 1 ], aoTangent[ i - 1 ], aoNormal[ i - 1 ], aoAt[ i ], aoTangent[ i ] );
    if ( m_bClosedPath )
    {
            // Carried once more round to the start, the frame comes back
            // turned; spread the turn back along the path.
        SV     oBack = Carry( aoAt[ nRings - 1 ], aoTangent[ nRings - 1 ], aoNormal[ nRings - 1 ], aoAt[ 0 ], aoTangent[ 0 ] );
        double nAngle = std::atan2( oBack.Cross( aoNormal[ 0 ] ).Dot( aoTangent[ 0 ] ), oBack.Dot( aoNormal[ 0 ] ) );

        for ( size_t i = 1; i < nRings; i++ )
            aoNormal[ i ] = Turn( aoNormal[ i ], aoTangent[ i ], nAngle * (double)i / (double)nRings );
    }

        // The cross-section: points and their outward normals, running
        // counterclockwise.
    Outline::Polygon             oSection;
    std::vector<Outline::SPoint> aoSectionNormal;

    if ( m_bOutline )
    {
        oSection = Profile::Flatten( m_oOutline, 0.001 * Profile::Extent( m_oOutline ) );
        if ( oSection.size() < 3 || std::fabs( Outline::SignedArea2( oSection ) ) <= 0.0 )
        {
            strError = "has an outline with no area";
            return FALSE;
        }
        if ( Outline::SignedArea2( oSection ) < 0.0 )
            oSection.assign( oSection.rbegin(), oSection.rend() );

        size_t nS = oSection.size();

        for ( size_t k = 0; k < nS; k++ )
        {
            const Outline::SPoint &a = oSection[ ( k + nS - 1 ) % nS ], &b = oSection[ k ], &c = oSection[ ( k + 1 ) % nS ];
            double                 l1 = std::hypot( b.x - a.x, b.y - a.y ), l2 = std::hypot( c.x - b.x, c.y - b.y );
            double                 nx = ( ( b.y - a.y ) / l1 ) + ( ( c.y - b.y ) / l2 ), ny = -( ( b.x - a.x ) / l1 ) - ( ( c.x - b.x ) / l2 );
            double                 l = std::hypot( nx, ny );
            Outline::SPoint        n = { ( l > 0.0 ) ? nx / l : 1.0, ( l > 0.0 ) ? ny / l : 0.0 };

            aoSectionNormal.push_back( n );
        }
    }
    else
    {
        for ( int k = 0; k < m_nSegments; k++ )
        {
            double          a = 2.0 * PI * k / m_nSegments;
            Outline::SPoint p = { m_nRadius * std::cos( a ), m_nRadius * std::sin( a ) };
            Outline::SPoint n = { std::cos( a ), std::sin( a ) };

            oSection.push_back( p );
            aoSectionNormal.push_back( n );
        }
    }

    size_t              nS = oSection.size();
    std::vector<double> anU( nS + 1, 0.0 ), anV( nRings + 1, 0.0 );

    for ( size_t k = 1; k <= nS; k++ )
        anU[ k ] = anU[ k - 1 ] + std::hypot( oSection[ k % nS ].x - oSection[ k - 1 ].x, oSection[ k % nS ].y - oSection[ k - 1 ].y );
    for ( size_t k = 1; k <= nS; k++ )
        anU[ k ] /= anU[ nS ];
    for ( size_t i = 1; i <= nRings; i++ )
    {
        if ( i == nRings && ! m_bClosedPath )
            break;
        anV[ i ] = anV[ i - 1 ] + ( aoAt[ i % nRings ] - aoAt[ i - 1 ] ).Length();
    }

    double nTotal = m_bClosedPath ? anV[ nRings ] : anV[ nRings - 1 ];

        // The rings, with a seam column repeated for u = 1, and for a
        // closed path the first ring repeated at the end for v = 1.
    Extruder::SBuilder oOut;
    size_t             nRows = m_bClosedPath ? nRings + 1 : nRings;
    std::vector<int>   anIndex( nRows * ( nS + 1 ) );

    for ( size_t i = 0; i < nRows; i++ )
    {
        size_t r = i % nRings;
        SV     t = aoTangent[ r ], n = aoNormal[ r ], b = t.Cross( n );

        for ( size_t k = 0; k <= nS; k++ )
        {
            const Outline::SPoint &q = oSection[ k % nS ], &m = aoSectionNormal[ k % nS ];
            SV                     p = aoAt[ r ] + ( n * q.x ) + ( b * q.y );
            SV                     nn = ( n * m.x ) + ( b * m.y );

            anIndex[ ( i * ( nS + 1 ) ) + k ] = oOut.AddUV( p.x, p.y, p.z, nn.x, nn.y, nn.z, anU[ k ], anV[ i ] / nTotal );
        }
    }
    for ( size_t i = 0; i + 1 < nRows; i++ )
    {
        for ( size_t k = 0; k < nS; k++ )
        {
            int a = anIndex[ ( i * ( nS + 1 ) ) + k ], b = anIndex[ ( i * ( nS + 1 ) ) + k + 1 ];
            int c = anIndex[ ( ( i + 1 ) * ( nS + 1 ) ) + k + 1 ], d = anIndex[ ( ( i + 1 ) * ( nS + 1 ) ) + k ];
            CVector oSum = oOut.aoNormals[ (size_t)a ] + oOut.aoNormals[ (size_t)b ] + oOut.aoNormals[ (size_t)c ] + oOut.aoNormals[ (size_t)d ];

            oOut.Quad( a, b, c, d, oSum.GetX(), oSum.GetY(), oSum.GetZ() );
        }
    }
    if ( ! m_bClosedPath && m_bCapped )
    {
        std::vector<Outline::Polygon> aoOutlines( 1, oSection );
        Outline::SShape               oShape;

        if ( ! Outline::Fill( aoOutlines, oShape, strError ) )
        {
            strError = "can't fill its cross-section: it " + strError;
            return FALSE;
        }
        for ( int nEnd = 0; nEnd < 2; nEnd++ )
        {
            size_t           r = nEnd ? nRings - 1 : 0;
            SV               t = aoTangent[ r ], n = aoNormal[ r ], b = t.Cross( n );
            SV               f = nEnd ? t : t * -1.0;
            std::vector<int> anCap( oShape.aoPoints.size() );

            for ( size_t k = 0; k < oShape.aoPoints.size(); k++ )
            {
                const Outline::SPoint &q = oShape.aoPoints[ k ];
                SV                     p = aoAt[ r ] + ( n * q.x ) + ( b * q.y );

                anCap[ k ] = oOut.AddUV( p.x, p.y, p.z, f.x, f.y, f.z, 0.5 + ( q.x / 2.0 ), 0.5 + ( q.y / 2.0 ) );
            }
            for ( size_t k = 0; k + 2 < oShape.anTriangles.size(); k += 3 )
                oOut.Face( anCap[ (size_t)oShape.anTriangles[ k ] ], anCap[ (size_t)oShape.anTriangles[ k + 1 ] ],
                           anCap[ (size_t)oShape.anTriangles[ k + 2 ] ], f.x, f.y, f.z );
        }
    }
    if ( ! SetGeometry( oOut.aoVertices, oOut.anFaces, TRUE, TRUE, oOut.aoNormals, oOut.anUVs ) )
    {
        strError = "made a bad mesh";
        return FALSE;
    }
    return TRUE;
}

void CSweep::ToJson( IASLib::CObjectNode &oNode )
{
    IASLib::CArrayNode *pPath = oNode.putArray( "path" );

    for ( const CVector &oPoint : m_aoPath )
        RayJson::AddVector( *pPath, oPoint );
    if ( ! m_bSmooth )
        RayJson::PutBool( oNode, "smooth", m_bSmooth );
    if ( m_bClosedPath )
        RayJson::PutBool( oNode, "closed", m_bClosedPath );
    if ( m_bOutline )
        Profile::Write( *oNode.putArray( "outline" ), m_oOutline );
    else
        RayJson::PutNumber( oNode, "radius", m_nRadius );
    if ( m_nSegments != 24 )
        oNode.put( "segments", m_nSegments );
    if ( m_nSteps != 12 )
        oNode.put( "steps", m_nSteps );
    if ( ! m_bCapped )
        RayJson::PutBool( oNode, "capped", m_bCapped );
    RayJson::PutColor( oNode, "color", m_oColor );
    m_oMaterials.ToJson( *oNode.putArray( "materials" ) );
    BumpToJson( oNode );
}

BOOL CSweep::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "path", "smooth", "closed", "radius", "outline", "segments", "steps", "capped",
                                              "color", "materials", "bump", NULL };

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetBool( oNode, "smooth", m_bSmooth, FALSE ) ||
         ! RayJson::GetBool( oNode, "closed", m_bClosedPath, FALSE ) ||
         ! RayJson::GetNumber( oNode, "radius", m_nRadius, FALSE ) ||
         ! RayJson::GetInt( oNode, "segments", m_nSegments, 3, 256, FALSE ) ||
         ! RayJson::GetInt( oNode, "steps", m_nSteps, 1, 256, FALSE ) ||
         ! RayJson::GetBool( oNode, "capped", m_bCapped, FALSE ) ||
         ! RayJson::GetColor( oNode, "color", m_oColor, FALSE ) ||
         ! m_oMaterials.FromJson( oNode, "materials" ) ||
         ! BumpFromJson( oNode ) )
        return FALSE;
    if ( ! ( m_nRadius > 0.0 ) )
        return RayJson::Fail( "radius must be more than 0" );

    const IASLib::CJsonNode *pPath = RayJson::Field( oNode, "path", TRUE );

    if ( ! pPath )
        return FALSE;
    {
        RayJson::CWhere oWhere( "path" );

        if ( ! pPath->isArray() || pPath->size() < 2 )
            return RayJson::Fail( "must be a list of at least two points [x, y, z]" );
        m_aoPath.clear();
        for ( int n = 0; n < pPath->size(); n++ )
        {
            RayJson::CWhere oElement( "", n );
            CVector         oPoint;

            if ( ! RayJson::ToVector( *pPath->get( n ), oPoint ) )
                return FALSE;
            m_aoPath.push_back( oPoint );
        }
    }

    const IASLib::CJsonNode *pOutline = oNode.get( "outline" );

    m_bOutline = pOutline ? TRUE : FALSE;
    if ( pOutline )
    {
        RayJson::CWhere oWhere( "outline" );

        if ( oNode.get( "radius" ) )
            return RayJson::Fail( "give either radius (a round tube) or outline, not both" );
        if ( ! Profile::Read( *pOutline, true, false, 3, m_oOutline ) )
            return FALSE;
    }

    std::string strError;

    if ( ! Build( strError ) )
        return RayJson::Fail( "the sweep " + strError );
    return TRUE;
}
