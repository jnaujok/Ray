#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "ImageTexture.h"
#include "SceneJson.h"
#include "JpegImage.h"
#include "PngImage.h"
#include "WebpImage.h"

#include <map>
#include <math.h>
#include <mutex>
#include <stdio.h>

#include <string>

namespace
{
        // The next whole number in a PPM header, skipping white space and
        // comments. FALSE at the end of the data.
    BOOL NextNumber( const std::string &strData, size_t &nAt, long &nValue )
    {
        while ( nAt < strData.size() )
        {
            char c = strData[ nAt ];

            if ( c == '#' )
            {
                while ( nAt < strData.size() && strData[ nAt ] != '\n' )
                    nAt++;
            }
            else if ( c == ' ' || c == '\t' || c == '\r' || c == '\n' )
                nAt++;
            else
                break;
        }
        if ( nAt >= strData.size() || strData[ nAt ] < '0' || strData[ nAt ] > '9' )
            return FALSE;
        nValue = 0;
        while ( nAt < strData.size() && strData[ nAt ] >= '0' && strData[ nAt ] <= '9' )
        {
            nValue = ( nValue * 10 ) + ( strData[ nAt ] - '0' );
            if ( nValue > 100000000L )
                return FALSE;
            nAt++;
        }
        return TRUE;
    }

        // Two numbers, as a [u, v] array.
    BOOL GetPair( const IASLib::CJsonNode &oNode, const char *strKey, FLT anValue[ 2 ] )
    {
        const IASLib::CJsonNode *pPair = oNode.get( strKey );

        if ( pPair == NULL )
            return TRUE;

        RayJson::CWhere oWhere( strKey );

        if ( ! pPair->isArray() || pPair->size() != 2 || ! pPair->get( 0 )->isNumber() || ! pPair->get( 1 )->isNumber() )
            return RayJson::Fail( "must be an array of two numbers, [u, v]" );
        anValue[ 0 ] = (FLT)pPair->get( 0 )->doubleValue();
        anValue[ 1 ] = (FLT)pPair->get( 1 )->doubleValue();
        return TRUE;
    }
}

namespace
{
        // Decoded images and file contents, shared by every texture that
        // reads the same bytes, in every copy of every scene (each piece of
        // a chess set using one picture, each thread's copy of the scene):
        // keyed by a hash of the bytes and kept only while something uses
        // them.
    struct SImageKey
    {
        uint64_t    anHash[ 2 ];
        size_t      nSize;
        int         nWidth;         // for carried pixels; -1 for a file

        bool operator<( const SImageKey &oOther ) const
        {
            if ( anHash[ 0 ] != oOther.anHash[ 0 ] )
                return anHash[ 0 ] < oOther.anHash[ 0 ];
            if ( anHash[ 1 ] != oOther.anHash[ 1 ] )
                return anHash[ 1 ] < oOther.anHash[ 1 ];
            if ( nSize != oOther.nSize )
                return nSize < oOther.nSize;
            return nWidth < oOther.nWidth;
        }
    };

    std::mutex                                                          s_oCacheLock;
    std::map<SImageKey, std::weak_ptr<const CImageTexture::SImage> >    s_oImages;
    std::map<SImageKey, std::weak_ptr<const std::string> >              s_oFiles;

    SImageKey KeyOf( const char *pBytes, size_t nSize, int nWidth )
    {
        SImageKey oKey;

        RayJson::HashBytes( pBytes, nSize, oKey.anHash );
        oKey.nSize = nSize;
        oKey.nWidth = nWidth;
        return oKey;
    }

    template <class T>
    std::shared_ptr<const T> Find( std::map<SImageKey, std::weak_ptr<const T> > &oCache, const SImageKey &oKey )
    {
        typename std::map<SImageKey, std::weak_ptr<const T> >::iterator itFound = oCache.find( oKey );

        return ( itFound != oCache.end() ) ? itFound->second.lock() : std::shared_ptr<const T>();
    }

    template <class T>
    void Remember( std::map<SImageKey, std::weak_ptr<const T> > &oCache, const SImageKey &oKey, const std::shared_ptr<const T> &pValue )
    {
            // Forget what nothing uses any more.
        for ( typename std::map<SImageKey, std::weak_ptr<const T> >::iterator it = oCache.begin(); it != oCache.end(); )
        {
            if ( it->second.expired() )
                it = oCache.erase( it );
            else
                ++it;
        }
        oCache[ oKey ] = pValue;
    }
}

std::shared_ptr<const CImageTexture::SImage> CImageTexture::SharedImage( const std::string &strData, const std::string &strName,
                                                                         std::string &strError )
{
    SImageKey                     oKey = KeyOf( strData.data(), strData.size(), -1 );
    std::shared_ptr<const SImage> pShared;

    {
        std::lock_guard<std::mutex> oLock( s_oCacheLock );

        pShared = Find( s_oImages, oKey );
    }
    if ( pShared )
        return pShared;

    std::shared_ptr<SImage> pImage( new SImage );

    if ( ! ParseImage( strData, strName, *pImage, strError ) )
        return std::shared_ptr<const SImage>();

    std::lock_guard<std::mutex> oLock( s_oCacheLock );

        // Another thread may have decoded it meanwhile: use the one kept.
    pShared = Find( s_oImages, oKey );
    if ( ! pShared )
    {
        pShared = pImage;
        Remember( s_oImages, oKey, pShared );
    }
    return pShared;
}

CImageTexture::CImageTexture( void ) :
    m_bRepeat( TRUE ),
    m_bSrgb( FALSE )
{
    m_anScale[ 0 ] = m_anScale[ 1 ] = 1.0;
    m_anOffset[ 0 ] = m_anOffset[ 1 ] = 0.0;
}

BOOL CImageTexture::ParseImage( const std::string &strData, const std::string &strPath, SImage &oImage, std::string &strError )
{
    if ( CPngImage::IsPng( strData ) || CJpegImage::IsJpeg( strData ) || CWebpImage::IsWebp( strData ) )
    {
        std::string strWhy;
        BOOL        bRead;

        if ( CPngImage::IsPng( strData ) )
            bRead = CPngImage::Decode( strData, oImage.nWidth, oImage.nHeight, oImage.anPixels, strWhy );
        else if ( CJpegImage::IsJpeg( strData ) )
            bRead = CJpegImage::Decode( strData, oImage.nWidth, oImage.nHeight, oImage.anPixels, strWhy );
        else
            bRead = CWebpImage::Decode( strData, oImage.nWidth, oImage.nHeight, oImage.anPixels, strWhy );
        if ( ! bRead )
        {
            strError = "\"" + strPath + "\" " + strWhy;
            return FALSE;
        }
        return TRUE;
    }
    if ( strData.size() >= 2 && strData[ 0 ] == 'P' && ( strData[ 1 ] == '6' || strData[ 1 ] == '3' ) )
    {
        size_t  nAt = 2;
        long    nWidth, nHeight, nMaximum;

        if ( ! NextNumber( strData, nAt, nWidth ) || ! NextNumber( strData, nAt, nHeight ) ||
             ! NextNumber( strData, nAt, nMaximum ) || nWidth < 1 || nHeight < 1 || nMaximum < 1 || nMaximum > 255 )
        {
            strError = "\"" + strPath + "\" isn't a PPM image with 8-bit channels";
            return FALSE;
        }
        oImage.nWidth = (int)nWidth;
        oImage.nHeight = (int)nHeight;
        oImage.anPixels.resize( (size_t)nWidth * (size_t)nHeight * 3 );

        if ( strData[ 1 ] == '6' )
        {
            nAt++;      // the single white space after the header
            if ( strData.size() < nAt + oImage.anPixels.size() )
            {
                strError = "\"" + strPath + "\" is shorter than its size says";
                return FALSE;
            }
            for ( size_t nIndex = 0; nIndex < oImage.anPixels.size(); nIndex++ )
                oImage.anPixels[ nIndex ] = (unsigned char)( ( (unsigned char)strData[ nAt + nIndex ] * 255 ) / nMaximum );
        }
        else
        {
            for ( size_t nIndex = 0; nIndex < oImage.anPixels.size(); nIndex++ )
            {
                long nValue;

                if ( ! NextNumber( strData, nAt, nValue ) || nValue > nMaximum )
                {
                    strError = "\"" + strPath + "\" has a bad or missing pixel value";
                    return FALSE;
                }
                oImage.anPixels[ nIndex ] = (unsigned char)( ( nValue * 255 ) / nMaximum );
            }
        }
        return TRUE;
    }

        // Ray's raw image: a 2-byte width and height, big-endian, then RGB.
    if ( strData.size() >= 4 )
    {
        int nWidth = ( (unsigned char)strData[ 0 ] << 8 ) | (unsigned char)strData[ 1 ];
        int nHeight = ( (unsigned char)strData[ 2 ] << 8 ) | (unsigned char)strData[ 3 ];

        if ( nWidth > 0 && nHeight > 0 && strData.size() == 4 + (size_t)nWidth * (size_t)nHeight * 3 )
        {
            oImage.nWidth = nWidth;
            oImage.nHeight = nHeight;
            oImage.anPixels.assign( strData.begin() + 4, strData.end() );
            return TRUE;
        }
    }

    strError = "\"" + strPath + "\" isn't a PNG, PPM or Ray raw image";
    return FALSE;
}

CColor CImageTexture::Pixel( int nX, int nY ) const
{
    int nWidth = m_pImage->nWidth;
    int nHeight = m_pImage->nHeight;

    if ( m_bRepeat )
    {
        nX = ( ( nX % nWidth ) + nWidth ) % nWidth;
        nY = ( ( nY % nHeight ) + nHeight ) % nHeight;
    }
    else
    {
        nX = ( nX < 0 ) ? 0 : ( nX >= nWidth ) ? nWidth - 1 : nX;
        nY = ( nY < 0 ) ? 0 : ( nY >= nHeight ) ? nHeight - 1 : nY;
    }

    const unsigned char *pPixel = &m_pImage->anPixels[ ( ( (size_t)nY * (size_t)nWidth ) + (size_t)nX ) * 3 ];
    FLT                  anChannel[ 3 ];

    for ( int nIndex = 0; nIndex < 3; nIndex++ )
    {
        FLT nValue = pPixel[ nIndex ] / 255.0;

        if ( m_bSrgb )
            nValue = ( nValue <= 0.04045 ) ? nValue / 12.92 : (FLT)pow( (double)( ( nValue + 0.055 ) / 1.055 ), 2.4 );
        anChannel[ nIndex ] = nValue;
    }
    return CColor( anChannel[ 0 ], anChannel[ 1 ], anChannel[ 2 ] );
}

CColor CImageTexture::ColorAt( const STexturePoint &oAt ) const
{
    if ( ! m_pImage )
        return CColor( 0, 0, 0 );

    FLT nU = ( oAt.bHasUV ? oAt.nU : 0.0 ) * m_anScale[ 0 ] + m_anOffset[ 0 ];
    FLT nV = ( oAt.bHasUV ? oAt.nV : 0.0 ) * m_anScale[ 1 ] + m_anOffset[ 1 ];

        // v = 0 is the bottom of the image, whose first row is its top.
    FLT nX = ( nU * m_pImage->nWidth ) - 0.5;
    FLT nY = ( ( 1.0 - nV ) * m_pImage->nHeight ) - 0.5;
    FLT nX0 = (FLT)floor( (double)nX );
    FLT nY0 = (FLT)floor( (double)nY );
    FLT nFX = nX - nX0;
    FLT nFY = nY - nY0;
    int nPX = (int)nX0;
    int nPY = (int)nY0;

    CColor oTop = Pixel( nPX, nPY ) * ( 1.0 - nFX );
    CColor oTopRight = Pixel( nPX + 1, nPY ) * nFX;
    CColor oBottom = Pixel( nPX, nPY + 1 ) * ( 1.0 - nFX );
    CColor oBottomRight = Pixel( nPX + 1, nPY + 1 ) * nFX;
    CColor oUpper = oTop + oTopRight;
    CColor oLower = oBottom + oBottomRight;

    return ( oUpper * ( 1.0 - nFY ) ) + ( oLower * nFY );
}

void CImageTexture::ToJson( IASLib::CObjectNode &oNode ) const
{
    if ( ! m_strFile.empty() || m_pImage )
    {
        std::shared_ptr<const std::string> pData = m_pFileData;

            // An image that came inside a scene copy has no file of its own:
            // to save it, make one, in Ray's raw format.
        if ( ! pData && m_pImage )
        {
            std::string strRaw;

            strRaw += (char)( ( m_pImage->nWidth >> 8 ) & 0xFF );
            strRaw += (char)( m_pImage->nWidth & 0xFF );
            strRaw += (char)( ( m_pImage->nHeight >> 8 ) & 0xFF );
            strRaw += (char)( m_pImage->nHeight & 0xFF );
            strRaw.append( (const char *)m_pImage->anPixels.data(), m_pImage->anPixels.size() );
            pData.reset( new std::string( strRaw ) );
        }

        oNode.put( "file", RayJson::SceneFileName( m_strFile.empty() ? std::string( "textures/image.raw" ) : m_strFile, m_strPath, pData ) );
    }
    if ( m_pImage && RayJson::EmbeddingData() )
    {
            // A copy for a task carries the image: the file itself where it
            // is smaller than its pixels (a PNG, JPEG or WebP), which the
            // task decodes to the same pixels; else the pixels.
        if ( m_pFileData && m_pFileData->size() < m_pImage->anPixels.size() )
        {
            std::vector<unsigned char> anData( m_pFileData->begin(), m_pFileData->end() );

            oNode.putBinary( "data", anData );
        }
        else
        {
            oNode.put( "width", m_pImage->nWidth );
            oNode.put( "height", m_pImage->nHeight );
            oNode.putBinary( "pixels", m_pImage->anPixels );
        }
    }
    oNode.putArray( "scale" )->add( (double)m_anScale[ 0 ] )->add( (double)m_anScale[ 1 ] );
    oNode.putArray( "offset" )->add( (double)m_anOffset[ 0 ] )->add( (double)m_anOffset[ 1 ] );
    RayJson::PutBool( oNode, "repeat", m_bRepeat );
    RayJson::PutBool( oNode, "srgb", m_bSrgb );
}

BOOL CImageTexture::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "file", "width", "height", "pixels", "data", "scale", "offset", "repeat", "srgb", NULL };
    std::shared_ptr<SImage> pImage( new SImage );
    std::shared_ptr<const SImage> pShared;

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetString( oNode, "file", m_strFile, FALSE ) ||
         ! GetPair( oNode, "scale", m_anScale ) ||
         ! GetPair( oNode, "offset", m_anOffset ) ||
         ! RayJson::GetBool( oNode, "repeat", m_bRepeat, FALSE ) ||
         ! RayJson::GetBool( oNode, "srgb", m_bSrgb, FALSE ) )
        return FALSE;

    const IASLib::CJsonNode *pPixels = oNode.get( "pixels" );
    const IASLib::CJsonNode *pData = oNode.get( "data" );

    if ( pData && ! pPixels )
    {
            // The file itself, carried in the scene (a copy sent to a task).
        std::vector<unsigned char> anData;
        std::string                strError;

        if ( ! RayJson::BinaryValue( *pData, anData ) )
            return RayJson::Fail( "data must be the image file, in base64" );

        std::string strData( anData.begin(), anData.end() );

        pShared = SharedImage( strData, m_strFile, strError );
        if ( ! pShared )
            return RayJson::Fail( strError );

        SImageKey                          oKey = KeyOf( strData.data(), strData.size(), -1 );
        std::shared_ptr<const std::string> pFile;
        std::lock_guard<std::mutex>        oLock( s_oCacheLock );

        pFile = Find( s_oFiles, oKey );
        if ( ! pFile )
        {
            pFile.reset( new std::string( strData ) );
            Remember( s_oFiles, oKey, pFile );
        }
        m_pFileData = pFile;
    }
    else if ( pPixels )
    {
            // Pixels carried in the scene (a copy sent to a task).
        if ( ! RayJson::GetInt( oNode, "width", pImage->nWidth, 1, 65535, TRUE ) ||
             ! RayJson::GetInt( oNode, "height", pImage->nHeight, 1, 65535, TRUE ) )
            return FALSE;
        if ( ! RayJson::BinaryValue( *pPixels, pImage->anPixels ) ||
             pImage->anPixels.size() != (size_t)pImage->nWidth * (size_t)pImage->nHeight * 3 )
            return RayJson::Fail( "pixels must be width x height RGB bytes, in base64" );

        SImageKey                    oKey = KeyOf( (const char *)pImage->anPixels.data(), pImage->anPixels.size(), pImage->nWidth );
        std::lock_guard<std::mutex>  oLock( s_oCacheLock );

        pShared = Find( s_oImages, oKey );
        if ( ! pShared )
        {
            pShared = pImage;
            Remember( s_oImages, oKey, pShared );
        }
    }
    else
    {
        std::string strError;

        if ( m_strFile.empty() )
            return RayJson::Fail( "needs \"file\"" );
        std::string strData;

        if ( ! RayJson::ReadSceneFile( m_strFile, strData, m_strPath, strError ) )
            return RayJson::Fail( "can't open the image \"" + m_strFile + "\"" );

        SImageKey                       oKey = KeyOf( strData.data(), strData.size(), -1 );
        std::shared_ptr<const std::string> pFile;

        pShared = SharedImage( strData, m_strFile, strError );
        if ( ! pShared )
            return RayJson::Fail( strError );

        std::lock_guard<std::mutex> oLock( s_oCacheLock );

        pFile = Find( s_oFiles, oKey );
        if ( ! pFile )
        {
            pFile.reset( new std::string( strData ) );
            Remember( s_oFiles, oKey, pFile );
        }
        m_pFileData = pFile;
    }
    m_pImage = pShared;
    return TRUE;
}
