/*************************************************************************
*   Decompress                                                           *
*                                                                        *
*   Unpacks the compressed files Ray reads: gzip (RFC 1952, every        *
*   member, through CPngImage's inflate) and Zstandard (RFC 8878, every  *
*   frame, skippable frames skipped; raw, RLE and compressed blocks with *
*   Huffman-coded literals and FSE-coded sequences). Blender saves       *
*   compressed .blend files in one or the other. Zstandard dictionaries  *
*   are refused; checksums are not checked.                              *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __DECOMPRESS_H__
#define __DECOMPRESS_H__

#include <cstddef>
#include <string>

namespace Decompress
{
    bool    IsGzip( const std::string &strData );
    bool    IsZstd( const std::string &strData );

        /// Each unpacks strData into strOut. FALSE, with why in strError, if
        /// the data is damaged or would unpack to more than nLimit bytes.
    bool    Gunzip( const std::string &strData, std::string &strOut, std::string &strError, size_t nLimit );
    bool    Unzstd( const std::string &strData, std::string &strOut, std::string &strError, size_t nLimit );
}

#endif // __DECOMPRESS_H__
