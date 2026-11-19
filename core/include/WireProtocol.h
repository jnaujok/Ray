/*************************************************************************
*   Wire protocol                                                        *
*                                                                        *
*   How a task master and a networked rayengine talk. Every message is   *
*                                                                        *
*       tag      1 byte (MsgTag.h)                                       *
*       length   4 bytes, big-endian                                     *
*       payload  length bytes, in the CRayStream format                  *
*                                                                        *
*   The payload is framed because a scene is read until its data runs    *
*   out. A connection opens with MESSAGE_HELLO from the engine, carrying *
*   RAY_WIRE_MAGIC and RAY_WIRE_VERSION.                                 *
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
#define RAY_WIRE_VERSION        ((uint32_t)1)
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
