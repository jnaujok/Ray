/*************************************************************************
*   Wire protocol                                                        *
*                                                                        *
*   How a task master and a networked rayengine talk. Every message is   *
*                                                                        *
*       tag      1 byte (MsgTag.h)                                       *
*       length   4 bytes, big-endian                                     *
*       payload  length bytes                                            *
*                                                                        *
*   A connection opens with MESSAGE_HELLO from the engine: 8 raw bytes,  *
*   RAY_WIRE_MAGIC and RAY_WIRE_VERSION, so an engine of another version *
*   is turned away. Then:                                                *
*                                                                        *
*       MESSAGE_SCENE   the scene as an SJSON packet (IASLib's compact   *
*                       binary JSON) of the scene file's JSON, unrounded *
*       MESSAGE_RAYS    72 bytes a ray: x, y, z, i, j, k, t, influence   *
*                       as IEEE-754 doubles, then the two 32-bit pixel   *
*                       tags, all big-endian; no rays tells the engine   *
*                       to stop                                          *
*       MESSAGE_COLOR   32 bytes a color: the two 32-bit pixel tags,     *
*                       then r, g, b as doubles                          *
*                                                                        *
*   Batches are fixed binary because they are sent constantly: SJSON     *
*   costs tens of microseconds a ray to encode, more than tracing it.    *
*   Numbers are carried exactly either way, so networked engines give    *
*   the same image as threads. ToReadable turns a batch into JSON for    *
*   reading. The master sends SJSON as sender 1, an engine as sender 2.  *
*                                                                        *
*************************************************************************/
#ifndef __WIREPROTOCOL_H__
#define __WIREPROTOCOL_H__

#include "Globals.h"
#include "MsgTag.h"
#include "Streams/Stream.h"

#include "SJSON/Sjson.h"

#include <stdint.h>
#include <string>

#define RAY_WIRE_MAGIC          ((uint32_t)0x52415931)     // "RAY1"
#define RAY_WIRE_VERSION        ((uint32_t)3)     // 2: JSON scene; 3: SJSON scene, plain binary batches
#define RAY_WIRE_DEFAULT_PORT   7997
#define RAY_WIRE_MAX_PAYLOAD    ((uint32_t)( 256u * 1024u * 1024u ))

    // SJSON sender ids.
#define RAY_WIRE_MASTER_ID      1u
#define RAY_WIRE_ENGINE_ID      2u

class CScene;
class CRayArray;
class CColorArray;

namespace IASLib
{
    class CJsonNode;
}

namespace RayWire
{
        /// Writes one message and flushes the stream.
    void        SendFrame( IASLib::CStream &oStream, uint8_t nTag, const std::string &strPayload );
        /// Reads one message. Returns false if the connection closed cleanly
        /// before a message began. Throws IASLib::CStreamException if the
        /// message is cut short or its length is not believable.
    bool        ReceiveFrame( IASLib::CStream &oStream, uint8_t &nTag, std::string &strPayload );

    std::string EncodeHello( void );
        /// A batch of rays or colors as indented JSON, for reading; empty if
        /// the payload isn't a valid batch.
    std::string ToReadable( uint8_t nTag, const std::string &strPayload );
    bool        DecodeHello( const std::string &strPayload );

        /// One direction of a connection, sending. The scene is an SJSON
        /// stream packet, which depends on the packets before it on the same
        /// connection, so each encoder sends, in order, to one CDecoder.
    class CEncoder
    {
        public:
            explicit            CEncoder( unsigned nSenderId );

            std::string         Scene( CScene &oScene );
            std::string         Rays( CRayArray &oRays );
            std::string         Colors( CColorArray &oColors );
                /// Any JSON document. Returns false if it can't be encoded.
            bool                Encode( const IASLib::CJsonNode &oDocument, std::string &strPacket );

        private:
            IASLib::CSjson      m_oSjson;

                                CEncoder( const CEncoder & );
            CEncoder           &operator =( const CEncoder & );
    };

        /// One direction of a connection, receiving. Each returns false if
        /// the packet can't be decoded or isn't the document expected.
    class CDecoder
    {
        public:
                                CDecoder( unsigned nOwnId, unsigned nPeerId );

            bool                Scene( const std::string &strPacket, CScene &oScene );
            bool                Rays( const std::string &strPacket, CRayArray &oRays );
            bool                Colors( const std::string &strPacket, CColorArray &oColors );
                /// Any SJSON packet as a JSON tree, which the caller owns; NULL
                /// on error. Readable with toPrettyString().
            IASLib::CJsonNode  *Decode( const std::string &strPacket );
                /// Why the last packet couldn't be read.
            const std::string  &GetError( void ) const { return m_strError; }

        private:
            IASLib::CSjson      m_oSjson;
            unsigned            m_nPeerId;
            std::string         m_strError;

                                CDecoder( const CDecoder & );
            CDecoder           &operator =( const CDecoder & );
    };
}

#endif // __WIREPROTOCOL_H__
