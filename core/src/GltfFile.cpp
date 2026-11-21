/*************************************************************************
*   GltfFile                                                             *
*                                                                        *
*   Reads glTF 2.0 models, .gltf (JSON, with its buffers in other files  *
*   or in data: URIs) and .glb (the binary container), into the same     *
*   triangle model as STL and OBJ. See ModelFile.h for what is read.     *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "ModelFile.h"
#include "PngImage.h"
#include "SceneJson.h"
#include "JSON/JsonNode.h"

#include <math.h>
#include <string.h>
#include <memory>

namespace
{
    uint32_t Get32( const unsigned char *pData )
    {
        return (uint32_t)pData[ 0 ] | ( (uint32_t)pData[ 1 ] << 8 ) | ( (uint32_t)pData[ 2 ] << 16 ) | ( (uint32_t)pData[ 3 ] << 24 );
    }

    BOOL FromBase64( const std::string &strText, std::string &strOut )
    {
        static const char *strDigits = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        uint32_t nBits = 0;
        int      nCount = 0;

        strOut.clear();
        for ( char chNext : strText )
        {
            if ( chNext == '=' )
                break;
            if ( chNext == '\r' || chNext == '\n' || chNext == ' ' )
                continue;

            const char *pFound = strchr( strDigits, chNext );

            if ( pFound == NULL || chNext == '\0' )
                return FALSE;
            nBits = ( nBits << 6 ) | (uint32_t)( pFound - strDigits );
            nCount += 6;
            if ( nCount >= 8 )
            {
                nCount -= 8;
                strOut += (char)( ( nBits >> nCount ) & 0xFF );
            }
        }
        return TRUE;
    }

        // A column-major 4x4 matrix, as glTF writes them.
    struct SMatrix
    {
        double ad[ 16 ];

        static SMatrix Identity( void )
        {
            SMatrix oMatrix;

            for ( int nIndex = 0; nIndex < 16; nIndex++ )
                oMatrix.ad[ nIndex ] = ( nIndex % 5 == 0 ) ? 1.0 : 0.0;
            return oMatrix;
        }

        SMatrix operator *( const SMatrix &oOther ) const
        {
            SMatrix oOut;

            for ( int nColumn = 0; nColumn < 4; nColumn++ )
            {
                for ( int nRow = 0; nRow < 4; nRow++ )
                {
                    double dSum = 0.0;

                    for ( int nK = 0; nK < 4; nK++ )
                        dSum += ad[ nRow + 4 * nK ] * oOther.ad[ nK + 4 * nColumn ];
                    oOut.ad[ nRow + 4 * nColumn ] = dSum;
                }
            }
            return oOut;
        }
    };

    class CGltfReader
    {
        public:
            CGltfReader( const ModelFile::SGltfFiles *pFiles, ModelFile::SModel &oModel, std::string &strError ) :
                m_pFiles( pFiles ), m_oModel( oModel ), m_strError( strError ), m_pRoot( NULL ),
                m_bNormals( TRUE ), m_bCoords( TRUE ), m_bAnyColor( FALSE ) {}
            ~CGltfReader( void ) { delete m_pRoot; }

            BOOL Read( const std::string &strData, const std::string &strNode );

        private:
            const ModelFile::SGltfFiles    *m_pFiles;
            ModelFile::SModel              &m_oModel;
            std::string                    &m_strError;
            IASLib::CJsonNode              *m_pRoot;
            std::vector<std::string>        m_astrBuffers;
            std::vector<CVector>            m_aoNormals;     // all the vertices' normals so far
            std::vector<FLT>                m_anCoords;      // and coordinates
            BOOL                            m_bNormals;      // every primitive so far had normals
            BOOL                            m_bCoords;       // and coordinates
            BOOL                            m_bAnyColor;
            std::vector<CColor>             m_aoFaceColors;
                // Decoded base color images, by image index (width 0: none).
            struct SImage { int nWidth = 0; int nHeight = 0; std::vector<BYTE> anPixels; BOOL bTried = FALSE; };
            std::vector<SImage>             m_aoImages;

            BOOL Fail( const std::string &strWhy ) { m_strError = strWhy; return FALSE; }
            const IASLib::CJsonNode *Array( const char *strName ) const;
            BOOL Number( const IASLib::CJsonNode *pNode, const char *strField, double &dValue ) const;
            BOOL LoadBuffers( const std::string &strBinary );
            BOOL Uri( const std::string &strUri, std::string &strOut, const char *strWhat );
            BOOL Accessor( int nIndex, int nWidth, std::vector<double> &adOut );
            SMatrix NodeMatrix( const IASLib::CJsonNode &oNode ) const;
            BOOL Node( int nIndex, const SMatrix &oMatrix, int nDepth );
            BOOL Primitive( const IASLib::CJsonNode &oPrimitive, const SMatrix &oMatrix );
            BOOL BaseColor( int nMaterial, CColor &oFactor, int &nImage, int &nTexCoord );
            const SImage *Image( int nImage );
    };

    const IASLib::CJsonNode *CGltfReader::Array( const char *strName ) const
    {
        const IASLib::CJsonNode *pArray = m_pRoot->get( strName );

        return ( pArray && pArray->isArray() ) ? pArray : NULL;
    }

    BOOL CGltfReader::Number( const IASLib::CJsonNode *pNode, const char *strField, double &dValue ) const
    {
        const IASLib::CJsonNode *pValue = pNode ? pNode->get( strField ) : NULL;

        if ( pValue == NULL || ! pValue->isNumber() )
            return FALSE;
        dValue = pValue->doubleValue();
        return TRUE;
    }

    BOOL CGltfReader::Uri( const std::string &strUri, std::string &strOut, const char *strWhat )
    {
        if ( strUri.compare( 0, 5, "data:" ) == 0 )
        {
            size_t nComma = strUri.find( ',' );

            if ( nComma == std::string::npos || strUri.rfind( ";base64", nComma ) == std::string::npos ||
                 ! FromBase64( strUri.substr( nComma + 1 ), strOut ) )
                return Fail( std::string( "a " ) + strWhat + "'s data: URI isn't base64" );
            return TRUE;
        }
        if ( m_pFiles )
        {
            for ( const auto &oFile : m_pFiles->aoFiles )
            {
                if ( oFile.first == strUri && oFile.second )
                {
                    strOut = *oFile.second;
                    return TRUE;
                }
            }
        }
        return Fail( std::string( "its " ) + strWhat + " \"" + strUri + "\" is missing" );
    }

    BOOL CGltfReader::LoadBuffers( const std::string &strBinary )
    {
        const IASLib::CJsonNode *pBuffers = Array( "buffers" );

        for ( int nIndex = 0; pBuffers && nIndex < pBuffers->size(); nIndex++ )
        {
            const IASLib::CJsonNode *pBuffer = pBuffers->get( nIndex );
            const IASLib::CJsonNode *pUri = pBuffer->get( "uri" );
            double                   dLength = 0.0;
            std::string              strData;

            if ( pUri && pUri->isTextual() )
            {
                if ( ! Uri( pUri->textString(), strData, "buffer" ) )
                    return FALSE;
            }
            else if ( nIndex == 0 )
                strData = strBinary;
            else
                return Fail( "a buffer has no data" );
            if ( Number( pBuffer, "byteLength", dLength ) && strData.size() < (size_t)dLength )
                return Fail( "a buffer is shorter than its byteLength" );
            m_astrBuffers.push_back( strData );
        }
        return TRUE;
    }

        // An accessor's values as doubles, nWidth a element (normalized
        // integers scaled to 0..1 or -1..1, as glTF says).
    BOOL CGltfReader::Accessor( int nIndex, int nWidth, std::vector<double> &adOut )
    {
        const IASLib::CJsonNode *pAccessors = Array( "accessors" );
        const IASLib::CJsonNode *pAccessor = pAccessors ? pAccessors->get( nIndex ) : NULL;

        if ( pAccessor == NULL || ! pAccessor->isObject() )
            return Fail( "an accessor is missing" );

        double      dType = 0.0, dCount = 0.0, dOffset = 0.0;
        const IASLib::CJsonNode *pType = pAccessor->get( "type" );
        std::string strType = ( pType && pType->isTextual() ) ? pType->textString() : "";
        int         nComponents = ( strType == "SCALAR" ) ? 1 : ( strType == "VEC2" ) ? 2 : ( strType == "VEC3" ) ? 3 :
                                  ( strType == "VEC4" ) ? 4 : ( strType == "MAT4" ) ? 16 : 0;
        const IASLib::CJsonNode *pNormalized = pAccessor->get( "normalized" );
        BOOL        bNormalized = ( pNormalized && pNormalized->isBoolean() && pNormalized->booleanValue() ) ? TRUE : FALSE;

        if ( ! Number( pAccessor, "componentType", dType ) || ! Number( pAccessor, "count", dCount ) || nComponents == 0 )
            return Fail( "an accessor lacks its componentType, count or type" );
        if ( nComponents < nWidth )
            return Fail( "an accessor has too few components" );
        Number( pAccessor, "byteOffset", dOffset );

        int nType = (int)dType;
        int nSize = ( nType == 5120 || nType == 5121 ) ? 1 : ( nType == 5122 || nType == 5123 ) ? 2 :
                    ( nType == 5125 || nType == 5126 ) ? 4 : 0;
        size_t nCount = (size_t)dCount;

        if ( nSize == 0 )
            return Fail( "an accessor has an unknown componentType" );
        if ( nCount > 100000000 )
            return Fail( "an accessor is too large" );
        adOut.assign( nCount * (size_t)nWidth, 0.0 );

        auto Value = [ & ]( const unsigned char *pAt ) -> double
        {
            switch ( nType )
            {
                case 5120: { double d = (double)(signed char)pAt[ 0 ]; return bNormalized ? (std::max)( d / 127.0, -1.0 ) : d; }
                case 5121: { double d = (double)pAt[ 0 ]; return bNormalized ? d / 255.0 : d; }
                case 5122: { double d = (double)(int16_t)( pAt[ 0 ] | ( pAt[ 1 ] << 8 ) ); return bNormalized ? (std::max)( d / 32767.0, -1.0 ) : d; }
                case 5123: { double d = (double)( pAt[ 0 ] | ( pAt[ 1 ] << 8 ) ); return bNormalized ? d / 65535.0 : d; }
                case 5125: return (double)Get32( pAt );
                default:   { uint32_t nBits = Get32( pAt ); float f; memcpy( &f, &nBits, 4 ); return (double)f; }
            }
        };

        auto ReadView = [ & ]( int nView, double dStart, size_t nItems, int nItemWidth, int nItemType, int nItemSize,
                               std::vector<double> *padValues, std::vector<size_t> *panIndices ) -> BOOL
        {
            const IASLib::CJsonNode *pViews = Array( "bufferViews" );
            const IASLib::CJsonNode *pView = pViews ? pViews->get( nView ) : NULL;
            double                   dBuffer = 0.0, dViewOffset = 0.0, dLength = 0.0, dStride = 0.0;

            if ( pView == NULL || ! pView->isObject() || ! Number( pView, "byteLength", dLength ) )
                return Fail( "a bufferView is missing" );
            Number( pView, "buffer", dBuffer );
            Number( pView, "byteOffset", dViewOffset );
            Number( pView, "byteStride", dStride );
            if ( (size_t)dBuffer >= m_astrBuffers.size() )
                return Fail( "a bufferView names a missing buffer" );

            const std::string &strBuffer = m_astrBuffers[ (size_t)dBuffer ];
            size_t nElement = (size_t)nItemSize * (size_t)nItemWidth;
            size_t nStride = ( dStride > 0.0 ) ? (size_t)dStride : nElement;
            size_t nBase = (size_t)dViewOffset + (size_t)dStart;

            if ( nItems > 0 && ( nBase + ( nItems - 1 ) * nStride + nElement > (size_t)dViewOffset + (size_t)dLength ||
                                 (size_t)dViewOffset + (size_t)dLength > strBuffer.size() ) )
                return Fail( "an accessor runs past its buffer" );

            const unsigned char *pData = (const unsigned char *)strBuffer.data();

            for ( size_t nItem = 0; nItem < nItems; nItem++ )
            {
                const unsigned char *pAt = pData + nBase + nItem * nStride;

                if ( panIndices )
                {
                    size_t nValue = ( nItemSize == 1 ) ? pAt[ 0 ] : ( nItemSize == 2 ) ? (size_t)( pAt[ 0 ] | ( pAt[ 1 ] << 8 ) ) : Get32( pAt );

                    ( *panIndices )[ nItem ] = nValue;
                }
                else
                {
                    for ( int nPart = 0; nPart < nWidth; nPart++ )
                        ( *padValues )[ nItem * (size_t)nWidth + (size_t)nPart ] = Value( pAt + (size_t)nPart * (size_t)nItemSize );
                }
            }
            (void)nItemType;
            return TRUE;
        };

        double dView = 0.0;

        if ( Number( pAccessor, "bufferView", dView ) &&
             ! ReadView( (int)dView, dOffset, nCount, nComponents, nType, nSize, &adOut, NULL ) )
            return FALSE;

            // Sparse: listed elements replaced.
        const IASLib::CJsonNode *pSparse = pAccessor->get( "sparse" );

        if ( pSparse && pSparse->isObject() )
        {
            const IASLib::CJsonNode *pIndices = pSparse->get( "indices" );
            const IASLib::CJsonNode *pValues = pSparse->get( "values" );
            double                   dSparse = 0.0, dIndexView = 0.0, dIndexType = 0.0, dValueView = 0.0;
            double                   dIndexOffset = 0.0, dValueOffset = 0.0;

            if ( ! Number( pSparse, "count", dSparse ) || ! Number( pIndices, "bufferView", dIndexView ) ||
                 ! Number( pIndices, "componentType", dIndexType ) || ! Number( pValues, "bufferView", dValueView ) )
                return Fail( "a sparse accessor is incomplete" );
            Number( pIndices, "byteOffset", dIndexOffset );
            Number( pValues, "byteOffset", dValueOffset );

            size_t              nSparse = (size_t)dSparse;
            int                 nIndexSize = ( (int)dIndexType == 5121 ) ? 1 : ( (int)dIndexType == 5123 ) ? 2 : 4;
            std::vector<size_t> anIndices( nSparse );
            std::vector<double> adValues( nSparse * (size_t)nWidth );

            if ( nSparse > nCount ||
                 ! ReadView( (int)dIndexView, dIndexOffset, nSparse, 1, (int)dIndexType, nIndexSize, NULL, &anIndices ) ||
                 ! ReadView( (int)dValueView, dValueOffset, nSparse, nComponents, nType, nSize, &adValues, NULL ) )
                return m_strError.empty() ? Fail( "a sparse accessor is damaged" ) : FALSE;
            for ( size_t nItem = 0; nItem < nSparse; nItem++ )
            {
                if ( anIndices[ nItem ] >= nCount )
                    return Fail( "a sparse accessor's index is out of range" );
                for ( int nPart = 0; nPart < nWidth; nPart++ )
                    adOut[ anIndices[ nItem ] * (size_t)nWidth + (size_t)nPart ] = adValues[ nItem * (size_t)nWidth + (size_t)nPart ];
            }
        }
        return TRUE;
    }

    SMatrix CGltfReader::NodeMatrix( const IASLib::CJsonNode &oNode ) const
    {
        SMatrix                  oMatrix = SMatrix::Identity();
        const IASLib::CJsonNode *pMatrix = oNode.get( "matrix" );

        if ( pMatrix && pMatrix->isArray() && pMatrix->size() == 16 )
        {
            for ( int nIndex = 0; nIndex < 16; nIndex++ )
                oMatrix.ad[ nIndex ] = pMatrix->get( nIndex )->doubleValue();
            return oMatrix;
        }

        double adT[ 3 ] = { 0.0, 0.0, 0.0 }, adR[ 4 ] = { 0.0, 0.0, 0.0, 1.0 }, adS[ 3 ] = { 1.0, 1.0, 1.0 };
        const IASLib::CJsonNode *pT = oNode.get( "translation" );
        const IASLib::CJsonNode *pR = oNode.get( "rotation" );
        const IASLib::CJsonNode *pS = oNode.get( "scale" );

        for ( int nIndex = 0; pT && pT->isArray() && nIndex < 3 && nIndex < pT->size(); nIndex++ )
            adT[ nIndex ] = pT->get( nIndex )->doubleValue();
        for ( int nIndex = 0; pR && pR->isArray() && nIndex < 4 && nIndex < pR->size(); nIndex++ )
            adR[ nIndex ] = pR->get( nIndex )->doubleValue();
        for ( int nIndex = 0; pS && pS->isArray() && nIndex < 3 && nIndex < pS->size(); nIndex++ )
            adS[ nIndex ] = pS->get( nIndex )->doubleValue();

        double x = adR[ 0 ], y = adR[ 1 ], z = adR[ 2 ], w = adR[ 3 ];
        double adRot[ 9 ] = { 1 - 2 * ( y * y + z * z ), 2 * ( x * y + z * w ), 2 * ( x * z - y * w ),
                              2 * ( x * y - z * w ), 1 - 2 * ( x * x + z * z ), 2 * ( y * z + x * w ),
                              2 * ( x * z + y * w ), 2 * ( y * z - x * w ), 1 - 2 * ( x * x + y * y ) };

        for ( int nColumn = 0; nColumn < 3; nColumn++ )
            for ( int nRow = 0; nRow < 3; nRow++ )
                oMatrix.ad[ nRow + 4 * nColumn ] = adRot[ nRow + 3 * nColumn ] * adS[ nColumn ];
        oMatrix.ad[ 12 ] = adT[ 0 ];
        oMatrix.ad[ 13 ] = adT[ 1 ];
        oMatrix.ad[ 14 ] = adT[ 2 ];
        return oMatrix;
    }

    BOOL CGltfReader::Node( int nIndex, const SMatrix &oMatrix, int nDepth )
    {
        const IASLib::CJsonNode *pNodes = Array( "nodes" );
        const IASLib::CJsonNode *pNode = pNodes ? pNodes->get( nIndex ) : NULL;

        if ( pNode == NULL || ! pNode->isObject() )
            return Fail( "a node is missing" );
        if ( nDepth > 64 )
            return Fail( "its nodes nest too deeply (or loop)" );

        double dMesh = 0.0;

        if ( Number( pNode, "mesh", dMesh ) )
        {
            const IASLib::CJsonNode *pMeshes = Array( "meshes" );
            const IASLib::CJsonNode *pMesh = pMeshes ? pMeshes->get( (int)dMesh ) : NULL;
            const IASLib::CJsonNode *pPrimitives = pMesh ? pMesh->get( "primitives" ) : NULL;

            if ( pPrimitives == NULL || ! pPrimitives->isArray() )
                return Fail( "a mesh is missing" );
            for ( int nPrimitive = 0; nPrimitive < pPrimitives->size(); nPrimitive++ )
            {
                if ( ! Primitive( *pPrimitives->get( nPrimitive ), oMatrix ) )
                    return FALSE;
            }
        }

        const IASLib::CJsonNode *pChildren = pNode->get( "children" );

        for ( int nChild = 0; pChildren && pChildren->isArray() && nChild < pChildren->size(); nChild++ )
        {
            int nChildIndex = pChildren->get( nChild )->intValue();
            const IASLib::CJsonNode *pChild = pNodes->get( nChildIndex );

            if ( pChild == NULL )
                return Fail( "a node's child is missing" );
            if ( ! Node( nChildIndex, oMatrix * NodeMatrix( *pChild ), nDepth + 1 ) )
                return FALSE;
        }
        return TRUE;
    }

    BOOL CGltfReader::BaseColor( int nMaterial, CColor &oFactor, int &nImage, int &nTexCoord )
    {
        oFactor = CColor( 1.0, 1.0, 1.0 );
        nImage = -1;
        nTexCoord = 0;

        const IASLib::CJsonNode *pMaterials = Array( "materials" );
        const IASLib::CJsonNode *pMaterial = ( pMaterials && nMaterial >= 0 ) ? pMaterials->get( nMaterial ) : NULL;
        const IASLib::CJsonNode *pPbr = pMaterial ? pMaterial->get( "pbrMetallicRoughness" ) : NULL;

        if ( pPbr == NULL )
            return FALSE;

        const IASLib::CJsonNode *pFactor = pPbr->get( "baseColorFactor" );

        if ( pFactor && pFactor->isArray() && pFactor->size() >= 3 )
            oFactor = CColor( pFactor->get( 0 )->doubleValue(), pFactor->get( 1 )->doubleValue(), pFactor->get( 2 )->doubleValue() );

        const IASLib::CJsonNode *pTexture = pPbr->get( "baseColorTexture" );
        double                   dTexture = 0.0, dCoord = 0.0;

        if ( Number( pTexture, "index", dTexture ) )
        {
            const IASLib::CJsonNode *pTextures = Array( "textures" );
            const IASLib::CJsonNode *pEntry = pTextures ? pTextures->get( (int)dTexture ) : NULL;
            double                   dSource = 0.0;

                // The plain source; extensions' sources (WebP, KTX) are
                // left out, since they need decoders Ray doesn't have.
            if ( Number( pEntry, "source", dSource ) )
                nImage = (int)dSource;
            if ( Number( pTexture, "texCoord", dCoord ) )
                nTexCoord = (int)dCoord;
        }
        return TRUE;
    }

    const CGltfReader::SImage *CGltfReader::Image( int nImage )
    {
        const IASLib::CJsonNode *pImages = Array( "images" );

        if ( pImages == NULL || nImage < 0 || nImage >= pImages->size() )
            return NULL;
        if ( m_aoImages.size() < (size_t)pImages->size() )
            m_aoImages.resize( (size_t)pImages->size() );

        SImage &oImage = m_aoImages[ (size_t)nImage ];

        if ( ! oImage.bTried )
        {
            const IASLib::CJsonNode *pImage = pImages->get( nImage );
            const IASLib::CJsonNode *pUri = pImage->get( "uri" );
            std::string              strData, strWhy;
            double                   dView = 0.0;

            oImage.bTried = TRUE;
            if ( Number( pImage, "bufferView", dView ) )
            {
                const IASLib::CJsonNode *pViews = Array( "bufferViews" );
                const IASLib::CJsonNode *pView = pViews ? pViews->get( (int)dView ) : NULL;
                double                   dBuffer = 0.0, dOffset = 0.0, dLength = 0.0;

                if ( pView && Number( pView, "byteLength", dLength ) )
                {
                    Number( pView, "buffer", dBuffer );
                    Number( pView, "byteOffset", dOffset );
                    if ( (size_t)dBuffer < m_astrBuffers.size() && (size_t)( dOffset + dLength ) <= m_astrBuffers[ (size_t)dBuffer ].size() )
                        strData = m_astrBuffers[ (size_t)dBuffer ].substr( (size_t)dOffset, (size_t)dLength );
                }
            }
            else if ( pUri && pUri->isTextual() )
            {
                std::string strSaved = m_strError;

                if ( ! Uri( pUri->textString(), strData, "image" ) )
                {
                    strData.clear();
                    m_strError = strSaved;
                }
            }
                // PNG only: a JPEG (or anything else) leaves the base color
                // factor alone, and the model says so.
            if ( CPngImage::IsPng( strData ) )
            {
                if ( ! CPngImage::Decode( strData, oImage.nWidth, oImage.nHeight, oImage.anPixels, strWhy ) )
                    oImage.nWidth = oImage.nHeight = 0;
            }
            else if ( ! strData.empty() || ( pUri && pUri->isTextual() ) )
                m_oModel.nSkippedImages++;
        }
        return ( oImage.nWidth > 0 ) ? &oImage : NULL;
    }

    BOOL CGltfReader::Primitive( const IASLib::CJsonNode &oPrimitive, const SMatrix &oMatrix )
    {
        double dMode = 4.0;

        Number( &oPrimitive, "mode", dMode );
        if ( dMode != 4.0 && dMode != 5.0 && dMode != 6.0 )
            return TRUE;        // points and lines have no faces

        const IASLib::CJsonNode *pExtensions = oPrimitive.get( "extensions" );

        if ( pExtensions && ( pExtensions->get( "KHR_draco_mesh_compression" ) || pExtensions->get( "EXT_meshopt_compression" ) ) )
            return Fail( "is compressed (Draco or meshopt), which Ray doesn't decode" );

        const IASLib::CJsonNode *pAttributes = oPrimitive.get( "attributes" );
        double                   dPosition = 0.0, dNormal = 0.0, dIndices = 0.0, dMaterial = -1.0;
        std::vector<double>      adPoints, adNormals, adCoords, adIndices;

        if ( ! Number( pAttributes, "POSITION", dPosition ) )
            return Fail( "a primitive has no POSITION" );
        if ( ! Accessor( (int)dPosition, 3, adPoints ) )
            return FALSE;

        size_t  nPoints = adPoints.size() / 3;
        CColor  oFactor( 1.0, 1.0, 1.0 );
        int     nImage = -1, nTexCoord = 0;
        BOOL    bColored = FALSE;

        if ( Number( &oPrimitive, "material", dMaterial ) )
            bColored = BaseColor( (int)dMaterial, oFactor, nImage, nTexCoord );

        BOOL   bHasNormals = Number( pAttributes, "NORMAL", dNormal );
        std::string strCoord = "TEXCOORD_" + std::to_string( nTexCoord );
        double dCoord = 0.0, dCoord0 = 0.0;
        BOOL   bHasCoords = Number( pAttributes, "TEXCOORD_0", dCoord0 );

        if ( bHasNormals && ! Accessor( (int)dNormal, 3, adNormals ) )
            return FALSE;
        if ( bHasCoords && ! Accessor( (int)dCoord0, 2, adCoords ) )
            return FALSE;
        if ( adNormals.size() != adPoints.size() )
            bHasNormals = FALSE;
        if ( adCoords.size() != nPoints * 2 )
            bHasCoords = FALSE;

        std::vector<double> adColorCoords( adCoords );

        if ( nTexCoord != 0 && Number( pAttributes, strCoord.c_str(), dCoord ) && ! Accessor( (int)dCoord, 2, adColorCoords ) )
            return FALSE;

        std::vector<size_t> anOrder;

        if ( Number( &oPrimitive, "indices", dIndices ) )
        {
            if ( ! Accessor( (int)dIndices, 1, adIndices ) )
                return FALSE;
            anOrder.resize( adIndices.size() );
            for ( size_t nItem = 0; nItem < adIndices.size(); nItem++ )
            {
                if ( adIndices[ nItem ] < 0.0 || (size_t)adIndices[ nItem ] >= nPoints )
                    return Fail( "an index is out of range" );
                anOrder[ nItem ] = (size_t)adIndices[ nItem ];
            }
        }
        else
        {
            anOrder.resize( nPoints );
            for ( size_t nItem = 0; nItem < nPoints; nItem++ )
                anOrder[ nItem ] = nItem;
        }

            // glTF is right-handed and Ray's scenes are left-handed: z is
            // negated, and each face turned back round, so the model isn't
            // mirrored. Normals go through the inverse transpose.
        const double *m = oMatrix.ad;
        double adInverse[ 9 ];
        {
            double a = m[ 0 ], b = m[ 4 ], c = m[ 8 ], d = m[ 1 ], e = m[ 5 ], f = m[ 9 ], g = m[ 2 ], h = m[ 6 ], i = m[ 10 ];
            double dDet = a * ( e * i - f * h ) - b * ( d * i - f * g ) + c * ( d * h - e * g );

            if ( dDet == 0.0 )
                dDet = 1.0;
                // The inverse's transpose, row by row.
            adInverse[ 0 ] = ( e * i - f * h ) / dDet; adInverse[ 1 ] = ( f * g - d * i ) / dDet; adInverse[ 2 ] = ( d * h - e * g ) / dDet;
            adInverse[ 3 ] = ( c * h - b * i ) / dDet; adInverse[ 4 ] = ( a * i - c * g ) / dDet; adInverse[ 5 ] = ( b * g - a * h ) / dDet;
            adInverse[ 6 ] = ( b * f - c * e ) / dDet; adInverse[ 7 ] = ( c * d - a * f ) / dDet; adInverse[ 8 ] = ( a * e - b * d ) / dDet;
        }

        size_t nFirst = m_oModel.aoVertices.size();

        for ( size_t nPoint = 0; nPoint < nPoints; nPoint++ )
        {
            double x = adPoints[ nPoint * 3 ], y = adPoints[ nPoint * 3 + 1 ], z = adPoints[ nPoint * 3 + 2 ];

            m_oModel.aoVertices.push_back( CVector( m[ 0 ] * x + m[ 4 ] * y + m[ 8 ] * z + m[ 12 ],
                                                    m[ 1 ] * x + m[ 5 ] * y + m[ 9 ] * z + m[ 13 ],
                                                    -( m[ 2 ] * x + m[ 6 ] * y + m[ 10 ] * z + m[ 14 ] ) ) );
            if ( bHasNormals )
            {
                double nx = adNormals[ nPoint * 3 ], ny = adNormals[ nPoint * 3 + 1 ], nz = adNormals[ nPoint * 3 + 2 ];
                double ox = adInverse[ 0 ] * nx + adInverse[ 1 ] * ny + adInverse[ 2 ] * nz;
                double oy = adInverse[ 3 ] * nx + adInverse[ 4 ] * ny + adInverse[ 5 ] * nz;
                double oz = adInverse[ 6 ] * nx + adInverse[ 7 ] * ny + adInverse[ 8 ] * nz;
                double dLength = sqrt( ox * ox + oy * oy + oz * oz );

                if ( dLength > 0.0 )
                {
                    ox /= dLength;
                    oy /= dLength;
                    oz /= dLength;
                }
                m_aoNormals.push_back( CVector( ox, oy, -oz ) );
            }
            else
                m_aoNormals.push_back( CVector( 0.0, 1.0, 0.0 ) );
            m_anCoords.push_back( bHasCoords ? adCoords[ nPoint * 2 ] : 0.0 );
            m_anCoords.push_back( bHasCoords ? adCoords[ nPoint * 2 + 1 ] : 0.0 );
        }
        if ( ! bHasNormals )
            m_bNormals = FALSE;
        if ( ! bHasCoords )
            m_bCoords = FALSE;

        std::vector<size_t> anFaces;

        if ( dMode == 4.0 )
        {
            for ( size_t nItem = 0; nItem + 2 < anOrder.size(); nItem += 3 )
            {
                anFaces.push_back( anOrder[ nItem ] );
                anFaces.push_back( anOrder[ nItem + 1 ] );
                anFaces.push_back( anOrder[ nItem + 2 ] );
            }
        }
        else if ( dMode == 5.0 )
        {
            for ( size_t nItem = 0; nItem + 2 < anOrder.size(); nItem++ )
            {
                BOOL bEven = ( nItem % 2 == 0 ) ? TRUE : FALSE;

                anFaces.push_back( anOrder[ bEven ? nItem : nItem + 1 ] );
                anFaces.push_back( anOrder[ bEven ? nItem + 1 : nItem ] );
                anFaces.push_back( anOrder[ nItem + 2 ] );
            }
        }
        else
        {
            for ( size_t nItem = 1; nItem + 1 < anOrder.size(); nItem++ )
            {
                anFaces.push_back( anOrder[ 0 ] );
                anFaces.push_back( anOrder[ nItem ] );
                anFaces.push_back( anOrder[ nItem + 1 ] );
            }
        }

        const SImage *pImage = ( bColored && nImage >= 0 ) ? Image( nImage ) : NULL;

        for ( size_t nFace = 0; nFace + 2 < anFaces.size(); nFace += 3 )
        {
            size_t a = anFaces[ nFace ], b = anFaces[ nFace + 1 ], c = anFaces[ nFace + 2 ];

            if ( a == b || b == c || a == c )
                continue;       // degenerate
                // Turned back round with the mirroring.
            m_oModel.anFaces.push_back( (int)( nFirst + a ) );
            m_oModel.anFaces.push_back( (int)( nFirst + c ) );
            m_oModel.anFaces.push_back( (int)( nFirst + b ) );

            CColor oColor( oFactor );

            if ( pImage && adColorCoords.size() == nPoints * 2 )
            {
                    // The texture at the face's middle, sRGB to linear,
                    // wrapped as glTF's default sampler does.
                double dU = ( adColorCoords[ a * 2 ] + adColorCoords[ b * 2 ] + adColorCoords[ c * 2 ] ) / 3.0;
                double dV = ( adColorCoords[ a * 2 + 1 ] + adColorCoords[ b * 2 + 1 ] + adColorCoords[ c * 2 + 1 ] ) / 3.0;
                int    nX = (int)( ( dU - floor( dU ) ) * pImage->nWidth );
                int    nY = (int)( ( dV - floor( dV ) ) * pImage->nHeight );

                nX = (std::min)( (std::max)( nX, 0 ), pImage->nWidth - 1 );
                nY = (std::min)( (std::max)( nY, 0 ), pImage->nHeight - 1 );

                const BYTE *pPixel = &pImage->anPixels[ ( (size_t)nY * (size_t)pImage->nWidth + (size_t)nX ) * 3 ];
                double      adLinear[ 3 ];

                for ( int nChannel = 0; nChannel < 3; nChannel++ )
                {
                    double dValue = pPixel[ nChannel ] / 255.0;

                    adLinear[ nChannel ] = ( dValue <= 0.04045 ) ? dValue / 12.92 : pow( ( dValue + 0.055 ) / 1.055, 2.4 );
                }
                oColor = CColor( oFactor.GetR() * adLinear[ 0 ], oFactor.GetG() * adLinear[ 1 ], oFactor.GetB() * adLinear[ 2 ] );
            }
            if ( bColored )
                m_bAnyColor = TRUE;
            m_aoFaceColors.push_back( oColor );
            m_oModel.abHasColor.push_back( bColored ? 1 : 0 );
        }
        return TRUE;
    }

    BOOL CGltfReader::Read( const std::string &strData, const std::string &strNode )
    {
        std::string strJson, strBinary;
        const unsigned char *pData = (const unsigned char *)strData.data();

        if ( strData.size() >= 12 && memcmp( pData, "glTF", 4 ) == 0 )
        {
            uint32_t nVersion = Get32( pData + 4 );
            uint32_t nLength = Get32( pData + 8 );
            size_t   nAt = 12;

            if ( nVersion != 2 )
                return Fail( "is glTF version " + std::to_string( nVersion ) + "; Ray reads version 2" );
            if ( nLength > strData.size() )
                return Fail( "is cut short" );
            while ( nAt + 8 <= nLength )
            {
                uint32_t nChunk = Get32( pData + nAt );
                uint32_t nType = Get32( pData + nAt + 4 );

                if ( nAt + 8 + (size_t)nChunk > nLength )
                    return Fail( "has a chunk that runs past its end" );
                if ( nType == 0x4E4F534A && strJson.empty() )
                    strJson = strData.substr( nAt + 8, nChunk );
                else if ( nType == 0x004E4942 && strBinary.empty() )
                    strBinary = strData.substr( nAt + 8, nChunk );
                nAt += 8 + (size_t)nChunk;
            }
            if ( strJson.empty() )
                return Fail( "has no JSON chunk" );
        }
        else
            strJson = strData;

        IASLib::CJsonNode *pRoot = RayJson::Parse( strJson );

        if ( pRoot == NULL || ! pRoot->isObject() )
        {
            delete pRoot;
            return Fail( "isn't glTF: " + RayJson::GetError() );
        }
        m_pRoot = pRoot;

        const IASLib::CJsonNode *pAsset = m_pRoot->get( "asset" );
        const IASLib::CJsonNode *pVersion = pAsset ? pAsset->get( "version" ) : NULL;

        if ( pVersion == NULL || ! pVersion->isTextual() || pVersion->textString().compare( 0, 1, "2" ) != 0 )
            return Fail( "isn't glTF 2.0 (its asset.version)" );

        const IASLib::CJsonNode *pRequired = Array( "extensionsRequired" );

        for ( int nIndex = 0; pRequired && nIndex < pRequired->size(); nIndex++ )
        {
            std::string strName = pRequired->get( nIndex )->textString();

            if ( strName == "KHR_draco_mesh_compression" || strName == "EXT_meshopt_compression" )
                return Fail( "needs " + strName + ", which Ray doesn't decode (decompress it first, e.g. with "
                             "tools/gltf_decompress.py)" );
        }
        if ( ! LoadBuffers( strBinary ) )
            return FALSE;

        const IASLib::CJsonNode *pNodes = Array( "nodes" );

        if ( ! strNode.empty() )
        {
                // The named node and its children, in its own frame.
            int nFound = -1;

            for ( int nIndex = 0; pNodes && nIndex < pNodes->size(); nIndex++ )
            {
                const IASLib::CJsonNode *pName = pNodes->get( nIndex )->get( "name" );

                if ( pName && pName->isTextual() && pName->textString() == strNode )
                {
                    nFound = nIndex;
                    break;
                }
            }
            if ( nFound < 0 )
                return Fail( "has no node named \"" + strNode + "\"" );
            if ( ! Node( nFound, SMatrix::Identity(), 0 ) )
                return FALSE;
        }
        else
        {
            const IASLib::CJsonNode *pScenes = Array( "scenes" );
            double                   dScene = 0.0;

            Number( m_pRoot, "scene", dScene );

            const IASLib::CJsonNode *pScene = pScenes ? pScenes->get( (int)dScene ) : NULL;
            const IASLib::CJsonNode *pRoots = pScene ? pScene->get( "nodes" ) : NULL;

            if ( pRoots && pRoots->isArray() )
            {
                for ( int nIndex = 0; nIndex < pRoots->size(); nIndex++ )
                {
                    int nRoot = pRoots->get( nIndex )->intValue();
                    const IASLib::CJsonNode *pNode = pNodes ? pNodes->get( nRoot ) : NULL;

                    if ( pNode == NULL )
                        return Fail( "a scene names a missing node" );
                    if ( ! Node( nRoot, NodeMatrix( *pNode ), 0 ) )
                        return FALSE;
                }
            }
            else
            {
                    // No scene: every node that is no node's child.
                std::vector<unsigned char> abChild( pNodes ? (size_t)pNodes->size() : 0, 0 );

                for ( int nIndex = 0; pNodes && nIndex < pNodes->size(); nIndex++ )
                {
                    const IASLib::CJsonNode *pChildren = pNodes->get( nIndex )->get( "children" );

                    for ( int nChild = 0; pChildren && pChildren->isArray() && nChild < pChildren->size(); nChild++ )
                    {
                        int nKid = pChildren->get( nChild )->intValue();

                        if ( nKid >= 0 && (size_t)nKid < abChild.size() )
                            abChild[ (size_t)nKid ] = 1;
                    }
                }
                for ( int nIndex = 0; pNodes && nIndex < pNodes->size(); nIndex++ )
                {
                    if ( ! abChild[ (size_t)nIndex ] && ! Node( nIndex, NodeMatrix( *pNodes->get( nIndex ) ), 0 ) )
                        return FALSE;
                }
            }
        }

        if ( m_oModel.anFaces.empty() )
            return Fail( "has no triangles" );
        if ( m_bNormals )
            m_oModel.aoNormals = m_aoNormals;
        if ( m_bCoords )
            m_oModel.anUVs = m_anCoords;
        if ( m_bAnyColor )
            m_oModel.aoFaceColors = m_aoFaceColors;
        else
            m_oModel.abHasColor.clear();
        return TRUE;
    }
}

namespace ModelFile
{

BOOL IsGltf( const std::string &strData, const std::string &strName )
{
    std::string strLower( strName );

    for ( char &chNext : strLower )
        chNext = (char)tolower( (unsigned char)chNext );
    if ( strData.size() >= 4 && memcmp( strData.data(), "glTF", 4 ) == 0 )
        return TRUE;
    return ( strLower.size() >= 5 && strLower.compare( strLower.size() - 5, 5, ".gltf" ) == 0 ) ||
           ( strLower.size() >= 4 && strLower.compare( strLower.size() - 4, 4, ".glb" ) == 0 );
}

BOOL ReadGltf( const std::string &strData, const SGltfFiles *pFiles, const std::string &strNode, SModel &oModel,
               std::string &strError )
{
    CGltfReader oReader( pFiles, oModel, strError );

    oModel = SModel();
    return oReader.Read( strData, strNode );
}

BOOL GltfUris( const std::string &strData, std::vector<std::string> &astrUris, std::string &strError )
{
    astrUris.clear();
    if ( strData.size() >= 4 && memcmp( strData.data(), "glTF", 4 ) == 0 )
    {
            // A .glb's JSON names files too, though they're rarely there.
        if ( strData.size() < 20 )
            return TRUE;

        uint32_t nChunk = Get32( (const unsigned char *)strData.data() + 12 );

        if ( 20 + (size_t)nChunk > strData.size() )
            return TRUE;
        return GltfUris( strData.substr( 20, nChunk ), astrUris, strError );
    }

    std::unique_ptr<IASLib::CJsonNode> pRoot( RayJson::Parse( strData ) );

    if ( ! pRoot || ! pRoot->isObject() )
    {
        strError = "isn't glTF: " + RayJson::GetError();
        return FALSE;
    }

    const char *astrKinds[] = { "buffers", "images" };

    for ( const char *strKind : astrKinds )
    {
        const IASLib::CJsonNode *pList = pRoot->get( strKind );

        for ( int nIndex = 0; pList && pList->isArray() && nIndex < pList->size(); nIndex++ )
        {
            const IASLib::CJsonNode *pUri = pList->get( nIndex )->get( "uri" );

            if ( pUri && pUri->isTextual() && pUri->textString().compare( 0, 5, "data:" ) != 0 )
            {
                std::string strUri = pUri->textString();
                std::string strLower( strUri );

                for ( char &chNext : strLower )
                    chNext = (char)tolower( (unsigned char)chNext );
                    // Images only if they're PNGs, the only kind Ray reads.
                if ( strcmp( strKind, "images" ) == 0 &&
                     ! ( strLower.size() >= 4 && strLower.compare( strLower.size() - 4, 4, ".png" ) == 0 ) )
                    continue;
                if ( strUri.find( ".." ) != std::string::npos || strUri.find( ':' ) != std::string::npos ||
                     ( ! strUri.empty() && ( strUri[ 0 ] == '/' || strUri[ 0 ] == '\\' ) ) )
                {
                    strError = "names a file outside its folder (\"" + strUri + "\")";
                    return FALSE;
                }
                astrUris.push_back( strUri );
            }
        }
    }
    return TRUE;
}

}
