/*************************************************************************
*   CImplicit                                                            *
*                                                                        *
*   A surface given by a formula. See Implicit.h.                        *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Implicit.h"
#include "SceneJson.h"

#include <map>
#include <math.h>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DYNAMIC_IMPLEMENTS( CImplicit, "implicit" );

CImplicit::CImplicit( void ) :
    m_strFormula( "x^2 + y^2 + z^2 - 1" ),
    m_oBounds( 1.0, 1.0, 1.0 ),
    m_bBoundsNumber( TRUE ),
    m_nSteps( 256 ),
    m_nLipschitz( 0.0 )
{
    Prepare();
}

CImplicit::~CImplicit( void )
{
}

BOOL CImplicit::Set( const CVector &oCenter, const std::string &strFormula, const CVector &oBounds, int nSteps, FLT nLipschitz,
                     FLT nScale )
{
    m_oCenter = oCenter;
    m_strFormula = strFormula;
    m_oBounds = oBounds;
    m_bBoundsNumber = ( m_oBounds.GetX() == m_oBounds.GetY() && m_oBounds.GetY() == m_oBounds.GetZ() ) ? TRUE : FALSE;
    m_nSteps = nSteps;
    m_nLipschitz = nLipschitz;
    m_nScale = nScale;
    return Prepare();
}

BOOL CImplicit::PrepareField( void )
{
    if ( ! ( m_oBounds.GetX() > 0.0 && m_oBounds.GetY() > 0.0 && m_oBounds.GetZ() > 0.0 ) )
        return RayJson::Fail( "bounds must be more than 0" );
    m_anHalf[ 0 ] = m_oBounds.GetX();
    m_anHalf[ 1 ] = m_oBounds.GetY();
    m_anHalf[ 2 ] = m_oBounds.GetZ();
    if ( m_nSteps < 4 || m_nSteps > 1000000 )
        return RayJson::Fail( "steps must be from 4 to 1000000" );
    if ( m_nLipschitz < 0.0 )
        return RayJson::Fail( "lipschitz must be more than 0" );

    std::map<std::string, double> aoParameters;
    std::string                   strError;

    for ( size_t n = 0; n < m_aoParameters.size(); n++ )
        aoParameters[ m_aoParameters[ n ].first ] = m_aoParameters[ n ].second;
    if ( ! m_oExpression.Compile( m_strFormula, aoParameters, strError ) )
    {
        RayJson::CWhere oWhere( "formula" );

        return RayJson::Fail( strError );
    }
    return TRUE;
}

double CImplicit::Field( const double anPoint[ 3 ] ) const
{
    return m_oExpression.Value( anPoint[ 0 ], anPoint[ 1 ], anPoint[ 2 ] );
}

void CImplicit::FieldBounds( double anLow[ 3 ], double anHigh[ 3 ] ) const
{
    for ( int i = 0; i < 3; i++ )
    {
        anHigh[ i ] = m_anHalf[ i ];
        anLow[ i ] = -m_anHalf[ i ];
    }
}

BOOL CImplicit::FieldHit( const double anOrigin[ 3 ], const double anDirection[ 3 ], double nLow, double nHigh, double &nT ) const
{
    double anLow[ 3 ], anHigh[ 3 ];
    double nFrom = nLow, nTo = nHigh;

    FieldBounds( anLow, anHigh );
    if ( ! ClipToBox( anOrigin, anDirection, anLow, anHigh, nFrom, nTo ) )
        return FALSE;

    double nDiagonal = 2.0 * sqrt( ( anHigh[ 0 ] * anHigh[ 0 ] ) + ( anHigh[ 1 ] * anHigh[ 1 ] ) + ( anHigh[ 2 ] * anHigh[ 2 ] ) );
    double nShort = nDiagonal / m_nSteps;
    double t = nFrom;
    double p[ 3 ];
    double f;

    for ( int i = 0; i < 3; i++ )
        p[ i ] = anOrigin[ i ] + ( t * anDirection[ i ] );
    f = Field( p );

        // Each step is the short step, or longer where the Lipschitz bound
        // says the surface can't be nearer. The count is limited, for a
        // formula that isn't finite anywhere along the ray.
    for ( int nStep = 0; t < nTo && nStep < ( 4 * m_nSteps ) + 16; nStep++ )
    {
        double nStepLength = nShort;

        if ( m_nLipschitz > 0.0 && ABS( f ) / m_nLipschitz > nShort )
            nStepLength = ABS( f ) / m_nLipschitz;

        double tNext = ( t + nStepLength < nTo ) ? t + nStepLength : nTo;
        double q[ 3 ];

        for ( int i = 0; i < 3; i++ )
            q[ i ] = anOrigin[ i ] + ( tNext * anDirection[ i ] );

        double fNext = Field( q );

        if ( fNext == 0.0 && tNext > nLow )
        {
            nT = tNext;
            return TRUE;
        }
        if ( ( f < 0.0 ) != ( fNext < 0.0 ) && f == f && fNext == fNext )
        {
                // Bisection between the two steps, down to rounding.
            double a = t, b = tNext, fa = f;

            for ( int nHalf = 0; nHalf < 100; nHalf++ )
            {
                double m = ( a + b ) / 2.0;

                if ( m <= a || m >= b )
                    break;

                double r[ 3 ];

                for ( int i = 0; i < 3; i++ )
                    r[ i ] = anOrigin[ i ] + ( m * anDirection[ i ] );

                double fm = Field( r );

                if ( ( fm < 0.0 ) == ( fa < 0.0 ) )
                {
                    a = m;
                    fa = fm;
                }
                else
                    b = m;
            }
            nT = ( a + b ) / 2.0;
            if ( nT > nLow && nT < nHigh )
                return TRUE;
        }
        t = tNext;
        f = fNext;
    }
    return FALSE;
}

void CImplicit::FieldNormal( const double anPoint[ 3 ], double anNormal[ 3 ] ) const
{
    m_oExpression.Gradient( anPoint[ 0 ], anPoint[ 1 ], anPoint[ 2 ], anNormal );

        // A gradient that isn't finite (a cusp, sqrt at 0): differences.
    if ( ! ( anNormal[ 0 ] == anNormal[ 0 ] && anNormal[ 1 ] == anNormal[ 1 ] && anNormal[ 2 ] == anNormal[ 2 ] ) ||
         ABS( anNormal[ 0 ] ) > 1e300 || ABS( anNormal[ 1 ] ) > 1e300 || ABS( anNormal[ 2 ] ) > 1e300 )
        FieldGradient( anPoint, 1e-6 * ( m_anHalf[ 0 ] + m_anHalf[ 1 ] + m_anHalf[ 2 ] ), anNormal );
}

const char *const *CImplicit::ShapeFields( void )
{
    static const char *const astrFields[] = { "formula", "parameters", "bounds", "steps", "lipschitz", "scale", NULL };

    return astrFields;
}

void CImplicit::FieldToJson( IASLib::CObjectNode &oNode )
{
    oNode.put( "formula", m_strFormula.c_str() );
    if ( ! m_aoParameters.empty() )
    {
        IASLib::CObjectNode *pParameters = oNode.putObject( "parameters" );

        for ( size_t n = 0; n < m_aoParameters.size(); n++ )
            RayJson::PutNumber( *pParameters, m_aoParameters[ n ].first.c_str(), m_aoParameters[ n ].second );
    }
    if ( m_bBoundsNumber )
        RayJson::PutNumber( oNode, "bounds", m_oBounds.GetX() );
    else
        RayJson::PutVector( oNode, "bounds", m_oBounds );
    if ( m_nSteps != 256 )
        RayJson::PutNumber( oNode, "steps", m_nSteps );
    if ( m_nLipschitz > 0.0 )
        RayJson::PutNumber( oNode, "lipschitz", m_nLipschitz );
}

BOOL CImplicit::FieldFromJson( const IASLib::CJsonNode &oNode )
{
    m_aoParameters.clear();
    m_nSteps = 256;
    m_nLipschitz = 0.0;
    if ( ! RayJson::GetString( oNode, "formula", m_strFormula, TRUE ) )
        return FALSE;

    const IASLib::CJsonNode *pParameters = oNode.get( "parameters" );

    if ( pParameters )
    {
        const IASLib::CObjectNode *pObject = dynamic_cast<const IASLib::CObjectNode *>( pParameters );
        RayJson::CWhere            oWhere( "parameters" );

        if ( pObject == NULL )
            return RayJson::Fail( "parameters must be an object of names and numbers" );
        for ( int n = 0; n < pObject->size(); n++ )
        {
            const std::string       &strName = pObject->fieldNameString( n );
            const IASLib::CJsonNode *pValue = pObject->fieldValue( n );
            BOOL                     bName = strName.empty() ? FALSE : TRUE;

            for ( size_t c = 0; c < strName.size(); c++ )
            {
                char ch = strName[ c ];

                if ( ! ( ( ch >= 'a' && ch <= 'z' ) || ( ch >= 'A' && ch <= 'Z' ) || ch == '_' || ( c > 0 && ch >= '0' && ch <= '9' ) ) )
                    bName = FALSE;
            }
            if ( ! bName )
                return RayJson::Fail( "\"" + strName + "\" isn't a name (letters, digits and _, not starting with a digit)" );
            if ( CExpression::IsReserved( strName ) )
                return RayJson::Fail( "\"" + strName + "\" is taken by the formula language" );
            if ( ! pValue->isNumber() )
                return RayJson::Fail( "parameter \"" + strName + "\" must be a number" );
            m_aoParameters.push_back( std::make_pair( strName, pValue->doubleValue() ) );
        }
    }

    const IASLib::CJsonNode *pBounds = RayJson::Field( oNode, "bounds", TRUE );

    if ( pBounds == NULL )
        return FALSE;
    if ( pBounds->isNumber() )
    {
        FLT nHalf = (FLT)pBounds->doubleValue();

        m_oBounds.Set( nHalf, nHalf, nHalf );
        m_bBoundsNumber = TRUE;
    }
    else
    {
        if ( ! RayJson::GetVector( oNode, "bounds", m_oBounds, TRUE ) )
            return FALSE;
        m_bBoundsNumber = FALSE;
    }

    FLT nSteps = 256.0;

    if ( ! RayJson::GetNumber( oNode, "steps", nSteps, FALSE ) || ! RayJson::GetNumber( oNode, "lipschitz", m_nLipschitz, FALSE ) )
        return FALSE;
    if ( nSteps != (int)nSteps )
    {
        RayJson::CWhere oWhere( "steps" );

        return RayJson::Fail( "steps must be a whole number" );
    }
    m_nSteps = (int)nSteps;
    return TRUE;
}

void CImplicit::ToJson( IASLib::CObjectNode &oNode )
{
    CLocalShape::ToJson( oNode );
}

BOOL CImplicit::FromJson( const IASLib::CJsonNode &oNode )
{
    return CLocalShape::FromJson( oNode );
}
