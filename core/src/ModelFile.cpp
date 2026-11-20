/*************************************************************************
*   ModelFile                                                            *
*                                                                        *
*   See ModelFile.h.                                                     *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "ModelFile.h"

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <map>
#include <tuple>
#include <unordered_map>

namespace
{
    std::string Lower( const std::string &strText )
    {
        std::string strLower( strText );

        for ( size_t nIndex = 0; nIndex < strLower.size(); nIndex++ )
        {
            if ( strLower[ nIndex ] >= 'A' && strLower[ nIndex ] <= 'Z' )
                strLower[ nIndex ] = (char)( strLower[ nIndex ] - 'A' + 'a' );
        }
        return strLower;
    }

    BOOL EndsWith( const std::string &strText, const char *strEnd )
    {
        size_t nLength = strlen( strEnd );

        return strText.size() >= nLength && strText.compare( strText.size() - nLength, nLength, strEnd ) == 0;
    }

    uint32_t Little32( const unsigned char *pData )
    {
        return (uint32_t)pData[ 0 ] | ( (uint32_t)pData[ 1 ] << 8 ) | ( (uint32_t)pData[ 2 ] << 16 ) | ( (uint32_t)pData[ 3 ] << 24 );
    }

    float LittleFloat( const unsigned char *pData )
    {
        uint32_t nBits = Little32( pData );
        float    fValue;

        memcpy( &fValue, &nBits, sizeof( fValue ) );
        return fValue;
    }

        // Joins corners at exactly the same place into one vertex.
    class CWelder
    {
        public:
            explicit CWelder( ModelFile::SModel &oModel ) : m_oModel( oModel ) {}

            int Add( double dX, double dY, double dZ )
            {
                SKey oKey = { dX, dY, dZ };
                auto it = m_oIndex.find( oKey );

                if ( it != m_oIndex.end() )
                    return it->second;

                int nIndex = (int)m_oModel.aoVertices.size();

                m_oModel.aoVertices.push_back( CVector( dX, dY, dZ ) );
                m_oIndex.emplace( oKey, nIndex );
                return nIndex;
            }

        private:
            struct SKey
            {
                double  adValue[ 3 ];

                bool operator ==( const SKey &oOther ) const
                {
                    return adValue[ 0 ] == oOther.adValue[ 0 ] && adValue[ 1 ] == oOther.adValue[ 1 ] && adValue[ 2 ] == oOther.adValue[ 2 ];
                }
            };
            struct SHash
            {
                size_t operator ()( const SKey &oKey ) const
                {
                    uint64_t nHash = 1469598103934665603ULL;

                    for ( int nAxis = 0; nAxis < 3; nAxis++ )
                    {
                            // +0.0 and -0.0 are the same place.
                        double   dValue = ( oKey.adValue[ nAxis ] == 0.0 ) ? 0.0 : oKey.adValue[ nAxis ];
                        uint64_t nBits;

                        memcpy( &nBits, &dValue, sizeof( nBits ) );
                        nHash = ( nHash ^ nBits ) * 1099511628211ULL;
                    }
                    return (size_t)nHash;
                }
            };

            ModelFile::SModel                          &m_oModel;
            std::unordered_map<SKey, int, SHash>        m_oIndex;
    };

    BOOL Finite( double dValue )
    {
        return std::isfinite( dValue ) ? TRUE : FALSE;
    }

    BOOL ReadBinaryStl( const std::string &strData, ModelFile::SModel &oModel, std::string &strError )
    {
        const unsigned char *pData = (const unsigned char *)strData.data();
        uint32_t             nCount = Little32( pData + 80 );
        CWelder              oWelder( oModel );
        std::string          strHeader( strData, 0, 80 );
        size_t               nColorAt = strHeader.find( "COLOR=" );
        BOOL                 bMagics = ( nColorAt != std::string::npos && nColorAt + 10 <= 80 );
        CColor               oModelColor( 1.0, 1.0, 1.0 );
        BOOL                 bAnyColor = FALSE;

        if ( bMagics )
        {
            const unsigned char *pColor = pData + nColorAt + 6;

            oModelColor.Set( pColor[ 0 ] / 255.0, pColor[ 1 ] / 255.0, pColor[ 2 ] / 255.0 );
        }
        oModel.anFaces.reserve( (size_t)nCount * 3 );
        oModel.aoFaceColors.resize( nCount );
        oModel.abHasColor.resize( nCount, 0 );
        for ( uint32_t nFace = 0; nFace < nCount; nFace++ )
        {
            const unsigned char *pFace = pData + 84 + ( (size_t)nFace * 50 );

            for ( int nCorner = 0; nCorner < 3; nCorner++ )
            {
                const unsigned char *pCorner = pFace + 12 + ( nCorner * 12 );
                double               dX = LittleFloat( pCorner );
                double               dY = LittleFloat( pCorner + 4 );
                double               dZ = LittleFloat( pCorner + 8 );

                if ( ! Finite( dX ) || ! Finite( dY ) || ! Finite( dZ ) )
                {
                    strError = "triangle " + std::to_string( (unsigned long)nFace + 1 ) + " has a corner that isn't a number";
                    return FALSE;
                }
                oModel.anFaces.push_back( oWelder.Add( dX, dY, dZ ) );
            }

            unsigned int nAttribute = (unsigned int)pFace[ 48 ] | ( (unsigned int)pFace[ 49 ] << 8 );
            unsigned int nLow = nAttribute & 31;
            unsigned int nMiddle = ( nAttribute >> 5 ) & 31;
            unsigned int nHigh = ( nAttribute >> 10 ) & 31;

            if ( bMagics )
            {
                    // Bit 15 clear: the face's own color, red first.
                if ( ( nAttribute & 0x8000 ) == 0 )
                    oModel.aoFaceColors[ nFace ].Set( nLow / 31.0, nMiddle / 31.0, nHigh / 31.0 );
                else
                    oModel.aoFaceColors[ nFace ] = oModelColor;
                oModel.abHasColor[ nFace ] = 1;
                bAnyColor = TRUE;
            }
            else if ( nAttribute & 0x8000 )
            {
                    // VisCAM and SolidView: bit 15 set, blue first.
                oModel.aoFaceColors[ nFace ].Set( nHigh / 31.0, nMiddle / 31.0, nLow / 31.0 );
                oModel.abHasColor[ nFace ] = 1;
                bAnyColor = TRUE;
            }
        }
        if ( ! bAnyColor )
        {
            oModel.aoFaceColors.clear();
            oModel.abHasColor.clear();
        }
        return TRUE;
    }

        // The next word of an ASCII file, or FALSE at its end.
    BOOL NextWord( const std::string &strData, size_t &nAt, std::string &strWord )
    {
        while ( nAt < strData.size() && isspace( (unsigned char)strData[ nAt ] ) )
            nAt++;
        if ( nAt >= strData.size() )
            return FALSE;

        size_t nStart = nAt;

        while ( nAt < strData.size() && ! isspace( (unsigned char)strData[ nAt ] ) )
            nAt++;
        strWord.assign( strData, nStart, nAt - nStart );
        return TRUE;
    }

    BOOL ToNumber( const std::string &strWord, double &dValue )
    {
        char *pEnd = NULL;

        if ( strWord.empty() )
            return FALSE;
        dValue = strtod( strWord.c_str(), &pEnd );
        return ( pEnd && *pEnd == '\0' && Finite( dValue ) ) ? TRUE : FALSE;
    }

    BOOL ReadAsciiStl( const std::string &strData, ModelFile::SModel &oModel, std::string &strError )
    {
        size_t      nAt = 0;
        std::string strWord;
        CWelder     oWelder( oModel );
        int         nCorners = 0;
        long        nFacets = 0;

        while ( NextWord( strData, nAt, strWord ) )
        {
            std::string strLower = Lower( strWord );

            if ( strLower == "facet" )
            {
                if ( nCorners != 0 )
                {
                    strError = "facet " + std::to_string( nFacets ) + " doesn't have three vertices";
                    return FALSE;
                }
                nFacets++;
            }
            else if ( strLower == "vertex" )
            {
                double adValue[ 3 ];

                for ( int nAxis = 0; nAxis < 3; nAxis++ )
                {
                    if ( ! NextWord( strData, nAt, strWord ) || ! ToNumber( strWord, adValue[ nAxis ] ) )
                    {
                        strError = "a vertex in facet " + std::to_string( nFacets ) + " doesn't have three numbers";
                        return FALSE;
                    }
                }
                if ( nCorners == 3 )
                {
                    strError = "facet " + std::to_string( nFacets ) + " has more than three vertices";
                    return FALSE;
                }
                oModel.anFaces.push_back( oWelder.Add( adValue[ 0 ], adValue[ 1 ], adValue[ 2 ] ) );
                nCorners++;
            }
            else if ( strLower == "endfacet" )
            {
                if ( nCorners != 3 )
                {
                    strError = "facet " + std::to_string( nFacets ) + " doesn't have three vertices";
                    return FALSE;
                }
                nCorners = 0;
            }
        }
        if ( nCorners != 0 )
        {
            strError = "the file ends inside facet " + std::to_string( nFacets );
            return FALSE;
        }
        return TRUE;
    }

        // Reads one OBJ index ("3", "-1"), counted from 1 or back from the
        // last of nCount read so far, into a 0-based index.
    BOOL ObjIndex( const std::string &strText, size_t nCount, int &nIndex )
    {
        char *pEnd = NULL;
        long  nValue;

        if ( strText.empty() )
            return FALSE;
        nValue = strtol( strText.c_str(), &pEnd, 10 );
        if ( ! pEnd || *pEnd != '\0' || nValue == 0 )
            return FALSE;
        if ( nValue < 0 )
            nValue += (long)nCount;
        else
            nValue -= 1;
        if ( nValue < 0 || nValue >= (long)nCount )
            return FALSE;
        nIndex = (int)nValue;
        return TRUE;
    }
}

namespace ModelFile
{

BOOL ReadStl( const std::string &strData, SModel &oModel, std::string &strError )
{
    oModel = SModel();
    if ( strData.size() >= 84 )
    {
        uint32_t nCount = Little32( (const unsigned char *)strData.data() + 80 );

        if ( 84 + ( (uint64_t)nCount * 50 ) == (uint64_t)strData.size() )
        {
            if ( nCount == 0 )
            {
                strError = "has no triangles";
                return FALSE;
            }
            return ReadBinaryStl( strData, oModel, strError );
        }
    }

    size_t nStart = 0;

    while ( nStart < strData.size() && isspace( (unsigned char)strData[ nStart ] ) )
        nStart++;
    if ( Lower( strData.substr( nStart, 5 ) ) != "solid" )
    {
        strError = "isn't an STL file (neither the size of a binary one nor starting \"solid\")";
        return FALSE;
    }
        // Text has no zero bytes; a binary file does, in its header or its
        // numbers, so one of the wrong length is reported as that.
    if ( strData.find( '\0' ) != std::string::npos )
    {
        strError = "is a binary STL whose size doesn't match its count of triangles";
        return FALSE;
    }
    if ( ! ReadAsciiStl( strData, oModel, strError ) )
        return FALSE;
    if ( oModel.anFaces.empty() )
    {
        strError = "has no triangles";
        return FALSE;
    }
    return TRUE;
}

BOOL ReadObj( const std::string &strData, SModel &oModel, std::string &strError )
{
    std::vector<CVector>    aoPositions;
    std::vector<FLT>        anTexCoords;        // u, v pairs
    std::vector<CVector>    aoNormals;
        // The corners of every triangle, as (position, coordinates, normal)
        // indices, -1 where a corner has none.
    std::vector< std::tuple<int, int, int> > aoCorners;
    size_t                  nAt = 0;
    long                    nLine = 0;

    oModel = SModel();
    while ( nAt < strData.size() )
    {
        size_t      nEnd = strData.find( '\n', nAt );
        std::string strLine( strData, nAt, ( nEnd == std::string::npos ) ? std::string::npos : nEnd - nAt );

        nAt = ( nEnd == std::string::npos ) ? strData.size() : nEnd + 1;
        nLine++;

        size_t nHash = strLine.find( '#' );

        if ( nHash != std::string::npos )
            strLine.erase( nHash );

        std::vector<std::string>    astrWords;
        size_t                      nWord = 0;
        std::string                 strWord;

        while ( NextWord( strLine, nWord, strWord ) )
            astrWords.push_back( strWord );
        if ( astrWords.empty() )
            continue;

        const std::string &strKey = astrWords[ 0 ];
        std::string        strWhere = "line " + std::to_string( nLine );

        if ( strKey == "v" || strKey == "vn" )
        {
            double adValue[ 3 ];

            if ( astrWords.size() < 4 )
            {
                strError = strWhere + ": \"" + strKey + "\" needs three numbers";
                return FALSE;
            }
            for ( int nAxis = 0; nAxis < 3; nAxis++ )
            {
                if ( ! ToNumber( astrWords[ (size_t)nAxis + 1 ], adValue[ nAxis ] ) )
                {
                    strError = strWhere + ": \"" + astrWords[ (size_t)nAxis + 1 ] + "\" isn't a number";
                    return FALSE;
                }
            }
            ( ( strKey == "v" ) ? aoPositions : aoNormals ).push_back( CVector( adValue[ 0 ], adValue[ 1 ], adValue[ 2 ] ) );
        }
        else if ( strKey == "vt" )
        {
            double dU;
            double dV = 0.0;

            if ( astrWords.size() < 2 || ! ToNumber( astrWords[ 1 ], dU ) ||
                 ( astrWords.size() >= 3 && ! ToNumber( astrWords[ 2 ], dV ) ) )
            {
                strError = strWhere + ": \"vt\" needs one or two numbers";
                return FALSE;
            }
            anTexCoords.push_back( (FLT)dU );
            anTexCoords.push_back( (FLT)dV );
        }
        else if ( strKey == "f" )
        {
            std::vector< std::tuple<int, int, int> > aoFace;

            if ( astrWords.size() < 4 )
            {
                strError = strWhere + ": a face needs at least three corners";
                return FALSE;
            }
            for ( size_t nCorner = 1; nCorner < astrWords.size(); nCorner++ )
            {
                const std::string &strCorner = astrWords[ nCorner ];
                size_t             nSlash1 = strCorner.find( '/' );
                size_t             nSlash2 = ( nSlash1 == std::string::npos ) ? std::string::npos : strCorner.find( '/', nSlash1 + 1 );
                std::string        strV = strCorner.substr( 0, nSlash1 );
                std::string        strT = ( nSlash1 == std::string::npos ) ? "" :
                                          strCorner.substr( nSlash1 + 1, ( nSlash2 == std::string::npos ) ? std::string::npos : nSlash2 - nSlash1 - 1 );
                std::string        strN = ( nSlash2 == std::string::npos ) ? "" : strCorner.substr( nSlash2 + 1 );
                int                nV = -1;
                int                nT = -1;
                int                nN = -1;

                if ( ! ObjIndex( strV, aoPositions.size(), nV ) ||
                     ( ! strT.empty() && ! ObjIndex( strT, anTexCoords.size() / 2, nT ) ) ||
                     ( ! strN.empty() && ! ObjIndex( strN, aoNormals.size(), nN ) ) )
                {
                    strError = strWhere + ": the corner \"" + strCorner + "\" names something not yet given";
                    return FALSE;
                }
                aoFace.push_back( std::make_tuple( nV, nT, nN ) );
            }
                // A fan of triangles from the first corner.
            for ( size_t nCorner = 1; nCorner + 1 < aoFace.size(); nCorner++ )
            {
                aoCorners.push_back( aoFace[ 0 ] );
                aoCorners.push_back( aoFace[ nCorner ] );
                aoCorners.push_back( aoFace[ nCorner + 1 ] );
            }
        }
            // Anything else (o, g, s, usemtl, mtllib, l, p, ...) is ignored.
    }

    if ( aoCorners.empty() )
    {
        strError = "has no faces";
        return FALSE;
    }

    BOOL bAllUVs = TRUE;
    BOOL bAllNormals = TRUE;

    for ( size_t nCorner = 0; nCorner < aoCorners.size(); nCorner++ )
    {
        if ( std::get<1>( aoCorners[ nCorner ] ) < 0 )
            bAllUVs = FALSE;
        if ( std::get<2>( aoCorners[ nCorner ] ) < 0 )
            bAllNormals = FALSE;
    }

        // One vertex for each different corner.
    std::map< std::tuple<int, int, int>, int > oVertexOf;

    oModel.anFaces.reserve( aoCorners.size() );
    for ( size_t nCorner = 0; nCorner < aoCorners.size(); nCorner++ )
    {
        std::tuple<int, int, int> oKey = std::make_tuple( std::get<0>( aoCorners[ nCorner ] ),
                                                          bAllUVs ? std::get<1>( aoCorners[ nCorner ] ) : -1,
                                                          bAllNormals ? std::get<2>( aoCorners[ nCorner ] ) : -1 );
        auto it = oVertexOf.find( oKey );

        if ( it == oVertexOf.end() )
        {
            int nIndex = (int)oModel.aoVertices.size();

            oModel.aoVertices.push_back( aoPositions[ (size_t)std::get<0>( oKey ) ] );
            if ( bAllUVs )
            {
                oModel.anUVs.push_back( anTexCoords[ (size_t)std::get<1>( oKey ) * 2 ] );
                oModel.anUVs.push_back( anTexCoords[ ( (size_t)std::get<1>( oKey ) * 2 ) + 1 ] );
            }
            if ( bAllNormals )
            {
                CVector oNormal( aoNormals[ (size_t)std::get<2>( oKey ) ] );

                if ( oNormal.Magnitude() == 0.0 )
                {
                    strError = "a normal is [0, 0, 0]";
                    return FALSE;
                }
                oNormal.Normalize();
                oModel.aoNormals.push_back( oNormal );
            }
            it = oVertexOf.emplace( oKey, nIndex ).first;
        }
        oModel.anFaces.push_back( it->second );
    }
    return TRUE;
}

BOOL Read( const std::string &strData, const std::string &strName, SModel &oModel, std::string &strError )
{
    std::string strLower = Lower( strName );

    if ( EndsWith( strLower, ".stl" ) )
        return ReadStl( strData, oModel, strError );
    if ( EndsWith( strLower, ".obj" ) )
        return ReadObj( strData, oModel, strError );

        // Otherwise by the contents: a binary STL's size, an ASCII STL's
        // "solid", or else OBJ.
    if ( strData.size() >= 84 &&
         84 + ( (uint64_t)Little32( (const unsigned char *)strData.data() + 80 ) * 50 ) == (uint64_t)strData.size() )
        return ReadStl( strData, oModel, strError );

    size_t nStart = 0;

    while ( nStart < strData.size() && isspace( (unsigned char)strData[ nStart ] ) )
        nStart++;
    if ( Lower( strData.substr( nStart, 5 ) ) == "solid" )
        return ReadStl( strData, oModel, strError );
    return ReadObj( strData, oModel, strError );
}

}
