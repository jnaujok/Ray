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
*   is turned away. The engine pulls its work:                           *
*                                                                        *
*       MESSAGE_SCENE   master: the scene as an SJSON packet (IASLib's   *
*                       compact binary JSON) of the scene file's JSON,   *
*                       unrounded, each one starting the stream afresh   *
*                       (sequence 0), so the master encodes it once for  *
*                       every engine. The engine answers with a request. *
*       MESSAGE_REQUEST engine: ready for a tile (no payload)            *
*       MESSAGE_TILE    master: a tile to trace: the trace number, then  *
*                       left, top, width and height, 32-bit each         *
*       MESSAGE_PIXELS  engine: the traced tile: the same five numbers,  *
*                       the primary rays traced (64-bit), then red,      *
*                       green, blue for each pixel, row by row, as       *
*                       IEEE-754 doubles. A request follows.             *
*       MESSAGE_STOP    master: the engine stops (no payload)            *
*                                                                        *
*   All numbers are big-endian. Pixels are doubles, carried exactly, so  *
*   networked engines give the same image as threads. When the master    *
*   has no tile for a request it leaves it unanswered until it has one   *
*   (a lost engine's tile) or a new scene comes. The trace number tells  *
*   a new trace's pixels from a stopped one's. ToReadable turns a tile   *
*   or pixels message into JSON for reading. The master sends SJSON as   *
*   sender 1, an engine as sender 2.                                     *
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
#define RAY_WIRE_VERSION        ((uint32_t)5)     // 2: JSON scene; 3: SJSON scene, binary batches; 4: tiles; 5: a scene starts a fresh stream
#define RAY_WIRE_DEFAULT_PORT   7997
#define RAY_WIRE_MAX_PAYLOAD    ((uint32_t)( 256u * 1024u * 1024u ))

    // SJSON sender ids.
#define RAY_WIRE_MASTER_ID      1u
#define RAY_WIRE_ENGINE_ID      2u

#include "TileTracer.h"

#include <vector>

class CScene;

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
    bool        DecodeHello( const std::string &strPayload );
    std::string EncodeTile( uint32_t nTrace, const STile &oTile );
    bool        DecodeTile( const std::string &strPayload, uint32_t &nTrace, STile &oTile );
    std::string EncodePixels( uint32_t nTrace, const STile &oTile, uint64_t nRays, const std::vector<double> &anColors );
        /// False if the payload isn't a whole tile of pixels.
    bool        DecodePixels( const std::string &strPayload, uint32_t &nTrace, STile &oTile, uint64_t &nRays,
                              std::vector<double> &anColors );
        /// A tile or pixels message as indented JSON, for reading; empty if
        /// the payload isn't a valid one.
    std::string ToReadable( uint8_t nTag, const std::string &strPayload );

        /// One direction of a connection, sending. The scene is an SJSON
        /// stream packet, which depends on the packets before it on the same
        /// connection, so each encoder sends, in order, to one CDecoder.
    class CEncoder
    {
        public:
            explicit            CEncoder( unsigned nSenderId );

            std::string         Scene( CScene &oScene );
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
