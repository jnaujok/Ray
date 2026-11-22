/*************************************************************************
*   CEnvironment                                                         *
*                                                                        *
*   What a ray that leaves the scene sees. See Environment.h.            *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#include "Globals.h"

#include "Environment.h"
#include "ImageTexture.h"
#include "SceneJson.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define MAXIMUM_ENVIRONMENT_SAMPLES 256

CEnvironment::CEnvironment( void ) :
m_eType( TYPE_SKY ),
m_oZenith( 0.25, 0.45, 0.85 ),
m_oHorizon( 0.85, 0.9, 1.0 ),
m_oGround( 0.3, 0.28, 0.25 ),
m_nExponent( 0.5 ),
m_nIntensity( 1.0 ),
m_nRotation( 0.0 ),
m_bSrgb( FALSE ),
m_nSamples( 0 )
{
}

void CEnvironment::ImageCoords( FLT nX, FLT nY, FLT nZ, FLT &nU, FLT &nV ) const
{
    FLT nLength = SQRT( ( nX * nX ) + ( nY * nY ) + ( nZ * nZ ) );
    FLT nHeight = nY / nLength;

    if ( nHeight > 1.0 )
        nHeight = 1.0;
    if ( nHeight < -1.0 )
        nHeight = -1.0;

        // Longitude 0 (the middle of the image) lies along +z; +x is a
        // quarter of the way to the right of it.
    FLT nLongitude = (FLT)atan2( (double)nX, (double)nZ ) - ( m_nRotation * PI / 180.0 );

    nU = 0.5 + ( nLongitude / ( 2.0 * PI ) );
    nU = nU - (FLT)floor( (double)nU );
    nV = (FLT)acos( (double)nHeight ) / PI;
}

CColor CEnvironment::Pixel( int nX, int nY ) const
{
    int nWidth = m_pImage->nWidth;
    int nHeight = m_pImage->nHeight;

        // Around the sides it wraps; at the top and bottom it stops.
    nX = ( ( nX % nWidth ) + nWidth ) % nWidth;
    nY = ( nY < 0 ) ? 0 : ( nY >= nHeight ) ? nHeight - 1 : nY;

    const float *pPixel = &m_pImage->anPixels[ ( ( (size_t)nY * (size_t)nWidth ) + (size_t)nX ) * 3 ];

    return CColor( pPixel[ 0 ], pPixel[ 1 ], pPixel[ 2 ] );
}

CColor CEnvironment::ImageColor( FLT nX, FLT nY, FLT nZ ) const
{
    FLT nU;
    FLT nV;

    ImageCoords( nX, nY, nZ, nU, nV );

    FLT nPX = ( nU * m_pImage->nWidth ) - 0.5;
    FLT nPY = ( nV * m_pImage->nHeight ) - 0.5;
    FLT nX0 = (FLT)floor( (double)nPX );
    FLT nY0 = (FLT)floor( (double)nPY );
    FLT nFX = nPX - nX0;
    FLT nFY = nPY - nY0;
    int nIX = (int)nX0;
    int nIY = (int)nY0;

    CColor oTop = Pixel( nIX, nIY ) * ( 1.0 - nFX );
    CColor oTopRight = Pixel( nIX + 1, nIY ) * nFX;
    CColor oBottom = Pixel( nIX, nIY + 1 ) * ( 1.0 - nFX );
    CColor oBottomRight = Pixel( nIX + 1, nIY + 1 ) * nFX;
    CColor oUpper = oTop + oTopRight;
    CColor oLower = oBottom + oBottomRight;
    CColor oUpperPart = oUpper * ( 1.0 - nFY );
    CColor oLowerPart = oLower * nFY;

    return oUpperPart + oLowerPart;
}

CColor CEnvironment::ColorIn( const CVector &oDirection ) const
{
    CVector oLook( oDirection );
    FLT     nX = oLook.GetX();
    FLT     nY = oLook.GetY();
    FLT     nZ = oLook.GetZ();
    CColor  oColor;

    if ( m_eType == TYPE_IMAGE && m_pImage )
        oColor = ImageColor( nX, nY, nZ );
    else
    {
        FLT     nLength = SQRT( ( nX * nX ) + ( nY * nY ) + ( nZ * nZ ) );
        FLT     nHeight = ( nLength > 0.0 ) ? nY / nLength : 0.0;
        CColor  oHorizon( m_oHorizon );
        CColor  oFar( ( nHeight >= 0.0 ) ? m_oZenith : m_oGround );
        FLT     nBlend = (FLT)pow( (double)ABS( nHeight ), (double)m_nExponent );

        if ( nBlend > 1.0 )
            nBlend = 1.0;

        CColor  oNear( oHorizon * ( 1.0 - nBlend ) );
        CColor  oAway( oFar * nBlend );

        oColor = oNear + oAway;
    }
    return oColor * m_nIntensity;
}

void CEnvironment::DirectionAt( FLT nU, FLT nV, FLT anDirection[ 3 ] )
{
    FLT nLongitude = ( nU - 0.5 ) * 2.0 * PI;
    FLT nPolar = nV * PI;
    FLT nAcross = (FLT)sin( (double)nPolar );

    anDirection[ 0 ] = (FLT)sin( (double)nLongitude ) * nAcross;
    anDirection[ 1 ] = (FLT)cos( (double)nPolar );
    anDirection[ 2 ] = (FLT)cos( (double)nLongitude ) * nAcross;
}

void CEnvironment::BuildIrradiance( void )
{
        // First the sky's light, cell by cell over a coarse grid laid out as
        // the image is: an image's pixels averaged over each cell (so a small
        // bright sun counts in full), or a sky's color at a few points in it.
    std::vector<FLT>    anRadiance( (size_t)RADIANCE_WIDTH * RADIANCE_HEIGHT * 3, 0.0 );
    BOOL                bImage = ( m_eType == TYPE_IMAGE && m_pImage && m_pImage->nWidth >= RADIANCE_WIDTH &&
                                   m_pImage->nHeight >= RADIANCE_HEIGHT );

    for ( int nRow = 0; nRow < RADIANCE_HEIGHT; nRow++ )
    {
        for ( int nColumn = 0; nColumn < RADIANCE_WIDTH; nColumn++ )
        {
            FLT    *pCell = &anRadiance[ ( ( (size_t)nRow * RADIANCE_WIDTH ) + nColumn ) * 3 ];
            FLT     anSum[ 3 ] = { 0.0, 0.0, 0.0 };
            int     nCount = 0;

            if ( bImage )
            {
                int nX0 = ( nColumn * m_pImage->nWidth ) / RADIANCE_WIDTH;
                int nX1 = ( ( nColumn + 1 ) * m_pImage->nWidth ) / RADIANCE_WIDTH;
                int nY0 = ( nRow * m_pImage->nHeight ) / RADIANCE_HEIGHT;
                int nY1 = ( ( nRow + 1 ) * m_pImage->nHeight ) / RADIANCE_HEIGHT;

                for ( int nY = nY0; nY < nY1; nY++ )
                {
                    for ( int nX = nX0; nX < nX1; nX++ )
                    {
                        const float *pPixel = &m_pImage->anPixels[ ( ( (size_t)nY * m_pImage->nWidth ) + nX ) * 3 ];

                        for ( int nIndex = 0; nIndex < 3; nIndex++ )
                            anSum[ nIndex ] += pPixel[ nIndex ];
                        nCount++;
                    }
                }
                for ( int nIndex = 0; nIndex < 3; nIndex++ )
                    pCell[ nIndex ] = ( anSum[ nIndex ] / nCount ) * m_nIntensity;
            }
            else
            {
                    // A few points in the cell, through ColorIn (which an
                    // image smaller than the grid also goes through; with
                    // no rotation, so the cell's direction is turned the
                    // image's way first).
                for ( int nSub = 0; nSub < 16; nSub++ )
                {
                    FLT     anDirection[ 3 ];

                    DirectionAt( ( nColumn + ( ( nSub % 4 ) + 0.5 ) / 4.0 ) / RADIANCE_WIDTH,
                                 ( nRow + ( ( nSub / 4 ) + 0.5 ) / 4.0 ) / RADIANCE_HEIGHT, anDirection );

                    FLT     nTurn = m_nRotation * PI / 180.0;
                    FLT     nCos = (FLT)cos( (double)nTurn );
                    FLT     nSin = (FLT)sin( (double)nTurn );
                    CVector oDirection( ( anDirection[ 0 ] * nCos ) + ( anDirection[ 2 ] * nSin ), anDirection[ 1 ],
                                        ( anDirection[ 2 ] * nCos ) - ( anDirection[ 0 ] * nSin ) );
                    CColor  oColor( ColorIn( oDirection ) );

                    anSum[ 0 ] += oColor.GetR();
                    anSum[ 1 ] += oColor.GetG();
                    anSum[ 2 ] += oColor.GetB();
                }
                for ( int nIndex = 0; nIndex < 3; nIndex++ )
                    pCell[ nIndex ] = anSum[ nIndex ] / 16.0;
            }
        }
    }

        // Then, for each way a surface can face, the sky's light over the
        // half of it in front, each cell weighed by its solid angle and the
        // cosine, over pi.
    std::vector<FLT>    anCellDirection( (size_t)RADIANCE_WIDTH * RADIANCE_HEIGHT * 3 );
    std::vector<FLT>    anCellWeight( (size_t)RADIANCE_WIDTH * RADIANCE_HEIGHT );
    FLT                 nAcross = 2.0 * PI / RADIANCE_WIDTH;

    for ( int nRow = 0; nRow < RADIANCE_HEIGHT; nRow++ )
    {
        FLT nTop = (FLT)cos( (double)( PI * nRow / RADIANCE_HEIGHT ) );
        FLT nBottom = (FLT)cos( (double)( PI * ( nRow + 1 ) / RADIANCE_HEIGHT ) );

        for ( int nColumn = 0; nColumn < RADIANCE_WIDTH; nColumn++ )
        {
            size_t nCell = ( (size_t)nRow * RADIANCE_WIDTH ) + nColumn;

            DirectionAt( ( nColumn + 0.5 ) / RADIANCE_WIDTH, ( nRow + 0.5 ) / RADIANCE_HEIGHT, &anCellDirection[ nCell * 3 ] );
            anCellWeight[ nCell ] = nAcross * ( nTop - nBottom ) / PI;
        }
    }

    std::shared_ptr<std::vector<float> > pMap( new std::vector<float>( (size_t)IRRADIANCE_WIDTH * IRRADIANCE_HEIGHT * 3 ) );

    for ( int nRow = 0; nRow < IRRADIANCE_HEIGHT; nRow++ )
    {
        for ( int nColumn = 0; nColumn < IRRADIANCE_WIDTH; nColumn++ )
        {
            FLT anNormal[ 3 ];
            FLT anSum[ 3 ] = { 0.0, 0.0, 0.0 };

            DirectionAt( ( nColumn + 0.5 ) / IRRADIANCE_WIDTH, ( nRow + 0.5 ) / IRRADIANCE_HEIGHT, anNormal );
            for ( size_t nCell = 0; nCell < anCellWeight.size(); nCell++ )
            {
                const FLT  *pDirection = &anCellDirection[ nCell * 3 ];
                FLT         nCos = ( anNormal[ 0 ] * pDirection[ 0 ] ) + ( anNormal[ 1 ] * pDirection[ 1 ] ) +
                                   ( anNormal[ 2 ] * pDirection[ 2 ] );

                if ( nCos <= 0.0 )
                    continue;

                FLT nWeight = nCos * anCellWeight[ nCell ];

                for ( int nIndex = 0; nIndex < 3; nIndex++ )
                    anSum[ nIndex ] += anRadiance[ ( nCell * 3 ) + nIndex ] * nWeight;
            }
            for ( int nIndex = 0; nIndex < 3; nIndex++ )
                ( *pMap )[ ( ( ( (size_t)nRow * IRRADIANCE_WIDTH ) + nColumn ) * 3 ) + nIndex ] = (float)anSum[ nIndex ];
        }
    }
    m_pIrradiance = pMap;
}

CColor CEnvironment::IrradianceIn( const CVector &oNormal ) const
{
    if ( ! m_pIrradiance )
        return CColor( 0, 0, 0 );

    CVector oFacing( oNormal );
    FLT     nU;
    FLT     nV;

    ImageCoords( oFacing.GetX(), oFacing.GetY(), oFacing.GetZ(), nU, nV );

    FLT     nX = ( nU * IRRADIANCE_WIDTH ) - 0.5;
    FLT     nY = ( nV * IRRADIANCE_HEIGHT ) - 0.5;
    FLT     nX0 = (FLT)floor( (double)nX );
    FLT     nY0 = (FLT)floor( (double)nY );
    FLT     nFX = nX - nX0;
    FLT     nFY = nY - nY0;
    FLT     anColor[ 3 ];

    for ( int nIndex = 0; nIndex < 3; nIndex++ )
    {
        FLT anCorner[ 4 ];

        for ( int nCorner = 0; nCorner < 4; nCorner++ )
        {
            int nCX = ( ( ( (int)nX0 + ( nCorner & 1 ) ) % IRRADIANCE_WIDTH ) + IRRADIANCE_WIDTH ) % IRRADIANCE_WIDTH;
            int nCY = (int)nY0 + ( nCorner >> 1 );

            nCY = ( nCY < 0 ) ? 0 : ( nCY >= IRRADIANCE_HEIGHT ) ? IRRADIANCE_HEIGHT - 1 : nCY;
            anCorner[ nCorner ] = ( *m_pIrradiance )[ ( ( ( (size_t)nCY * IRRADIANCE_WIDTH ) + nCX ) * 3 ) + nIndex ];
        }

        FLT nTop = ( anCorner[ 0 ] * ( 1.0 - nFX ) ) + ( anCorner[ 1 ] * nFX );
        FLT nBottom = ( anCorner[ 2 ] * ( 1.0 - nFX ) ) + ( anCorner[ 3 ] * nFX );

        anColor[ nIndex ] = ( nTop * ( 1.0 - nFY ) ) + ( nBottom * nFY );
    }
    return CColor( anColor[ 0 ], anColor[ 1 ], anColor[ 2 ] );
}

namespace
{
        // The next line of an HDR header, without its newline. FALSE at the
        // end of the data.
    BOOL NextLine( const std::string &strData, size_t &nAt, std::string &strLine )
    {
        if ( nAt >= strData.size() )
            return FALSE;

        size_t nEnd = strData.find( '\n', nAt );

        if ( nEnd == std::string::npos )
            return FALSE;
        strLine = strData.substr( nAt, nEnd - nAt );
        nAt = nEnd + 1;
        return TRUE;
    }

    float FromRgbe( unsigned char nMantissa, unsigned char nExponent )
    {
        if ( nExponent == 0 )
            return 0.0f;
        return (float)( ( (double)nMantissa + 0.5 ) * ldexp( 1.0, (int)nExponent - ( 128 + 8 ) ) );
    }
}

BOOL CEnvironment::ParseHdr( const std::string &strData, int &nWidth, int &nHeight, std::vector<float> &anPixels,
                             std::string &strError )
{
    size_t      nAt = 0;
    std::string strLine;

    if ( ! NextLine( strData, nAt, strLine ) || ( strLine.compare( 0, 10, "#?RADIANCE" ) != 0 && strLine.compare( 0, 6, "#?RGBE" ) != 0 ) )
    {
        strError = "isn't a Radiance HDR file";
        return FALSE;
    }

        // Header lines, up to a blank one.
    for ( ;; )
    {
        if ( ! NextLine( strData, nAt, strLine ) )
        {
            strError = "its HDR header has no end";
            return FALSE;
        }
        if ( strLine.empty() )
            break;
        if ( strLine.compare( 0, 7, "FORMAT=" ) == 0 && strLine != "FORMAT=32-bit_rle_rgbe" )
        {
            strError = "is an HDR file in a format other than RGBE (" + strLine + ")";
            return FALSE;
        }
    }

    char achY[ 3 ] = { 0 };
    char achX[ 3 ] = { 0 };
    long nRows = 0;
    long nColumns = 0;

    if ( ! NextLine( strData, nAt, strLine ) ||
         sscanf( strLine.c_str(), "%2s %ld %2s %ld", achY, &nRows, achX, &nColumns ) != 4 ||
         strcmp( achY, "-Y" ) != 0 || strcmp( achX, "+X" ) != 0 )
    {
        strError = "has an HDR size line other than \"-Y height +X width\"";
        return FALSE;
    }
    if ( nRows < 1 || nRows > 32767 || nColumns < 1 || nColumns > 32767 )
    {
        strError = "has an HDR size out of range";
        return FALSE;
    }

    nWidth = (int)nColumns;
    nHeight = (int)nRows;
    anPixels.assign( (size_t)nWidth * (size_t)nHeight * 3, 0.0f );

    const unsigned char        *pData = (const unsigned char *)strData.data();
    size_t                      nSize = strData.size();
    std::vector<unsigned char>  anRow( (size_t)nWidth * 4 );

    for ( int nRow = 0; nRow < nHeight; nRow++ )
    {
        BOOL bEncoded = ( nWidth >= 8 && nWidth <= 32767 && nAt + 4 <= nSize && pData[ nAt ] == 2 && pData[ nAt + 1 ] == 2 &&
                          ( ( ( (int)pData[ nAt + 2 ] << 8 ) | (int)pData[ nAt + 3 ] ) == nWidth ) );

        if ( bEncoded )
        {
                // Each of the four channels in turn, as runs and literals.
            nAt += 4;
            for ( int nChannel = 0; nChannel < 4; nChannel++ )
            {
                int nColumn = 0;

                while ( nColumn < nWidth )
                {
                    if ( nAt >= nSize )
                    {
                        strError = "its HDR data ends early";
                        return FALSE;
                    }

                    int nCount = pData[ nAt++ ];

                    if ( nCount > 128 )
                    {
                        nCount -= 128;
                        if ( nAt >= nSize )
                        {
                            strError = "its HDR data ends early";
                            return FALSE;
                        }
                        if ( nColumn + nCount > nWidth )
                        {
                            strError = "has a bad HDR run";
                            return FALSE;
                        }
                        for ( int nIndex = 0; nIndex < nCount; nIndex++ )
                            anRow[ ( (size_t)( nColumn + nIndex ) * 4 ) + nChannel ] = pData[ nAt ];
                        nAt++;
                    }
                    else
                    {
                        if ( nAt + nCount > nSize )
                        {
                            strError = "its HDR data ends early";
                            return FALSE;
                        }
                        if ( nCount == 0 || nColumn + nCount > nWidth )
                        {
                            strError = "has a bad HDR run";
                            return FALSE;
                        }
                        for ( int nIndex = 0; nIndex < nCount; nIndex++ )
                            anRow[ ( (size_t)( nColumn + nIndex ) * 4 ) + nChannel ] = pData[ nAt + nIndex ];
                        nAt += nCount;
                    }
                    nColumn += nCount;
                }
            }
        }
        else
        {
                // Flat: four bytes a pixel.
            if ( nAt + anRow.size() > nSize )
            {
                strError = "its HDR data ends early";
                return FALSE;
            }
            memcpy( anRow.data(), pData + nAt, anRow.size() );
            nAt += anRow.size();
        }

        for ( int nColumn = 0; nColumn < nWidth; nColumn++ )
        {
            const unsigned char *pRgbe = &anRow[ (size_t)nColumn * 4 ];
            float               *pOut = &anPixels[ ( ( (size_t)nRow * (size_t)nWidth ) + (size_t)nColumn ) * 3 ];

            for ( int nIndex = 0; nIndex < 3; nIndex++ )
                pOut[ nIndex ] = FromRgbe( pRgbe[ nIndex ], pRgbe[ 3 ] );
        }
    }
    return TRUE;
}

    // Reads the image from a file's contents into oImage.
static BOOL DecodeImage( const std::string &strData, const std::string &strName, BOOL bSrgb, CEnvironment::SImage &oImage,
                         std::string &strError )
{
    if ( strData.compare( 0, 2, "#?" ) == 0 )
    {
        if ( ! CEnvironment::ParseHdr( strData, oImage.nWidth, oImage.nHeight, oImage.anPixels, strError ) )
        {
            strError = "\"" + strName + "\" " + strError;
            return FALSE;
        }
        return TRUE;
    }

    CImageTexture::SImage oBytes;

    if ( ! CImageTexture::ParseImage( strData, strName, oBytes, strError ) )
        return FALSE;

    oImage.nWidth = oBytes.nWidth;
    oImage.nHeight = oBytes.nHeight;
    oImage.anPixels.resize( oBytes.anPixels.size() );

    float anTable[ 256 ];

    for ( int nIndex = 0; nIndex < 256; nIndex++ )
    {
        double dValue = nIndex / 255.0;

        if ( bSrgb )
            dValue = ( dValue <= 0.04045 ) ? dValue / 12.92 : pow( ( dValue + 0.055 ) / 1.055, 2.4 );
        anTable[ nIndex ] = (float)dValue;
    }
    for ( size_t nIndex = 0; nIndex < oBytes.anPixels.size(); nIndex++ )
        oImage.anPixels[ nIndex ] = anTable[ oBytes.anPixels[ nIndex ] ];
    return TRUE;
}

void CEnvironment::ToJson( IASLib::CObjectNode &oNode ) const
{
    oNode.put( "type", ( m_eType == TYPE_IMAGE ) ? "image" : "sky" );
    if ( m_eType == TYPE_IMAGE )
    {
        oNode.put( "file", RayJson::SceneFileName( m_strFile, m_strPath, m_pFileData ) );
        if ( m_pFileData && RayJson::EmbeddingData() )
        {
            std::vector<unsigned char> anData( m_pFileData->begin(), m_pFileData->end() );

            oNode.putBinary( "data", anData );
        }
        RayJson::PutNumber( oNode, "rotation", m_nRotation );
        RayJson::PutBool( oNode, "srgb", m_bSrgb );
    }
    else
    {
        RayJson::PutColor( oNode, "zenith", m_oZenith );
        RayJson::PutColor( oNode, "horizon", m_oHorizon );
        RayJson::PutColor( oNode, "ground", m_oGround );
        RayJson::PutNumber( oNode, "exponent", m_nExponent );
    }
    RayJson::PutNumber( oNode, "intensity", m_nIntensity );
    oNode.put( "samples", m_nSamples );
}

BOOL CEnvironment::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrSky[] = { "zenith", "horizon", "ground", "exponent", "intensity", "samples", NULL };
    static const char *const astrImage[] = { "file", "data", "rotation", "srgb", "intensity", "samples", NULL };
    std::string strType;

    if ( ! oNode.isObject() )
        return RayJson::Fail( "must be an object" );
    if ( ! RayJson::GetString( oNode, "type", strType, TRUE ) )
        return FALSE;
    if ( strType == "sky" )
        m_eType = TYPE_SKY;
    else if ( strType == "image" )
        m_eType = TYPE_IMAGE;
    else
        return RayJson::Fail( "unknown environment type \"" + strType + "\" (sky or image)" );

    if ( ! RayJson::OnlyFields( oNode, ( m_eType == TYPE_SKY ) ? astrSky : astrImage ) ||
         ! RayJson::GetNumber( oNode, "intensity", m_nIntensity, FALSE ) ||
         ! RayJson::GetInt( oNode, "samples", m_nSamples, 0, MAXIMUM_ENVIRONMENT_SAMPLES, FALSE ) )
        return FALSE;
    if ( ! ( m_nIntensity >= 0.0 ) )
        return RayJson::Fail( "intensity must be 0 or more" );

    if ( m_eType == TYPE_SKY )
    {
        if ( ! RayJson::GetColor( oNode, "zenith", m_oZenith, FALSE ) ||
             ! RayJson::GetColor( oNode, "horizon", m_oHorizon, FALSE ) ||
             ! RayJson::GetColor( oNode, "ground", m_oGround, FALSE ) ||
             ! RayJson::GetNumber( oNode, "exponent", m_nExponent, FALSE ) )
            return FALSE;
        if ( ! ( m_nExponent > 0.0 ) )
            return RayJson::Fail( "exponent must be more than 0" );
        m_pImage.reset();
        m_pIrradiance.reset();
        if ( m_nSamples > 0 )
            BuildIrradiance();
        return TRUE;
    }

    if ( ! RayJson::GetString( oNode, "file", m_strFile, TRUE ) ||
         ! RayJson::GetNumber( oNode, "rotation", m_nRotation, FALSE ) ||
         ! RayJson::GetBool( oNode, "srgb", m_bSrgb, FALSE ) )
        return FALSE;

    std::string                 strData;
    std::string                 strError;
    const IASLib::CJsonNode    *pData = oNode.get( "data" );

    if ( pData )
    {
            // The file's bytes, carried in a copy of the scene sent to a task.
        std::vector<unsigned char> anData;

        if ( ! RayJson::BinaryValue( *pData, anData ) )
            return RayJson::Fail( "data must be the image file's bytes, in base64" );
        strData.assign( anData.begin(), anData.end() );
        m_strPath.clear();
    }
    else if ( ! RayJson::ReadSceneFile( m_strFile, strData, m_strPath, strError ) )
        return RayJson::Fail( "can't open the image \"" + m_strFile + "\"" );

    std::shared_ptr<SImage> pImage( new SImage );

    if ( ! DecodeImage( strData, m_strFile, m_bSrgb, *pImage, strError ) )
        return RayJson::Fail( strError );
    m_pFileData.reset( new std::string( strData ) );
    m_pImage = pImage;
    m_pIrradiance.reset();
    if ( m_nSamples > 0 )
        BuildIrradiance();
    return TRUE;
}
