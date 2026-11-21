/*************************************************************************
*   CText                                                                *
*                                                                        *
*   Solid letters from a TrueType font. See Text.h.                      *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Text.h"
#include "FontFile.h"
#include "LocalShape.h"
#include "Outline.h"
#include "SceneJson.h"

#include <cmath>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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

        // Collects the mesh: vertices with normals and surface coordinates,
        // and faces, each turned so it faces the way it should.
    struct SBuilder
    {
        std::vector<CVector>    aoVertices;
        std::vector<CVector>    aoNormals;
        std::vector<FLT>        anUVs;
        std::vector<int>        anFaces;
        FLT                     nPerEm;         // 1 / size, for u and v

        int Add( double x, double y, double z, double nx, double ny, double nz )
        {
            double l = std::sqrt( ( nx * nx ) + ( ny * ny ) + ( nz * nz ) );

            aoVertices.push_back( CVector( x, y, z ) );
            aoNormals.push_back( CVector( nx / l, ny / l, nz / l ) );
            anUVs.push_back( x * nPerEm );
            anUVs.push_back( y * nPerEm );
            return (int)aoVertices.size() - 1;
        }

        void Face( int a, int b, int c, double nx, double ny, double nz )
        {
            CVector &A = aoVertices[ (size_t)a ], &B = aoVertices[ (size_t)b ], &C = aoVertices[ (size_t)c ];
            CVector  e1 = B - A, e2 = C - B;
            CVector  n = e1.Cross( e2 );

                // The rule CTriangle uses for which way a face faces.
            if ( ( n.GetX() * nx ) + ( n.GetY() * ny ) + ( n.GetZ() * nz ) < 0.0 )
                std::swap( b, c );
            anFaces.push_back( a );
            anFaces.push_back( b );
            anFaces.push_back( c );
        }

        void Quad( int a, int b, int c, int d, double nx, double ny, double nz )
        {
            Face( a, b, c, nx, ny, nz );
            Face( a, c, d, nx, ny, nz );
        }
    };

        // Twice the area of a shape's triangles.
    double TriangleArea2( const Outline::SShape &oShape )
    {
        double nArea = 0.0;

        for ( size_t t = 0; t + 2 < oShape.anTriangles.size(); t += 3 )
        {
            const Outline::SPoint &a = oShape.aoPoints[ (size_t)oShape.anTriangles[ t ] ];
            const Outline::SPoint &b = oShape.aoPoints[ (size_t)oShape.anTriangles[ t + 1 ] ];
            const Outline::SPoint &c = oShape.aoPoints[ (size_t)oShape.anTriangles[ t + 2 ] ];

            nArea += ( ( b.x - a.x ) * ( c.y - a.y ) ) - ( ( b.y - a.y ) * ( c.x - a.x ) );
        }
        return nArea;
    }

        // A flat face of the shape at depth z, facing nFacing (-1 or 1) in z.
    void Cap( SBuilder &oOut, const Outline::SShape &oShape, double z, double nFacing )
    {
        std::vector<int> anIndex( oShape.aoPoints.size() );

        for ( size_t i = 0; i < oShape.aoPoints.size(); i++ )
            anIndex[ i ] = oOut.Add( oShape.aoPoints[ i ].x, oShape.aoPoints[ i ].y, z, 0.0, 0.0, nFacing );
        for ( size_t t = 0; t + 2 < oShape.anTriangles.size(); t += 3 )
            oOut.Face( anIndex[ (size_t)oShape.anTriangles[ t ] ], anIndex[ (size_t)oShape.anTriangles[ t + 1 ] ],
                       anIndex[ (size_t)oShape.anTriangles[ t + 2 ] ], 0.0, 0.0, nFacing );
    }

        // Each loop vertex's outward normal (in the plane) for the edge
        // before it and the edge after it: the same, averaged, where the
        // outline turns gently; each edge's own at a corner.
    void LoopNormals( const std::vector<Outline::SPoint> &aoLoop, std::vector<Outline::SPoint> &aoBefore,
                      std::vector<Outline::SPoint> &aoAfter )
    {
        size_t                       nN = aoLoop.size();
        std::vector<Outline::SPoint> aoEdge( nN );
        const double                 nSmooth = std::cos( 35.0 * PI / 180.0 );

        for ( size_t k = 0; k < nN; k++ )
        {
            const Outline::SPoint &a = aoLoop[ k ], &b = aoLoop[ ( k + 1 ) % nN ];
            double                 l = std::hypot( b.x - a.x, b.y - a.y );
            Outline::SPoint        n = { ( b.y - a.y ) / l, -( b.x - a.x ) / l };

            aoEdge[ k ] = n;
        }
        aoBefore.resize( nN );
        aoAfter.resize( nN );
        for ( size_t k = 0; k < nN; k++ )
        {
            const Outline::SPoint &p = aoEdge[ ( k + nN - 1 ) % nN ], &q = aoEdge[ k ];

            if ( ( p.x * q.x ) + ( p.y * q.y ) >= nSmooth )
            {
                double          l = std::hypot( p.x + q.x, p.y + q.y );
                Outline::SPoint m = { ( p.x + q.x ) / l, ( p.y + q.y ) / l };

                aoBefore[ k ] = aoAfter[ k ] = m;
            }
            else
            {
                aoBefore[ k ] = p;
                aoAfter[ k ] = q;
            }
        }
    }

        // The band between two copies of a loop: aoFrom at depth z0 and aoTo
        // at z1, the normals leaning nLean in z (0 for the straight sides).
    void Band( SBuilder &oOut, const std::vector<Outline::SPoint> &aoFrom, double z0, const std::vector<Outline::SPoint> &aoTo, double z1,
               const std::vector<Outline::SPoint> &aoBefore, const std::vector<Outline::SPoint> &aoAfter, double nLean )
    {
        size_t nN = aoFrom.size();

        for ( size_t k = 0; k < nN; k++ )
        {
            size_t                 k1 = ( k + 1 ) % nN;
            const Outline::SPoint &n0 = aoAfter[ k ], &n1 = aoBefore[ k1 ];
            double                 ex = aoTo[ k1 ].y - aoTo[ k ].y, ey = -( aoTo[ k1 ].x - aoTo[ k ].x );
            int                    a = oOut.Add( aoFrom[ k ].x, aoFrom[ k ].y, z0, n0.x, n0.y, nLean );
            int                    b = oOut.Add( aoFrom[ k1 ].x, aoFrom[ k1 ].y, z0, n1.x, n1.y, nLean );
            int                    c = oOut.Add( aoTo[ k1 ].x, aoTo[ k1 ].y, z1, n1.x, n1.y, nLean );
            int                    d = oOut.Add( aoTo[ k ].x, aoTo[ k ].y, z1, n0.x, n0.y, nLean );
            double                 l = std::hypot( ex, ey );

            if ( l > 0.0 )
                oOut.Quad( a, b, c, d, ex / l, ey / l, nLean );
        }
    }

    std::vector<Outline::SPoint> LoopPoints( const Outline::SShape &oShape, size_t nLoop )
    {
        std::vector<Outline::SPoint> aoOut;

        for ( int i : oShape.aanLoops[ nLoop ] )
            aoOut.push_back( oShape.aoPoints[ (size_t)i ] );
        return aoOut;
    }

        // A glyph's filled shape, extruded nDepth back with a front bevel.
    void Extrude( SBuilder &oOut, const Outline::SShape &oShape, double nDepth, double nBevel )
    {
        Outline::SShape               oFront;
        bool                          bBevel = false;
        std::vector<Outline::Polygon> aoInset;

        if ( nBevel > 0.0 )
        {
                // The bevel's inner edge: the outline moved in. Used only if
                // it fills cleanly and keeps every point (a stroke too thin
                // for the bevel is left square).
            std::string strError;

            aoInset = Outline::Inset( oShape, nBevel );

            if ( Outline::Fill( aoInset, oFront, strError ) && oFront.aanLoops.size() == oShape.aanLoops.size() )
            {
                double nInner = TriangleArea2( oFront ), nOuter = TriangleArea2( oShape );

                bBevel = nInner > 0.0 && nInner < nOuter;
                for ( size_t n = 0; bBevel && n < oShape.aanLoops.size(); n++ )
                    bBevel = oFront.aanLoops[ n ].size() == oShape.aanLoops[ n ].size();
            }
        }

        double nSide = bBevel ? nBevel : 0.0;

        Cap( oOut, bBevel ? oFront : oShape, 0.0, -1.0 );
        Cap( oOut, oShape, nDepth, 1.0 );
        for ( size_t n = 0; n < oShape.aanLoops.size(); n++ )
        {
            std::vector<Outline::SPoint> aoLoop = LoopPoints( oShape, n );
            std::vector<Outline::SPoint> aoBefore, aoAfter;

            LoopNormals( aoLoop, aoBefore, aoAfter );
            if ( bBevel )
            {
                    // The bevel from the inset outline (point for point
                    // with the loop) at the face back to the loop itself.
                Band( oOut, aoInset[ n ], 0.0, aoLoop, nSide, aoBefore, aoAfter, -1.0 );
            }
            Band( oOut, aoLoop, nSide, aoLoop, nDepth, aoBefore, aoAfter, 0.0 );
        }
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

#ifdef RAY_GUI
void CText::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    HTREEITEM   hText;

    hText = pTree->AddObject( this, 24, "Text", hUnder );
    CObjects::MatToTree( pTree, hText );
}
#endif

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
    SBuilder                oOut;
    size_t                  nLineStart = 0;
    int                     nLine = 0;

    oOut.nPerEm = 1.0 / m_nSize;
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
            Extrude( oOut, oShape, m_nDepth, m_nBevel );
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
    FLT nToRadians = PI / 180.0;

    for ( size_t i = 0; i < oOut.aoVertices.size(); i++ )
    {
        FLT anPoint[ 3 ] = { oOut.aoVertices[ i ].GetX(), oOut.aoVertices[ i ].GetY(), oOut.aoVertices[ i ].GetZ() };
        FLT anNormal[ 3 ] = { oOut.aoNormals[ i ].GetX(), oOut.aoNormals[ i ].GetY(), oOut.aoNormals[ i ].GetZ() };

        CLocalShape::RotateXYZ( anPoint, m_oTextRotation.GetX() * nToRadians, m_oTextRotation.GetY() * nToRadians,
                                m_oTextRotation.GetZ() * nToRadians );
        CLocalShape::RotateXYZ( anNormal, m_oTextRotation.GetX() * nToRadians, m_oTextRotation.GetY() * nToRadians,
                                m_oTextRotation.GetZ() * nToRadians );
        oOut.aoVertices[ i ] = CVector( anPoint[ 0 ], anPoint[ 1 ], anPoint[ 2 ] ) + m_oPosition;
        oOut.aoNormals[ i ] = CVector( anNormal[ 0 ], anNormal[ 1 ], anNormal[ 2 ] );
    }
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

        if ( ! pData->binaryValue( anData ) )
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
