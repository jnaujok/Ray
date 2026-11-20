#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "SolidTexture.h"
#include "SceneJson.h"

#include <math.h>
#include <stdint.h>

#define MAXIMUM_OCTAVES     12

namespace
{
        // SplitMix64, which the samplers use too: a seed's stream of numbers.
    uint64_t NextMix( uint64_t &nState )
    {
        uint64_t nValue = ( nState += 0x9E3779B97F4A7C15ULL );

        nValue = ( nValue ^ ( nValue >> 30 ) ) * 0xBF58476D1CE4E5B9ULL;
        nValue = ( nValue ^ ( nValue >> 27 ) ) * 0x94D049BB133111EBULL;
        return nValue ^ ( nValue >> 31 );
    }

        // Perlin's fade, 6t^5 - 15t^4 + 10t^3: flat at both ends, so the
        // noise is smooth across the lattice.
    inline FLT Fade( FLT nT )
    {
        return nT * nT * nT * ( ( nT * ( ( nT * 6.0 ) - 15.0 ) ) + 10.0 );
    }

    inline FLT Lerp( FLT nT, FLT nA, FLT nB )
    {
        return nA + ( nT * ( nB - nA ) );
    }

        // The dot product with one of twelve gradients (the middles of a
        // cube's edges; four repeat to make sixteen).
    inline FLT Gradient( int nHash, FLT nX, FLT nY, FLT nZ )
    {
        int nWhich = nHash & 15;
        FLT nU = ( nWhich < 8 ) ? nX : nY;
        FLT nV = ( nWhich < 4 ) ? nY : ( ( nWhich == 12 || nWhich == 14 ) ? nX : nZ );

        return ( ( nWhich & 1 ) ? -nU : nU ) + ( ( nWhich & 2 ) ? -nV : nV );
    }
}

CSolidTexture::CSolidTexture( const FLT *anColors, int nColors, FLT nTurbulence ) :
    m_nScale( 1.0 ),
    m_oOrigin( 0.0, 0.0, 0.0 ),
    m_nTurbulence( nTurbulence ),
    m_nOctaves( 6 ),
    m_nSeed( 0 )
{
    for ( int nIndex = 0; nIndex < nColors; nIndex++ )
        m_aoColors.push_back( CColor( anColors[ nIndex * 3 ], anColors[ ( nIndex * 3 ) + 1 ], anColors[ ( nIndex * 3 ) + 2 ] ) );
    Shuffle();
}

void CSolidTexture::Shuffle( void )
{
    uint64_t nState = (uint64_t)(uint32_t)m_nSeed;

    for ( int nIndex = 0; nIndex < 256; nIndex++ )
        m_anPermute[ nIndex ] = (unsigned char)nIndex;
        // Fisher-Yates, from the top down.
    for ( int nIndex = 255; nIndex > 0; nIndex-- )
    {
        int             nOther = (int)( NextMix( nState ) % (uint64_t)( nIndex + 1 ) );
        unsigned char   nSwap = m_anPermute[ nIndex ];

        m_anPermute[ nIndex ] = m_anPermute[ nOther ];
        m_anPermute[ nOther ] = nSwap;
    }
    for ( int nIndex = 0; nIndex < 256; nIndex++ )
        m_anPermute[ nIndex + 256 ] = m_anPermute[ nIndex ];
}

FLT CSolidTexture::Noise( FLT nX, FLT nY, FLT nZ ) const
{
    FLT     nFloorX = floor( nX );
    FLT     nFloorY = floor( nY );
    FLT     nFloorZ = floor( nZ );
        // The lattice cell, wrapped to 256 (two's complement keeps negative
        // cells in step with positive ones).
    int     nCellX = (int)( (long long)nFloorX & 255 );
    int     nCellY = (int)( (long long)nFloorY & 255 );
    int     nCellZ = (int)( (long long)nFloorZ & 255 );

    nX -= nFloorX;
    nY -= nFloorY;
    nZ -= nFloorZ;

    FLT     nU = Fade( nX );
    FLT     nV = Fade( nY );
    FLT     nW = Fade( nZ );

    int     nA = m_anPermute[ nCellX ] + nCellY;
    int     nAA = m_anPermute[ nA ] + nCellZ;
    int     nAB = m_anPermute[ nA + 1 ] + nCellZ;
    int     nB = m_anPermute[ nCellX + 1 ] + nCellY;
    int     nBA = m_anPermute[ nB ] + nCellZ;
    int     nBB = m_anPermute[ nB + 1 ] + nCellZ;

    return Lerp( nW, Lerp( nV, Lerp( nU, Gradient( m_anPermute[ nAA ], nX, nY, nZ ),
                                         Gradient( m_anPermute[ nBA ], nX - 1.0, nY, nZ ) ),
                               Lerp( nU, Gradient( m_anPermute[ nAB ], nX, nY - 1.0, nZ ),
                                         Gradient( m_anPermute[ nBB ], nX - 1.0, nY - 1.0, nZ ) ) ),
                     Lerp( nV, Lerp( nU, Gradient( m_anPermute[ nAA + 1 ], nX, nY, nZ - 1.0 ),
                                         Gradient( m_anPermute[ nBA + 1 ], nX - 1.0, nY, nZ - 1.0 ) ),
                               Lerp( nU, Gradient( m_anPermute[ nAB + 1 ], nX, nY - 1.0, nZ - 1.0 ),
                                         Gradient( m_anPermute[ nBB + 1 ], nX - 1.0, nY - 1.0, nZ - 1.0 ) ) ) );
}

FLT CSolidTexture::Turbulence( FLT nX, FLT nY, FLT nZ ) const
{
    FLT nSum = 0.0;
    FLT nSize = 1.0;

    for ( int nOctave = 0; nOctave < m_nOctaves; nOctave++ )
    {
        nSum += fabs( Noise( nX * nSize, nY * nSize, nZ * nSize ) ) / nSize;
        nSize *= 2.0;
    }
    return nSum;
}

CColor CSolidTexture::RampAt( FLT nAt ) const
{
    int nLast = (int)m_aoColors.size() - 1;

    if ( ! ( nAt > 0.0 ) )
        return m_aoColors[ 0 ];
    if ( nAt >= 1.0 )
        return m_aoColors[ nLast ];

    FLT     nPlace = nAt * nLast;
    int     nIndex = (int)floor( nPlace );

    if ( nIndex >= nLast )
        return m_aoColors[ nLast ];

    FLT     nPart = nPlace - nIndex;
    CColor  oFrom( m_aoColors[ nIndex ] );
    CColor  oTo( m_aoColors[ nIndex + 1 ] );

    return CColor( Lerp( nPart, oFrom.GetR(), oTo.GetR() ),
                   Lerp( nPart, oFrom.GetG(), oTo.GetG() ),
                   Lerp( nPart, oFrom.GetB(), oTo.GetB() ) );
}

CColor CSolidTexture::ColorAt( const STexturePoint &oAt ) const
{
    CVector oPoint( oAt.oPoint );
    CVector oOrigin( m_oOrigin );

    return RampAt( Pattern( ( oPoint.GetX() - oOrigin.GetX() ) / m_nScale,
                            ( oPoint.GetY() - oOrigin.GetY() ) / m_nScale,
                            ( oPoint.GetZ() - oOrigin.GetZ() ) / m_nScale ) );
}

void CSolidTexture::ToJson( IASLib::CObjectNode &oNode ) const
{
    IASLib::CArrayNode *pColors = oNode.putArray( "colors" );

    for ( size_t nIndex = 0; nIndex < m_aoColors.size(); nIndex++ )
    {
        CColor oColor( m_aoColors[ nIndex ] );
        pColors->addArray()->add( (double)oColor.GetR() )->add( (double)oColor.GetG() )->add( (double)oColor.GetB() );
    }
    RayJson::PutNumber( oNode, "scale", m_nScale );
    RayJson::PutVector( oNode, "origin", m_oOrigin );
    RayJson::PutNumber( oNode, "turbulence", m_nTurbulence );
    oNode.put( "octaves", m_nOctaves );
    oNode.put( "seed", m_nSeed );
    ExtraToJson( oNode );
}

BOOL CSolidTexture::FromJson( const IASLib::CJsonNode &oNode )
{
    std::vector<const char *>   astrFields;
    const char *const          *astrExtra = ExtraFields();
    static const char *const    astrBase[] = { "colors", "scale", "origin", "turbulence", "octaves", "seed" };

    for ( size_t nIndex = 0; nIndex < sizeof( astrBase ) / sizeof( astrBase[ 0 ] ); nIndex++ )
        astrFields.push_back( astrBase[ nIndex ] );
    for ( int nIndex = 0; astrExtra && astrExtra[ nIndex ]; nIndex++ )
        astrFields.push_back( astrExtra[ nIndex ] );
    astrFields.push_back( NULL );

    if ( ! RayJson::OnlyFields( oNode, &astrFields[ 0 ] ) ||
         ! RayJson::GetNumber( oNode, "scale", m_nScale, FALSE ) ||
         ! RayJson::GetVector( oNode, "origin", m_oOrigin, FALSE ) ||
         ! RayJson::GetNumber( oNode, "turbulence", m_nTurbulence, FALSE ) ||
         ! RayJson::GetInt( oNode, "octaves", m_nOctaves, 1, MAXIMUM_OCTAVES, FALSE ) ||
         ! RayJson::GetInt( oNode, "seed", m_nSeed, 0, 2147483647, FALSE ) )
        return FALSE;
    if ( ! ( m_nScale > 0.0 ) )
        return RayJson::Fail( "scale must be positive" );
    if ( ! ( m_nTurbulence >= 0.0 ) )
        return RayJson::Fail( "turbulence can't be negative" );

    const IASLib::CJsonNode *pColors = oNode.get( "colors" );

    if ( pColors )
    {
        RayJson::CWhere oWhere( "colors" );

        if ( ! pColors->isArray() || pColors->size() < 2 )
            return RayJson::Fail( "must be an array of two or more colors" );
        m_aoColors.clear();
        for ( int nIndex = 0; nIndex < (int)pColors->size(); nIndex++ )
        {
            RayJson::CWhere oElement( "", nIndex );
            CVector         oColor;

            if ( ! RayJson::ToVector( *pColors->get( nIndex ), oColor ) )
                return FALSE;
            m_aoColors.push_back( CColor( oColor.GetX(), oColor.GetY(), oColor.GetZ() ) );
        }
    }
    Shuffle();
    return ExtraFromJson( oNode );
}
