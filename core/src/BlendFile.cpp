/*************************************************************************
*   BlendFile                                                            *
*                                                                        *
*   Reads the meshes in a Blender .blend file (see ModelFile.h). A       *
*   .blend file is Blender's memory written out in blocks, with a        *
*   catalog of every structure's fields (the "SDNA") at the end, so the  *
*   reader finds each field by its name and works whatever version       *
*   moved it. Meshes are read in either of Blender's layouts:            *
*                                                                        *
*   - up to 3.x: MVert, MPoly and MLoop arrays (the CD_MVERT, CD_MPOLY   *
*     and CD_MLOOP layers), UVs in MLoopUV layers;                       *
*   - 3.5 and later: a "position" float3 attribute, face offsets         *
*     (poly_offset_indices), a ".corner_vert" attribute, UVs as float2   *
*     corner attributes, and "material_index" on faces.                  *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#include "Globals.h"
#include "Decompress.h"
#include "ModelFile.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
        // Blender's CustomData layer types this reads.
    enum { CD_MVERT = 0, CD_PROP_INT32 = 11, CD_MLOOPUV = 16, CD_MPOLY = 25, CD_MLOOP = 26, CD_PROP_FLOAT3 = 48, CD_PROP_FLOAT2 = 49 };
    const int OB_MESH = 1;

    struct SField
    {
        size_t nOffset = 0;
        int    nType = 0;        // index in the type table
        bool   bPointer = false;
        int    nCount = 1;       // array elements
    };

    struct SStruct
    {
        size_t                          nSize = 0;
        std::map<std::string, SField>   oFields;
    };

    struct SBlock
    {
        char        acCode[ 4 ];
        size_t      nLength = 0;
        uint64_t    nOld = 0;
        int         nStruct = 0;
        size_t      nCount = 0;
        size_t      nData = 0;      // where its data starts
    };

    class CBlend
    {
        public:
            CBlend( const std::string &strData ) : m_strData( strData ), m_pData( (const unsigned char *)strData.data() ) {}

            BOOL Parse( std::string &strError );
            BOOL ReadMeshes( const std::string &strObject, ModelFile::SModel &oModel, std::string &strError );

        private:
            const std::string                   &m_strData;
            const unsigned char                 *m_pData;
            size_t                               m_nPointer = 8;
            bool                                 m_bLittle = true;
            int                                  m_nVersion = 0;
            std::vector<SBlock>                  m_aoBlocks;
            std::unordered_map<uint64_t, size_t> m_oByAddress;
            std::vector<std::string>             m_astrTypes;
            std::vector<int>                     m_anTypeSize;
            std::map<std::string, SStruct>       m_oStructs;

            uint32_t U32( size_t nAt ) const
            {
                const unsigned char *p = m_pData + nAt;

                return m_bLittle ? ( (uint32_t)p[ 0 ] | ( (uint32_t)p[ 1 ] << 8 ) | ( (uint32_t)p[ 2 ] << 16 ) | ( (uint32_t)p[ 3 ] << 24 ) )
                                 : ( (uint32_t)p[ 3 ] | ( (uint32_t)p[ 2 ] << 8 ) | ( (uint32_t)p[ 1 ] << 16 ) | ( (uint32_t)p[ 0 ] << 24 ) );
            }
            uint16_t U16( size_t nAt ) const
            {
                const unsigned char *p = m_pData + nAt;

                return m_bLittle ? (uint16_t)( p[ 0 ] | ( p[ 1 ] << 8 ) ) : (uint16_t)( p[ 1 ] | ( p[ 0 ] << 8 ) );
            }
            int32_t I32( size_t nAt ) const { return (int32_t)U32( nAt ); }
            float   F32( size_t nAt ) const { uint32_t n = U32( nAt ); float f; memcpy( &f, &n, 4 ); return f; }
            uint64_t Pointer( size_t nAt ) const
            {
                if ( m_nPointer == 4 )
                    return U32( nAt );

                uint64_t nLow = U32( nAt ), nHigh = U32( nAt + 4 );

                return m_bLittle ? ( nLow | ( nHigh << 32 ) ) : ( nHigh | ( nLow << 32 ) );
            }
            const SStruct *Struct( const char *strName ) const
            {
                auto oFound = m_oStructs.find( strName );

                return ( oFound == m_oStructs.end() ) ? NULL : &oFound->second;
            }
            const SField *Field( const char *strStruct, const char *strField ) const
            {
                const SStruct *pStruct = Struct( strStruct );

                if ( pStruct == NULL )
                    return NULL;

                auto oFound = pStruct->oFields.find( strField );

                return ( oFound == pStruct->oFields.end() ) ? NULL : &oFound->second;
            }
                // A field's value in a structure at nBase; FALSE if the
                // structure has no such field.
            bool    GetInt( size_t nBase, const char *strStruct, const char *strField, int &nValue ) const
            {
                const SField *pField = Field( strStruct, strField );

                if ( pField == NULL || pField->bPointer )
                    return false;

                int nSize = m_anTypeSize[ (size_t)pField->nType ];

                if ( nSize == 4 ) nValue = I32( nBase + pField->nOffset );
                else if ( nSize == 2 ) nValue = (int16_t)U16( nBase + pField->nOffset );
                else if ( nSize == 1 ) nValue = (int8_t)m_pData[ nBase + pField->nOffset ];
                else return false;
                return true;
            }
            bool    GetFloats( size_t nBase, const char *strStruct, const char *strField, float *pfOut, int nCount ) const
            {
                const SField *pField = Field( strStruct, strField );

                if ( pField == NULL || pField->bPointer || pField->nCount < nCount || m_anTypeSize[ (size_t)pField->nType ] != 4 )
                    return false;
                for ( int i = 0; i < nCount; i++ )
                    pfOut[ i ] = F32( nBase + pField->nOffset + 4 * (size_t)i );
                return true;
            }
            uint64_t GetPointer( size_t nBase, const char *strStruct, const char *strField ) const
            {
                const SField *pField = Field( strStruct, strField );

                return ( pField && pField->bPointer ) ? Pointer( nBase + pField->nOffset ) : 0;
            }
            const SBlock *Block( uint64_t nAddress ) const
            {
                if ( nAddress == 0 )
                    return NULL;

                auto oFound = m_oByAddress.find( nAddress );

                return ( oFound == m_oByAddress.end() ) ? NULL : &m_aoBlocks[ oFound->second ];
            }
            std::string Name( size_t nBase ) const
            {
                const SField *pName = Field( "ID", "name" );

                if ( pName == NULL )
                    return "";

                const char *p = (const char *)m_pData + nBase + pName->nOffset;
                size_t      nLength = strnlen( p, (size_t)pName->nCount );

                return ( nLength > 2 ) ? std::string( p + 2, nLength - 2 ) : "";
            }

            struct SLayer { int nType = 0; std::string strName; int nActive = 0; const SBlock *pData = NULL; };
            void Layers( size_t nBase, const char *strField, std::vector<SLayer> &aoLayers ) const;
            bool MeshData( const SBlock &oMesh, std::vector<float> &afPositions, std::vector<int> &anFaceStart,
                           std::vector<int> &anCorners, std::vector<float> &afUVs, std::vector<int> &anMaterial,
                           std::string &strError ) const;
            void Matrix( const SBlock &oObject, double adOut[ 16 ], int nDepth ) const;
            void Colors( const SBlock &oObject, const SBlock &oMesh, std::vector<CColor> &aoColors, std::vector<char> &abHave,
                         std::vector<std::string> &astrNames ) const;
    };

    BOOL CBlend::Parse( std::string &strError )
    {
        size_t nSize = m_strData.size();

        if ( nSize < 12 || memcmp( m_pData, "BLENDER", 7 ) != 0 )
        {
            strError = "isn't a Blender file";
            return FALSE;
        }
        if ( m_pData[ 7 ] >= '0' && m_pData[ 7 ] <= '9' )
        {
            strError = "is a Blender 5 file; Ray reads .blend files from Blender 2.8 to 4.x (save it from Blender 4, or as glTF)";
            return FALSE;
        }
        if ( m_pData[ 7 ] != '_' && m_pData[ 7 ] != '-' )
        {
            strError = "has a damaged Blender header";
            return FALSE;
        }
        m_nPointer = ( m_pData[ 7 ] == '-' ) ? 8 : 4;
        m_bLittle = ( m_pData[ 8 ] == 'v' );
        m_nVersion = atoi( std::string( (const char *)m_pData + 9, 3 ).c_str() );

        size_t nAt = 12;
        size_t nHead = 16 + m_nPointer;
        bool   bEnd = false;

        while ( nAt + 8 <= nSize )
        {
            SBlock oBlock;

            memcpy( oBlock.acCode, m_pData + nAt, 4 );
            if ( memcmp( oBlock.acCode, "ENDB", 4 ) == 0 )
            {
                bEnd = true;
                break;
            }
            if ( nAt + nHead > nSize )
                break;

            int32_t nLength = I32( nAt + 4 );

            oBlock.nOld = Pointer( nAt + 8 );
            oBlock.nStruct = I32( nAt + 8 + m_nPointer );

            int32_t nCount = I32( nAt + 12 + m_nPointer );

            if ( nLength < 0 || nCount < 0 || nAt + nHead + (size_t)nLength > nSize )
            {
                strError = "is a damaged Blender file (a block runs past its end)";
                return FALSE;
            }
            oBlock.nLength = (size_t)nLength;
            oBlock.nCount = (size_t)nCount;
            oBlock.nData = nAt + nHead;
            if ( oBlock.nOld )
                m_oByAddress[ oBlock.nOld ] = m_aoBlocks.size();
            m_aoBlocks.push_back( oBlock );
            nAt += nHead + (size_t)nLength;
        }
        if ( ! bEnd )
        {
            strError = "is a damaged Blender file (it ends before its last block)";
            return FALSE;
        }

            // The structure catalog. Its sections are aligned to four bytes
            // from the block's start.
        const SBlock *pDna = NULL;

        for ( const SBlock &oBlock : m_aoBlocks )
        {
            if ( memcmp( oBlock.acCode, "DNA1", 4 ) == 0 )
                pDna = &oBlock;
        }
        if ( pDna == NULL || pDna->nLength < 16 )
        {
            strError = "is a damaged Blender file (it has no structure catalog)";
            return FALSE;
        }

        size_t nStart = pDna->nData;
        size_t nEnd = pDna->nData + pDna->nLength;
        size_t p = nStart;
        auto   Expect = [ & ]( const char *strTag ) -> bool
        {
            if ( p + 4 > nEnd || memcmp( m_pData + p, strTag, 4 ) != 0 )
                return false;
            p += 4;
            return true;
        };
        auto   Align = [ & ]() { p = nStart + ( ( p - nStart + 3 ) & ~(size_t)3 ); };
        auto   Strings = [ & ]( std::vector<std::string> &astrOut ) -> bool
        {
            if ( p + 4 > nEnd )
                return false;

            int32_t nCount = I32( p );

            p += 4;
            if ( nCount < 0 || nCount > 1000000 )
                return false;
            for ( int32_t i = 0; i < nCount; i++ )
            {
                const char *pString = (const char *)m_pData + p;
                size_t      nLength = strnlen( pString, nEnd - p );

                if ( p + nLength >= nEnd )
                    return false;
                astrOut.push_back( std::string( pString, nLength ) );
                p += nLength + 1;
            }
            Align();
            return true;
        };
        std::vector<std::string> astrNames;

        if ( ! Expect( "SDNA" ) || ! Expect( "NAME" ) || ! Strings( astrNames ) || ! Expect( "TYPE" ) || ! Strings( m_astrTypes ) ||
             ! Expect( "TLEN" ) || p + 2 * m_astrTypes.size() > nEnd )
        {
            strError = "is a damaged Blender file (its structure catalog)";
            return FALSE;
        }
        for ( size_t i = 0; i < m_astrTypes.size(); i++ )
            m_anTypeSize.push_back( U16( p + 2 * i ) );
        p += 2 * m_astrTypes.size();
        Align();
        if ( ! Expect( "STRC" ) || p + 4 > nEnd )
        {
            strError = "is a damaged Blender file (its structure catalog)";
            return FALSE;
        }

        int32_t nStructs = I32( p );

        p += 4;
        for ( int32_t s = 0; s < nStructs; s++ )
        {
            if ( p + 4 > nEnd )
                break;

            int nType = U16( p );
            int nFields = U16( p + 2 );

            p += 4;
            if ( (size_t)nType >= m_astrTypes.size() || p + 4 * (size_t)nFields > nEnd )
                break;

            SStruct oStruct;
            size_t  nOffset = 0;

            oStruct.nSize = (size_t)m_anTypeSize[ (size_t)nType ];
            for ( int f = 0; f < nFields; f++ )
            {
                int nFieldType = U16( p );
                int nName = U16( p + 2 );

                p += 4;
                if ( (size_t)nFieldType >= m_astrTypes.size() || (size_t)nName >= astrNames.size() )
                    continue;

                const std::string &strName = astrNames[ (size_t)nName ];
                SField             oField;
                std::string        strBase;
                bool               bFunction = strName.compare( 0, 2, "(*" ) == 0;

                oField.nOffset = nOffset;
                oField.nType = nFieldType;
                oField.bPointer = bFunction || ( ! strName.empty() && strName[ 0 ] == '*' );
                for ( size_t c = 0; c < strName.size(); c++ )
                {
                    if ( strName[ c ] == '[' )
                    {
                        oField.nCount *= atoi( strName.c_str() + c + 1 );
                        while ( c < strName.size() && strName[ c ] != ']' )
                            c++;
                    }
                    else if ( strName[ c ] != '*' && strName[ c ] != '(' && strName[ c ] != ')' )
                        strBase += strName[ c ];
                }

                size_t nElement = oField.bPointer ? m_nPointer : (size_t)m_anTypeSize[ (size_t)nFieldType ];

                nOffset += nElement * (size_t)oField.nCount;
                oStruct.oFields[ strBase ] = oField;
            }
            m_oStructs[ m_astrTypes[ (size_t)nType ] ] = oStruct;
        }
        if ( ! Struct( "Mesh" ) || ! Struct( "Object" ) || ! Struct( "ID" ) || ! Struct( "CustomData" ) || ! Struct( "CustomDataLayer" ) )
        {
            strError = "is a Blender file without the structures Ray reads";
            return FALSE;
        }
        return TRUE;
    }

    void CBlend::Layers( size_t nBase, const char *strField, std::vector<SLayer> &aoLayers ) const
    {
        const SField *pData = Field( "Mesh", strField );

        aoLayers.clear();
        if ( pData == NULL )
            return;

        size_t        nCustom = nBase + pData->nOffset;
        int           nCount = 0;
        const SBlock *pLayers = Block( GetPointer( nCustom, "CustomData", "layers" ) );

        if ( pLayers == NULL || ! GetInt( nCustom, "CustomData", "totlayer", nCount ) || nCount <= 0 )
            return;

        const SStruct *pLayer = Struct( "CustomDataLayer" );
        const SField  *pName = Field( "CustomDataLayer", "name" );

        for ( int i = 0; i < nCount; i++ )
        {
            size_t nAt = pLayers->nData + (size_t)i * pLayer->nSize;

            if ( (size_t)( i + 1 ) * pLayer->nSize > pLayers->nLength )
                break;

            SLayer oLayer;

            GetInt( nAt, "CustomDataLayer", "type", oLayer.nType );
            GetInt( nAt, "CustomDataLayer", "active", oLayer.nActive );
            if ( pName )
            {
                const char *p = (const char *)m_pData + nAt + pName->nOffset;

                oLayer.strName.assign( p, strnlen( p, (size_t)pName->nCount ) );
            }
            oLayer.pData = Block( GetPointer( nAt, "CustomDataLayer", "data" ) );
            aoLayers.push_back( oLayer );
        }
    }

    bool CBlend::MeshData( const SBlock &oMesh, std::vector<float> &afPositions, std::vector<int> &anFaceStart,
                           std::vector<int> &anCorners, std::vector<float> &afUVs, std::vector<int> &anMaterial,
                           std::string &strError ) const
    {
        size_t nBase = oMesh.nData;
        int    nVertices = 0, nFaces = 0, nLoops = 0;

        if ( ! GetInt( nBase, "Mesh", "totvert", nVertices ) )
            GetInt( nBase, "Mesh", "verts_num", nVertices );
        if ( ! GetInt( nBase, "Mesh", "totpoly", nFaces ) )
            GetInt( nBase, "Mesh", "faces_num", nFaces );
        if ( ! GetInt( nBase, "Mesh", "totloop", nLoops ) )
            GetInt( nBase, "Mesh", "corners_num", nLoops );
        if ( nVertices < 0 || nFaces < 0 || nLoops < 0 )
        {
            strError = "has a damaged mesh";
            return false;
        }

        std::vector<SLayer> aoVertex, aoFace, aoCorner;

        Layers( nBase, "vdata", aoVertex );
        Layers( nBase, "pdata", aoFace );
        Layers( nBase, "ldata", aoCorner );

            // Positions.
        afPositions.assign( (size_t)nVertices * 3, 0.0f );

        bool bPositions = false;

        for ( const SLayer &oLayer : aoVertex )
        {
            if ( oLayer.pData == NULL )
                continue;
            if ( oLayer.nType == CD_MVERT )
            {
                const SStruct *pVertex = Struct( "MVert" );

                if ( pVertex == NULL || oLayer.pData->nLength < pVertex->nSize * (size_t)nVertices )
                    continue;
                for ( int v = 0; v < nVertices; v++ )
                    GetFloats( oLayer.pData->nData + pVertex->nSize * (size_t)v, "MVert", "co", &afPositions[ (size_t)v * 3 ], 3 );
                bPositions = true;
                break;
            }
            if ( oLayer.nType == CD_PROP_FLOAT3 && oLayer.strName == "position" )
            {
                if ( oLayer.pData->nLength < 12 * (size_t)nVertices )
                    continue;
                for ( size_t i = 0; i < (size_t)nVertices * 3; i++ )
                    afPositions[ i ] = F32( oLayer.pData->nData + 4 * i );
                bPositions = true;
                break;
            }
        }
        if ( ! bPositions && nVertices > 0 )
        {
            strError = "has a mesh whose vertex positions Ray can't find";
            return false;
        }

            // Faces: (first corner, corner count) pairs, and each's material.
        anFaceStart.assign( (size_t)nFaces * 2, 0 );
        anMaterial.assign( (size_t)nFaces, 0 );

        bool bFaces = false;

        for ( const SLayer &oLayer : aoFace )
        {
            if ( oLayer.nType != CD_MPOLY || oLayer.pData == NULL )
                continue;

            const SStruct *pPoly = Struct( "MPoly" );

            if ( pPoly == NULL || oLayer.pData->nLength < pPoly->nSize * (size_t)nFaces )
                continue;

            anFaceStart.assign( (size_t)nFaces * 2, 0 );
            for ( int f = 0; f < nFaces; f++ )
            {
                size_t nAt = oLayer.pData->nData + pPoly->nSize * (size_t)f;
                int    nStart = 0, nCount = 0, nMaterial = 0;

                GetInt( nAt, "MPoly", "loopstart", nStart );
                GetInt( nAt, "MPoly", "totloop", nCount );
                GetInt( nAt, "MPoly", "mat_nr", nMaterial );
                if ( nStart < 0 || nCount < 0 || nStart + nCount > nLoops )
                {
                    strError = "has a damaged mesh (a face's corners)";
                    return false;
                }
                anFaceStart[ (size_t)f * 2 ] = nStart;
                anFaceStart[ (size_t)f * 2 + 1 ] = nCount;
                anMaterial[ (size_t)f ] = nMaterial;
            }
            bFaces = true;
            break;
        }
        if ( ! bFaces )
        {
            uint64_t      nOffsets = GetPointer( nBase, "Mesh", "poly_offset_indices" );

            if ( nOffsets == 0 )
                nOffsets = GetPointer( nBase, "Mesh", "face_offset_indices" );

            const SBlock *pOffsets = Block( nOffsets );

            if ( pOffsets && pOffsets->nLength >= 4 * ( (size_t)nFaces + 1 ) )
            {
                anFaceStart.assign( (size_t)nFaces * 2, 0 );
                for ( int f = 0; f < nFaces; f++ )
                {
                    int nStart = I32( pOffsets->nData + 4 * (size_t)f );
                    int nNext = I32( pOffsets->nData + 4 * ( (size_t)f + 1 ) );

                    if ( nStart < 0 || nNext < nStart || nNext > nLoops )
                    {
                        strError = "has a damaged mesh (its face offsets)";
                        return false;
                    }
                    anFaceStart[ (size_t)f * 2 ] = nStart;
                    anFaceStart[ (size_t)f * 2 + 1 ] = nNext - nStart;
                }
                bFaces = true;
            }
            for ( const SLayer &oLayer : aoFace )
            {
                if ( oLayer.nType == CD_PROP_INT32 && oLayer.strName == "material_index" && oLayer.pData &&
                     oLayer.pData->nLength >= 4 * (size_t)nFaces )
                {
                    for ( int f = 0; f < nFaces; f++ )
                        anMaterial[ (size_t)f ] = I32( oLayer.pData->nData + 4 * (size_t)f );
                }
            }
        }
        if ( ! bFaces && nFaces > 0 )
        {
            strError = "has a mesh whose faces Ray can't find";
            return false;
        }

            // Each corner's vertex.
        anCorners.assign( (size_t)nLoops, 0 );

        bool bCorners = false;

        for ( const SLayer &oLayer : aoCorner )
        {
            if ( oLayer.pData == NULL )
                continue;
            if ( oLayer.nType == CD_MLOOP )
            {
                const SStruct *pLoop = Struct( "MLoop" );

                if ( pLoop == NULL || oLayer.pData->nLength < pLoop->nSize * (size_t)nLoops )
                    continue;
                for ( int c = 0; c < nLoops; c++ )
                    GetInt( oLayer.pData->nData + pLoop->nSize * (size_t)c, "MLoop", "v", anCorners[ (size_t)c ] );
                bCorners = true;
                break;
            }
            if ( oLayer.nType == CD_PROP_INT32 && oLayer.strName == ".corner_vert" )
            {
                if ( oLayer.pData->nLength < 4 * (size_t)nLoops )
                    continue;
                for ( int c = 0; c < nLoops; c++ )
                    anCorners[ (size_t)c ] = I32( oLayer.pData->nData + 4 * (size_t)c );
                bCorners = true;
                break;
            }
        }
        if ( ! bCorners && nLoops > 0 )
        {
            strError = "has a mesh whose corners Ray can't find";
            return false;
        }
        for ( int nVertex : anCorners )
        {
            if ( nVertex < 0 || nVertex >= nVertices )
            {
                strError = "has a damaged mesh (a corner's vertex)";
                return false;
            }
        }

            // The active UV map: an MLoopUV layer, or a float2 corner
            // attribute (Blender's own hidden ones start with a dot).
        afUVs.clear();
        for ( int nPass = 0; nPass < 2 && afUVs.empty(); nPass++ )
        {
            int nType = ( nPass == 0 ) ? CD_MLOOPUV : CD_PROP_FLOAT2;
            int nFirst = -1;

            for ( size_t i = 0; i < aoCorner.size(); i++ )
            {
                if ( aoCorner[ i ].nType == nType )
                {
                    nFirst = (int)i;
                    break;
                }
            }
            if ( nFirst < 0 )
                continue;

            size_t nPick = (size_t)nFirst + (size_t)( ( aoCorner[ (size_t)nFirst ].nActive >= 0 ) ? aoCorner[ (size_t)nFirst ].nActive : 0 );

            if ( nPick >= aoCorner.size() || aoCorner[ nPick ].nType != nType )
                nPick = (size_t)nFirst;

            const SLayer &oLayer = aoCorner[ nPick ];

            if ( oLayer.pData == NULL || ( nType == CD_PROP_FLOAT2 && ! oLayer.strName.empty() && oLayer.strName[ 0 ] == '.' ) )
                continue;

            size_t nStride = 8;

            if ( nType == CD_MLOOPUV )
            {
                const SStruct *pUV = Struct( "MLoopUV" );

                if ( pUV == NULL )
                    continue;
                nStride = pUV->nSize;
            }
            if ( oLayer.pData->nLength < nStride * (size_t)nLoops )
                continue;
            afUVs.assign( (size_t)nLoops * 2, 0.0f );
            for ( int c = 0; c < nLoops; c++ )
            {
                afUVs[ (size_t)c * 2 ] = F32( oLayer.pData->nData + nStride * (size_t)c );
                afUVs[ (size_t)c * 2 + 1 ] = F32( oLayer.pData->nData + nStride * (size_t)c + 4 );
            }
        }
        return true;
    }

        // An object's world matrix (column-major, as Blender keeps them).
    void CBlend::Matrix( const SBlock &oObject, double adOut[ 16 ], int nDepth ) const
    {
        size_t nBase = oObject.nData;
        float  afObmat[ 16 ];

        if ( GetFloats( nBase, "Object", "obmat", afObmat, 16 ) )
        {
            for ( int i = 0; i < 16; i++ )
                adOut[ i ] = afObmat[ i ];
            return;
        }

            // Blender 4.2 and later keep no world matrix in the file: build
            // it from the location, rotation and scale, then the parent.
        float afLoc[ 3 ] = { 0, 0, 0 }, afDeltaLoc[ 3 ] = { 0, 0, 0 };
        float afRot[ 3 ] = { 0, 0, 0 }, afSize[ 3 ] = { 1, 1, 1 }, afDeltaSize[ 3 ] = { 1, 1, 1 };
        float afQuat[ 4 ] = { 1, 0, 0, 0 }, afAxis[ 3 ] = { 0, 1, 0 };
        float fAngle = 0.0f;
        int   nMode = 1;

        GetFloats( nBase, "Object", "loc", afLoc, 3 );
        GetFloats( nBase, "Object", "dloc", afDeltaLoc, 3 );
        GetFloats( nBase, "Object", "rot", afRot, 3 );
        GetFloats( nBase, "Object", "size", afSize, 3 );
        GetFloats( nBase, "Object", "dscale", afDeltaSize, 3 );
        GetFloats( nBase, "Object", "quat", afQuat, 4 );
        GetFloats( nBase, "Object", "rotAxis", afAxis, 3 );
        GetFloats( nBase, "Object", "rotAngle", &fAngle, 1 );
        GetInt( nBase, "Object", "rotmode", nMode );

        double ad3[ 9 ];      // rotation, rows

        auto Rx = []( double a, double m[ 9 ] ) { double c = cos( a ), s = sin( a ); double r[ 9 ] = { 1, 0, 0, 0, c, -s, 0, s, c }; memcpy( m, r, sizeof r ); };
        auto Ry = []( double a, double m[ 9 ] ) { double c = cos( a ), s = sin( a ); double r[ 9 ] = { c, 0, s, 0, 1, 0, -s, 0, c }; memcpy( m, r, sizeof r ); };
        auto Rz = []( double a, double m[ 9 ] ) { double c = cos( a ), s = sin( a ); double r[ 9 ] = { c, -s, 0, s, c, 0, 0, 0, 1 }; memcpy( m, r, sizeof r ); };
        auto Mul = []( const double a[ 9 ], const double b[ 9 ], double o[ 9 ] )
        {
            double r[ 9 ];

            for ( int i = 0; i < 3; i++ )
                for ( int j = 0; j < 3; j++ )
                    r[ i * 3 + j ] = a[ i * 3 ] * b[ j ] + a[ i * 3 + 1 ] * b[ 3 + j ] + a[ i * 3 + 2 ] * b[ 6 + j ];
            memcpy( o, r, sizeof r );
        };

        if ( nMode == 0 || nMode == -1 )
        {
            double w, x, y, z;

            if ( nMode == 0 )
            {
                w = afQuat[ 0 ]; x = afQuat[ 1 ]; y = afQuat[ 2 ]; z = afQuat[ 3 ];
            }
            else
            {
                double dLength = sqrt( (double)afAxis[ 0 ] * afAxis[ 0 ] + (double)afAxis[ 1 ] * afAxis[ 1 ] + (double)afAxis[ 2 ] * afAxis[ 2 ] );

                if ( dLength == 0.0 ) dLength = 1.0;
                w = cos( fAngle / 2.0 );
                x = sin( fAngle / 2.0 ) * afAxis[ 0 ] / dLength;
                y = sin( fAngle / 2.0 ) * afAxis[ 1 ] / dLength;
                z = sin( fAngle / 2.0 ) * afAxis[ 2 ] / dLength;
            }

            double n = sqrt( w * w + x * x + y * y + z * z );

            if ( n == 0.0 ) n = 1.0;
            w /= n; x /= n; y /= n; z /= n;

            double r[ 9 ] = { 1 - 2 * ( y * y + z * z ), 2 * ( x * y - w * z ), 2 * ( x * z + w * y ),
                              2 * ( x * y + w * z ), 1 - 2 * ( x * x + z * z ), 2 * ( y * z - w * x ),
                              2 * ( x * z - w * y ), 2 * ( y * z + w * x ), 1 - 2 * ( x * x + y * y ) };

            memcpy( ad3, r, sizeof r );
        }
        else
        {
                // Euler angles: the order is the order they're applied in
                // (XYZ: X first, so R = Rz Ry Rx).
            double mx[ 9 ], my[ 9 ], mz[ 9 ];
            const double *apOrder[ 3 ];

            Rx( afRot[ 0 ], mx );
            Ry( afRot[ 1 ], my );
            Rz( afRot[ 2 ], mz );
            switch ( nMode )
            {
                case 2:  apOrder[ 0 ] = mx; apOrder[ 1 ] = mz; apOrder[ 2 ] = my; break;    // XZY
                case 3:  apOrder[ 0 ] = my; apOrder[ 1 ] = mx; apOrder[ 2 ] = mz; break;    // YXZ
                case 4:  apOrder[ 0 ] = my; apOrder[ 1 ] = mz; apOrder[ 2 ] = mx; break;    // YZX
                case 5:  apOrder[ 0 ] = mz; apOrder[ 1 ] = mx; apOrder[ 2 ] = my; break;    // ZXY
                case 6:  apOrder[ 0 ] = mz; apOrder[ 1 ] = my; apOrder[ 2 ] = mx; break;    // ZYX
                default: apOrder[ 0 ] = mx; apOrder[ 1 ] = my; apOrder[ 2 ] = mz; break;    // XYZ
            }

            double t[ 9 ];

            Mul( apOrder[ 1 ], apOrder[ 0 ], t );
            Mul( apOrder[ 2 ], t, ad3 );
        }

        double adLocal[ 16 ];

        for ( int c = 0; c < 3; c++ )
        {
            double dScale = (double)afSize[ c ] * afDeltaSize[ c ];

            for ( int r = 0; r < 3; r++ )
                adLocal[ c * 4 + r ] = ad3[ r * 3 + c ] * dScale;
            adLocal[ c * 4 + 3 ] = 0.0;
        }
        adLocal[ 12 ] = (double)afLoc[ 0 ] + afDeltaLoc[ 0 ];
        adLocal[ 13 ] = (double)afLoc[ 1 ] + afDeltaLoc[ 1 ];
        adLocal[ 14 ] = (double)afLoc[ 2 ] + afDeltaLoc[ 2 ];
        adLocal[ 15 ] = 1.0;

        auto Mul4 = []( const double a[ 16 ], const double b[ 16 ], double o[ 16 ] )
        {
            double r[ 16 ];

            for ( int c = 0; c < 4; c++ )
                for ( int rr = 0; rr < 4; rr++ )
                    r[ c * 4 + rr ] = a[ rr ] * b[ c * 4 ] + a[ 4 + rr ] * b[ c * 4 + 1 ] + a[ 8 + rr ] * b[ c * 4 + 2 ] + a[ 12 + rr ] * b[ c * 4 + 3 ];
            memcpy( o, r, sizeof r );
        };

        const SBlock *pParent = Block( GetPointer( nBase, "Object", "parent" ) );

        if ( pParent && nDepth < 64 )
        {
            double adParent[ 16 ], adInverse[ 16 ], adTemp[ 16 ];
            float  afInverse[ 16 ];

            Matrix( *pParent, adParent, nDepth + 1 );
            if ( GetFloats( nBase, "Object", "parentinv", afInverse, 16 ) )
            {
                for ( int i = 0; i < 16; i++ )
                    adInverse[ i ] = afInverse[ i ];
            }
            else
            {
                for ( int i = 0; i < 16; i++ )
                    adInverse[ i ] = ( i % 5 == 0 ) ? 1.0 : 0.0;
            }
            Mul4( adParent, adInverse, adTemp );
            Mul4( adTemp, adLocal, adOut );
        }
        else
            memcpy( adOut, adLocal, sizeof adLocal );
    }

        // The viewport color of each material slot: the object's own
        // material where its slot says so, else the mesh's.
    void CBlend::Colors( const SBlock &oObject, const SBlock &oMesh, std::vector<CColor> &aoColors, std::vector<char> &abHave,
                         std::vector<std::string> &astrNames ) const
    {
        int nMeshCount = 0, nObjectCount = 0;

        GetInt( oMesh.nData, "Mesh", "totcol", nMeshCount );
        GetInt( oObject.nData, "Object", "totcol", nObjectCount );

        int           nCount = ( nMeshCount > nObjectCount ) ? nMeshCount : nObjectCount;
        const SBlock *pMeshList = Block( GetPointer( oMesh.nData, "Mesh", "mat" ) );
        const SBlock *pObjectList = Block( GetPointer( oObject.nData, "Object", "mat" ) );
        const SBlock *pBits = Block( GetPointer( oObject.nData, "Object", "matbits" ) );

        aoColors.assign( (size_t)( nCount > 0 ? nCount : 0 ), CColor( 1.0, 1.0, 1.0 ) );
        abHave.assign( aoColors.size(), 0 );
        astrNames.assign( aoColors.size(), std::string() );
        for ( int i = 0; i < nCount && i < 1024; i++ )
        {
            bool          bObject = pBits && (size_t)i < pBits->nLength && m_pData[ pBits->nData + (size_t)i ] != 0;
            const SBlock *pList = bObject ? pObjectList : pMeshList;

            if ( pList == NULL || (size_t)( i + 1 ) * m_nPointer > pList->nLength )
                continue;

            const SBlock *pMaterial = Block( Pointer( pList->nData + (size_t)i * m_nPointer ) );
            float         af[ 3 ];

            if ( pMaterial )
                astrNames[ (size_t)i ] = Name( pMaterial->nData );

            if ( pMaterial && GetFloats( pMaterial->nData, "Material", "r", af, 1 ) && GetFloats( pMaterial->nData, "Material", "g", af + 1, 1 ) &&
                 GetFloats( pMaterial->nData, "Material", "b", af + 2, 1 ) )
            {
                aoColors[ (size_t)i ] = CColor( af[ 0 ], af[ 1 ], af[ 2 ] );
                abHave[ (size_t)i ] = 1;
            }
        }
    }

    BOOL CBlend::ReadMeshes( const std::string &strObject, ModelFile::SModel &oModel, std::string &strError )
    {
        bool bAny = false;
        bool bFound = strObject.empty();
        bool bAllUVs = true;
        bool bAnyColor = false;
        std::vector<FLT> anUVs;

        for ( const SBlock &oObject : m_aoBlocks )
        {
            if ( memcmp( oObject.acCode, "OB\0\0", 4 ) != 0 )
                continue;

            int nType = 0;

            if ( ! GetInt( oObject.nData, "Object", "type", nType ) || nType != OB_MESH )
                continue;
            if ( ! strObject.empty() )
            {
                if ( Name( oObject.nData ) != strObject )
                    continue;
                bFound = true;
            }

            const SBlock *pMesh = Block( GetPointer( oObject.nData, "Object", "data" ) );

            if ( pMesh == NULL || memcmp( pMesh->acCode, "ME\0\0", 4 ) != 0 )
                continue;

            std::vector<float> afPositions, afUVs;
            std::vector<int>   anFaces, anCorners, anMaterial;

            if ( ! MeshData( *pMesh, afPositions, anFaces, anCorners, afUVs, anMaterial, strError ) )
            {
                strError = "\"" + Name( oObject.nData ) + "\" " + strError;
                return FALSE;
            }

            double adMatrix[ 16 ];

            Matrix( oObject, adMatrix, 0 );

            std::vector<CColor> aoColors;
            std::vector<char>   abHave;
            std::vector<std::string> astrNames;

            Colors( oObject, *pMesh, aoColors, abHave, astrNames );

                // Each different (vertex, UV) pair becomes a vertex.
            std::map<std::pair<int, std::pair<uint32_t, uint32_t> >, int> oJoined;
            size_t nFaces = anFaces.size() / 2;

            if ( afUVs.empty() )
                bAllUVs = false;
            for ( size_t f = 0; f < nFaces; f++ )
            {
                int nStart = anFaces[ f * 2 ];
                int nCount = anFaces[ f * 2 + 1 ];

                if ( nCount < 3 )
                    continue;

                std::vector<int> anFaceVertices;

                for ( int k = 0; k < nCount; k++ )
                {
                    int      c = nStart + k;
                    int      v = anCorners[ (size_t)c ];
                    uint32_t nU = 0, nV = 0;

                    if ( ! afUVs.empty() )
                    {
                        memcpy( &nU, &afUVs[ (size_t)c * 2 ], 4 );
                        memcpy( &nV, &afUVs[ (size_t)c * 2 + 1 ], 4 );
                    }

                    auto oKey = std::make_pair( v, std::make_pair( nU, nV ) );
                    auto oFound = oJoined.find( oKey );

                    if ( oFound != oJoined.end() )
                    {
                        anFaceVertices.push_back( oFound->second );
                        continue;
                    }

                    double x = afPositions[ (size_t)v * 3 ], y = afPositions[ (size_t)v * 3 + 1 ], z = afPositions[ (size_t)v * 3 + 2 ];
                    double wx = adMatrix[ 0 ] * x + adMatrix[ 4 ] * y + adMatrix[ 8 ] * z + adMatrix[ 12 ];
                    double wy = adMatrix[ 1 ] * x + adMatrix[ 5 ] * y + adMatrix[ 9 ] * z + adMatrix[ 13 ];
                    double wz = adMatrix[ 2 ] * x + adMatrix[ 6 ] * y + adMatrix[ 10 ] * z + adMatrix[ 14 ];
                    int    nNew = (int)oModel.aoVertices.size();

                        // Blender is z up and right-handed; Ray's scenes are
                        // y up and left-handed: (x, y, z) becomes (x, z, y),
                        // and each face is turned back round.
                    oModel.aoVertices.push_back( CVector( wx, wz, wy ) );
                    anUVs.push_back( afUVs.empty() ? 0.0 : afUVs[ (size_t)c * 2 ] );
                    anUVs.push_back( afUVs.empty() ? 0.0 : afUVs[ (size_t)c * 2 + 1 ] );
                    oJoined[ oKey ] = nNew;
                    anFaceVertices.push_back( nNew );
                }

                int     nMaterial = anMaterial[ f ];
                bool    bHave = nMaterial >= 0 && (size_t)nMaterial < abHave.size() && abHave[ (size_t)nMaterial ];
                CColor  oColor = bHave ? aoColors[ (size_t)nMaterial ] : CColor( 1.0, 1.0, 1.0 );

                if ( bHave )
                    bAnyColor = true;

                    // The face's part: its material's name.
                int nPart = -1;

                if ( nMaterial >= 0 && (size_t)nMaterial < astrNames.size() && ! astrNames[ (size_t)nMaterial ].empty() )
                {
                    for ( size_t nAt = 0; nAt < oModel.astrParts.size() && nPart < 0; nAt++ )
                    {
                        if ( oModel.astrParts[ nAt ] == astrNames[ (size_t)nMaterial ] )
                            nPart = (int)nAt;
                    }
                    if ( nPart < 0 )
                    {
                        nPart = (int)oModel.astrParts.size();
                        oModel.astrParts.push_back( astrNames[ (size_t)nMaterial ] );
                    }
                }
                for ( size_t k = 1; k + 1 < anFaceVertices.size(); k++ )
                {
                    int a = anFaceVertices[ 0 ], b = anFaceVertices[ k ], c = anFaceVertices[ k + 1 ];

                    if ( a == b || b == c || a == c )
                        continue;
                    oModel.anFaces.push_back( a );
                    oModel.anFaces.push_back( c );
                    oModel.anFaces.push_back( b );
                    oModel.aoFaceColors.push_back( oColor );
                    oModel.abHasColor.push_back( bHave ? 1 : 0 );
                    oModel.anFacePart.push_back( nPart );
                }
            }
            bAny = true;
        }
        if ( ! bFound )
        {
            strError = "has no mesh object named \"" + strObject + "\"";
            return FALSE;
        }
        if ( ! bAny || oModel.anFaces.empty() )
        {
            bool bLinked = false;

            for ( const SBlock &oBlock : m_aoBlocks )
            {
                if ( memcmp( oBlock.acCode, "LI\0\0", 4 ) == 0 )
                    bLinked = true;
            }
            strError = bLinked ? "has no mesh with faces of its own (it links its meshes from another .blend file, which Ray doesn't follow)"
                               : "has no mesh with faces";
            return FALSE;
        }
        if ( bAllUVs )
            oModel.anUVs = anUVs;
        if ( ! bAnyColor )
        {
            oModel.aoFaceColors.clear();
            oModel.abHasColor.clear();
        }
        if ( oModel.astrParts.empty() )
            oModel.anFacePart.clear();
        return TRUE;
    }
}

namespace ModelFile
{

BOOL IsBlend( const std::string &strData, const std::string &strName )
{
    if ( strData.size() >= 7 && memcmp( strData.data(), "BLENDER", 7 ) == 0 )
        return TRUE;

    std::string strLower;

    for ( char chNext : strName )
        strLower += (char)tolower( (unsigned char)chNext );
    return ( strLower.size() >= 6 && strLower.compare( strLower.size() - 6, 6, ".blend" ) == 0 ) ? TRUE : FALSE;
}

BOOL ReadBlend( const std::string &strData, const std::string &strObject, SModel &oModel, std::string &strError )
{
    std::string        strUnpacked;
    const std::string *pData = &strData;
    const size_t       nLimit = (size_t)1 << 31;

    if ( Decompress::IsGzip( strData ) )
    {
        if ( ! Decompress::Gunzip( strData, strUnpacked, strError, nLimit ) )
            return FALSE;
        pData = &strUnpacked;
    }
    else if ( Decompress::IsZstd( strData ) )
    {
        if ( ! Decompress::Unzstd( strData, strUnpacked, strError, nLimit ) )
            return FALSE;
        pData = &strUnpacked;
    }

    CBlend oBlend( *pData );

    oModel = SModel();
    if ( ! oBlend.Parse( strError ) )
        return FALSE;
    return oBlend.ReadMeshes( strObject, oModel, strError );
}

}
