#ifdef RAY_GUI
#include "StdAfx.h"
#endif

#include "WireProtocol.h"
#include "Scene.h"
#include "RayArray.h"
#include "ColorArray.h"

#include "Exceptions/StreamException.h"

namespace
{
    template <class F>
    std::string Encode( F fnWrite )
    {
        IASLib::CStringStream   oBuffer;
        CRayStream              oStream( oBuffer );

        fnWrite( oStream );
        return oBuffer.GetOutput();
    }
}

namespace RayWire
{
    void SendFrame( IASLib::CStream &oStream, uint8_t nTag, const std::string &strPayload )
    {
        uint32_t    nLength = (uint32_t)strPayload.size();
        std::string strFrame;

            // One write per message, so small messages are not held back
            // waiting for an acknowledgement (Nagle's algorithm).
        strFrame.reserve( 5 + strPayload.size() );
        strFrame += (char)nTag;
        strFrame += (char)( ( nLength >> 24 ) & 0xFF );
        strFrame += (char)( ( nLength >> 16 ) & 0xFF );
        strFrame += (char)( ( nLength >> 8 ) & 0xFF );
        strFrame += (char)( nLength & 0xFF );
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
        return Encode( []( CRayStream &oStream )
        {
            oStream.WriteUInt32( RAY_WIRE_MAGIC );
            oStream.WriteUInt32( RAY_WIRE_VERSION );
        } );
    }

    bool DecodeHello( const std::string &strPayload )
    {
        IASLib::CStringStream   oBuffer( strPayload.data(), strPayload.size() );
        CRayStream              oStream( oBuffer );

        if ( strPayload.size() != 8 )
            return false;
        if ( oStream.ReadUInt32() != RAY_WIRE_MAGIC )
            return false;
        return oStream.ReadUInt32() == RAY_WIRE_VERSION;
    }

        // The scene travels as the same JSON as a scene file, on one line.
    std::string EncodeScene( CScene &oScene )
    {
        return oScene.ToJson( TRUE );
    }

    bool DecodeScene( const std::string &strPayload, CScene &oScene )
    {
        return oScene.FromJson( strPayload ) == 0;
    }

    std::string EncodeRays( CRayArray &oRays )
    {
        return Encode( [&oRays]( CRayStream &oStream ) { oStream << oRays; } );
    }

    void DecodeRays( const std::string &strPayload, CRayArray &oRays )
    {
        IASLib::CStringStream   oBuffer( strPayload.data(), strPayload.size() );
        CRayStream              oStream( oBuffer );

        oStream >> oRays;
    }

    std::string EncodeColors( CColorArray &oColors )
    {
        return Encode( [&oColors]( CRayStream &oStream ) { oStream << oColors; } );
    }

    void DecodeColors( const std::string &strPayload, CColorArray &oColors )
    {
        IASLib::CStringStream   oBuffer( strPayload.data(), strPayload.size() );
        CRayStream              oStream( oBuffer );

        oStream >> oColors;
    }
}
