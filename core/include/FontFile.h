/*************************************************************************
*   CFontFile                                                            *
*                                                                        *
*   Reads a TrueType font (a .ttf, or a .otf with TrueType outlines):    *
*   which glyph draws a character (the cmap, formats 4 and 12), each     *
*   glyph's advance width (hmtx), its outline (glyf, simple and          *
*   composite glyphs), the line metrics (hhea), and pair kerning from    *
*   the old kern table (format 0) or the OpenType GPOS table's "kern"    *
*   feature (pair adjustment lookups, formats 1 and 2, through extension *
*   lookups too). Everything is in the font's own units, unitsPerEm to   *
*   the em; y is up.                                                     *
*                                                                        *
*   Fonts with CFF (PostScript, cubic) outlines, font collections and    *
*   anything else are refused with a reason. Only the outlines are read: *
*   hinting is ignored, which is right for a ray tracer, which draws     *
*   letters at any size and angle.                                       *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __FONTFILE_H__
#define __FONTFILE_H__

#include <cstdint>
#include <string>
#include <vector>

class CFontFile
{
    public:
            // A point of a glyph's outline: on the curve, or the control
            // point of a quadratic Bezier between its neighbours.
        struct SPoint
        {
            double      x;
            double      y;
            bool        bOnCurve;
        };
        typedef std::vector<SPoint> Contour;

    private:
        std::string         m_strData;
        uint32_t            m_nGlyf, m_nGlyfLength;
        uint32_t            m_nLoca;
        uint32_t            m_nHmtx;
        uint32_t            m_nKern, m_nKernLength;
        uint32_t            m_nGpos, m_nGposLength;
        uint32_t            m_nCmapTable;       // the subtable used
        int                 m_nCmapFormat;
        int                 m_nGlyphs;
        int                 m_nHMetrics;
        int                 m_nLocaFormat;
        int                 m_nUnitsPerEm;
        int                 m_nAscender, m_nDescender, m_nLineGap;
        std::vector<uint32_t> m_anKernLookups;  // offsets (in the file) of GPOS pair-adjustment subtables, in order

        bool                Has( uint32_t nOffset, uint32_t nLength ) const;
        unsigned            U8( uint32_t nOffset ) const;
        unsigned            U16( uint32_t nOffset ) const;
        int                 S16( uint32_t nOffset ) const;
        uint32_t            U32( uint32_t nOffset ) const;
        bool                GlyphRange( int nGlyph, uint32_t &nStart, uint32_t &nLength ) const;
        bool                ReadGlyph( int nGlyph, std::vector<Contour> &aoContours, int nDepth, std::string &strError ) const;
        void                FindKernLookups( void );
        int                 CoverageIndex( uint32_t nCoverage, int nGlyph ) const;
        int                 ClassOf( uint32_t nClassDef, int nGlyph ) const;
        bool                PairAdjust( uint32_t nSubtable, int nLeft, int nRight, int &nAdjust ) const;

    public:
                            CFontFile( void );

            /// Reads a font from its bytes. FALSE, with a reason, if it isn't
            /// a TrueType font Ray can use.
        bool                Load( const std::string &strData, std::string &strError );

        int                 UnitsPerEm( void ) const { return m_nUnitsPerEm; }
        int                 Ascender( void ) const { return m_nAscender; }
        int                 Descender( void ) const { return m_nDescender; }
        int                 LineGap( void ) const { return m_nLineGap; }
        int                 GlyphCount( void ) const { return m_nGlyphs; }
            /// The glyph for a Unicode code point (0, the missing glyph, if
            /// the font has none).
        int                 GlyphIndex( uint32_t nCodePoint ) const;
        int                 Advance( int nGlyph ) const;
            /// How much to add to the advance between two glyphs (usually 0
            /// or negative), from the font's kerning.
        int                 Kerning( int nLeft, int nRight ) const;
            /// Whether the font has any kerning Ray reads.
        bool                HasKerning( void ) const { return m_nKern != 0 || ! m_anKernLookups.empty(); }
            /// A glyph's outline as closed contours (composite glyphs put
            /// together). FALSE, with a reason, if the glyph is damaged.
        bool                Outline( int nGlyph, std::vector<Contour> &aoContours, std::string &strError ) const;
};

#endif // __FONTFILE_H__
