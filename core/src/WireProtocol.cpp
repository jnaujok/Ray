#ifdef RAY_GUI
#include "StdAfx.h"
#endif

#include "WireProtocol.h"
#include "Scene.h"
#include "SceneJson.h"

#include "Exceptions/StreamException.h"
#include "JSON/ArrayNode.h"
#include "JSON/ObjectNode.h"

#include <memory>
#include <string.h>
#include <vector>

namespace
{
    void PutUInt32( std::string &strOut, uint32_t nValue )
    {
        strOut += (char)( ( nValue >> 24 ) & 0xFF );
        strOut += (char)( ( nValue >> 16 ) & 0xFF );
        strOut += (char)( ( nValue >> 8 ) & 0xFF );
        strOut += (char)( nValue & 0xFF );
    }

    uint32_t GetUInt32( const std::string &strIn, size_t nAt )
    {
        return ( (uint32_t)(unsigned char)strIn[ nAt ] << 24 ) | ( (uint32_t)(unsigned char)strIn[ nAt + 1 ] << 16 ) |
               ( (uint32_t)(unsigned char)strIn[ nAt + 2 ] << 8 ) | (uint32_t)(unsigned char)strIn[ nAt + 3 ];
    }

        // A double as its IEEE-754 bits, big-endian: exact, and the same on
        // every platform.
    void PutDouble( std::string &strOut, double dValue )
    {
        uint64_t nBits;

        memcpy( &nBits, &dValue, sizeof( nBits ) );
        PutUInt32( strOut, (uint32_t)( nBits >> 32 ) );
        PutUInt32( strOut, (uint32_t)( nBits & 0xFFFFFFFFu ) );
    }

    double GetDouble( const std::string &strIn, size_t nAt )
    {
        uint64_t    nBits = ( (uint64_t)GetUInt32( strIn, nAt ) << 32 ) | GetUInt32( strIn, nAt + 4 );
        double      dValue;

        memcpy( &dValue, &nBits, sizeof( dValue ) );
        return dValue;
    }

        // A tile's header: the trace number, left, top, width, height.
    const size_t s_nTileBytes = 5 * 4;
        // The most pixels a tile may have.
    const uint64_t s_nMaxTilePixels = 4096u * 4096u;

    void PutTile( std::string &strOut, uint32_t nTrace, const STile &oTile )
    {
        PutUInt32( strOut, nTrace );
        PutUInt32( strOut, (uint32_t)oTile.nLeft );
        PutUInt32( strOut, (uint32_t)oTile.nTop );
        PutUInt32( strOut, (uint32_t)oTile.nWidth );
        PutUInt32( strOut, (uint32_t)oTile.nHeight );
    }

    bool GetTile( const std::string &strIn, uint32_t &nTrace, STile &oTile )
    {
        if ( strIn.size() < s_nTileBytes )
            return false;
        nTrace = GetUInt32( strIn, 0 );
        oTile.nLeft = (int)GetUInt32( strIn, 4 );
        oTile.nTop = (int)GetUInt32( strIn, 8 );
        oTile.nWidth = (int)GetUInt32( strIn, 12 );
        oTile.nHeight = (int)GetUInt32( strIn, 16 );
        return oTile.nLeft >= 0 && oTile.nTop >= 0 && oTile.nWidth > 0 && oTile.nHeight > 0 &&
               (uint64_t)oTile.nWidth * (uint64_t)oTile.nHeight <= s_nMaxTilePixels;
    }
}

namespace RayWire
{
    void SendFrame( IASLib::CStream &oStream, uint8_t nTag, const std::string &strPayload )
    {
        std::string strFrame;

            // One write per message, so small messages are not held back
            // waiting for an acknowledgement (Nagle's algorithm).
        strFrame.reserve( 5 + strPayload.size() );
        strFrame += (char)nTag;
        PutUInt32( strFrame, (uint32_t)strPayload.size() );
        strFrame += strPayload;

        oStream.PutBuffer( strFrame.data(), (int)strFrame.size() );
        oStream.Flush();
    }

    bool ReceiveFrame( IASLib::CStream &oStream, uint8_t &nTag, std::string &strPayload )
    {
        char    chTag;

            // A clean close between messages is not an error.
        if ( oStream.GetBuffer( &chTag, 1 ) != 1 )
            return false;

        nTag = (uint8_t)chTag;

        uint32_t nLength = oStream.GetUInt32();
        if ( nLength > RAY_WIRE_MAX_PAYLOAD )
            throw IASLib::CStreamException( "Message is larger than the protocol allows" );

        strPayload.assign( nLength, '\0' );
        if ( nLength )
            oStream.GetExact( &strPayload[ 0 ], (int)nLength );
        return true;
    }

    std::string EncodeHello( void )
    {
        std::string strHello;

        PutUInt32( strHello, RAY_WIRE_MAGIC );
        PutUInt32( strHello, RAY_WIRE_VERSION );
        return strHello;
    }

    bool DecodeHello( const std::string &strPayload )
    {
        return ( strPayload.size() == 8 ) && ( GetUInt32( strPayload, 0 ) == RAY_WIRE_MAGIC ) &&
               ( GetUInt32( strPayload, 4 ) == RAY_WIRE_VERSION );
    }

    CEncoder::CEncoder( unsigned nSenderId ) :
        m_oSjson( nSenderId, true )
    {
    }

    bool CEncoder::Encode( const IASLib::CJsonNode &oDocument, std::string &strPacket )
    {
        std::vector<unsigned char> aPacket;

        if ( ! m_oSjson.encode( oDocument, aPacket ) )
            return false;
        strPacket.assign( (const char *)aPacket.data(), aPacket.size() );
        return true;
    }

    std::string CEncoder::Scene( CScene &oScene )
    {
        RayJson::CEmbedData                     oEmbed;     // engines may not have the scene's files
        std::unique_ptr<IASLib::CObjectNode>    pScene( oScene.ToJsonTree() );
        std::string                             strPacket;

        Encode( *pScene, strPacket );
        return strPacket;
    }

    CDecoder::CDecoder( unsigned nOwnId, unsigned nPeerId ) :
        m_oSjson( nOwnId ),
        m_nPeerId( nPeerId )
    {
    }

    IASLib::CJsonNode *CDecoder::Decode( const std::string &strPacket )
    {
        IASLib::CJsonNode *pDocument = m_oSjson.decode( (const unsigned char *)strPacket.data(), strPacket.size(), m_nPeerId );

        if ( pDocument == NULL )
            m_strError = "a packet could not be decoded: " + m_oSjson.lastErrorText();
        return pDocument;
    }

    bool CDecoder::Scene( const std::string &strPacket, CScene &oScene )
    {
        std::unique_ptr<IASLib::CJsonNode> pScene( Decode( strPacket ) );

        if ( ! pScene )
            return false;
        if ( oScene.FromJsonTree( *pScene ) != 0 )
        {
            m_strError = std::string( "the scene could not be read: " ) + oScene.GetLoadError();
            return false;
        }
        return true;
    }

    std::string EncodeTile( uint32_t nTrace, const STile &oTile )
    {
        std::string strPayload;

        PutTile( strPayload, nTrace, oTile );
        return strPayload;
    }

    bool DecodeTile( const std::string &strPayload, uint32_t &nTrace, STile &oTile )
    {
        return ( strPayload.size() == s_nTileBytes ) && GetTile( strPayload, nTrace, oTile );
    }

    std::string EncodePixels( uint32_t nTrace, const STile &oTile, uint64_t nRays, const std::vector<double> &anColors )
    {
        std::string strPayload;

        strPayload.reserve( s_nTileBytes + 8 + ( anColors.size() * 8 ) );
        PutTile( strPayload, nTrace, oTile );
        PutUInt32( strPayload, (uint32_t)( nRays >> 32 ) );
        PutUInt32( strPayload, (uint32_t)( nRays & 0xFFFFFFFFu ) );
        for ( size_t nIndex = 0; nIndex < anColors.size(); nIndex++ )
            PutDouble( strPayload, anColors[ nIndex ] );
        return strPayload;
    }

    bool DecodePixels( const std::string &strPayload, uint32_t &nTrace, STile &oTile, uint64_t &nRays, std::vector<double> &anColors )
    {
        if ( ! GetTile( strPayload, nTrace, oTile ) )
            return false;

        size_t nValues = (size_t)oTile.nWidth * (size_t)oTile.nHeight * 3;

        if ( strPayload.size() != s_nTileBytes + 8 + ( nValues * 8 ) )
            return false;
        nRays = ( (uint64_t)GetUInt32( strPayload, s_nTileBytes ) << 32 ) | GetUInt32( strPayload, s_nTileBytes + 4 );
        anColors.resize( nValues );
        for ( size_t nIndex = 0; nIndex < nValues; nIndex++ )
            anColors[ nIndex ] = GetDouble( strPayload, s_nTileBytes + 8 + ( nIndex * 8 ) );
        return true;
    }

    std::string ToReadable( uint8_t nTag, const std::string &strPayload )
    {
        IASLib::CObjectNode oDocument;
        uint32_t            nTrace;
        STile               oTile;

        if ( nTag == MESSAGE_TILE )
        {
            if ( ! DecodeTile( strPayload, nTrace, oTile ) )
                return "";
        }
        else if ( nTag == MESSAGE_PIXELS )
        {
            uint64_t            nRays;
            std::vector<double> anColors;

            if ( ! DecodePixels( strPayload, nTrace, oTile, nRays, anColors ) )
                return "";
            oDocument.put( "rays", (double)nRays );

            IASLib::CArrayNode *pPixels = oDocument.putArray( "pixels" );

            for ( size_t nAt = 0; nAt < anColors.size(); nAt += 3 )
                pPixels->addArray()->add( anColors[ nAt ] )->add( anColors[ nAt + 1 ] )->add( anColors[ nAt + 2 ] );
        }
        else
            return "";
        oDocument.put( "trace", (double)nTrace );
        oDocument.put( "left", oTile.nLeft );
        oDocument.put( "top", oTile.nTop );
        oDocument.put( "width", oTile.nWidth );
        oDocument.put( "height", oTile.nHeight );
        return oDocument.toJson( 2 );
    }
}
