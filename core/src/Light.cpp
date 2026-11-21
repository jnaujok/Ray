#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Light.h"
#include "Sampling.h"
#include "SceneJson.h"

#include <math.h>
#include <string.h>
#include <algorithm>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CLight::CLight( void ) :
m_nAttenuation( ATTENUATION_NONE ),
m_nDistance( 1.0 ),
m_nFilterSize( 1.0 ),
m_nProfile( PROFILE_SMOOTHSTEP ),
m_nExponent( 8.0 ),
m_nSpread( 0.0 ),
m_nHotspot( 0.0 ),
m_nRim( 0.35 )
{
}

CLight::CLight( FLT nRed, FLT nGreen, FLT nBlue, FLT nX, FLT nY, FLT nZ ) :
m_oColor( nRed, nGreen, nBlue ), m_oPosition( nX, nY, nZ ),
m_nAttenuation( ATTENUATION_NONE ),
m_nDistance( 1.0 ),
m_nFilterSize( 1.0 ),
m_nProfile( PROFILE_SMOOTHSTEP ),
m_nExponent( 8.0 ),
m_nSpread( 0.0 ),
m_nHotspot( 0.0 ),
m_nRim( 0.35 )
{
}

CLight::~CLight( void )
{
}

namespace
{
    const char *const s_astrAttenuations[] = { "none", "inverse", "inverseSquare" };
    const char *const s_astrProfiles[] = { "smoothstep", "linear", "cosine", "gaussian", "fresnel", "table" };

    inline FLT Smooth( FLT nT )
    {
        return nT * nT * ( 3.0 - ( 2.0 * nT ) );
    }
}

FLT CLight::ProfileFactor( FLT nCos, FLT nAngle, FLT nFalloff ) const
{
    FLT nOuter = (FLT)cos( (double)( nAngle * PI / 180.0 ) );
    FLT nInner = (FLT)cos( (double)( ( nAngle - nFalloff ) * PI / 180.0 ) );
    FLT nEdge;

    if ( nCos <= nOuter )
        return 0.0;
    if ( nCos >= nInner )
        nEdge = 1.0;
    else
    {
        FLT nT = ( nCos - nOuter ) / ( nInner - nOuter );

        nEdge = ( m_nProfile == PROFILE_LINEAR ) ? nT : Smooth( nT );
    }
    if ( m_nProfile == PROFILE_SMOOTHSTEP || m_nProfile == PROFILE_LINEAR )
        return nEdge;

        // The beam's shape inside the cone, by the angle from the axis.
    FLT nTheta = (FLT)( acos( (double)(std::min)( (FLT)1.0, nCos ) ) * 180.0 / PI );
    FLT nShape = 1.0;

    switch ( m_nProfile )
    {
        case PROFILE_COSINE:
            nShape = (FLT)pow( (double)nCos, (double)m_nExponent );
            break;
        case PROFILE_GAUSSIAN:
        {
            FLT nWidth = ( m_nSpread > 0.0 ) ? m_nSpread : ( nAngle / 3.0 );

            nShape = (FLT)exp( -0.5 * (double)( ( nTheta / nWidth ) * ( nTheta / nWidth ) ) );
            break;
        }
        case PROFILE_FRESNEL:
        {
                // A bright center that falls smoothly, over as wide again, to
                // the rim's level, which holds out to the edge.
            FLT nCenter = ( m_nHotspot > 0.0 ) ? m_nHotspot : ( nAngle / 3.0 );
            FLT nT = ( nTheta - nCenter ) / nCenter;

            if ( nT <= 0.0 )
                nShape = 1.0;
            else if ( nT >= 1.0 )
                nShape = m_nRim;
            else
                nShape = 1.0 - ( ( 1.0 - m_nRim ) * Smooth( nT ) );
            break;
        }
        case PROFILE_TABLE:
        {
            if ( m_anTable.size() == 1 )
                nShape = m_anTable[ 0 ];
            else if ( ! m_anTable.empty() )
            {
                FLT     nPlace = ( nTheta / nAngle ) * (FLT)( m_anTable.size() - 1 );
                int     nIndex = (int)floor( nPlace );

                if ( nIndex >= (int)m_anTable.size() - 1 )
                    nShape = m_anTable.back();
                else
                {
                    FLT nPart = nPlace - nIndex;

                    nShape = m_anTable[ nIndex ] + ( nPart * ( m_anTable[ nIndex + 1 ] - m_anTable[ nIndex ] ) );
                }
            }
            break;
        }
        default:
            break;
    }
    return nShape * nEdge;
}

void CLight::FilterAt( const CVector &oPoint, const CVector &oToLight, STexturePoint &oAt ) const
{
        // All around: the direction from the light, as a point on a sphere
        // of the filter's size (for solid textures) and as longitude and
        // latitude (for images).
    CVector oFromLight( oToLight );

    (void)oPoint;
    oFromLight = oFromLight * -1.0;
    oFromLight.Normalize();
    oAt.oPoint = oFromLight * m_nFilterSize;
    oAt.nU = (FLT)( ( atan2( (double)oFromLight.GetZ(), (double)oFromLight.GetX() ) / ( 2.0 * PI ) ) + 0.5 );
    oAt.nV = (FLT)( acos( (double)(std::max)( (FLT)-1.0, (std::min)( (FLT)1.0, oFromLight.GetY() ) ) ) / PI );
    oAt.bHasUV = TRUE;
}

CColor CLight::ShadeAt( const CVector &oPoint, const CVector &oToLight )
{
    CColor oLight( GetShade( oToLight ) );

    if ( m_nAttenuation != ATTENUATION_NONE )
    {
        CVector oTo( oToLight );
        FLT     nRatio = m_nDistance / (std::max)( oTo.Magnitude(), (FLT)1.0e-9 );

        oLight = oLight * ( ( m_nAttenuation == ATTENUATION_INVERSE ) ? nRatio : ( nRatio * nRatio ) );
    }
    if ( m_pFilter )
    {
        STexturePoint oAt;

        oAt.bHasUV = FALSE;
        oAt.bHasColor = FALSE;
        oAt.nU = oAt.nV = 0.0;

            // Projected from the light's middle (its position, or along a
            // sun's direction), not the point of an area light this sample
            // comes from, so the picture stays sharp, as a projector's slide
            // does, instead of breaking into grain.
        CVector oFrom( oPoint );
        CVector oToMiddle( ToLight( oFrom ) );

        FilterAt( oPoint, oToMiddle, oAt );

        CColor oFilter( m_pFilter->ColorAt( oAt ) );

        oLight = CColor( oLight.GetR() * oFilter.GetR(), oLight.GetG() * oFilter.GetG(), oLight.GetB() * oFilter.GetB() );
    }
    return oLight;
}

void CLight::OptionsToJson( IASLib::CObjectNode &oNode, BOOL bProfile ) const
{
    if ( m_nAttenuation != ATTENUATION_NONE )
    {
        oNode.put( "attenuation", s_astrAttenuations[ m_nAttenuation ] );
        RayJson::PutNumber( oNode, "distance", m_nDistance );
    }
    if ( bProfile && m_nProfile != PROFILE_SMOOTHSTEP )
    {
        oNode.put( "profile", s_astrProfiles[ m_nProfile ] );
        switch ( m_nProfile )
        {
            case PROFILE_COSINE:
                RayJson::PutNumber( oNode, "exponent", m_nExponent );
                break;
            case PROFILE_GAUSSIAN:
                if ( m_nSpread > 0.0 )
                    RayJson::PutNumber( oNode, "spread", m_nSpread );
                break;
            case PROFILE_FRESNEL:
                if ( m_nHotspot > 0.0 )
                    RayJson::PutNumber( oNode, "hotspot", m_nHotspot );
                RayJson::PutNumber( oNode, "rim", m_nRim );
                break;
            case PROFILE_TABLE:
            {
                IASLib::CArrayNode *pTable = oNode.putArray( "table" );

                for ( size_t nIndex = 0; nIndex < m_anTable.size(); nIndex++ )
                    pTable->add( RayJson::Rounded( (double)m_anTable[ nIndex ] ) );
                break;
            }
            default:
                break;
        }
    }
    if ( m_pFilter )
    {
        IASLib::CObjectNode *pFilter = oNode.putObject( "filter" );

        m_pFilter->Write( *pFilter, "texture" );
        RayJson::PutNumber( *pFilter, "size", m_nFilterSize );
    }
}

namespace
{
    BOOL PickName( const IASLib::CJsonNode &oNode, const char *strKey, const char *const *astrNames, int nNames, int &nValue,
                   const char *strChoices )
    {
        std::string strName;

        if ( ! RayJson::GetString( oNode, strKey, strName, FALSE ) )
            return FALSE;
        if ( strName.empty() )
            return TRUE;
        for ( int nIndex = 0; nIndex < nNames; nIndex++ )
        {
            if ( strName == astrNames[ nIndex ] )
            {
                nValue = nIndex;
                return TRUE;
            }
        }

        RayJson::CWhere oWhere( strKey );

        return RayJson::Fail( std::string( "must be " ) + strChoices );
    }
}

BOOL CLight::OptionsFromJson( const IASLib::CJsonNode &oNode, BOOL bAttenuation, BOOL bProfile )
{
    if ( bAttenuation )
    {
        if ( ! PickName( oNode, "attenuation", s_astrAttenuations, 3, m_nAttenuation,
                         "\"none\", \"inverse\" or \"inverseSquare\"" ) ||
             ! RayJson::GetNumber( oNode, "distance", m_nDistance, FALSE ) )
            return FALSE;
        if ( ! ( m_nDistance > 0.0 ) )
        {
            RayJson::CWhere oWhere( "distance" );

            return RayJson::Fail( "must be more than 0" );
        }
    }
    if ( bProfile )
    {
        if ( ! PickName( oNode, "profile", s_astrProfiles, 6, m_nProfile,
                         "\"smoothstep\", \"linear\", \"cosine\", \"gaussian\", \"fresnel\" or \"table\"" ) ||
             ! RayJson::GetNumber( oNode, "exponent", m_nExponent, FALSE ) ||
             ! RayJson::GetNumber( oNode, "spread", m_nSpread, FALSE ) ||
             ! RayJson::GetNumber( oNode, "hotspot", m_nHotspot, FALSE ) ||
             ! RayJson::GetNumber( oNode, "rim", m_nRim, FALSE ) )
            return FALSE;
        if ( ! ( m_nExponent >= 0.0 && m_nExponent <= 1000.0 ) )
        {
            RayJson::CWhere oWhere( "exponent" );

            return RayJson::Fail( "must be from 0 to 1000" );
        }
        if ( ! ( m_nSpread >= 0.0 ) || ! ( m_nHotspot >= 0.0 ) )
            return RayJson::Fail( "spread and hotspot must be 0 or more degrees" );
        if ( ! ( m_nRim >= 0.0 && m_nRim <= 1.0 ) )
        {
            RayJson::CWhere oWhere( "rim" );

            return RayJson::Fail( "must be from 0 to 1" );
        }

        const IASLib::CJsonNode *pTable = oNode.get( "table" );

        if ( pTable )
        {
            RayJson::CWhere oWhere( "table" );

            if ( ! pTable->isArray() || pTable->size() < 1 )
                return RayJson::Fail( "must be an array of one or more numbers (brightness from the axis to the edge)" );
            m_anTable.clear();
            for ( int nIndex = 0; nIndex < pTable->size(); nIndex++ )
            {
                const IASLib::CJsonNode *pValue = pTable->get( nIndex );

                if ( ! pValue->isNumber() || ! ( pValue->asDouble() >= 0.0 ) )
                    return RayJson::Fail( "must hold numbers of 0 or more" );
                m_anTable.push_back( (FLT)pValue->asDouble() );
            }
        }
        if ( m_nProfile == PROFILE_TABLE && m_anTable.empty() )
            return RayJson::Fail( "a \"table\" profile needs a \"table\" of brightnesses" );
    }

    const IASLib::CJsonNode *pFilter = oNode.get( "filter" );

    if ( pFilter )
    {
        static const char *const astrFields[] = { "texture", "size", NULL };
        RayJson::CWhere          oWhere( "filter" );

        if ( ! pFilter->isObject() )
            return RayJson::Fail( "must be an object with a \"texture\"" );
        if ( ! RayJson::OnlyFields( *pFilter, astrFields ) ||
             ! RayJson::GetNumber( *pFilter, "size", m_nFilterSize, FALSE ) )
            return FALSE;
        if ( ! ( m_nFilterSize > 0.0 ) )
        {
            RayJson::CWhere oSize( "size" );

            return RayJson::Fail( "must be more than 0" );
        }

        const IASLib::CJsonNode *pTexture = pFilter->get( "texture" );

        if ( pTexture == NULL )
            return RayJson::Fail( "needs a \"texture\"" );

        RayJson::CWhere oTexture( "texture" );
        CTexture       *pNew = CTexture::Read( *pTexture );

        if ( pNew == NULL )
            return FALSE;
        m_pFilter.reset( pNew );
    }
    return TRUE;
}

BOOL CLight::OnlyLightFields( const IASLib::CJsonNode &oNode, const char *const *astrOwn, BOOL bAttenuation, BOOL bProfile )
{
    std::vector<const char *>   astrFields;

    for ( int nIndex = 0; astrOwn[ nIndex ]; nIndex++ )
        astrFields.push_back( astrOwn[ nIndex ] );
    astrFields.push_back( "filter" );
    if ( bAttenuation )
    {
        astrFields.push_back( "attenuation" );
        astrFields.push_back( "distance" );
    }
    if ( bProfile )
    {
        static const char *const astrProfile[] = { "profile", "exponent", "spread", "hotspot", "rim", "table", NULL };

        for ( int nIndex = 0; astrProfile[ nIndex ]; nIndex++ )
            astrFields.push_back( astrProfile[ nIndex ] );
    }
    astrFields.push_back( NULL );
    return RayJson::OnlyFields( oNode, &astrFields[ 0 ] );
}
