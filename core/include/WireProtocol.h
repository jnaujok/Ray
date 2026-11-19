/*************************************************************************
*   Wire protocol                                                        *
*                                                                        *
*   How a task master and a networked rayengine talk. Every message is   *
*                                                                        *
*       tag      1 byte (MsgTag.h)                                       *
*       length   4 bytes, big-endian                                     *
*       payload  length bytes                                            *
*                                                                        *
*   The scene is sent as JSON text, the same as a scene file but on one  *
*   line; batches of rays and colors, which are sent constantly, stay in *
*   the compact CRayStream binary format. A connection opens with        *
*   MESSAGE_HELLO from the engine, carrying RAY_WIRE_MAGIC and           *
*   RAY_WIRE_VERSION, so an engine of another version is turned away.    *
*                                                                        *
*************************************************************************/
#ifndef __WIREPROTOCOL_H__
#define __WIREPROTOCOL_H__

#include "Globals.h"
#include "MsgTag.h"
#include "RayStream.h"
#include "Streams/Stream.h"
#include "Streams/StringStream.h"

#include <stdint.h>
#include <string>

#define RAY_WIRE_MAGIC          ((uint32_t)0x52415931)     // "RAY1"
#define RAY_WIRE_VERSION        ((uint32_t)2)     // 2: the scene is sent as JSON
#define RAY_WIRE_DEFAULT_PORT   7997
#define RAY_WIRE_MAX_PAYLOAD    ((uint32_t)( 256u * 1024u * 1024u ))

class CScene;
class CRayArray;
class CColorArray;

namespace RayWire
{
        /// Writes one message and flushes the stream.
    void        SendFrame( IASLib::CStream &oStream, uint8_t nTag, const std::string &strPayload );
        /// Reads one message. Returns false if the connection closed cleanly
        /// before a message began. Throws IASLib::CStreamException if the
        /// message is cut short or its length is not believable.
    bool        ReceiveFrame( IASLib::CStream &oStream, uint8_t &nTag, std::string &strPayload );

    std::string EncodeHello( void );
    bool        DecodeHello( const std::string &strPayload );

    std::string EncodeScene( CScene &oScene );
    bool        DecodeScene( const std::string &strPayload, CScene &oScene );

    std::string EncodeRays( CRayArray &oRays );
    void        DecodeRays( const std::string &strPayload, CRayArray &oRays );

    std::string EncodeColors( CColorArray &oColors );
    void        DecodeColors( const std::string &strPayload, CColorArray &oColors );
}

#endif // __WIREPROTOCOL_H__
