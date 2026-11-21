#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "FilmColorModel.h"
#include "SrgbColorModel.h"
#include "ImageFile.h"
#include "Color.h"
#include "SceneJson.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

namespace
{
        // The Kodak films flown on Apollo (approximate; see the header).
    const CFilmColorModel::SStock s_aoStocks[] =
    {
        { "ektachrome-ms", "Kodak Ektachrome MS (SO-368), ISO 64 color reversal: Apollo's workhorse",
          TRUE, 64.0,
          { { 0.92, 0.08, 0.00 }, { 0.05, 0.89, 0.06 }, { 0.00, 0.04, 0.96 } },
          { 1.75, 1.80, 1.85 }, 0.18, 3.30, 0.80, 0.011,
          0.10, 0.006, 0.030, 0.080, { 1.00, 0.45, 0.15 } },
        { "ektachrome-ef", "Kodak Ektachrome EF (SO-168), ISO 160 color reversal for low light",
          TRUE, 160.0,
          { { 0.90, 0.10, 0.00 }, { 0.06, 0.87, 0.07 }, { 0.00, 0.05, 0.95 } },
          { 1.65, 1.70, 1.70 }, 0.22, 3.20, 0.82, 0.015,
          0.12, 0.008, 0.040, 0.100, { 1.00, 0.45, 0.15 } },
        { "panatomic-x", "Kodak Panatomic-X (3400), ISO 80 fine-grain black and white, as a print",
          FALSE, 80.0,
          { { 0.22, 0.60, 0.18 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } },
          { 1.55, 1.55, 1.55 }, 0.10, 2.40, 0.75, 0.006,
          0.06, 0.004, 0.015, 0.060, { 1.00, 1.00, 1.00 } },
        { "high-speed-2485", "Kodak High Speed Recording (2485), ISO 6000 black and white, as a print",
          FALSE, 6000.0,
          { { 0.40, 0.45, 0.15 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } },
          { 1.25, 1.25, 1.25 }, 0.20, 2.00, 0.80, 0.045,
          0.20, 0.012, 0.080, 0.150, { 1.00, 1.00, 1.00 } },
    };

        // A grain value for a pixel and layer: a normal deviate from a hash,
        // the same on every run.
    uint64_t Mix( uint64_t nValue )
    {
        nValue ^= nValue >> 30;
        nValue *= 0xbf58476d1ce4e5b9ull;
        nValue ^= nValue >> 27;
        nValue *= 0x94d049bb133111ebull;
        nValue ^= nValue >> 31;
        return nValue;
    }

    double Normal( int nSeed, int nX, int nY, int nLayer )
    {
        uint64_t nHash = Mix( Mix( Mix( Mix( (uint64_t)(uint32_t)nSeed ) ^ (uint64_t)(uint32_t)nX ) ^ (uint64_t)(uint32_t)nY ) ^
                              (uint64_t)(uint32_t)nLayer );
        double   dU = ( (double)( nHash >> 11 ) + 0.5 ) / 9007199254740992.0;
        double   dV = ( (double)( Mix( nHash ) >> 11 ) + 0.5 ) / 9007199254740992.0;

        return sqrt( -2.0 * log( dU ) ) * cos( 2.0 * 3.14159265358979323846 * dV );
    }
}

const CFilmColorModel::SStock *CFilmColorModel::GetStocks( int &nCount )
{
    nCount = (int)( sizeof( s_aoStocks ) / sizeof( s_aoStocks[ 0 ] ) );
    return s_aoStocks;
}

CFilmColorModel::CFilmColorModel( void ) :
    m_strStock( s_aoStocks[ 0 ].strName ),
    m_pStock( &s_aoStocks[ 0 ] ),
    m_nIso( 0.0 ),
    m_nShutter( 1.0 / 250.0 ),
    m_nFStop( 11.0 ),
    m_nWidth( 55.0 ),
    m_nGrain( 1.0 ),
    m_nSpread( 1.0 ),
    m_nSeed( 0 )
{
}

BOOL CFilmColorModel::SetStock( const char *strName )
{
    int             nCount;
    const SStock   *aoStocks = GetStocks( nCount );

    for ( int nIndex = 0; nIndex < nCount; nIndex++ )
    {
        if ( strcmp( aoStocks[ nIndex ].strName, strName ) == 0 )
        {
            m_pStock = &aoStocks[ nIndex ];
            m_strStock = strName;
            return TRUE;
        }
    }
    return FALSE;
}

FLT CFilmColorModel::ExposureFactor( void ) const
{
    FLT nIso = ( m_nIso > 0.0 ) ? m_nIso : m_pStock->nIso;

    return 256.0 * m_nShutter * nIso / ( m_nFStop * m_nFStop );
}

void CFilmColorModel::LayerExposures( const CColor &oColor, FLT anExposure[ 3 ] ) const
{
    CColor  oSource( oColor );
    FLT     anRGB[ 3 ] = { oSource.GetR(), oSource.GetG(), oSource.GetB() };
    FLT     nFactor = ExposureFactor();
    int     nLayers = m_pStock->bColor ? 3 : 1;

    for ( int nLayer = 0; nLayer < 3; nLayer++ )
    {
        FLT nSum = 0.0;

        if ( nLayer < nLayers )
        {
            for ( int nChannel = 0; nChannel < 3; nChannel++ )
            {
                if ( anRGB[ nChannel ] > 0.0 )
                    nSum += m_pStock->anSensitivity[ nLayer ][ nChannel ] * anRGB[ nChannel ];
            }
        }
        anExposure[ nLayer ] = nSum * nFactor;
    }
}

FLT CFilmColorModel::Density( int nLayer, FLT nExposure ) const
{
        // A logistic curve in the logarithm of exposure, falling from the
        // maximum density to the base (a positive: a slide, or a print of a
        // negative), as steep as the layer's gamma where it is steepest,
        // and through the mid density at mid gray.
    const SStock   &oStock = *m_pStock;
    double          dRange = (double)( oStock.nMaxDensity - oStock.nMinDensity );
    double          dSteep = 4.0 * (double)oStock.anGamma[ ( nLayer >= 0 && nLayer < 3 ) ? nLayer : 0 ] / dRange;
    double          dCenter = -log( dRange / (double)( oStock.nMidDensity - oStock.nMinDensity ) - 1.0 ) / dSteep;
    double          dLog = log10( ( (double)nExposure > 1e-12 ) ? (double)nExposure / 0.18 : 1e-12 / 0.18 );

    return (FLT)( oStock.nMinDensity + dRange / ( 1.0 + exp( dSteep * ( dLog - dCenter ) ) ) );
}

FLT CFilmColorModel::Transmission( FLT nDensity ) const
{
    return (FLT)pow( 10.0, -(double)( nDensity - m_pStock->nMinDensity ) );
}

void CFilmColorModel::DensitiesToRGB( const FLT anDensity[ 3 ], BYTE &nRed, BYTE &nGreen, BYTE &nBlue ) const
{
    if ( ! m_pStock->bColor )
    {
        BYTE nGray = ToByte( CSrgbColorModel::Encode( Transmission( anDensity[ 0 ] ) ) );

        nRed = nGreen = nBlue = nGray;
        return;
    }
    nRed = ToByte( CSrgbColorModel::Encode( Transmission( anDensity[ 0 ] ) ) );
    nGreen = ToByte( CSrgbColorModel::Encode( Transmission( anDensity[ 1 ] ) ) );
    nBlue = ToByte( CSrgbColorModel::Encode( Transmission( anDensity[ 2 ] ) ) );
}

void CFilmColorModel::ToRGB( const CColor &oColor, BYTE &nRed, BYTE &nGreen, BYTE &nBlue ) const
{
    FLT anExposure[ 3 ];
    FLT anDensity[ 3 ];

    LayerExposures( oColor, anExposure );
    for ( int nLayer = 0; nLayer < 3; nLayer++ )
        anDensity[ nLayer ] = Density( nLayer, anExposure[ nLayer ] );
    DensitiesToRGB( anDensity, nRed, nGreen, nBlue );
}

CColor CFilmColorModel::ToLinear( const CColor &oColor ) const
{
    return oColor;
}

std::vector<FLT> CFilmColorModel::Kernel( FLT nSigma )
{
    std::vector<FLT> anKernel;

    if ( ! ( nSigma > 0.0 ) )
    {
        anKernel.push_back( 1.0 );
        return anKernel;
    }

    int     nRadius = (int)ceil( 3.0 * (double)nSigma );
    double  dSum = 0.0;

    if ( nRadius < 1 )
        nRadius = 1;
    for ( int nOffset = 0; nOffset <= nRadius; nOffset++ )
    {
        double dValue = exp( -0.5 * ( (double)nOffset * nOffset ) / ( (double)nSigma * nSigma ) );

        anKernel.push_back( (FLT)dValue );
        dSum += ( nOffset == 0 ) ? dValue : 2.0 * dValue;
    }
    for ( size_t nIndex = 0; nIndex < anKernel.size(); nIndex++ )
        anKernel[ nIndex ] = (FLT)( (double)anKernel[ nIndex ] / dSum );
    return anKernel;
}

namespace
{
        // A separable blur with a one-sided kernel, edges held (so every
        // pixel's weights still sum to one).
    void Blur( const std::vector<FLT> &anIn, std::vector<FLT> &anOut, int nWidth, int nHeight, const std::vector<FLT> &anKernel )
    {
        int                 nRadius = (int)anKernel.size() - 1;
        std::vector<FLT>    anRow( anIn.size() );

        for ( int nY = 0; nY < nHeight; nY++ )
        {
            for ( int nX = 0; nX < nWidth; nX++ )
            {
                double dSum = 0.0;

                for ( int nOffset = -nRadius; nOffset <= nRadius; nOffset++ )
                {
                    int nAt = nX + nOffset;

                    nAt = ( nAt < 0 ) ? 0 : ( ( nAt >= nWidth ) ? nWidth - 1 : nAt );
                    dSum += (double)anKernel[ (size_t)( ( nOffset < 0 ) ? -nOffset : nOffset ) ] *
                            (double)anIn[ (size_t)nY * (size_t)nWidth + (size_t)nAt ];
                }
                anRow[ (size_t)nY * (size_t)nWidth + (size_t)nX ] = (FLT)dSum;
            }
        }
        anOut.assign( anIn.size(), 0.0 );
        for ( int nY = 0; nY < nHeight; nY++ )
        {
            for ( int nX = 0; nX < nWidth; nX++ )
            {
                double dSum = 0.0;

                for ( int nOffset = -nRadius; nOffset <= nRadius; nOffset++ )
                {
                    int nAt = nY + nOffset;

                    nAt = ( nAt < 0 ) ? 0 : ( ( nAt >= nHeight ) ? nHeight - 1 : nAt );
                    dSum += (double)anKernel[ (size_t)( ( nOffset < 0 ) ? -nOffset : nOffset ) ] *
                            (double)anRow[ (size_t)nAt * (size_t)nWidth + (size_t)nX ];
                }
                anOut[ (size_t)nY * (size_t)nWidth + (size_t)nX ] = (FLT)dSum;
            }
        }
    }
}

void CFilmColorModel::Spread( std::vector<FLT> &anLayer, int nWidth, int nHeight,
                              FLT nShareA, FLT nSigmaA, FLT nShareB, FLT nSigmaB )
{
    std::vector<FLT> anA;
    std::vector<FLT> anB;

    if ( nWidth <= 0 || nHeight <= 0 || ( ! ( nShareA > 0.0 ) && ! ( nShareB > 0.0 ) ) )
        return;
    if ( nShareA > 0.0 )
        Blur( anLayer, anA, nWidth, nHeight, Kernel( nSigmaA ) );
    if ( nShareB > 0.0 )
        Blur( anLayer, anB, nWidth, nHeight, Kernel( nSigmaB ) );

    FLT nKeep = 1.0 - ( ( nShareA > 0.0 ) ? nShareA : 0.0 ) - ( ( nShareB > 0.0 ) ? nShareB : 0.0 );

    for ( size_t nIndex = 0; nIndex < anLayer.size(); nIndex++ )
    {
        double dValue = (double)nKeep * anLayer[ nIndex ];

        if ( nShareA > 0.0 )
            dValue += (double)nShareA * anA[ nIndex ];
        if ( nShareB > 0.0 )
            dValue += (double)nShareB * anB[ nIndex ];
        anLayer[ nIndex ] = (FLT)dValue;
    }
}

void CFilmColorModel::Finish( CImageFile &oImage ) const
{
    if ( ! oImage.KeepsLinear() )
        return;

    int                         nWidth = (int)oImage.GetWidth();
    int                         nHeight = (int)oImage.GetHeight();
    const std::vector<float>   &anLinear = oImage.GetLinear();
    size_t                      nPixels = (size_t)nWidth * (size_t)nHeight;
    int                         nLayers = m_pStock->bColor ? 3 : 1;
    std::vector<FLT>            aanLayer[ 3 ];
    FLT                         nPixelMm = ( nWidth > 0 ) ? m_nWidth / (FLT)nWidth : 1.0;

    if ( anLinear.size() < nPixels * 3 )
        return;

        // Expose each layer.
    for ( int nLayer = 0; nLayer < nLayers; nLayer++ )
        aanLayer[ nLayer ].resize( nPixels );
    for ( size_t nIndex = 0; nIndex < nPixels; nIndex++ )
    {
        CColor  oColor( anLinear[ nIndex * 3 ], anLinear[ nIndex * 3 + 1 ], anLinear[ nIndex * 3 + 2 ] );
        FLT     anExposure[ 3 ];

        LayerExposures( oColor, anExposure );
        for ( int nLayer = 0; nLayer < nLayers; nLayer++ )
            aanLayer[ nLayer ][ nIndex ] = anExposure[ nLayer ];
    }

        // Spread the light in the emulsion: a bright area overexposes its
        // surroundings by the same share of its light, so the more it is
        // overexposed, the farther its glow reaches past the shoulder.
    for ( int nLayer = 0; nLayer < nLayers; nLayer++ )
    {
        Spread( aanLayer[ nLayer ], nWidth, nHeight,
                m_nSpread * m_pStock->nIrradiation, m_pStock->nIrradiationRadius / nPixelMm,
                m_nSpread * m_pStock->nHalation * m_pStock->anHalationTint[ nLayer ], m_pStock->nHalationRadius / nPixelMm );
    }

        // Develop: the curve, then grain in density (Selwyn: its RMS goes as
        // the aperture's width, here a pixel's).
    FLT nGrain = m_nGrain * m_pStock->nGrain * ( 0.048 / nPixelMm );

    for ( int nY = 0; nY < nHeight; nY++ )
    {
        for ( int nX = 0; nX < nWidth; nX++ )
        {
            size_t  nIndex = (size_t)nY * (size_t)nWidth + (size_t)nX;
            FLT     anDensity[ 3 ] = { 0.0, 0.0, 0.0 };
            BYTE    nRed, nGreen, nBlue;

            for ( int nLayer = 0; nLayer < nLayers; nLayer++ )
            {
                FLT nDensity = Density( nLayer, aanLayer[ nLayer ][ nIndex ] );

                if ( nGrain > 0.0 )
                    nDensity += nGrain * (FLT)sqrt( (double)( ( nDensity > 0.0 ) ? nDensity : 0.0 ) ) *
                                (FLT)Normal( m_nSeed, nX, nY, nLayer );
                anDensity[ nLayer ] = nDensity;
            }
            DensitiesToRGB( anDensity, nRed, nGreen, nBlue );
            oImage.PutRGB( nX, nY, nRed, nGreen, nBlue );
        }
    }
}

void CFilmColorModel::ToJson( IASLib::CObjectNode &oNode ) const
{
    oNode.put( "stock", m_strStock );
    if ( m_nIso > 0.0 )
        RayJson::PutNumber( oNode, "iso", m_nIso );
    RayJson::PutNumber( oNode, "shutter", m_nShutter );
    RayJson::PutNumber( oNode, "fStop", m_nFStop );
    RayJson::PutNumber( oNode, "width", m_nWidth );
    if ( m_nGrain != 1.0 )
        RayJson::PutNumber( oNode, "grain", m_nGrain );
    if ( m_nSpread != 1.0 )
        RayJson::PutNumber( oNode, "spread", m_nSpread );
    if ( m_nSeed != 0 )
        oNode.put( "seed", m_nSeed );
}

BOOL CFilmColorModel::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "stock", "iso", "shutter", "fStop", "width", "grain", "spread", "seed", NULL };
    std::string strStock = m_strStock;
    FLT         nIso = m_nIso;
    FLT         nShutter = m_nShutter;
    FLT         nFStop = m_nFStop;
    FLT         nWidth = m_nWidth;
    FLT         nGrain = m_nGrain;
    FLT         nSpread = m_nSpread;
    int         nSeed = m_nSeed;

    if ( ! oNode.isObject() )
        return TRUE;
    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetString( oNode, "stock", strStock, FALSE ) ||
         ! RayJson::GetNumber( oNode, "iso", nIso, FALSE ) ||
         ! RayJson::GetNumber( oNode, "shutter", nShutter, FALSE ) ||
         ! RayJson::GetNumber( oNode, "fStop", nFStop, FALSE ) ||
         ! RayJson::GetNumber( oNode, "width", nWidth, FALSE ) ||
         ! RayJson::GetNumber( oNode, "grain", nGrain, FALSE ) ||
         ! RayJson::GetNumber( oNode, "spread", nSpread, FALSE ) ||
         ! RayJson::GetInt( oNode, "seed", nSeed, 0, 2147483647, FALSE ) )
        return FALSE;
    if ( ! SetStock( strStock.c_str() ) )
    {
        RayJson::CWhere oWhere( "stock" );

        return RayJson::Fail( "unknown film stock \"" + strStock + "\"" );
    }
    if ( nIso < 0.0 || nIso != nIso || ( oNode.get( "iso" ) != NULL && ! ( nIso > 0.0 ) ) )
        return RayJson::Fail( "iso must be a positive film speed" );
    if ( ! ( nShutter > 0.0 ) || ! isfinite( (double)nShutter ) )
        return RayJson::Fail( "shutter must be a positive time in seconds" );
    if ( ! ( nFStop > 0.0 ) || ! isfinite( (double)nFStop ) )
        return RayJson::Fail( "fStop must be positive" );
    if ( ! ( nWidth > 0.0 ) || ! isfinite( (double)nWidth ) )
        return RayJson::Fail( "width (the image's width on the film, in mm) must be positive" );
    if ( ! ( nGrain >= 0.0 ) || ! isfinite( (double)nGrain ) )
        return RayJson::Fail( "grain can't be negative" );
    if ( ! ( nSpread >= 0.0 ) || ! ( nSpread * ( m_pStock->nIrradiation + m_pStock->nHalation ) < 1.0 ) )
        return RayJson::Fail( "spread must be at least 0 and leave some light where it falls" );
    m_nIso = nIso;
    m_nShutter = nShutter;
    m_nFStop = nFStop;
    m_nWidth = nWidth;
    m_nGrain = nGrain;
    m_nSpread = nSpread;
    m_nSeed = nSeed;
    return TRUE;
}
