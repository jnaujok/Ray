/*************************************************************************
*   CFontFile                                                            *
*                                                                        *
*   Reads TrueType fonts. See FontFile.h.                                *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#endif

#include "FontFile.h"

#include <cstring>

CFontFile::CFontFile( void ) :
    m_nGlyf( 0 ), m_nGlyfLength( 0 ), m_nLoca( 0 ), m_nHmtx( 0 ), m_nKern( 0 ), m_nKernLength( 0 ),
    m_nGpos( 0 ), m_nGposLength( 0 ), m_nCmapTable( 0 ), m_nCmapFormat( 0 ), m_nGlyphs( 0 ), m_nHMetrics( 0 ),
    m_nLocaFormat( 0 ), m_nUnitsPerEm( 1000 ), m_nAscender( 0 ), m_nDescender( 0 ), m_nLineGap( 0 )
{
}

bool CFontFile::Has( uint32_t nOffset, uint32_t nLength ) const
{
    return (uint64_t)nOffset + nLength <= m_strData.size();
}

unsigned CFontFile::U8( uint32_t nOffset ) const
{
    return Has( nOffset, 1 ) ? (unsigned char)m_strData[ nOffset ] : 0;
}

unsigned CFontFile::U16( uint32_t nOffset ) const
{
    return ( U8( nOffset ) << 8 ) | U8( nOffset + 1 );
}

int CFontFile::S16( uint32_t nOffset ) const
{
    return (int16_t)U16( nOffset );
}

uint32_t CFontFile::U32( uint32_t nOffset ) const
{
    return ( (uint32_t)U16( nOffset ) << 16 ) | U16( nOffset + 2 );
}

bool CFontFile::Load( const std::string &strData, std::string &strError )
{
    m_strData = strData;
    m_anKernLookups.clear();
    m_nKern = m_nGpos = 0;
    if ( m_strData.size() < 12 )
    {
        strError = "is too short to be a font";
        return false;
    }

    uint32_t nTag = U32( 0 );

    if ( nTag == 0x4F54544FU )  // "OTTO"
    {
        strError = "has CFF (PostScript) outlines, which Ray can't read; use a font with TrueType outlines (most .ttf files)";
        return false;
    }
    if ( nTag == 0x74746366U )  // "ttcf"
    {
        strError = "is a font collection; use a single font file";
        return false;
    }
    if ( nTag != 0x00010000U && nTag != 0x74727565U )  // 1.0 or "true"
    {
        strError = "isn't a TrueType font";
        return false;
    }

    unsigned nTables = U16( 4 );
    uint32_t nHead = 0, nMaxp = 0, nHhea = 0, nCmap = 0;
    uint32_t nHmtxLength = 0, nLocaLength = 0;

    if ( ! Has( 12, nTables * 16 ) )
    {
        strError = "is damaged (its table directory is cut off)";
        return false;
    }
    m_nGlyf = m_nLoca = m_nHmtx = 0;
    for ( unsigned n = 0; n < nTables; n++ )
    {
        uint32_t nRecord = 12 + ( n * 16 );
        uint32_t nName = U32( nRecord ), nOffset = U32( nRecord + 8 ), nLength = U32( nRecord + 12 );

        if ( ! Has( nOffset, nLength ) )
        {
            strError = "is damaged (a table runs past its end)";
            return false;
        }
        switch ( nName )
        {
            case 0x68656164U: nHead = nOffset; break;                                // head
            case 0x6D617870U: nMaxp = nOffset; break;                                // maxp
            case 0x68686561U: nHhea = nOffset; break;                                // hhea
            case 0x636D6170U: nCmap = nOffset; break;                                // cmap
            case 0x686D7478U: m_nHmtx = nOffset; nHmtxLength = nLength; break;       // hmtx
            case 0x6C6F6361U: m_nLoca = nOffset; nLocaLength = nLength; break;       // loca
            case 0x676C7966U: m_nGlyf = nOffset; m_nGlyfLength = nLength; break;     // glyf
            case 0x6B65726EU: m_nKern = nOffset; m_nKernLength = nLength; break;     // kern
            case 0x47504F53U: m_nGpos = nOffset; m_nGposLength = nLength; break;     // GPOS
            default: break;
        }
    }
    if ( ! nHead || ! nMaxp || ! nHhea || ! nCmap || ! m_nHmtx || ! m_nLoca || ! m_nGlyf )
    {
        strError = "lacks a table Ray needs (head, maxp, hhea, cmap, hmtx, loca and glyf)";
        return false;
    }

    m_nUnitsPerEm = (int)U16( nHead + 18 );
    m_nLocaFormat = S16( nHead + 50 );
    m_nGlyphs = (int)U16( nMaxp + 4 );
    m_nAscender = S16( nHhea + 4 );
    m_nDescender = S16( nHhea + 6 );
    m_nLineGap = S16( nHhea + 8 );
    m_nHMetrics = (int)U16( nHhea + 34 );
    if ( m_nUnitsPerEm < 16 || m_nUnitsPerEm > 16384 || m_nGlyphs < 1 || m_nHMetrics < 1 || m_nHMetrics > m_nGlyphs ||
         ( m_nLocaFormat != 0 && m_nLocaFormat != 1 ) )
    {
        strError = "is damaged (bad head, maxp or hhea values)";
        return false;
    }
    if ( nHmtxLength < (uint32_t)( ( m_nHMetrics * 4 ) + ( ( m_nGlyphs - m_nHMetrics ) * 2 ) ) ||
         nLocaLength < (uint32_t)( ( m_nGlyphs + 1 ) * ( m_nLocaFormat ? 4 : 2 ) ) )
    {
        strError = "is damaged (hmtx or loca is too short)";
        return false;
    }

        // The cmap subtable: Unicode in full (format 12) if there is one,
        // else the basic plane (format 4).
    unsigned nEncodings = U16( nCmap + 2 );

    m_nCmapTable = 0;
    m_nCmapFormat = 0;
    for ( unsigned n = 0; n < nEncodings; n++ )
    {
        uint32_t nRecord = nCmap + 4 + ( n * 8 );
        unsigned nPlatform = U16( nRecord ), nEncoding = U16( nRecord + 2 );
        uint32_t nSubtable = nCmap + U32( nRecord + 4 );
        unsigned nFormat = U16( nSubtable );
        bool     bUnicode = ( nPlatform == 0 ) || ( nPlatform == 3 && ( nEncoding == 1 || nEncoding == 10 ) );

        if ( ! bUnicode || ! Has( nSubtable, 8 ) )
            continue;
        if ( nFormat == 12 || ( nFormat == 4 && m_nCmapFormat != 12 ) )
        {
            m_nCmapTable = nSubtable;
            m_nCmapFormat = (int)nFormat;
        }
    }
    if ( m_nCmapFormat == 0 )
    {
        strError = "has no Unicode character map Ray can read (formats 4 and 12)";
        return false;
    }

    FindKernLookups();
    return true;
}

int CFontFile::GlyphIndex( uint32_t nCodePoint ) const
{
    uint32_t t = m_nCmapTable;

    if ( m_nCmapFormat == 12 )
    {
        uint32_t nGroups = U32( t + 12 );

        for ( uint32_t n = 0; n < nGroups && Has( t + 16 + ( n * 12 ), 12 ); n++ )
        {
            uint32_t nGroup = t + 16 + ( n * 12 );
            uint32_t nFirst = U32( nGroup ), nLast = U32( nGroup + 4 );

            if ( nCodePoint >= nFirst && nCodePoint <= nLast )
            {
                uint32_t nGlyph = U32( nGroup + 8 ) + ( nCodePoint - nFirst );

                return ( nGlyph < (uint32_t)m_nGlyphs ) ? (int)nGlyph : 0;
            }
        }
        return 0;
    }

        // Format 4: segments of the basic plane.
    if ( nCodePoint > 0xFFFF )
        return 0;

    unsigned nSegments = U16( t + 6 ) / 2;
    uint32_t nEnds = t + 14, nStarts = nEnds + ( nSegments * 2 ) + 2;
    uint32_t nDeltas = nStarts + ( nSegments * 2 ), nRanges = nDeltas + ( nSegments * 2 );

    for ( unsigned n = 0; n < nSegments; n++ )
    {
        unsigned nEnd = U16( nEnds + ( n * 2 ) );

        if ( nCodePoint > nEnd )
            continue;

        unsigned nStart = U16( nStarts + ( n * 2 ) );

        if ( nCodePoint < nStart )
            return 0;

        unsigned nDelta = U16( nDeltas + ( n * 2 ) );
        unsigned nRange = U16( nRanges + ( n * 2 ) );
        unsigned nGlyph;

        if ( nRange == 0 )
            nGlyph = ( nCodePoint + nDelta ) & 0xFFFF;
        else
        {
            uint32_t nAt = nRanges + ( n * 2 ) + nRange + ( ( nCodePoint - nStart ) * 2 );

            nGlyph = U16( nAt );
            if ( nGlyph != 0 )
                nGlyph = ( nGlyph + nDelta ) & 0xFFFF;
        }
        return ( nGlyph < (unsigned)m_nGlyphs ) ? (int)nGlyph : 0;
    }
    return 0;
}

int CFontFile::Advance( int nGlyph ) const
{
    if ( nGlyph < 0 || nGlyph >= m_nGlyphs )
        return 0;
    if ( nGlyph >= m_nHMetrics )
        nGlyph = m_nHMetrics - 1;
    return (int)U16( m_nHmtx + ( nGlyph * 4 ) );
}

bool CFontFile::GlyphRange( int nGlyph, uint32_t &nStart, uint32_t &nLength ) const
{
    if ( nGlyph < 0 || nGlyph >= m_nGlyphs )
        return false;

    uint32_t nFrom, nTo;

    if ( m_nLocaFormat == 0 )
    {
        nFrom = U16( m_nLoca + ( nGlyph * 2 ) ) * 2;
        nTo = U16( m_nLoca + ( ( nGlyph + 1 ) * 2 ) ) * 2;
    }
    else
    {
        nFrom = U32( m_nLoca + ( nGlyph * 4 ) );
        nTo = U32( m_nLoca + ( ( nGlyph + 1 ) * 4 ) );
    }
    if ( nTo < nFrom || nTo > m_nGlyfLength )
        return false;
    nStart = m_nGlyf + nFrom;
    nLength = nTo - nFrom;
    return true;
}

bool CFontFile::Outline( int nGlyph, std::vector<Contour> &aoContours, std::string &strError ) const
{
    aoContours.clear();
    return ReadGlyph( nGlyph, aoContours, 0, strError );
}

bool CFontFile::ReadGlyph( int nGlyph, std::vector<Contour> &aoContours, int nDepth, std::string &strError ) const
{
    uint32_t nStart, nLength;

    if ( nDepth > 8 )
    {
        strError = "has composite glyphs nested too deeply";
        return false;
    }
    if ( ! GlyphRange( nGlyph, nStart, nLength ) )
    {
        strError = "is damaged (a glyph is out of range)";
        return false;
    }
    if ( nLength == 0 )
        return true;    // a space: no outline
    if ( nLength < 10 )
    {
        strError = "is damaged (a glyph is cut off)";
        return false;
    }

    uint32_t nEnd = nStart + nLength;
    int      nContours = S16( nStart );
    uint32_t p = nStart + 10;

    if ( nContours >= 0 )
    {
            // A simple glyph: contour ends, instructions, flags, x, y.
        std::vector<int> anEnds( (size_t)nContours );

        if ( p + ( nContours * 2 ) + 2 > nEnd )
        {
            strError = "is damaged (a glyph is cut off)";
            return false;
        }
        for ( int n = 0; n < nContours; n++ )
        {
            anEnds[ (size_t)n ] = (int)U16( p );
            p += 2;
            if ( n > 0 && anEnds[ (size_t)n ] < anEnds[ (size_t)( n - 1 ) ] )
            {
                strError = "is damaged (a glyph's contours are out of order)";
                return false;
            }
        }
        p += 2 + U16( p );

        size_t                     nPoints = nContours ? (size_t)anEnds.back() + 1 : 0;
        std::vector<unsigned char> anFlags;

        anFlags.reserve( nPoints );
        while ( anFlags.size() < nPoints )
        {
            if ( p >= nEnd )
            {
                strError = "is damaged (a glyph's flags are cut off)";
                return false;
            }

            unsigned char nFlag = (unsigned char)U8( p++ );

            anFlags.push_back( nFlag );
            if ( nFlag & 0x08 )
            {
                if ( p >= nEnd )
                {
                    strError = "is damaged (a glyph's flags are cut off)";
                    return false;
                }

                unsigned nRepeat = U8( p++ );

                for ( unsigned r = 0; r < nRepeat && anFlags.size() < nPoints; r++ )
                    anFlags.push_back( nFlag );
            }
        }

        std::vector<double> anX( nPoints ), anY( nPoints );

        for ( int nAxis = 0; nAxis < 2; nAxis++ )
        {
            unsigned char nShort = ( nAxis == 0 ) ? 0x02 : 0x04;
            unsigned char nSame = ( nAxis == 0 ) ? 0x10 : 0x20;
            int           nValue = 0;

            for ( size_t n = 0; n < nPoints; n++ )
            {
                unsigned char nFlag = anFlags[ n ];

                if ( nFlag & nShort )
                {
                    if ( p + 1 > nEnd )
                    {
                        strError = "is damaged (a glyph's points are cut off)";
                        return false;
                    }

                    int nStep = (int)U8( p++ );

                    nValue += ( nFlag & nSame ) ? nStep : -nStep;
                }
                else if ( ! ( nFlag & nSame ) )
                {
                    if ( p + 2 > nEnd )
                    {
                        strError = "is damaged (a glyph's points are cut off)";
                        return false;
                    }
                    nValue += S16( p );
                    p += 2;
                }
                ( nAxis == 0 ? anX : anY )[ n ] = nValue;
            }
        }

        size_t nFirst = 0;

        for ( int n = 0; n < nContours; n++ )
        {
            Contour oContour;

            for ( size_t i = nFirst; i <= (size_t)anEnds[ (size_t)n ]; i++ )
            {
                SPoint oPoint = { anX[ i ], anY[ i ], ( anFlags[ i ] & 0x01 ) != 0 };

                oContour.push_back( oPoint );
            }
            nFirst = (size_t)anEnds[ (size_t)n ] + 1;
            if ( oContour.size() >= 2 )
                aoContours.push_back( oContour );
        }
        return true;
    }

        // A composite glyph: other glyphs, each placed by an offset and an
        // optional scale or 2x2 matrix.
    for ( ;; )
    {
        if ( p + 4 > nEnd )
        {
            strError = "is damaged (a composite glyph is cut off)";
            return false;
        }

        unsigned nFlags = U16( p ), nPart = U16( p + 2 );
        double   dx = 0.0, dy = 0.0;
        double   a = 1.0, b = 0.0, c = 0.0, d = 1.0;

        p += 4;
        if ( nFlags & 0x0001 )
        {
            if ( nFlags & 0x0002 )
            {
                dx = S16( p );
                dy = S16( p + 2 );
            }
            p += 4;
        }
        else
        {
            if ( nFlags & 0x0002 )
            {
                dx = (signed char)U8( p );
                dy = (signed char)U8( p + 1 );
            }
            p += 2;
        }
        if ( nFlags & 0x0008 )
        {
            a = d = S16( p ) / 16384.0;
            p += 2;
        }
        else if ( nFlags & 0x0040 )
        {
            a = S16( p ) / 16384.0;
            d = S16( p + 2 ) / 16384.0;
            p += 4;
        }
        else if ( nFlags & 0x0080 )
        {
            a = S16( p ) / 16384.0;
            b = S16( p + 2 ) / 16384.0;
            c = S16( p + 4 ) / 16384.0;
            d = S16( p + 6 ) / 16384.0;
            p += 8;
        }
        if ( p > nEnd )
        {
            strError = "is damaged (a composite glyph is cut off)";
            return false;
        }

        std::vector<Contour> aoPart;

        if ( ! ReadGlyph( (int)nPart, aoPart, nDepth + 1, strError ) )
            return false;
        for ( Contour &oContour : aoPart )
        {
            for ( SPoint &oPoint : oContour )
            {
                double x = oPoint.x, y = oPoint.y;

                oPoint.x = ( a * x ) + ( c * y ) + dx;
                oPoint.y = ( b * x ) + ( d * y ) + dy;
            }
            aoContours.push_back( oContour );
        }
        if ( ! ( nFlags & 0x0020 ) )
            break;
    }
    return true;
}

void CFontFile::FindKernLookups( void )
{
    if ( m_nGpos == 0 || m_nGposLength < 10 )
        return;

    uint32_t g = m_nGpos;
    uint32_t nFeatures = g + U16( g + 6 );
    uint32_t nLookups = g + U16( g + 8 );
    unsigned nFeatureCount = U16( nFeatures );
    unsigned nLookupCount = U16( nLookups );
    std::vector<bool> abKern( nLookupCount, false );

        // Every lookup any "kern" feature uses (for any script).
    for ( unsigned n = 0; n < nFeatureCount; n++ )
    {
        uint32_t nRecord = nFeatures + 2 + ( n * 6 );

        if ( U32( nRecord ) != 0x6B65726EU )  // "kern"
            continue;

        uint32_t nFeature = nFeatures + U16( nRecord + 4 );
        unsigned nIndices = U16( nFeature + 2 );

        for ( unsigned i = 0; i < nIndices; i++ )
        {
            unsigned nLookup = U16( nFeature + 4 + ( i * 2 ) );

            if ( nLookup < nLookupCount )
                abKern[ nLookup ] = true;
        }
    }
    for ( unsigned n = 0; n < nLookupCount; n++ )
    {
        if ( ! abKern[ n ] )
            continue;

        uint32_t nLookup = nLookups + U16( nLookups + 2 + ( n * 2 ) );
        unsigned nType = U16( nLookup );
        unsigned nSubtables = U16( nLookup + 4 );

        for ( unsigned s = 0; s < nSubtables; s++ )
        {
            uint32_t nSubtable = nLookup + U16( nLookup + 6 + ( s * 2 ) );
            unsigned nSubType = nType;

            if ( nType == 9 && U16( nSubtable ) == 1 )
            {
                    // An extension lookup points on to the real subtable.
                nSubType = U16( nSubtable + 2 );
                nSubtable = nSubtable + U32( nSubtable + 4 );
            }
            if ( nSubType == 2 && Has( nSubtable, 10 ) )
                m_anKernLookups.push_back( nSubtable );
        }
        // Each lookup's subtables are kept in order; the first that covers a
        // pair gives its adjustment.
        m_anKernLookups.push_back( 0 );
    }
}

int CFontFile::CoverageIndex( uint32_t nCoverage, int nGlyph ) const
{
    unsigned nFormat = U16( nCoverage );

    if ( nFormat == 1 )
    {
        unsigned nCount = U16( nCoverage + 2 );
        int      nLow = 0, nHigh = (int)nCount - 1;

        while ( nLow <= nHigh )
        {
            int      nMiddle = ( nLow + nHigh ) / 2;
            unsigned nAt = U16( nCoverage + 4 + ( nMiddle * 2 ) );

            if ( (int)nAt == nGlyph )
                return nMiddle;
            if ( (int)nAt < nGlyph )
                nLow = nMiddle + 1;
            else
                nHigh = nMiddle - 1;
        }
        return -1;
    }
    if ( nFormat == 2 )
    {
        unsigned nRanges = U16( nCoverage + 2 );

        for ( unsigned n = 0; n < nRanges; n++ )
        {
            uint32_t nRange = nCoverage + 4 + ( n * 6 );
            int      nFirst = (int)U16( nRange ), nLast = (int)U16( nRange + 2 );

            if ( nGlyph >= nFirst && nGlyph <= nLast )
                return (int)U16( nRange + 4 ) + ( nGlyph - nFirst );
        }
    }
    return -1;
}

int CFontFile::ClassOf( uint32_t nClassDef, int nGlyph ) const
{
    unsigned nFormat = U16( nClassDef );

    if ( nFormat == 1 )
    {
        int      nFirst = (int)U16( nClassDef + 2 );
        unsigned nCount = U16( nClassDef + 4 );

        if ( nGlyph >= nFirst && nGlyph < nFirst + (int)nCount )
            return (int)U16( nClassDef + 6 + ( ( nGlyph - nFirst ) * 2 ) );
        return 0;
    }
    if ( nFormat == 2 )
    {
        unsigned nRanges = U16( nClassDef + 2 );

        for ( unsigned n = 0; n < nRanges; n++ )
        {
            uint32_t nRange = nClassDef + 4 + ( n * 6 );

            if ( nGlyph >= (int)U16( nRange ) && nGlyph <= (int)U16( nRange + 2 ) )
                return (int)U16( nRange + 4 );
        }
    }
    return 0;
}

    // The number of bits set: each set bit of a value format is one 16-bit
    // field of the value record.
static int Bits( unsigned nValue )
{
    int nCount = 0;

    for ( ; nValue; nValue >>= 1 )
        nCount += (int)( nValue & 1 );
    return nCount;
}

bool CFontFile::PairAdjust( uint32_t nSubtable, int nLeft, int nRight, int &nAdjust ) const
{
    unsigned nFormat = U16( nSubtable );
    uint32_t nCoverage = nSubtable + U16( nSubtable + 2 );
    unsigned nFormat1 = U16( nSubtable + 4 ), nFormat2 = U16( nSubtable + 6 );
    int      nSize1 = Bits( nFormat1 ) * 2, nSize2 = Bits( nFormat2 ) * 2;
        // Where the x advance sits in the first glyph's value record.
    int      nXAdvance = ( nFormat1 & 0x0004 ) ? Bits( nFormat1 & 0x0003 ) * 2 : -1;
    int      nIndex = CoverageIndex( nCoverage, nLeft );

    if ( nIndex < 0 )
        return false;

    nAdjust = 0;
    if ( nFormat == 1 )
    {
        uint32_t nSet = nSubtable + U16( nSubtable + 10 + ( nIndex * 2 ) );
        unsigned nPairs = U16( nSet );
        int      nRecord = 2 + nSize1 + nSize2;

        for ( unsigned n = 0; n < nPairs; n++ )
        {
            uint32_t nPair = nSet + 2 + ( n * nRecord );

            if ( (int)U16( nPair ) == nRight )
            {
                if ( nXAdvance >= 0 )
                    nAdjust = S16( nPair + 2 + nXAdvance );
                return true;
            }
        }
        return false;
    }
    if ( nFormat == 2 )
    {
        uint32_t nClassDef1 = nSubtable + U16( nSubtable + 8 );
        uint32_t nClassDef2 = nSubtable + U16( nSubtable + 10 );
        unsigned nClass1Count = U16( nSubtable + 12 ), nClass2Count = U16( nSubtable + 14 );
        int      nClass1 = ClassOf( nClassDef1, nLeft ), nClass2 = ClassOf( nClassDef2, nRight );

        if ( nClass1 >= (int)nClass1Count || nClass2 >= (int)nClass2Count )
            return false;

        uint32_t nRecord = nSubtable + 16 + ( ( ( nClass1 * nClass2Count ) + nClass2 ) * ( nSize1 + nSize2 ) );

        if ( nXAdvance >= 0 )
            nAdjust = S16( nRecord + nXAdvance );
        return true;
    }
    return false;
}

int CFontFile::Kerning( int nLeft, int nRight ) const
{
    if ( ! m_anKernLookups.empty() )
    {
            // GPOS: for each lookup (subtables until a 0 marker), the first
            // subtable covering the pair applies; the lookups add up.
        int  nTotal = 0;
        bool bDone = false;

        for ( uint32_t nSubtable : m_anKernLookups )
        {
            if ( nSubtable == 0 )
            {
                bDone = false;
                continue;
            }
            if ( bDone )
                continue;

            int nAdjust;

            if ( PairAdjust( nSubtable, nLeft, nRight, nAdjust ) )
            {
                nTotal += nAdjust;
                bDone = true;
            }
        }
        return nTotal;
    }
    if ( m_nKern != 0 && m_nKernLength >= 18 )
    {
            // The old kern table: the first format 0 subtable, horizontal.
        uint32_t k = m_nKern + 4;
        unsigned nCoverage = U16( k + 4 );

        if ( ( nCoverage >> 8 ) == 0 && ( nCoverage & 0x0001 ) )
        {
            unsigned nPairs = U16( k + 6 );
            uint32_t nKey = ( (uint32_t)nLeft << 16 ) | (uint32_t)nRight;
            int      nLow = 0, nHigh = (int)nPairs - 1;

            while ( nLow <= nHigh )
            {
                int      nMiddle = ( nLow + nHigh ) / 2;
                uint32_t nAt = U32( k + 14 + ( nMiddle * 6 ) );

                if ( nAt == nKey )
                    return S16( k + 18 + ( nMiddle * 6 ) );
                if ( nAt < nKey )
                    nLow = nMiddle + 1;
                else
                    nHigh = nMiddle - 1;
            }
        }
    }
    return 0;
}
