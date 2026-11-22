/*************************************************************************
*   CText                                                                *
*                                                                        *
*   Solid letters from a TrueType font. See Text.h.                      *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

    #include "Globals.h"

#include "Text.h"
#include "FontFile.h"
#include "LocalShape.h"
#include "Outline.h"
#include "Extruder.h"
#include "SceneJson.h"

#include <cmath>


DYNAMIC_IMPLEMENTS( CText, "text" );

namespace
{
    const char *const s_astrAligns[] = { "left", "center", "right" };

        // The code points of a UTF-8 string (a bad byte stands for itself).
    std::vector<uint32_t> Decode( const std::string &strText )
    {
        std::vector<uint32_t> anOut;

        for ( size_t i = 0; i < strText.size(); )
        {
            unsigned char c = (unsigned char)strText[ i ];
            int           nMore = ( c >= 0xF0 ) ? 3 : ( c >= 0xE0 ) ? 2 : ( c >= 0xC0 ) ? 1 : 0;
            uint32_t      nCode = ( nMore == 3 ) ? ( c & 0x07 ) : ( nMore == 2 ) ? ( c & 0x0F ) : ( nMore == 1 ) ? ( c & 0x1F ) : c;

            if ( nMore > 0 && i + (size_t)nMore >= strText.size() )
            {
                nMore = 0;
                nCode = c;
            }
            for ( int k = 1; k <= nMore; k++ )
                nCode = ( nCode << 6 ) | ( (unsigned char)strText[ i + (size_t)k ] & 0x3F );
            anOut.push_back( nCode );
            i += 1 + (size_t)nMore;
        }
        return anOut;
    }
}

CText::CText( void ) :
    m_nSize( 1.0 ),
    m_nDepth( 0.2 ),
    m_nBevel( 0.0 ),
    m_nAlign( 0 ),
    m_nLineSpacing( 1.0 ),
    m_bKerning( TRUE ),
    m_oPosition( 0.0, 0.0, 0.0 ),
    m_oTextRotation( 0.0, 0.0, 0.0 )
{
}

CText::~CText( void )
{
}


BOOL CText::SetText( const std::string &strString, const std::string &strFontData, FLT nSize, FLT nDepth, FLT nBevel, std::string &strError )
{
    m_strString = strString;
    m_pFontData.reset( new std::string( strFontData ) );
    m_nSize = nSize;
    m_nDepth = nDepth;
    m_nBevel = nBevel;
    return Build( strError );
}

BOOL CText::Build( std::string &strError )
{
    CFontFile oFont;

    if ( ! m_pFontData || ! oFont.Load( *m_pFontData, strError ) )
    {
        if ( ! m_pFontData )
            strError = "has no font";
        return FALSE;
    }

    double                  nScale = m_nSize / oFont.UnitsPerEm();
    double                  nTolerance = 0.0015 * oFont.UnitsPerEm();
    double                  nLineHeight = ( oFont.Ascender() - oFont.Descender() + oFont.LineGap() ) * m_nLineSpacing;
    std::vector<uint32_t>   anCodes = Decode( m_strString );
    Extruder::SBuilder      oOut;
    size_t                  nLineStart = 0;
    int                     nLine = 0;

    oOut.nPerUnit = 1.0 / m_nSize;
    while ( nLineStart <= anCodes.size() )
    {
        size_t nLineEnd = nLineStart;

        while ( nLineEnd < anCodes.size() && anCodes[ nLineEnd ] != '\n' )
            nLineEnd++;

            // Where each glyph of the line goes, and the line's width.
        std::vector<int>    anGlyphs;
        std::vector<double> anPen;
        double              nPen = 0.0;
        int                 nPrevious = -1;

        for ( size_t i = nLineStart; i < nLineEnd; i++ )
        {
            if ( anCodes[ i ] == '\r' )
                continue;

            int nGlyph = oFont.GlyphIndex( anCodes[ i ] );

            if ( m_bKerning && nPrevious >= 0 )
                nPen += oFont.Kerning( nPrevious, nGlyph );
            anGlyphs.push_back( nGlyph );
            anPen.push_back( nPen );
            nPen += oFont.Advance( nGlyph );
            nPrevious = nGlyph;
        }

        double nShift = ( m_nAlign == 1 ) ? -nPen / 2.0 : ( m_nAlign == 2 ) ? -nPen : 0.0;
        double nBaseline = -nLine * nLineHeight;

        for ( size_t g = 0; g < anGlyphs.size(); g++ )
        {
            std::vector<CFontFile::Contour> aoContours;
            std::vector<Outline::Polygon>   aoOutlines;
            Outline::SShape                 oShape;

            if ( ! oFont.Outline( anGlyphs[ g ], aoContours, strError ) )
                return FALSE;
            for ( const CFontFile::Contour &oContour : aoContours )
            {
                Outline::Polygon oPolygon = Outline::Flatten( oContour, nTolerance );

                for ( Outline::SPoint &p : oPolygon )
                {
                    p.x = ( p.x + anPen[ g ] + nShift ) * nScale;
                    p.y = ( p.y + nBaseline ) * nScale;
                }
                if ( oPolygon.size() >= 3 )
                    aoOutlines.push_back( oPolygon );
            }
            if ( aoOutlines.empty() )
                continue;
            if ( ! Outline::Fill( aoOutlines, oShape, strError ) )
            {
                strError = "can't fill a glyph of the text: it " + strError;
                return FALSE;
            }
            Extruder::Extrude( oOut, oShape, m_nDepth, m_nBevel );
        }
        nLineStart = nLineEnd + 1;
        nLine++;
    }
    if ( oOut.anFaces.empty() )
    {
        strError = "has nothing to draw (the string is empty or only spaces)";
        return FALSE;
    }

        // Turned, then moved to its place.
    Extruder::Place( oOut, m_oTextRotation, m_oPosition );
        // Double-sided, so a ray inside a letter (through glass, or in a
        // csg) meets the faces around it from within.
    if ( ! SetGeometry( oOut.aoVertices, oOut.anFaces, TRUE, TRUE, oOut.aoNormals, oOut.anUVs ) )
    {
        strError = "made a bad mesh";
        return FALSE;
    }
    return TRUE;
}

void CText::ToJson( IASLib::CObjectNode &oNode )
{
    oNode.put( "string", m_strString.c_str() );
    oNode.put( "font", RayJson::SceneFileName( m_strFont, m_strFontPath, m_pFontData ) );
    if ( m_pFontData && RayJson::EmbeddingData() )
    {
        std::vector<unsigned char> anData( m_pFontData->begin(), m_pFontData->end() );

        oNode.putBinary( "data", anData );
    }
    RayJson::PutNumber( oNode, "size", m_nSize );
    RayJson::PutNumber( oNode, "depth", m_nDepth );
    if ( m_nBevel > 0.0 )
        RayJson::PutNumber( oNode, "bevel", m_nBevel );
    if ( m_nAlign != 0 )
        oNode.put( "align", s_astrAligns[ m_nAlign ] );
    if ( m_nLineSpacing != 1.0 )
        RayJson::PutNumber( oNode, "lineSpacing", m_nLineSpacing );
    if ( ! m_bKerning )
        RayJson::PutBool( oNode, "kerning", m_bKerning );
    RayJson::PutVector( oNode, "position", m_oPosition );
    if ( m_oTextRotation.GetX() != 0.0 || m_oTextRotation.GetY() != 0.0 || m_oTextRotation.GetZ() != 0.0 )
        RayJson::PutVector( oNode, "rotation", m_oTextRotation );
    RayJson::PutColor( oNode, "color", m_oColor );
    m_oMaterials.ToJson( *oNode.putArray( "materials" ) );
    BumpToJson( oNode );
}

BOOL CText::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "string", "font", "data", "size", "depth", "bevel", "align", "lineSpacing", "kerning",
                                              "position", "rotation", "color", "materials", "bump", NULL };
    std::string strAlign = s_astrAligns[ m_nAlign ];

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetString( oNode, "string", m_strString, TRUE ) ||
         ! RayJson::GetString( oNode, "font", m_strFont, TRUE ) ||
         ! RayJson::GetNumber( oNode, "size", m_nSize, FALSE ) ||
         ! RayJson::GetNumber( oNode, "depth", m_nDepth, FALSE ) ||
         ! RayJson::GetNumber( oNode, "bevel", m_nBevel, FALSE ) ||
         ! RayJson::GetString( oNode, "align", strAlign, FALSE ) ||
         ! RayJson::GetNumber( oNode, "lineSpacing", m_nLineSpacing, FALSE ) ||
         ! RayJson::GetBool( oNode, "kerning", m_bKerning, FALSE ) ||
         ! RayJson::GetVector( oNode, "position", m_oPosition, FALSE ) ||
         ! RayJson::GetVector( oNode, "rotation", m_oTextRotation, FALSE ) ||
         ! RayJson::GetColor( oNode, "color", m_oColor, FALSE ) ||
         ! m_oMaterials.FromJson( oNode, "materials" ) ||
         ! BumpFromJson( oNode ) )
        return FALSE;
    if ( m_strFont.empty() )
        return RayJson::Fail( "font can't be empty" );
    if ( ! ( m_nSize > 0.0 ) )
        return RayJson::Fail( "size must be more than 0" );
    if ( ! ( m_nDepth > 0.0 ) )
        return RayJson::Fail( "depth must be more than 0" );
    if ( m_nBevel < 0.0 || ! ( m_nBevel < m_nDepth ) )
        return RayJson::Fail( "bevel must be at least 0 and less than the depth" );
    if ( ! ( m_nLineSpacing > 0.0 ) )
        return RayJson::Fail( "lineSpacing must be more than 0" );

    m_nAlign = -1;
    for ( int nAlign = 0; nAlign < 3; nAlign++ )
    {
        if ( strAlign == s_astrAligns[ nAlign ] )
            m_nAlign = nAlign;
    }
    if ( m_nAlign < 0 )
    {
        m_nAlign = 0;
        return RayJson::Fail( "unknown align \"" + strAlign + "\" (left, center or right)" );
    }

    std::string              strData;
    std::string              strError;
    const IASLib::CJsonNode *pData = oNode.get( "data" );

    if ( pData )
    {
            // The font's bytes, carried in a copy of the scene sent to a task.
        std::vector<unsigned char> anData;

        if ( ! RayJson::BinaryValue( *pData, anData ) )
            return RayJson::Fail( "data must be the font file's bytes, in base64" );
        strData.assign( anData.begin(), anData.end() );
        m_strFontPath.clear();
    }
    else if ( ! RayJson::ReadSceneFile( m_strFont, strData, m_strFontPath, strError ) )
        return RayJson::Fail( "can't open the font \"" + m_strFont + "\"" );

    m_pFontData.reset( new std::string( strData ) );
    if ( ! Build( strError ) )
        return RayJson::Fail( "the font \"" + m_strFont + "\" " + strError );
    return TRUE;
}
