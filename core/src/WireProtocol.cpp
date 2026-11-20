#ifdef RAY_GUI
#include "StdAfx.h"
#endif

#include "WireProtocol.h"
#include "Scene.h"
#include "RayArray.h"
#include "ColorArray.h"
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

    const size_t s_nRayBytes = 8 * 8 + 2 * 4;
    const size_t s_nColorBytes = 2 * 4 + 3 * 8;
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

    std::string CEncoder::Rays( CRayArray &oRays )
    {
        std::string strPacket;

        strPacket.reserve( (size_t)oRays.GetSize() * s_nRayBytes );
        for ( int nIndex = 0; nIndex < oRays.GetSize(); nIndex++ )
        {
            CRay &oRay = oRays[ nIndex ];

            PutDouble( strPacket, (double)oRay.GetX() );
            PutDouble( strPacket, (double)oRay.GetY() );
            PutDouble( strPacket, (double)oRay.GetZ() );
            PutDouble( strPacket, (double)oRay.GetI() );
            PutDouble( strPacket, (double)oRay.GetJ() );
            PutDouble( strPacket, (double)oRay.GetK() );
            PutDouble( strPacket, (double)oRay.GetT() );
            PutDouble( strPacket, (double)oRay.GetInfluence() );
            PutUInt32( strPacket, (uint32_t)oRay.GetPixelX() );
            PutUInt32( strPacket, (uint32_t)oRay.GetPixelY() );
        }
        return strPacket;
    }

    std::string CEncoder::Colors( CColorArray &oColors )
    {
        std::string strPacket;

        strPacket.reserve( (size_t)oColors.GetSize() * s_nColorBytes );
        for ( int nIndex = 0; nIndex < oColors.GetSize(); nIndex++ )
        {
            CColor &oColor = oColors[ nIndex ];

            PutUInt32( strPacket, (uint32_t)oColors.GetX( nIndex ) );
            PutUInt32( strPacket, (uint32_t)oColors.GetY( nIndex ) );
            PutDouble( strPacket, (double)oColor.GetR() );
            PutDouble( strPacket, (double)oColor.GetG() );
            PutDouble( strPacket, (double)oColor.GetB() );
        }
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

    bool CDecoder::Rays( const std::string &strPacket, CRayArray &oRays )
    {
        CRay oRay;

        oRays.RemoveAll();
        if ( strPacket.size() % s_nRayBytes )
        {
            m_strError = "a batch of rays has the wrong length";
            return false;
        }
        for ( size_t nAt = 0; nAt < strPacket.size(); nAt += s_nRayBytes )
        {
            oRay.Set( (FLT)GetDouble( strPacket, nAt ), (FLT)GetDouble( strPacket, nAt + 8 ), (FLT)GetDouble( strPacket, nAt + 16 ),
                      (FLT)GetDouble( strPacket, nAt + 24 ), (FLT)GetDouble( strPacket, nAt + 32 ), (FLT)GetDouble( strPacket, nAt + 40 ),
                      (FLT)GetDouble( strPacket, nAt + 48 ), (FLT)GetDouble( strPacket, nAt + 56 ),
                      (int)GetUInt32( strPacket, nAt + 64 ), (int)GetUInt32( strPacket, nAt + 68 ) );
            oRays.Add( oRay );
        }
        return true;
    }

    bool CDecoder::Colors( const std::string &strPacket, CColorArray &oColors )
    {
        oColors.RemoveAll();
        if ( strPacket.size() % s_nColorBytes )
        {
            m_strError = "a batch of colors has the wrong length";
            return false;
        }
        for ( size_t nAt = 0; nAt < strPacket.size(); nAt += s_nColorBytes )
            oColors.Add( CColor( (FLT)GetDouble( strPacket, nAt + 8 ), (FLT)GetDouble( strPacket, nAt + 16 ),
                                 (FLT)GetDouble( strPacket, nAt + 24 ) ),
                         (int)GetUInt32( strPacket, nAt ), (int)GetUInt32( strPacket, nAt + 4 ) );
        return true;
    }

    std::string ToReadable( uint8_t nTag, const std::string &strPayload )
    {
        CDecoder                oDecoder( RAY_WIRE_ENGINE_ID, RAY_WIRE_MASTER_ID );
        IASLib::CObjectNode     oDocument;

        if ( nTag == MESSAGE_RAYS )
        {
            CRayArray           oRays;
            IASLib::CArrayNode *pRays = oDocument.putArray( "rays" );

            if ( ! oDecoder.Rays( strPayload, oRays ) )
                return "";
            for ( int nIndex = 0; nIndex < oRays.GetSize(); nIndex++ )
            {
                CRay &oRay = oRays[ nIndex ];

                pRays->addObject()
                    ->put( "x", (double)oRay.GetX() )->put( "y", (double)oRay.GetY() )->put( "z", (double)oRay.GetZ() )
                    ->put( "i", (double)oRay.GetI() )->put( "j", (double)oRay.GetJ() )->put( "k", (double)oRay.GetK() )
                    ->put( "t", (double)oRay.GetT() )->put( "influence", (double)oRay.GetInfluence() )
                    ->put( "px", oRay.GetPixelX() )->put( "py", oRay.GetPixelY() );
            }
        }
        else if ( nTag == MESSAGE_COLOR )
        {
            CColorArray         oColors;
            IASLib::CArrayNode *pColors = oDocument.putArray( "colors" );

            if ( ! oDecoder.Colors( strPayload, oColors ) )
                return "";
            for ( int nIndex = 0; nIndex < oColors.GetSize(); nIndex++ )
            {
                CColor &oColor = oColors[ nIndex ];

                pColors->addObject()
                    ->put( "px", oColors.GetX( nIndex ) )->put( "py", oColors.GetY( nIndex ) )
                    ->put( "r", (double)oColor.GetR() )->put( "g", (double)oColor.GetG() )->put( "b", (double)oColor.GetB() );
            }
        }
        else
            return "";
        return oDocument.toJson( 2 );
    }
}
