/*************************************************************************
*   Decompress                                                           *
*                                                                        *
*   See Decompress.h. The Zstandard decoder follows RFC 8878 section by  *
*   section: frames, blocks, the literals section (raw, RLE, Huffman     *
*   with one or four streams, or the last table again), the sequences    *
*   section (predefined, RLE, FSE-described or repeated tables for       *
*   literal lengths, offsets and match lengths) and sequence execution   *
*   with the three repeat offsets.                                       *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#include "Globals.h"
#include "Decompress.h"
#include "PngImage.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace
{
    struct SFail {};

    inline uint32_t Little16( const unsigned char *p ) { return (uint32_t)p[ 0 ] | ( (uint32_t)p[ 1 ] << 8 ); }
    inline uint32_t Little24( const unsigned char *p ) { return Little16( p ) | ( (uint32_t)p[ 2 ] << 16 ); }
    inline uint32_t Little32( const unsigned char *p ) { return Little24( p ) | ( (uint32_t)p[ 3 ] << 24 ); }

    int HighBit( uint32_t n )
    {
        int nBit = -1;

        while ( n )
        {
            nBit++;
            n >>= 1;
        }
        return nBit;
    }

        // A bitstream read backward from its end, as FSE and Huffman
        // streams are: the last byte's highest set bit marks the end.
    class CBackBits
    {
        public:
            CBackBits( const unsigned char *pData, size_t nSize ) : m_pData( pData ), m_nSize( nSize )
            {
                if ( nSize == 0 || pData[ nSize - 1 ] == 0 )
                    throw SFail();
                m_nAt = (int64_t)nSize * 8 - 8 + HighBit( pData[ nSize - 1 ] );
            }
                // nBits bits ending where the stream is now (beyond its start
                // they read as zeros).
            uint64_t Peek( int nBits ) const
            {
                if ( nBits == 0 )
                    return 0;

                int64_t  nStart = m_nAt - nBits;
                uint64_t nValue = 0;

                for ( int64_t nBit = m_nAt - 1; nBit >= nStart; nBit-- )
                {
                    nValue <<= 1;
                    if ( nBit >= 0 )
                        nValue |= ( m_pData[ nBit >> 3 ] >> ( nBit & 7 ) ) & 1u;
                }
                return nValue;
            }
            uint64_t Read( int nBits )
            {
                uint64_t nValue = Peek( nBits );

                m_nAt -= nBits;
                return nValue;
            }
            int64_t  Left( void ) const { return m_nAt; }

        private:
            const unsigned char *m_pData;
            size_t               m_nSize;
            int64_t              m_nAt;      // bits not yet read
    };

        // An FSE decoding table.
    struct SFse
    {
        int                     nLog = 0;
        std::vector<uint16_t>   anSymbol;
        std::vector<uint8_t>    anBits;
        std::vector<uint16_t>   anBase;

        void Build( const std::vector<int> &anProb, int nAccuracy )
        {
            int nSize = 1 << nAccuracy;
            int nHigh = nSize - 1;
            std::vector<int> anNext( anProb.size() );

            nLog = nAccuracy;
            anSymbol.assign( (size_t)nSize, 0 );
            anBits.assign( (size_t)nSize, 0 );
            anBase.assign( (size_t)nSize, 0 );
            for ( size_t s = 0; s < anProb.size(); s++ )
            {
                if ( anProb[ s ] == -1 )
                {
                    if ( nHigh < 0 )
                        throw SFail();
                    anSymbol[ (size_t)nHigh-- ] = (uint16_t)s;
                    anNext[ s ] = 1;
                }
                else
                    anNext[ s ] = anProb[ s ];
            }

            int nStep = ( nSize >> 1 ) + ( nSize >> 3 ) + 3;
            int nMask = nSize - 1;
            int nPos = 0;

            for ( size_t s = 0; s < anProb.size(); s++ )
            {
                for ( int i = 0; i < anProb[ s ]; i++ )
                {
                    anSymbol[ (size_t)nPos ] = (uint16_t)s;
                    do
                        nPos = ( nPos + nStep ) & nMask;
                    while ( nPos > nHigh );
                }
            }
            if ( nPos != 0 )
                throw SFail();
            for ( int u = 0; u < nSize; u++ )
            {
                int s = anSymbol[ (size_t)u ];
                int nNextState = anNext[ (size_t)s ]++;

                if ( nNextState <= 0 )
                    throw SFail();

                int nBitCount = nAccuracy - HighBit( (uint32_t)nNextState );

                anBits[ (size_t)u ] = (uint8_t)nBitCount;
                anBase[ (size_t)u ] = (uint16_t)( ( nNextState << nBitCount ) - nSize );
            }
        }
        void Rle( int nSymbol )
        {
            nLog = 0;
            anSymbol.assign( 1, (uint16_t)nSymbol );
            anBits.assign( 1, 0 );
            anBase.assign( 1, 0 );
        }
    };

        // Reads an FSE table description (RFC 8878 4.1.1) from the front of
        // pData; returns the bytes it took.
    size_t ReadFseTable( const unsigned char *pData, size_t nSize, int nMaxLog, int nMaxSymbol, SFse &oTable )
    {
        size_t   nBitAt = 0;
        auto     Bits = [ & ]( int n ) -> uint32_t
        {
            uint32_t nValue = 0;

            for ( int i = 0; i < n; i++, nBitAt++ )
            {
                if ( ( nBitAt >> 3 ) >= nSize )
                    throw SFail();
                nValue |= (uint32_t)( ( pData[ nBitAt >> 3 ] >> ( nBitAt & 7 ) ) & 1u ) << i;
            }
            return nValue;
        };
        auto     PeekBits = [ & ]( int n ) -> uint32_t
        {
            uint32_t nValue = 0;

            for ( int i = 0; i < n; i++ )
            {
                size_t nAt = nBitAt + (size_t)i;

                if ( ( nAt >> 3 ) < nSize )
                    nValue |= (uint32_t)( ( pData[ nAt >> 3 ] >> ( nAt & 7 ) ) & 1u ) << i;
            }
            return nValue;
        };
        int      nAccuracy = (int)Bits( 4 ) + 5;

        if ( nAccuracy > nMaxLog )
            throw SFail();

        int              nRemaining = ( 1 << nAccuracy ) + 1;
        int              nThreshold = 1 << nAccuracy;
        int              nBitCount = nAccuracy + 1;
        std::vector<int> anProb;

        while ( nRemaining > 1 )
        {
            if ( (int)anProb.size() > nMaxSymbol )
                throw SFail();

            int      nMax = ( 2 * nThreshold - 1 ) - nRemaining;
            int      nCount;
            uint32_t nLow = PeekBits( nBitCount - 1 );

            if ( (int)nLow < nMax )
            {
                nCount = (int)nLow;
                nBitAt += (size_t)( nBitCount - 1 );
            }
            else
            {
                nCount = (int)PeekBits( nBitCount );
                if ( nCount >= nThreshold )
                    nCount -= nMax;
                nBitAt += (size_t)nBitCount;
            }
            nCount--;
            nRemaining -= ( nCount < 0 ) ? -nCount : nCount;
            anProb.push_back( nCount );
            if ( nCount == 0 )
            {
                    // Runs of zero probabilities: two-bit repeat flags.
                while ( true )
                {
                    uint32_t nRepeat = Bits( 2 );

                    for ( uint32_t i = 0; i < nRepeat; i++ )
                        anProb.push_back( 0 );
                    if ( nRepeat != 3 )
                        break;
                }
            }
            while ( nRemaining < nThreshold && nThreshold > 1 )
            {
                nBitCount--;
                nThreshold >>= 1;
            }
        }
        if ( nRemaining != 1 || (int)anProb.size() > nMaxSymbol + 1 )
            throw SFail();
        oTable.Build( anProb, nAccuracy );
        return ( nBitAt + 7 ) / 8;
    }

        // A Huffman decoding table.
    struct SHuffman
    {
        int                     nMaxBits = 0;
        std::vector<uint8_t>    anSymbol;
        std::vector<uint8_t>    anBits;

        bool Valid( void ) const { return nMaxBits > 0; }

        void Build( const std::vector<int> &anWeight )
        {
            uint32_t nTotal = 0;

            for ( int w : anWeight )
            {
                if ( w > 11 )
                    throw SFail();
                if ( w > 0 )
                    nTotal += 1u << ( w - 1 );
            }
            if ( nTotal == 0 )
                throw SFail();

            int      nMax = HighBit( nTotal ) + 1;
            uint32_t nLeft = ( 1u << nMax ) - nTotal;

            if ( nLeft == 0 || ( nLeft & ( nLeft - 1 ) ) != 0 || nMax > 11 )
                throw SFail();

            std::vector<int> anAll( anWeight );

            anAll.push_back( HighBit( nLeft ) + 1 );
            if ( anAll.size() > 256 )
                throw SFail();
            nMaxBits = nMax;

            int              nSize = 1 << nMaxBits;
            std::vector<int> anCount( (size_t)nMaxBits + 1, 0 );
            std::vector<int> anBitsOf( anAll.size(), 0 );

            for ( size_t s = 0; s < anAll.size(); s++ )
            {
                if ( anAll[ s ] > 0 )
                {
                    anBitsOf[ s ] = nMaxBits + 1 - anAll[ s ];
                    anCount[ (size_t)anBitsOf[ s ] ]++;
                }
            }
            anSymbol.assign( (size_t)nSize, 0 );
            anBits.assign( (size_t)nSize, 0 );

            std::vector<int> anRank( (size_t)nMaxBits + 1, 0 );

            anRank[ (size_t)nMaxBits ] = 0;
            for ( int i = nMaxBits; i >= 1; i-- )
            {
                int nNext = anRank[ (size_t)i ] + anCount[ (size_t)i ] * ( 1 << ( nMaxBits - i ) );

                if ( i - 1 >= 0 )
                    anRank[ (size_t)( i - 1 ) ] = nNext;
                if ( nNext > nSize )
                    throw SFail();
                for ( int j = anRank[ (size_t)i ]; j < nNext; j++ )
                    anBits[ (size_t)j ] = (uint8_t)i;
            }
            for ( size_t s = 0; s < anAll.size(); s++ )
            {
                if ( anBitsOf[ s ] == 0 )
                    continue;

                int nCode = anRank[ (size_t)anBitsOf[ s ] ];
                int nLength = 1 << ( nMaxBits - anBitsOf[ s ] );

                for ( int j = 0; j < nLength; j++ )
                    anSymbol[ (size_t)( nCode + j ) ] = (uint8_t)s;
                anRank[ (size_t)anBitsOf[ s ] ] += nLength;
            }
        }
        void Decode( const unsigned char *pData, size_t nSize, size_t nCount, unsigned char *pOut ) const
        {
            CBackBits oBits( pData, nSize );

            for ( size_t i = 0; i < nCount; i++ )
            {
                uint32_t nState = (uint32_t)oBits.Peek( nMaxBits );

                pOut[ i ] = anSymbol[ nState ];
                oBits.Read( anBits[ nState ] );
            }
            if ( oBits.Left() != 0 )
                throw SFail();
        }
    };

        // Reads a Huffman tree description; returns the bytes it took.
    size_t ReadHuffmanTree( const unsigned char *pData, size_t nSize, SHuffman &oTable )
    {
        if ( nSize < 1 )
            throw SFail();

        int              nHeader = pData[ 0 ];
        std::vector<int> anWeight;

        if ( nHeader >= 128 )
        {
            int    nCount = nHeader - 127;
            size_t nBytes = (size_t)( nCount + 1 ) / 2;

            if ( 1 + nBytes > nSize )
                throw SFail();
            for ( int i = 0; i < nCount; i++ )
            {
                unsigned char nByte = pData[ 1 + i / 2 ];

                anWeight.push_back( ( i % 2 == 0 ) ? ( nByte >> 4 ) : ( nByte & 15 ) );
            }
            oTable.Build( anWeight );
            return 1 + nBytes;
        }
        if ( 1 + (size_t)nHeader > nSize || nHeader == 0 )
            throw SFail();

            // FSE-compressed weights, two interleaved states.
        SFse   oFse;
        size_t nTable = ReadFseTable( pData + 1, (size_t)nHeader, 6, 255, oFse );

        if ( nTable >= (size_t)nHeader )
            throw SFail();

        CBackBits oBits( pData + 1 + nTable, (size_t)nHeader - nTable );
        uint32_t  nState1 = (uint32_t)oBits.Read( oFse.nLog );
        uint32_t  nState2 = (uint32_t)oBits.Read( oFse.nLog );

        while ( true )
        {
            if ( anWeight.size() > 255 )
                throw SFail();
            anWeight.push_back( oFse.anSymbol[ nState1 ] );
            nState1 = oFse.anBase[ nState1 ] + (uint32_t)oBits.Read( oFse.anBits[ nState1 ] );
            if ( oBits.Left() < 0 )
            {
                anWeight.push_back( oFse.anSymbol[ nState2 ] );
                break;
            }
            anWeight.push_back( oFse.anSymbol[ nState2 ] );
            nState2 = oFse.anBase[ nState2 ] + (uint32_t)oBits.Read( oFse.anBits[ nState2 ] );
            if ( oBits.Left() < 0 )
            {
                anWeight.push_back( oFse.anSymbol[ nState1 ] );
                break;
            }
        }
        oTable.Build( anWeight );
        return 1 + (size_t)nHeader;
    }

    const int s_anLiteralBase[ 36 ] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 20, 22, 24, 28, 32, 40,
                                        48, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536 };
    const int s_anLiteralExtra[ 36 ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3,
                                         4, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    const int s_anMatchBase[ 53 ] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
                                      27, 28, 29, 30, 31, 32, 33, 34, 35, 37, 39, 41, 43, 47, 51, 59, 67, 83, 99, 131, 259, 515,
                                      1027, 2051, 4099, 8195, 16387, 32771, 65539 };
    const int s_anMatchExtra[ 53 ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                       1, 1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    const int s_anLiteralDefault[ 36 ] = { 4, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 1, 1, 1, 1, 1,
                                           -1, -1, -1, -1 };
    const int s_anMatchDefault[ 53 ] = { 1, 4, 3, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                         1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1, -1, -1 };
    const int s_anOffsetDefault[ 29 ] = { 1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1 };

        // One frame's decoding state.
    struct SFrame
    {
        std::vector<unsigned char> anOut;
        SHuffman                   oHuffman;
        SFse                       oLiteralTable, oOffsetTable, oMatchTable;
        bool                       bLiteralTable = false, bOffsetTable = false, bMatchTable = false;
        uint64_t                   anRepeat[ 3 ] = { 1, 4, 8 };
        size_t                     nLimit = 0;
        size_t                     nFrameStart = 0;
    };

    void CompressedBlock( const unsigned char *p, size_t nSize, SFrame &f )
    {
        if ( nSize < 1 )
            throw SFail();

            // Literals.
        int                        nType = p[ 0 ] & 3;
        int                        nSizeFormat = ( p[ 0 ] >> 2 ) & 3;
        size_t                     nAt = 0;
        size_t                     nRegenerated = 0;
        std::vector<unsigned char> anLiterals;

        if ( nType == 0 || nType == 1 )
        {
            if ( nSizeFormat == 0 || nSizeFormat == 2 )
            {
                nRegenerated = p[ 0 ] >> 3;
                nAt = 1;
            }
            else if ( nSizeFormat == 1 )
            {
                if ( nSize < 2 ) throw SFail();
                nRegenerated = ( p[ 0 ] >> 4 ) + ( (size_t)p[ 1 ] << 4 );
                nAt = 2;
            }
            else
            {
                if ( nSize < 3 ) throw SFail();
                nRegenerated = ( p[ 0 ] >> 4 ) + ( (size_t)p[ 1 ] << 4 ) + ( (size_t)p[ 2 ] << 12 );
                nAt = 3;
            }
            if ( nType == 0 )
            {
                if ( nAt + nRegenerated > nSize ) throw SFail();
                anLiterals.assign( p + nAt, p + nAt + nRegenerated );
                nAt += nRegenerated;
            }
            else
            {
                if ( nAt + 1 > nSize ) throw SFail();
                anLiterals.assign( nRegenerated, p[ nAt ] );
                nAt += 1;
            }
        }
        else
        {
            int    nStreams = ( nSizeFormat == 0 ) ? 1 : 4;
            int    nBits = ( nSizeFormat <= 1 ) ? 10 : ( nSizeFormat == 2 ) ? 14 : 18;
            size_t nHeader = ( nSizeFormat <= 1 ) ? 3 : ( nSizeFormat == 2 ) ? 4 : 5;

            if ( nSize < nHeader ) throw SFail();

            uint64_t nValue = 0;

            for ( size_t i = 0; i < nHeader; i++ )
                nValue |= (uint64_t)p[ i ] << ( 8 * i );
            nRegenerated = (size_t)( ( nValue >> 4 ) & ( ( 1u << nBits ) - 1u ) );

            size_t nCompressed = (size_t)( ( nValue >> ( 4 + nBits ) ) & ( ( 1u << nBits ) - 1u ) );

            nAt = nHeader;
            if ( nAt + nCompressed > nSize ) throw SFail();

            const unsigned char *pStreams = p + nAt;
            size_t               nStreamBytes = nCompressed;

            if ( nType == 2 )
            {
                size_t nTree = ReadHuffmanTree( pStreams, nStreamBytes, f.oHuffman );

                pStreams += nTree;
                nStreamBytes -= nTree;
            }
            else if ( ! f.oHuffman.Valid() )
                throw SFail();
            anLiterals.resize( nRegenerated );
            if ( nStreams == 1 )
                f.oHuffman.Decode( pStreams, nStreamBytes, nRegenerated, anLiterals.data() );
            else
            {
                if ( nStreamBytes < 6 ) throw SFail();

                size_t anLength[ 4 ];

                anLength[ 0 ] = Little16( pStreams );
                anLength[ 1 ] = Little16( pStreams + 2 );
                anLength[ 2 ] = Little16( pStreams + 4 );
                if ( 6 + anLength[ 0 ] + anLength[ 1 ] + anLength[ 2 ] > nStreamBytes ) throw SFail();
                anLength[ 3 ] = nStreamBytes - 6 - anLength[ 0 ] - anLength[ 1 ] - anLength[ 2 ];

                size_t               nEach = ( nRegenerated + 3 ) / 4;
                const unsigned char *pStream = pStreams + 6;
                size_t               nDone = 0;

                for ( int i = 0; i < 4; i++ )
                {
                    size_t nCount = ( i < 3 ) ? nEach : nRegenerated - 3 * nEach;

                    if ( i < 3 && nDone + nCount > nRegenerated ) throw SFail();
                    if ( i == 3 && nRegenerated < 3 * nEach ) throw SFail();
                    f.oHuffman.Decode( pStream, anLength[ i ], nCount, anLiterals.data() + nDone );
                    nDone += nCount;
                    pStream += anLength[ i ];
                }
            }
            nAt += nCompressed;
        }

            // Sequences.
        if ( nAt >= nSize ) throw SFail();

        size_t nSequences;
        int    nByte0 = p[ nAt ];

        if ( nByte0 < 128 )
        {
            nSequences = (size_t)nByte0;
            nAt += 1;
        }
        else if ( nByte0 < 255 )
        {
            if ( nAt + 2 > nSize ) throw SFail();
            nSequences = ( (size_t)( nByte0 - 128 ) << 8 ) + p[ nAt + 1 ];
            nAt += 2;
        }
        else
        {
            if ( nAt + 3 > nSize ) throw SFail();
            nSequences = p[ nAt + 1 ] + ( (size_t)p[ nAt + 2 ] << 8 ) + 0x7F00;
            nAt += 3;
        }

        std::vector<unsigned char> &out = f.anOut;
        size_t                      nLiteral = 0;

        if ( nSequences > 0 )
        {
            if ( nAt >= nSize ) throw SFail();

            int nModes = p[ nAt++ ];

            if ( nModes & 3 ) throw SFail();

            auto Table = [ & ]( int nMode, SFse &oTable, bool &bHave, const int *pnDefault, int nDefaultCount, int nDefaultLog,
                                int nMaxLog, int nMaxSymbol )
            {
                if ( nMode == 0 )
                {
                    std::vector<int> anProb( pnDefault, pnDefault + nDefaultCount );

                    oTable.Build( anProb, nDefaultLog );
                }
                else if ( nMode == 1 )
                {
                    if ( nAt >= nSize || p[ nAt ] > nMaxSymbol ) throw SFail();
                    oTable.Rle( p[ nAt++ ] );
                }
                else if ( nMode == 2 )
                    nAt += ReadFseTable( p + nAt, nSize - nAt, nMaxLog, nMaxSymbol, oTable );
                else if ( ! bHave )
                    throw SFail();
                bHave = true;
            };
            Table( ( nModes >> 6 ) & 3, f.oLiteralTable, f.bLiteralTable, s_anLiteralDefault, 36, 6, 9, 35 );
            Table( ( nModes >> 4 ) & 3, f.oOffsetTable, f.bOffsetTable, s_anOffsetDefault, 29, 5, 8, 31 );
            Table( ( nModes >> 2 ) & 3, f.oMatchTable, f.bMatchTable, s_anMatchDefault, 53, 6, 9, 52 );
            if ( nAt > nSize ) throw SFail();

            CBackBits oBits( p + nAt, nSize - nAt );
            uint32_t  nLiteralState = (uint32_t)oBits.Read( f.oLiteralTable.nLog );
            uint32_t  nOffsetState = (uint32_t)oBits.Read( f.oOffsetTable.nLog );
            uint32_t  nMatchState = (uint32_t)oBits.Read( f.oMatchTable.nLog );

            for ( size_t s = 0; s < nSequences; s++ )
            {
                int nOffsetCode = f.oOffsetTable.anSymbol[ nOffsetState ];
                int nLiteralCode = f.oLiteralTable.anSymbol[ nLiteralState ];
                int nMatchCode = f.oMatchTable.anSymbol[ nMatchState ];

                if ( nLiteralCode > 35 || nMatchCode > 52 || nOffsetCode > 31 )
                    throw SFail();

                uint64_t nOffsetValue = ( (uint64_t)1 << nOffsetCode ) + oBits.Read( nOffsetCode );
                uint64_t nMatch = (uint64_t)s_anMatchBase[ nMatchCode ] + oBits.Read( s_anMatchExtra[ nMatchCode ] );
                uint64_t nLiterals = (uint64_t)s_anLiteralBase[ nLiteralCode ] + oBits.Read( s_anLiteralExtra[ nLiteralCode ] );

                if ( s + 1 < nSequences )
                {
                    nLiteralState = f.oLiteralTable.anBase[ nLiteralState ] + (uint32_t)oBits.Read( f.oLiteralTable.anBits[ nLiteralState ] );
                    nMatchState = f.oMatchTable.anBase[ nMatchState ] + (uint32_t)oBits.Read( f.oMatchTable.anBits[ nMatchState ] );
                    nOffsetState = f.oOffsetTable.anBase[ nOffsetState ] + (uint32_t)oBits.Read( f.oOffsetTable.anBits[ nOffsetState ] );
                }

                    // The offset, with the three repeat offsets.
                uint64_t nOffset;

                if ( nOffsetValue > 3 )
                {
                    nOffset = nOffsetValue - 3;
                    f.anRepeat[ 2 ] = f.anRepeat[ 1 ];
                    f.anRepeat[ 1 ] = f.anRepeat[ 0 ];
                    f.anRepeat[ 0 ] = nOffset;
                }
                else
                {
                    uint64_t nIndex = nOffsetValue - 1 + ( nLiterals == 0 ? 1 : 0 );

                    if ( nIndex == 0 )
                        nOffset = f.anRepeat[ 0 ];
                    else
                    {
                        nOffset = ( nIndex < 3 ) ? f.anRepeat[ nIndex ] : f.anRepeat[ 0 ] - 1;
                        if ( nIndex > 1 )
                            f.anRepeat[ 2 ] = f.anRepeat[ 1 ];
                        f.anRepeat[ 1 ] = f.anRepeat[ 0 ];
                        f.anRepeat[ 0 ] = nOffset;
                    }
                }

                if ( nLiteral + nLiterals > anLiterals.size() )
                    throw SFail();
                if ( out.size() + nLiterals + nMatch > f.nLimit )
                    throw SFail();
                out.insert( out.end(), anLiterals.begin() + (std::ptrdiff_t)nLiteral, anLiterals.begin() + (std::ptrdiff_t)( nLiteral + nLiterals ) );
                nLiteral += (size_t)nLiterals;
                if ( nOffset == 0 || nOffset > out.size() - f.nFrameStart )
                    throw SFail();

                size_t nFrom = out.size() - (size_t)nOffset;

                for ( uint64_t i = 0; i < nMatch; i++ )
                    out.push_back( out[ nFrom + (size_t)i ] );
            }
            if ( oBits.Left() != 0 )
                throw SFail();
        }
        if ( out.size() + ( anLiterals.size() - nLiteral ) > f.nLimit )
            throw SFail();
        out.insert( out.end(), anLiterals.begin() + (std::ptrdiff_t)nLiteral, anLiterals.end() );
    }
}

bool Decompress::IsGzip( const std::string &strData )
{
    return strData.size() >= 18 && (unsigned char)strData[ 0 ] == 0x1F && (unsigned char)strData[ 1 ] == 0x8B;
}

bool Decompress::IsZstd( const std::string &strData )
{
    return strData.size() >= 4 && Little32( (const unsigned char *)strData.data() ) == 0xFD2FB528u;
}

bool Decompress::Gunzip( const std::string &strData, std::string &strOut, std::string &strError, size_t nLimit )
{
    const unsigned char *p = (const unsigned char *)strData.data();
    size_t               nSize = strData.size();
    size_t               nAt = 0;

    strOut.clear();
    while ( nAt < nSize )
    {
        if ( nSize - nAt < 18 || p[ nAt ] != 0x1F || p[ nAt + 1 ] != 0x8B || p[ nAt + 2 ] != 8 )
        {
            if ( nAt > 0 )
                break;      // trailing bytes after the last member
            strError = "isn't gzip data";
            return false;
        }

        int    nFlags = p[ nAt + 3 ];
        size_t nAfter = nAt + 10;

        if ( nFlags & 4 )
        {
            if ( nAfter + 2 > nSize ) break;
            nAfter += 2 + Little16( p + nAfter );
        }
        if ( nFlags & 8 )
        {
            while ( nAfter < nSize && p[ nAfter ] != 0 ) nAfter++;
            nAfter++;
        }
        if ( nFlags & 16 )
        {
            while ( nAfter < nSize && p[ nAfter ] != 0 ) nAfter++;
            nAfter++;
        }
        if ( nFlags & 2 )
            nAfter += 2;
        if ( nAfter >= nSize )
        {
            strError = "is damaged gzip data";
            return false;
        }

        std::vector<BYTE> anOut;
        size_t            nConsumed = 0;

        if ( ! CPngImage::InflateRaw( p + nAfter, nSize - nAfter, anOut, nLimit - strOut.size(), strError, &nConsumed ) )
        {
            strError = "is damaged gzip data (" + strError + ")";
            return false;
        }
        strOut.append( (const char *)anOut.data(), anOut.size() );
        nAt = nAfter + nConsumed + 8;     // CRC-32 and size
    }
    return true;
}

bool Decompress::Unzstd( const std::string &strData, std::string &strOut, std::string &strError, size_t nLimit )
{
    const unsigned char *p = (const unsigned char *)strData.data();
    size_t               nSize = strData.size();
    size_t               nAt = 0;
    SFrame               oFrame;

    oFrame.nLimit = nLimit;
    strOut.clear();
    try
    {
        while ( nAt + 4 <= nSize )
        {
            uint32_t nMagic = Little32( p + nAt );

            if ( ( nMagic & 0xFFFFFFF0u ) == 0x184D2A50u )
            {
                if ( nAt + 8 > nSize ) throw SFail();

                size_t nSkip = Little32( p + nAt + 4 );

                if ( nAt + 8 + nSkip > nSize ) throw SFail();
                nAt += 8 + nSkip;
                continue;
            }
            if ( nMagic != 0xFD2FB528u )
                throw SFail();
            nAt += 4;
            if ( nAt >= nSize ) throw SFail();

            int  nDescriptor = p[ nAt++ ];
            int  nContentSizeFlag = nDescriptor >> 6;
            bool bSingle = ( nDescriptor >> 5 ) & 1;
            bool bChecksum = ( nDescriptor >> 2 ) & 1;
            int  nDictionaryFlag = nDescriptor & 3;

            if ( nDescriptor & 8 ) throw SFail();      // reserved bit
            if ( ! bSingle )
                nAt += 1;   // window descriptor

            static const int s_anDictionary[ 4 ] = { 0, 1, 2, 4 };

            for ( int i = 0; i < s_anDictionary[ nDictionaryFlag ]; i++ )
            {
                if ( nAt >= nSize ) throw SFail();
                if ( p[ nAt++ ] != 0 )
                {
                    strError = "needs a Zstandard dictionary, which Ray doesn't have";
                    return false;
                }
            }

            static const int s_anContentSize[ 4 ] = { 0, 2, 4, 8 };
            int nContentBytes = ( nContentSizeFlag == 0 && bSingle ) ? 1 : s_anContentSize[ nContentSizeFlag ];

            nAt += (size_t)nContentBytes;
            if ( nAt > nSize ) throw SFail();

                // A frame's blocks share its tables and repeat offsets.
            oFrame.oHuffman = SHuffman();
            oFrame.bLiteralTable = oFrame.bOffsetTable = oFrame.bMatchTable = false;
            oFrame.anRepeat[ 0 ] = 1;
            oFrame.anRepeat[ 1 ] = 4;
            oFrame.anRepeat[ 2 ] = 8;
            oFrame.nFrameStart = oFrame.anOut.size();

            bool bLast = false;

            while ( ! bLast )
            {
                if ( nAt + 3 > nSize ) throw SFail();

                uint32_t nHeader = Little24( p + nAt );
                int      nType = ( nHeader >> 1 ) & 3;
                size_t   nBlock = nHeader >> 3;

                bLast = ( nHeader & 1 ) != 0;
                nAt += 3;
                if ( nType == 0 )
                {
                    if ( nAt + nBlock > nSize || oFrame.anOut.size() + nBlock > nLimit ) throw SFail();
                    oFrame.anOut.insert( oFrame.anOut.end(), p + nAt, p + nAt + nBlock );
                    nAt += nBlock;
                }
                else if ( nType == 1 )
                {
                    if ( nAt + 1 > nSize || oFrame.anOut.size() + nBlock > nLimit ) throw SFail();
                    oFrame.anOut.insert( oFrame.anOut.end(), nBlock, p[ nAt ] );
                    nAt += 1;
                }
                else if ( nType == 2 )
                {
                    if ( nAt + nBlock > nSize || nBlock > 128 * 1024 ) throw SFail();
                    CompressedBlock( p + nAt, nBlock, oFrame );
                    nAt += nBlock;
                }
                else
                    throw SFail();
            }
            if ( bChecksum )
                nAt += 4;
            if ( nAt > nSize ) throw SFail();
        }
        if ( nAt != nSize )
            throw SFail();
    }
    catch ( const SFail & )
    {
        if ( strError.empty() )
            strError = "is damaged Zstandard data";
        return false;
    }
    catch ( const std::bad_alloc & )
    {
        strError = "is Zstandard data too large to unpack";
        return false;
    }
    strOut.assign( (const char *)oFrame.anOut.data(), oFrame.anOut.size() );
    return true;
}
