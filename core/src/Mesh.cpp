#ifdef RAY_GUI
    #include "StdAfx.h"
    #include "resource.h"
#else
    #include "Globals.h"
#endif

#include "Mesh.h"
#include "ModelFile.h"
#include "SceneJson.h"

#include <math.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DYNAMIC_IMPLEMENTS( CMesh, "mesh" );

namespace
{
        // Turns anVector about x, then y, then z, by the angles (radians), as
        // a box's rotation does.
    void RotateXYZ( FLT anVector[ 3 ], FLT nX, FLT nY, FLT nZ )
    {
        FLT nA, nB;

        nA = ( anVector[ 1 ] * COS( nX ) ) - ( anVector[ 2 ] * SIN( nX ) );
        nB = ( anVector[ 1 ] * SIN( nX ) ) + ( anVector[ 2 ] * COS( nX ) );
        anVector[ 1 ] = nA;
        anVector[ 2 ] = nB;

        nA = ( anVector[ 0 ] * COS( nY ) ) + ( anVector[ 2 ] * SIN( nY ) );
        nB = ( anVector[ 2 ] * COS( nY ) ) - ( anVector[ 0 ] * SIN( nY ) );
        anVector[ 0 ] = nA;
        anVector[ 2 ] = nB;

        nA = ( anVector[ 0 ] * COS( nZ ) ) - ( anVector[ 1 ] * SIN( nZ ) );
        nB = ( anVector[ 0 ] * SIN( nZ ) ) + ( anVector[ 1 ] * COS( nZ ) );
        anVector[ 0 ] = nA;
        anVector[ 1 ] = nB;
    }

        // Models already read, by their contents, so the copies of a scene
        // each task reads share one reading of a large file.
    struct SModelKey
    {
        size_t      nSize;
        uint64_t    nHash;

        bool operator <( const SModelKey &oOther ) const
        {
            return ( nSize != oOther.nSize ) ? ( nSize < oOther.nSize ) : ( nHash < oOther.nHash );
        }
    };

    std::mutex                                                          s_oModelLock;
    std::map< SModelKey, std::weak_ptr<const ModelFile::SModel> >       s_oModels;

    std::shared_ptr<const ModelFile::SModel> ReadModelOnce( const std::string &strData, const std::string &strName, std::string &strError )
    {
        SModelKey oKey;
        uint64_t  nHash = 1469598103934665603ULL;

        for ( size_t nIndex = 0; nIndex < strData.size(); nIndex++ )
            nHash = ( nHash ^ (unsigned char)strData[ nIndex ] ) * 1099511628211ULL;
            // The name picks the format, so it is part of the key.
        for ( size_t nIndex = 0; nIndex < strName.size(); nIndex++ )
            nHash = ( nHash ^ (unsigned char)strName[ nIndex ] ) * 1099511628211ULL;
        oKey.nSize = strData.size();
        oKey.nHash = nHash;

        {
            std::lock_guard<std::mutex>                 oLock( s_oModelLock );
            std::shared_ptr<const ModelFile::SModel>    pFound = s_oModels[ oKey ].lock();

            if ( pFound )
                return pFound;
        }

        std::shared_ptr<ModelFile::SModel> pModel( new ModelFile::SModel );

        if ( ! ModelFile::Read( strData, strName, *pModel, strError ) )
            return std::shared_ptr<const ModelFile::SModel>();

        std::lock_guard<std::mutex> oLock( s_oModelLock );

        s_oModels[ oKey ] = pModel;
        return pModel;
    }

        // Model files' bytes, by their contents, so every copy of a scene
        // holds one copy of a large file between them.
    std::mutex                                                          s_oDataLock;
    std::map< SModelKey, std::weak_ptr<const std::string> >             s_oData;

    std::shared_ptr<const std::string> ShareFileData( std::string &strData )
    {
        SModelKey oKey;
        uint64_t  nHash = 1469598103934665603ULL;

        for ( size_t nIndex = 0; nIndex < strData.size(); nIndex++ )
            nHash = ( nHash ^ (unsigned char)strData[ nIndex ] ) * 1099511628211ULL;
        oKey.nSize = strData.size();
        oKey.nHash = nHash;

        std::lock_guard<std::mutex>         oLock( s_oDataLock );
        std::shared_ptr<const std::string>  pFound = s_oData[ oKey ].lock();

        if ( pFound && *pFound == strData )
            return pFound;

        std::shared_ptr<const std::string>  pData( new std::string( std::move( strData ) ) );

        s_oData[ oKey ] = pData;
        return pData;
    }
}

BOOL CMesh::LoadModel( const std::string &strData, const std::string &strName, std::string &strError )
{
    std::shared_ptr<const ModelFile::SModel> pModel = ReadModelOnce( strData, strName, strError );

    if ( ! pModel )
        return FALSE;
    m_pModel = pModel;

    const ModelFile::SModel &oModel = *pModel;
    const FLT               nToRadians = PI / 180.0;
    FLT                     nRX = m_oRotation.GetX() * nToRadians;
    FLT                     nRY = m_oRotation.GetY() * nToRadians;
    FLT                     nRZ = m_oRotation.GetZ() * nToRadians;
    FLT                     anScale[ 3 ] = { m_oScale.GetX(), m_oScale.GetY(), m_oScale.GetZ() };
    FLT                     anMin[ 3 ] = { 1e300, 1e300, 1e300 };
    FLT                     anMax[ 3 ] = { -1e300, -1e300, -1e300 };
    std::vector<CVector>    aoVertices( oModel.aoVertices.size() );

    if ( m_nFitSize > 0.0 )
    {
            // Uniformly, so the turned model's box is m_nFitSize across at
            // its widest.
        for ( size_t nIndex = 0; nIndex < oModel.aoVertices.size(); nIndex++ )
        {
            CVector oVertex( oModel.aoVertices[ nIndex ] );
            FLT     anValue[ 3 ] = { oVertex.GetX(), oVertex.GetY(), oVertex.GetZ() };

            RotateXYZ( anValue, nRX, nRY, nRZ );
            for ( int nAxis = 0; nAxis < 3; nAxis++ )
            {
                anMin[ nAxis ] = (std::min)( anMin[ nAxis ], anValue[ nAxis ] );
                anMax[ nAxis ] = (std::max)( anMax[ nAxis ], anValue[ nAxis ] );
            }
        }

        FLT nWidest = (std::max)( anMax[ 0 ] - anMin[ 0 ], (std::max)( anMax[ 1 ] - anMin[ 1 ], anMax[ 2 ] - anMin[ 2 ] ) );

        if ( ! ( nWidest > 0.0 ) )
        {
            strError = "has no size to fit";
            return FALSE;
        }
        anScale[ 0 ] = anScale[ 1 ] = anScale[ 2 ] = m_nFitSize / nWidest;
        for ( int nAxis = 0; nAxis < 3; nAxis++ )
        {
            anMin[ nAxis ] = 1e300;
            anMax[ nAxis ] = -1e300;
        }
    }

    for ( size_t nIndex = 0; nIndex < oModel.aoVertices.size(); nIndex++ )
    {
        CVector oVertex( oModel.aoVertices[ nIndex ] );
        FLT     anValue[ 3 ] = { oVertex.GetX() * anScale[ 0 ], oVertex.GetY() * anScale[ 1 ], oVertex.GetZ() * anScale[ 2 ] };

        RotateXYZ( anValue, nRX, nRY, nRZ );
        for ( int nAxis = 0; nAxis < 3; nAxis++ )
        {
            anMin[ nAxis ] = (std::min)( anMin[ nAxis ], anValue[ nAxis ] );
            anMax[ nAxis ] = (std::max)( anMax[ nAxis ], anValue[ nAxis ] );
        }
        aoVertices[ nIndex ].Set( anValue[ 0 ], anValue[ 1 ], anValue[ 2 ] );
    }

    FLT anShift[ 3 ] = { m_oTranslation.GetX(), m_oTranslation.GetY(), m_oTranslation.GetZ() };

    if ( m_nAlign == ALIGN_CENTER || m_nAlign == ALIGN_BOTTOM )
    {
        for ( int nAxis = 0; nAxis < 3; nAxis++ )
            anShift[ nAxis ] -= ( anMin[ nAxis ] + anMax[ nAxis ] ) / 2.0;
        if ( m_nAlign == ALIGN_BOTTOM )
            anShift[ 1 ] += ( ( anMin[ 1 ] + anMax[ 1 ] ) / 2.0 ) - anMin[ 1 ];
    }
    for ( size_t nIndex = 0; nIndex < aoVertices.size(); nIndex++ )
    {
        CVector &oVertex = aoVertices[ nIndex ];

        oVertex.Set( oVertex.GetX() + anShift[ 0 ], oVertex.GetY() + anShift[ 1 ], oVertex.GetZ() + anShift[ 2 ] );
    }

        // Normals turn with the model and scale by the inverse of its scale.
    std::vector<CVector> aoNormals( oModel.aoNormals.size() );

    for ( size_t nIndex = 0; nIndex < oModel.aoNormals.size(); nIndex++ )
    {
        CVector oNormal( oModel.aoNormals[ nIndex ] );
        FLT     anValue[ 3 ] = { oNormal.GetX() / anScale[ 0 ], oNormal.GetY() / anScale[ 1 ], oNormal.GetZ() / anScale[ 2 ] };

        RotateXYZ( anValue, nRX, nRY, nRZ );
        aoNormals[ nIndex ].Set( anValue[ 0 ], anValue[ 1 ], anValue[ 2 ] );
        aoNormals[ nIndex ].Normalize();
    }

        // A mirroring scale turns the faces inside out; put their corners
        // back the other way round.
    std::vector<int> anFaces( oModel.anFaces );

    if ( anScale[ 0 ] * anScale[ 1 ] * anScale[ 2 ] < 0.0 )
    {
        for ( size_t nFace = 0; nFace + 2 < anFaces.size(); nFace += 3 )
            std::swap( anFaces[ nFace + 1 ], anFaces[ nFace + 2 ] );
    }

        // Textures are looked up back in the file's coordinates: undo the
        // shift, the turn (its transpose) and the scale.
    std::shared_ptr<STextureFrame> pFrame( new STextureFrame );
    FLT                            anColumns[ 3 ][ 3 ];

    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        FLT anBasis[ 3 ] = { 0.0, 0.0, 0.0 };

        anBasis[ nAxis ] = 1.0;
        RotateXYZ( anBasis, nRX, nRY, nRZ );
        for ( int nRow = 0; nRow < 3; nRow++ )
            anColumns[ nAxis ][ nRow ] = anBasis[ nRow ];
    }
    for ( int nRow = 0; nRow < 3; nRow++ )
    {
        FLT nOffset = 0.0;

        for ( int nColumn = 0; nColumn < 3; nColumn++ )
        {
                // Row nRow of the inverse turn is the turn's column nRow.
            FLT nValue = anColumns[ nRow ][ nColumn ] / anScale[ nRow ];

            pFrame->anMatrix[ ( nRow * 4 ) + nColumn ] = nValue;
            nOffset -= nValue * anShift[ nColumn ];
        }
        pFrame->anMatrix[ ( nRow * 4 ) + 3 ] = nOffset;
    }
    SetTextureFrame( pFrame );

    if ( ! SetGeometry( aoVertices, anFaces, m_bSmooth || ! aoNormals.empty(), m_bDoubleSided, aoNormals, oModel.anUVs,
                        oModel.aoFaceColors, oModel.abHasColor ) )
    {
        strError = "has a face with a corner out of range";
        return FALSE;
    }
    return TRUE;
}

namespace
{
        // A face of a mesh, made when it is shaded (or a csg asks for it):
        // an ordinary triangle that knows which face of which mesh it is, so
        // rays leaving it skip that face, and glass knows a ray came back
        // out through the same surface.
    class CMeshFace : public CTriangle
    {
        protected:
            const CObjects *m_pMesh;
            int             m_nFace;

        public:
                            CMeshFace( const CObjects *pMesh, int nFace, const CMesh::SGeometry &oGeometry, CMaterialList *pMaterials ) :
                                CTriangle( oGeometry.aoVertices[ (size_t)oGeometry.anKept[ (size_t)nFace * 3 ] ],
                                           oGeometry.aoVertices[ (size_t)oGeometry.anKept[ ( (size_t)nFace * 3 ) + 1 ] ],
                                           oGeometry.aoVertices[ (size_t)oGeometry.anKept[ ( (size_t)nFace * 3 ) + 2 ] ],
                                           oGeometry.bDoubleSided, oGeometry.bSmooth, pMaterials ),
                                m_pMesh( pMesh ),
                                m_nFace( nFace )
                            {
                                size_t nAt = (size_t)nFace * 3;

                                if ( ! oGeometry.aoFaceColors.empty() && oGeometry.abFaceHasColor[ (size_t)nFace ] )
                                    SetFaceColor( oGeometry.aoFaceColors[ (size_t)nFace ] );
                                if ( ! oGeometry.anCornerUVs.empty() )
                                    SetCornerUVs( &oGeometry.anCornerUVs[ (size_t)nFace * 6 ] );
                                if ( oGeometry.bSmooth )
                                    SetCornerNormals( oGeometry.aoVertexNormals[ (size_t)oGeometry.anKept[ nAt ] ],
                                                      oGeometry.aoVertexNormals[ (size_t)oGeometry.anKept[ nAt + 1 ] ],
                                                      oGeometry.aoVertexNormals[ (size_t)oGeometry.anKept[ nAt + 2 ] ] );
                            }

            virtual int     MeshFaceOf( const CObjects *pMesh ) const { return ( pMesh == m_pMesh ) ? m_nFace : -1; }
            virtual BOOL    SameSurface( const CObjects *pOther ) const
                            { return ( pOther == this || ( pOther && pOther->MeshFaceOf( m_pMesh ) == m_nFace ) ) ? TRUE : FALSE; }
    };

        // Hashing and comparing a geometry's inputs bit for bit, so meshes
        // given the same shape share one.
    inline uint64_t Bits( FLT nValue )
    {
        double      dValue = (double)nValue;
        uint64_t    nBits;

        memcpy( &nBits, &dValue, sizeof( nBits ) );
        return nBits;
    }

    inline void Mix( uint64_t &nHash, uint64_t nValue )
    {
        nHash ^= nValue + 0x9E3779B97F4A7C15ULL + ( nHash << 6 ) + ( nHash >> 2 );
        nHash *= 0x100000001B3ULL;
    }

    inline void MixVector( uint64_t &nHash, const CVector &oVector )
    {
        CVector &oValue = const_cast<CVector &>( oVector );

        Mix( nHash, Bits( oValue.GetX() ) );
        Mix( nHash, Bits( oValue.GetY() ) );
        Mix( nHash, Bits( oValue.GetZ() ) );
    }

    inline void MixColor( uint64_t &nHash, const CColor &oColor )
    {
        CColor &oValue = const_cast<CColor &>( oColor );

        Mix( nHash, Bits( oValue.GetR() ) );
        Mix( nHash, Bits( oValue.GetG() ) );
        Mix( nHash, Bits( oValue.GetB() ) );
    }

    uint64_t HashInputs( const CMesh::SGeometry &oGeometry )
    {
        uint64_t nHash = 0xCBF29CE484222325ULL;

        Mix( nHash, oGeometry.aoVertices.size() );
        for ( size_t nIndex = 0; nIndex < oGeometry.aoVertices.size(); nIndex++ )
            MixVector( nHash, oGeometry.aoVertices[ nIndex ] );
        Mix( nHash, oGeometry.anFaces.size() );
        for ( size_t nIndex = 0; nIndex < oGeometry.anFaces.size(); nIndex++ )
            Mix( nHash, (uint64_t)(int64_t)oGeometry.anFaces[ nIndex ] );
        Mix( nHash, oGeometry.aoGivenNormals.size() );
        for ( size_t nIndex = 0; nIndex < oGeometry.aoGivenNormals.size(); nIndex++ )
            MixVector( nHash, oGeometry.aoGivenNormals[ nIndex ] );
        Mix( nHash, oGeometry.anUVs.size() );
        for ( size_t nIndex = 0; nIndex < oGeometry.anUVs.size(); nIndex++ )
            Mix( nHash, Bits( oGeometry.anUVs[ nIndex ] ) );
        Mix( nHash, oGeometry.aoGivenColors.size() );
        for ( size_t nIndex = 0; nIndex < oGeometry.aoGivenColors.size(); nIndex++ )
        {
            MixColor( nHash, oGeometry.aoGivenColors[ nIndex ] );
            Mix( nHash, oGeometry.abGivenHasColor[ nIndex ] );
        }
        Mix( nHash, (uint64_t)( oGeometry.bSmooth ? 1 : 0 ) );
        Mix( nHash, (uint64_t)( oGeometry.bDoubleSided ? 1 : 0 ) );
        Mix( nHash, (uint64_t)(int64_t)oGeometry.oUVMap.nType );
        Mix( nHash, (uint64_t)(int64_t)oGeometry.oUVMap.nAxis );
        Mix( nHash, (uint64_t)( oGeometry.oUVMap.bCenterGiven ? 1 : 0 ) );
        MixVector( nHash, oGeometry.oUVMap.oCenter );
        Mix( nHash, Bits( oGeometry.oUVMap.anScale[ 0 ] ) );
        Mix( nHash, Bits( oGeometry.oUVMap.anScale[ 1 ] ) );
        return nHash;
    }

    inline BOOL SameVector( const CVector &oA, const CVector &oB )
    {
        CVector &oFirst = const_cast<CVector &>( oA );
        CVector &oSecond = const_cast<CVector &>( oB );

        return ( Bits( oFirst.GetX() ) == Bits( oSecond.GetX() ) && Bits( oFirst.GetY() ) == Bits( oSecond.GetY() ) &&
                 Bits( oFirst.GetZ() ) == Bits( oSecond.GetZ() ) ) ? TRUE : FALSE;
    }

    BOOL SameVectors( const std::vector<CVector> &aoA, const std::vector<CVector> &aoB )
    {
        if ( aoA.size() != aoB.size() )
            return FALSE;
        for ( size_t nIndex = 0; nIndex < aoA.size(); nIndex++ )
        {
            if ( ! SameVector( aoA[ nIndex ], aoB[ nIndex ] ) )
                return FALSE;
        }
        return TRUE;
    }

    BOOL SameInputs( const CMesh::SGeometry &oA, const CMesh::SGeometry &oB )
    {
        if ( oA.bSmooth != oB.bSmooth || oA.bDoubleSided != oB.bDoubleSided || oA.anFaces != oB.anFaces ||
             oA.abGivenHasColor != oB.abGivenHasColor || oA.anUVs.size() != oB.anUVs.size() ||
             oA.aoGivenColors.size() != oB.aoGivenColors.size() ||
             oA.oUVMap.nType != oB.oUVMap.nType || oA.oUVMap.nAxis != oB.oUVMap.nAxis ||
             oA.oUVMap.bCenterGiven != oB.oUVMap.bCenterGiven || ! SameVector( oA.oUVMap.oCenter, oB.oUVMap.oCenter ) ||
             Bits( oA.oUVMap.anScale[ 0 ] ) != Bits( oB.oUVMap.anScale[ 0 ] ) ||
             Bits( oA.oUVMap.anScale[ 1 ] ) != Bits( oB.oUVMap.anScale[ 1 ] ) ||
             ! SameVectors( oA.aoVertices, oB.aoVertices ) || ! SameVectors( oA.aoGivenNormals, oB.aoGivenNormals ) )
            return FALSE;
        for ( size_t nIndex = 0; nIndex < oA.anUVs.size(); nIndex++ )
        {
            if ( Bits( oA.anUVs[ nIndex ] ) != Bits( oB.anUVs[ nIndex ] ) )
                return FALSE;
        }
        for ( size_t nIndex = 0; nIndex < oA.aoGivenColors.size(); nIndex++ )
        {
            CColor &oFirst = const_cast<CColor &>( oA.aoGivenColors[ nIndex ] );
            CColor &oSecond = const_cast<CColor &>( oB.aoGivenColors[ nIndex ] );

            if ( Bits( oFirst.GetR() ) != Bits( oSecond.GetR() ) || Bits( oFirst.GetG() ) != Bits( oSecond.GetG() ) ||
                 Bits( oFirst.GetB() ) != Bits( oSecond.GetB() ) )
                return FALSE;
        }
        return TRUE;
    }

        // The geometries alive, by the hash of what they were made from.
        // Entries are weak: a geometry goes when the last mesh using it
        // does. The lock is held while a geometry is built, so threads
        // loading copies of a scene at once build each shape once.
    std::mutex                                                          g_oGeometryLock;
    std::multimap<uint64_t, std::weak_ptr<const CMesh::SGeometry>>      g_oGeometries;

    const std::vector<CVector> g_aoNoVertices;
}

CMesh::CMesh( void ) :
    m_oScale( 1.0, 1.0, 1.0 ),
    m_nFitSize( 0.0 ),
    m_oRotation( 0.0, 0.0, 0.0 ),
    m_nAlign( ALIGN_NONE ),
    m_oTranslation( 0.0, 0.0, 0.0 ),
    m_bDoubleSided( FALSE ),
    m_bSmooth( FALSE ),
    m_nHitFace( -1 )
{
    m_oUVMap.nType = UVMAP_NONE;
    m_oUVMap.nAxis = 1;
    m_oUVMap.bCenterGiven = FALSE;
    m_oUVMap.anScale[ 0 ] = m_oUVMap.anScale[ 1 ] = 1.0;
    m_oMaterials.Add( new CDiffuseMaterial, 1.0 );
    m_oColor.Set( 0.5, 0.5, 0.5 );
}

CMesh::~CMesh( void )
{
}

int CMesh::SharedGeometries( void )
{
    std::lock_guard<std::mutex> oLock( g_oGeometryLock );
    int                         nCount = 0;

    for ( auto itEntry = g_oGeometries.begin(); itEntry != g_oGeometries.end(); ++itEntry )
    {
        if ( ! itEntry->second.expired() )
            nCount++;
    }
    return nCount;
}

const std::vector<CVector> &CMesh::GetVertices( void ) const
{
    return m_pGeometry ? m_pGeometry->aoVertices : g_aoNoVertices;
}

CTriangle *CMesh::MakeFace( int nFace )
{
    CMeshFace  *pFace = new CMeshFace( this, nFace, *m_pGeometry, &m_oMaterials );

    pFace->SetBump( m_pBump );
    if ( m_pTextureFrame )
        pFace->SetTextureFrame( m_pTextureFrame );
    return pFace;
}

BOOL CMesh::FaceColor( int nFace, CColor &oColor )
{
    std::unique_ptr<CTriangle>  pFace( MakeFace( nFace ) );
    CVector                     oAny;

    return pFace->SurfaceColor( oAny, oColor );
}

BOOL CMesh::FaceCoordsAt( int nFace, CVector oPoint, FLT &nU, FLT &nV )
{
    std::unique_ptr<CTriangle> pFace( MakeFace( nFace ) );

    return pFace->SurfaceCoords( oPoint, nU, nV );
}

CVector CMesh::FaceNormalAt( int nFace, CVector oPoint )
{
    std::unique_ptr<CTriangle> pFace( MakeFace( nFace ) );

    return pFace->NormalAt( oPoint );
}

BOOL CMesh::FaceHasBump( int nFace )
{
    std::unique_ptr<CTriangle> pFace( MakeFace( nFace ) );

    return pFace->GetBump() ? TRUE : FALSE;
}

CObjects *CMesh::HitPart( BOOL &bFlip )
{
    bFlip = FALSE;
    if ( m_nHitFace < 0 || ! m_pGeometry )
        return this;

        // The same face always gets the same stand-in, so a csg comparing
        // the parts it crossed sees one surface.
    for ( size_t nIndex = 0; nIndex < m_anLeafFace.size(); nIndex++ )
    {
        if ( m_anLeafFace[ nIndex ] == m_nHitFace )
            return m_apLeafFaces[ nIndex ].get();
    }
    m_apLeafFaces.push_back( std::unique_ptr<CTriangle>( MakeFace( m_nHitFace ) ) );
    m_anLeafFace.push_back( m_nHitFace );
    return m_apLeafFaces.back().get();
}

BOOL CMesh::SetGeometry( const std::vector<CVector> &aoVertices, const std::vector<int> &anFaces, BOOL bSmooth, BOOL bDoubleSided,
                         const std::vector<CVector> &aoNormals, const std::vector<FLT> &anUVs,
                         const std::vector<CColor> &aoFaceColors, const std::vector<unsigned char> &abHasColor )
{
    if ( ! aoNormals.empty() && aoNormals.size() != aoVertices.size() )
        return FALSE;
    if ( ! anUVs.empty() && anUVs.size() != aoVertices.size() * 2 )
        return FALSE;
    if ( ! aoFaceColors.empty() && ( aoFaceColors.size() != anFaces.size() / 3 || abHasColor.size() != aoFaceColors.size() ) )
        return FALSE;

    for ( size_t nIndex = 0; nIndex < anFaces.size(); nIndex++ )
    {
        if ( anFaces[ nIndex ] < 0 || anFaces[ nIndex ] >= (int)aoVertices.size() )
            return FALSE;
    }

    std::shared_ptr<SGeometry> pGeometry = std::make_shared<SGeometry>();

    pGeometry->aoVertices = aoVertices;
    pGeometry->anFaces = anFaces;
    pGeometry->aoGivenNormals = aoNormals;
    pGeometry->anUVs = anUVs;
    pGeometry->aoGivenColors = aoFaceColors;
    pGeometry->abGivenHasColor = abHasColor;
    pGeometry->bSmooth = bSmooth;
    pGeometry->bDoubleSided = bDoubleSided;
    pGeometry->oUVMap = m_oUVMap;

    m_bSmooth = bSmooth;
    m_bDoubleSided = bDoubleSided;
    m_nHitFace = -1;
    m_apLeafFaces.clear();
    m_anLeafFace.clear();
    m_pGeometry.reset();

    uint64_t                    nHash = HashInputs( *pGeometry );
    std::lock_guard<std::mutex> oLock( g_oGeometryLock );

    for ( auto itEntry = g_oGeometries.lower_bound( nHash ); itEntry != g_oGeometries.end() && itEntry->first == nHash; ++itEntry )
    {
        std::shared_ptr<const SGeometry> pShared = itEntry->second.lock();

        if ( pShared && SameInputs( *pShared, *pGeometry ) )
        {
            m_pGeometry = pShared;
            return TRUE;
        }
    }

        // Not built yet: build it, and forget any that have gone.
    Build( *pGeometry );
    for ( auto itEntry = g_oGeometries.begin(); itEntry != g_oGeometries.end(); )
    {
        if ( itEntry->second.expired() )
            itEntry = g_oGeometries.erase( itEntry );
        else
            ++itEntry;
    }
    m_pGeometry = pGeometry;
    g_oGeometries.insert( std::make_pair( nHash, std::weak_ptr<const SGeometry>( m_pGeometry ) ) );
    return TRUE;
}

void CMesh::Build( SGeometry &oGeometry )
{
        // Each vertex's normal is the sum of the normals of the faces around
        // it, then made a unit vector.
    std::vector<CVector> aoSums( oGeometry.aoVertices.size(), CVector( 0.0, 0.0, 0.0 ) );
    std::vector<size_t>  anKeptFrom;        // where each kept face's indices start

    for ( size_t nFace = 0; nFace + 2 < oGeometry.anFaces.size(); nFace += 3 )
    {
        CVector oNormal = CTriangle::FaceNormal( oGeometry.aoVertices[ (size_t)oGeometry.anFaces[ nFace ] ],
                                                 oGeometry.aoVertices[ (size_t)oGeometry.anFaces[ nFace + 1 ] ],
                                                 oGeometry.aoVertices[ (size_t)oGeometry.anFaces[ nFace + 2 ] ] );

        if ( oNormal.Magnitude() == 0.0 )
            continue;       // its corners are in a line: it has no surface
        if ( ! oGeometry.aoGivenColors.empty() )
        {
            oGeometry.aoFaceColors.push_back( oGeometry.aoGivenColors[ nFace / 3 ] );
            oGeometry.abFaceHasColor.push_back( oGeometry.abGivenHasColor[ nFace / 3 ] );
        }
        oGeometry.aoFaceNormals.push_back( oNormal );
        for ( int nCorner = 0; nCorner < 3; nCorner++ )
            oGeometry.anKept.push_back( oGeometry.anFaces[ nFace + (size_t)nCorner ] );
        anKeptFrom.push_back( nFace );
        for ( int nCorner = 0; nCorner < 3; nCorner++ )
        {
            CVector &oSum = aoSums[ (size_t)oGeometry.anFaces[ nFace + (size_t)nCorner ] ];
            oSum = oSum + oNormal;
        }
    }
    oGeometry.aoFaceNormals.shrink_to_fit();
    oGeometry.anKept.shrink_to_fit();

    for ( size_t nIndex = 0; nIndex < aoSums.size(); nIndex++ )
        aoSums[ nIndex ].Normalize();

    if ( oGeometry.bSmooth )
        oGeometry.aoVertexNormals = oGeometry.aoGivenNormals.empty() ? aoSums : oGeometry.aoGivenNormals;

    if ( ! oGeometry.anUVs.empty() )
    {
        oGeometry.anCornerUVs.resize( anKeptFrom.size() * 6 );
        for ( size_t nKept = 0; nKept < anKeptFrom.size(); nKept++ )
        {
            for ( int nCorner = 0; nCorner < 3; nCorner++ )
            {
                size_t nVertex = (size_t)oGeometry.anFaces[ anKeptFrom[ nKept ] + (size_t)nCorner ];

                oGeometry.anCornerUVs[ ( nKept * 6 ) + ( (size_t)nCorner * 2 ) ] = oGeometry.anUVs[ nVertex * 2 ];
                oGeometry.anCornerUVs[ ( nKept * 6 ) + ( (size_t)nCorner * 2 ) + 1 ] = oGeometry.anUVs[ ( nVertex * 2 ) + 1 ];
            }
        }
    }

        // The bounding box, a little larger than the vertices so rays that
        // graze an edge are not lost to rounding.
    if ( ! oGeometry.aoVertices.empty() )
    {
        FLT anMin[ 3 ] = { 1e300, 1e300, 1e300 };
        FLT anMax[ 3 ] = { -1e300, -1e300, -1e300 };

        for ( size_t nIndex = 0; nIndex < oGeometry.aoVertices.size(); nIndex++ )
        {
            CVector &oVertex = oGeometry.aoVertices[ nIndex ];
            FLT      anValue[ 3 ] = { oVertex.GetX(), oVertex.GetY(), oVertex.GetZ() };

            for ( int nAxis = 0; nAxis < 3; nAxis++ )
            {
                if ( anValue[ nAxis ] < anMin[ nAxis ] ) anMin[ nAxis ] = anValue[ nAxis ];
                if ( anValue[ nAxis ] > anMax[ nAxis ] ) anMax[ nAxis ] = anValue[ nAxis ];
            }
        }
        FLT nMargin = 1e-6 * ( 1.0 + ABS( anMax[ 0 ] - anMin[ 0 ] ) + ABS( anMax[ 1 ] - anMin[ 1 ] ) + ABS( anMax[ 2 ] - anMin[ 2 ] ) );
        oGeometry.oMinimum.Set( anMin[ 0 ] - nMargin, anMin[ 1 ] - nMargin, anMin[ 2 ] - nMargin );
        oGeometry.oMaximum.Set( anMax[ 0 ] + nMargin, anMax[ 1 ] + nMargin, anMax[ 2 ] + nMargin );
        BuildTree( oGeometry, nMargin );
    }

    if ( oGeometry.anUVs.empty() && oGeometry.oUVMap.nType != UVMAP_NONE )
        MapUVs( oGeometry );
}

void CMesh::MapUVs( SGeometry &oGeometry )
{
        // For each axis, the two across it that u and v follow: along y, x
        // and z (a floor); along z, x and y (a wall); along x, z and y. Angles
        // about an axis run from the first toward the second, as on a sphere.
    static const int anAcross[ 3 ][ 2 ] = { { 2, 1 }, { 0, 2 }, { 0, 1 } };
    int     nA = oGeometry.oUVMap.nAxis;
    int     nB = anAcross[ nA ][ 0 ];
    int     nC = anAcross[ nA ][ 1 ];
    FLT     anMin[ 3 ] = { 1e300, 1e300, 1e300 };
    FLT     anMax[ 3 ] = { -1e300, -1e300, -1e300 };
    FLT     anCenter[ 3 ];
    FLT     anSize[ 3 ];

        // The vertices' own bounds (not the padded box rays are tested with).
    for ( size_t nIndex = 0; nIndex < oGeometry.aoVertices.size(); nIndex++ )
    {
        FLT anValue[ 3 ] = { oGeometry.aoVertices[ nIndex ].GetX(), oGeometry.aoVertices[ nIndex ].GetY(), oGeometry.aoVertices[ nIndex ].GetZ() };

        for ( int nAxis = 0; nAxis < 3; nAxis++ )
        {
            if ( anValue[ nAxis ] < anMin[ nAxis ] ) anMin[ nAxis ] = anValue[ nAxis ];
            if ( anValue[ nAxis ] > anMax[ nAxis ] ) anMax[ nAxis ] = anValue[ nAxis ];
        }
    }

    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        anSize[ nAxis ] = ( anMax[ nAxis ] > anMin[ nAxis ] ) ? anMax[ nAxis ] - anMin[ nAxis ] : 1.0;
        anCenter[ nAxis ] = ( anMin[ nAxis ] + anMax[ nAxis ] ) / 2.0;
    }
    if ( oGeometry.oUVMap.bCenterGiven )
    {
        anCenter[ 0 ] = oGeometry.oUVMap.oCenter.GetX();
        anCenter[ 1 ] = oGeometry.oUVMap.oCenter.GetY();
        anCenter[ 2 ] = oGeometry.oUVMap.oCenter.GetZ();
    }

    oGeometry.anCornerUVs.assign( (size_t)oGeometry.FaceCount() * 6, 0.0 );
    for ( size_t nFace = 0; nFace < (size_t)oGeometry.FaceCount(); nFace++ )
    {
        FLT         anUVs[ 6 ];
        BOOL        abPole[ 3 ] = { FALSE, FALSE, FALSE };
        int         nProject = nA;      // for box: the axis the face looks along most

        if ( oGeometry.oUVMap.nType == UVMAP_BOX )
        {
            CVector &oNormal = oGeometry.aoFaceNormals[ nFace ];
            FLT      anNormal[ 3 ] = { ABS( oNormal.GetX() ), ABS( oNormal.GetY() ), ABS( oNormal.GetZ() ) };

            nProject = ( anNormal[ 0 ] >= anNormal[ 1 ] && anNormal[ 0 ] >= anNormal[ 2 ] ) ? 0 : ( anNormal[ 1 ] >= anNormal[ 2 ] ) ? 1 : 2;
        }

        for ( int nCorner = 0; nCorner < 3; nCorner++ )
        {
            CVector &oCorner = oGeometry.aoVertices[ (size_t)oGeometry.anKept[ ( nFace * 3 ) + (size_t)nCorner ] ];
            FLT      anPoint[ 3 ] = { oCorner.GetX() - anCenter[ 0 ], oCorner.GetY() - anCenter[ 1 ], oCorner.GetZ() - anCenter[ 2 ] };
            FLT      nU = 0.0;
            FLT      nV = 0.0;

            switch ( oGeometry.oUVMap.nType )
            {
                case UVMAP_PLANAR:
                case UVMAP_BOX:
                {
                        // Projected along the axis onto the bounding box's
                        // side, 0 to 1 across it.
                    int nPB = anAcross[ nProject ][ 0 ];
                    int nPC = anAcross[ nProject ][ 1 ];

                    nU = ( anPoint[ nPB ] / anSize[ nPB ] ) + 0.5;
                    nV = ( anPoint[ nPC ] / anSize[ nPC ] ) + 0.5;
                    break;
                }

                case UVMAP_CYLINDRICAL:
                case UVMAP_SPHERICAL:
                {
                    FLT nAcross = SQRT( ( anPoint[ nB ] * anPoint[ nB ] ) + ( anPoint[ nC ] * anPoint[ nC ] ) );

                    abPole[ nCorner ] = ( nAcross < 1e-12 );
                    nU = 0.5 + ( (FLT)atan2( (double)anPoint[ nC ], (double)anPoint[ nB ] ) / ( 2.0 * PI ) );
                    if ( oGeometry.oUVMap.nType == UVMAP_CYLINDRICAL )
                        nV = ( anPoint[ nA ] / anSize[ nA ] ) + 0.5;
                    else
                    {
                        FLT nLength = SQRT( ( nAcross * nAcross ) + ( anPoint[ nA ] * anPoint[ nA ] ) );
                        nV = ( nLength > 0.0 ) ? 0.5 + ( ASIN( anPoint[ nA ] / nLength ) / PI ) : 0.5;
                    }
                    break;
                }
            }
            anUVs[ nCorner * 2 ] = nU;
            anUVs[ ( nCorner * 2 ) + 1 ] = nV;
        }

        if ( oGeometry.oUVMap.nType == UVMAP_CYLINDRICAL || oGeometry.oUVMap.nType == UVMAP_SPHERICAL )
        {
                // A corner on the axis has no angle of its own: give it the
                // others' average.
            FLT nSum = 0.0;
            int nCount = 0;

            for ( int nCorner = 0; nCorner < 3; nCorner++ )
            {
                if ( ! abPole[ nCorner ] )
                {
                    nSum += anUVs[ nCorner * 2 ];
                    nCount++;
                }
            }
                // A face across the seam (u wrapping from 1 to 0) is unwrapped
                // so its u values run on past 1 instead of back across the
                // whole texture.
            FLT nLow = 2.0, nHigh = -1.0;

            for ( int nCorner = 0; nCorner < 3; nCorner++ )
            {
                if ( abPole[ nCorner ] )
                    continue;
                if ( anUVs[ nCorner * 2 ] < nLow ) nLow = anUVs[ nCorner * 2 ];
                if ( anUVs[ nCorner * 2 ] > nHigh ) nHigh = anUVs[ nCorner * 2 ];
            }
            if ( nHigh - nLow > 0.5 )
            {
                nSum = 0.0;
                for ( int nCorner = 0; nCorner < 3; nCorner++ )
                {
                    if ( abPole[ nCorner ] )
                        continue;
                    if ( anUVs[ nCorner * 2 ] < 0.5 )
                        anUVs[ nCorner * 2 ] += 1.0;
                    nSum += anUVs[ nCorner * 2 ];
                }
            }
            for ( int nCorner = 0; nCorner < 3; nCorner++ )
            {
                if ( abPole[ nCorner ] && nCount > 0 )
                    anUVs[ nCorner * 2 ] = nSum / nCount;
            }
        }

        for ( int nCorner = 0; nCorner < 3; nCorner++ )
        {
            anUVs[ nCorner * 2 ] *= oGeometry.oUVMap.anScale[ 0 ];
            anUVs[ ( nCorner * 2 ) + 1 ] *= oGeometry.oUVMap.anScale[ 1 ];
        }
        for ( int nIndex = 0; nIndex < 6; nIndex++ )
            oGeometry.anCornerUVs[ ( nFace * 6 ) + (size_t)nIndex ] = anUVs[ nIndex ];
    }
}

    // Leaves hold at least this many faces before a split is tried, and
    // a leaf no split helps can hold four times as many.
#define LEAF_FACES  4
    // The deepest the surface area heuristic builds; below it the tree splits
    // at the median. Traversal keeps a stack of TREE_STACK nodes, which this
    // leaves room for (median splits add at most 32 levels more).
#define MAXIMUM_SAH_DEPTH   64
#define TREE_STACK          256

void CMesh::BuildTree( SGeometry &oGeometry, FLT nMargin )
{
    std::vector<FLT> anCenters( (size_t)oGeometry.FaceCount() * 3 );

    oGeometry.aoNodes.clear();
    oGeometry.anOrder.resize( (size_t)oGeometry.FaceCount() );
    for ( size_t nFace = 0; nFace < (size_t)oGeometry.FaceCount(); nFace++ )
    {
        oGeometry.anOrder[ nFace ] = (int)nFace;
        for ( int nCorner = 0; nCorner < 3; nCorner++ )
        {
            CVector &oCorner = oGeometry.aoVertices[ (size_t)oGeometry.anKept[ ( (size_t)nFace * 3 ) + (size_t)nCorner ] ];

            anCenters[ ( nFace * 3 ) ] += oCorner.GetX() / 3.0;
            anCenters[ ( nFace * 3 ) + 1 ] += oCorner.GetY() / 3.0;
            anCenters[ ( nFace * 3 ) + 2 ] += oCorner.GetZ() / 3.0;
        }
    }
    if ( ! ( oGeometry.FaceCount() == 0 ) )
    {
        oGeometry.aoNodes.reserve( ( (size_t)oGeometry.FaceCount() * 2 ) / LEAF_FACES + 2 );
        BuildNode( oGeometry, 0, (int)(size_t)oGeometry.FaceCount(), anCenters, nMargin, 0 );
    }
}

int CMesh::BuildNode( SGeometry &oGeometry, int nFirst, int nCount, const std::vector<FLT> &anCenters, FLT nMargin, int nDepth )
{
    int     nNode = (int)oGeometry.aoNodes.size();
    SNode   oNode;
    FLT     anLow[ 3 ] = { 1e300, 1e300, 1e300 };     // the faces' centers' extent
    FLT     anHigh[ 3 ] = { -1e300, -1e300, -1e300 };

    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        oNode.anMin[ nAxis ] = 1e300;
        oNode.anMax[ nAxis ] = -1e300;
    }
    for ( int nIndex = nFirst; nIndex < nFirst + nCount; nIndex++ )
    {
        int nFace = oGeometry.anOrder[ (size_t)nIndex ];

        for ( int nCorner = 0; nCorner < 3; nCorner++ )
        {
            CVector &oCorner = oGeometry.aoVertices[ (size_t)oGeometry.anKept[ ( (size_t)nFace * 3 ) + (size_t)nCorner ] ];
            FLT      anValue[ 3 ] = { oCorner.GetX(), oCorner.GetY(), oCorner.GetZ() };

            for ( int nAxis = 0; nAxis < 3; nAxis++ )
            {
                oNode.anMin[ nAxis ] = (std::min)( oNode.anMin[ nAxis ], anValue[ nAxis ] );
                oNode.anMax[ nAxis ] = (std::max)( oNode.anMax[ nAxis ], anValue[ nAxis ] );
            }
        }
        for ( int nAxis = 0; nAxis < 3; nAxis++ )
        {
            anLow[ nAxis ] = (std::min)( anLow[ nAxis ], anCenters[ ( (size_t)nFace * 3 ) + nAxis ] );
            anHigh[ nAxis ] = (std::max)( anHigh[ nAxis ], anCenters[ ( (size_t)nFace * 3 ) + nAxis ] );
        }
    }
        // A little larger, as the mesh's own box is: a face's hit test
        // allows a point a hair outside its edges.
    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        oNode.anMin[ nAxis ] -= nMargin;
        oNode.anMax[ nAxis ] += nMargin;
    }
    oNode.nFirst = nFirst;
    oNode.nCount = nCount;
    oNode.nLeft = oNode.nRight = -1;
    oGeometry.aoNodes.push_back( oNode );

    if ( nCount <= LEAF_FACES )
        return nNode;

        // Split where the surface area heuristic says a ray will test the
        // fewest faces: the faces' centers are sorted into bins along each
        // axis, and each boundary between bins is tried, costing each side
        // its faces times its box's area (the chance a ray through this
        // box passes through that one).
    const int   BINS = 16;
    int         nBestAxis = -1;
    int         nBestBin = -1;
    FLT         nBestCost = 1e300;
    FLT         anSize[ 3 ] = { oNode.anMax[ 0 ] - oNode.anMin[ 0 ], oNode.anMax[ 1 ] - oNode.anMin[ 1 ], oNode.anMax[ 2 ] - oNode.anMin[ 2 ] };
    FLT         nArea = ( anSize[ 0 ] * anSize[ 1 ] ) + ( anSize[ 1 ] * anSize[ 2 ] ) + ( anSize[ 2 ] * anSize[ 0 ] );

        // Past a depth the traversal stack allows, split at the median, which
        // halves the faces and so ends within log2 of them more levels.
    for ( int nAxis = 0; nAxis < 3 && nArea > 0.0 && nDepth < MAXIMUM_SAH_DEPTH; nAxis++ )
    {
        FLT nExtent = anHigh[ nAxis ] - anLow[ nAxis ];

        if ( ! ( nExtent > 0.0 ) )
            continue;

        int nBinCount[ BINS ] = { 0 };
        FLT anBinMin[ BINS ][ 3 ];
        FLT anBinMax[ BINS ][ 3 ];

        for ( int nBin = 0; nBin < BINS; nBin++ )
        {
            for ( int nEach = 0; nEach < 3; nEach++ )
            {
                anBinMin[ nBin ][ nEach ] = 1e300;
                anBinMax[ nBin ][ nEach ] = -1e300;
            }
        }
        for ( int nIndex = nFirst; nIndex < nFirst + nCount; nIndex++ )
        {
            int nFace = oGeometry.anOrder[ (size_t)nIndex ];
            int nBin = (int)( ( anCenters[ ( (size_t)nFace * 3 ) + nAxis ] - anLow[ nAxis ] ) * BINS / nExtent );

            nBin = (std::min)( (std::max)( nBin, 0 ), BINS - 1 );
            nBinCount[ nBin ]++;
            for ( int nCorner = 0; nCorner < 3; nCorner++ )
            {
                CVector &oCorner = oGeometry.aoVertices[ (size_t)oGeometry.anKept[ ( (size_t)nFace * 3 ) + (size_t)nCorner ] ];
                FLT      anValue[ 3 ] = { oCorner.GetX(), oCorner.GetY(), oCorner.GetZ() };

                for ( int nEach = 0; nEach < 3; nEach++ )
                {
                    anBinMin[ nBin ][ nEach ] = (std::min)( anBinMin[ nBin ][ nEach ], anValue[ nEach ] );
                    anBinMax[ nBin ][ nEach ] = (std::max)( anBinMax[ nBin ][ nEach ], anValue[ nEach ] );
                }
            }
        }

            // The area and count left of each boundary, then right of it.
        FLT anLeftArea[ BINS ];
        int anLeftCount[ BINS ];
        FLT anMin[ 3 ] = { 1e300, 1e300, 1e300 };
        FLT anMax[ 3 ] = { -1e300, -1e300, -1e300 };
        int nSum = 0;

        for ( int nBin = 0; nBin < BINS - 1; nBin++ )
        {
            nSum += nBinCount[ nBin ];
            for ( int nEach = 0; nEach < 3; nEach++ )
            {
                anMin[ nEach ] = (std::min)( anMin[ nEach ], anBinMin[ nBin ][ nEach ] );
                anMax[ nEach ] = (std::max)( anMax[ nEach ], anBinMax[ nBin ][ nEach ] );
            }
            anLeftCount[ nBin ] = nSum;
            anLeftArea[ nBin ] = ( nSum == 0 ) ? 0.0 :
                                 ( ( anMax[ 0 ] - anMin[ 0 ] ) * ( anMax[ 1 ] - anMin[ 1 ] ) ) +
                                 ( ( anMax[ 1 ] - anMin[ 1 ] ) * ( anMax[ 2 ] - anMin[ 2 ] ) ) +
                                 ( ( anMax[ 2 ] - anMin[ 2 ] ) * ( anMax[ 0 ] - anMin[ 0 ] ) );
        }
        for ( int nEach = 0; nEach < 3; nEach++ )
        {
            anMin[ nEach ] = 1e300;
            anMax[ nEach ] = -1e300;
        }
        nSum = 0;
        for ( int nBin = BINS - 1; nBin > 0; nBin-- )
        {
            nSum += nBinCount[ nBin ];
            for ( int nEach = 0; nEach < 3; nEach++ )
            {
                anMin[ nEach ] = (std::min)( anMin[ nEach ], anBinMin[ nBin ][ nEach ] );
                anMax[ nEach ] = (std::max)( anMax[ nEach ], anBinMax[ nBin ][ nEach ] );
            }
            if ( nSum == 0 || anLeftCount[ nBin - 1 ] == 0 )
                continue;

            FLT nRightArea = ( ( anMax[ 0 ] - anMin[ 0 ] ) * ( anMax[ 1 ] - anMin[ 1 ] ) ) +
                             ( ( anMax[ 1 ] - anMin[ 1 ] ) * ( anMax[ 2 ] - anMin[ 2 ] ) ) +
                             ( ( anMax[ 2 ] - anMin[ 2 ] ) * ( anMax[ 0 ] - anMin[ 0 ] ) );
            FLT nCost = 1.0 + ( ( ( anLeftArea[ nBin - 1 ] * anLeftCount[ nBin - 1 ] ) + ( nRightArea * nSum ) ) / nArea );

            if ( nCost < nBestCost )
            {
                nBestCost = nCost;
                nBestAxis = nAxis;
                nBestBin = nBin - 1;
            }
        }
    }

        // A small node no split helps stays a leaf.
    if ( nBestAxis >= 0 && nBestCost >= (FLT)nCount && nCount <= 4 * LEAF_FACES )
        return nNode;

    if ( nBestAxis >= 0 )
    {
        FLT  nLow = anLow[ nBestAxis ];
        FLT  nExtent = anHigh[ nBestAxis ] - nLow;
        auto itMiddle = std::partition( oGeometry.anOrder.begin() + nFirst, oGeometry.anOrder.begin() + nFirst + nCount,
                                        [ & ]( int nFace )
                                        {
                                            int nBin = (int)( ( anCenters[ ( (size_t)nFace * 3 ) + nBestAxis ] - nLow ) * BINS / nExtent );

                                            return (std::min)( (std::max)( nBin, 0 ), BINS - 1 ) <= nBestBin;
                                        } );
        int  nLeftCount = (int)( itMiddle - ( oGeometry.anOrder.begin() + nFirst ) );

        if ( nLeftCount > 0 && nLeftCount < nCount )
        {
            int nLeft = BuildNode( oGeometry, nFirst, nLeftCount, anCenters, nMargin, nDepth + 1 );
            int nRight = BuildNode( oGeometry, nFirst + nLeftCount, nCount - nLeftCount, anCenters, nMargin, nDepth + 1 );

            oGeometry.aoNodes[ (size_t)nNode ].nLeft = nLeft;
            oGeometry.aoNodes[ (size_t)nNode ].nRight = nRight;
            oGeometry.aoNodes[ (size_t)nNode ].nCount = 0;
            return nNode;
        }
    }

        // Otherwise (every center in one place) split at the median center
        // along the axis the centers spread furthest on.
    int nSplitAxis = 0;

    for ( int nAxis = 1; nAxis < 3; nAxis++ )
    {
        if ( anHigh[ nAxis ] - anLow[ nAxis ] > anHigh[ nSplitAxis ] - anLow[ nSplitAxis ] )
            nSplitAxis = nAxis;
    }

    int nHalf = nCount / 2;

    std::nth_element( oGeometry.anOrder.begin() + nFirst, oGeometry.anOrder.begin() + nFirst + nHalf, oGeometry.anOrder.begin() + nFirst + nCount,
                      [ & ]( int nA, int nB )
                      {
                          FLT nCenterA = anCenters[ ( (size_t)nA * 3 ) + nSplitAxis ];
                          FLT nCenterB = anCenters[ ( (size_t)nB * 3 ) + nSplitAxis ];

                          return ( nCenterA != nCenterB ) ? ( nCenterA < nCenterB ) : ( nA < nB );
                      } );

    int nLeft = BuildNode( oGeometry, nFirst, nHalf, anCenters, nMargin, nDepth + 1 );
    int nRight = BuildNode( oGeometry, nFirst + nHalf, nCount - nHalf, anCenters, nMargin, nDepth + 1 );

    oGeometry.aoNodes[ (size_t)nNode ].nLeft = nLeft;
    oGeometry.aoNodes[ (size_t)nNode ].nRight = nRight;
    oGeometry.aoNodes[ (size_t)nNode ].nCount = 0;
    return nNode;
}

BOOL CMesh::HitsNode( const SNode &oNode, const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLimit )
{
    FLT nEntry;

    return NodeEntry( oNode, anOrigin, anDirection, nLimit, nEntry );
}

BOOL CMesh::NodeEntry( const SNode &oNode, const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLimit, FLT &nEntry )
{
    FLT nNear = 0.0;
    FLT nFar = nLimit;

    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        if ( anDirection[ nAxis ] == 0.0 )
        {
            if ( anOrigin[ nAxis ] < oNode.anMin[ nAxis ] || anOrigin[ nAxis ] > oNode.anMax[ nAxis ] )
                return FALSE;
            continue;
        }

        FLT nT1 = ( oNode.anMin[ nAxis ] - anOrigin[ nAxis ] ) / anDirection[ nAxis ];
        FLT nT2 = ( oNode.anMax[ nAxis ] - anOrigin[ nAxis ] ) / anDirection[ nAxis ];

        if ( nT1 > nT2 )
            std::swap( nT1, nT2 );
        if ( nT1 > nNear )
            nNear = nT1;
        if ( nT2 < nFar )
            nFar = nT2;
        if ( nNear > nFar )
            return FALSE;
    }
    nEntry = nNear;
    return TRUE;
}

    // The slab test: whether the ray passes through the box before its
    // current closest hit.
BOOL CMesh::HitsBox( CRay &oRay )
{
    if ( ! m_pGeometry || m_pGeometry->FaceCount() == 0 )
        return FALSE;

    CVector oMinimum( m_pGeometry->oMinimum );
    CVector oMaximum( m_pGeometry->oMaximum );
    FLT anOrigin[ 3 ] = { oRay.GetX(), oRay.GetY(), oRay.GetZ() };
    FLT anDirection[ 3 ] = { oRay.GetI(), oRay.GetJ(), oRay.GetK() };
    FLT anMin[ 3 ] = { oMinimum.GetX(), oMinimum.GetY(), oMinimum.GetZ() };
    FLT anMax[ 3 ] = { oMaximum.GetX(), oMaximum.GetY(), oMaximum.GetZ() };
    FLT nNear = 0.0;
    FLT nFar = oRay.GetT();

    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        if ( anDirection[ nAxis ] == 0.0 )
        {
            if ( anOrigin[ nAxis ] < anMin[ nAxis ] || anOrigin[ nAxis ] > anMax[ nAxis ] )
                return FALSE;
            continue;
        }

        FLT nT1 = ( anMin[ nAxis ] - anOrigin[ nAxis ] ) / anDirection[ nAxis ];
        FLT nT2 = ( anMax[ nAxis ] - anOrigin[ nAxis ] ) / anDirection[ nAxis ];

        if ( nT1 > nT2 )
        {
            FLT nSwap = nT1;
            nT1 = nT2;
            nT2 = nSwap;
        }
        if ( nT1 > nNear )
            nNear = nT1;
        if ( nT2 < nFar )
            nFar = nT2;
        if ( nNear > nFar )
            return FALSE;
    }
    return TRUE;
}

    // Whether a ray meets face nFace of a geometry closer than nLimit.
static inline BOOL HitKeptFace( const CMesh::SGeometry &oGeometry, int nFace, const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ],
                                FLT nLimit, FLT &nT, CVector &oPoint )
{
    size_t nAt = (size_t)nFace * 3;

    return CTriangle::HitFace( oGeometry.aoVertices[ (size_t)oGeometry.anKept[ nAt ] ], oGeometry.aoVertices[ (size_t)oGeometry.anKept[ nAt + 1 ] ],
                               oGeometry.aoVertices[ (size_t)oGeometry.anKept[ nAt + 2 ] ], oGeometry.aoFaceNormals[ (size_t)nFace ],
                               oGeometry.bDoubleSided, anOrigin, anDirection, nLimit, nT, oPoint );
}

BOOL CMesh::Intersect( CRay &oRay, CVector &oIntersect )
{
    return Intersect( oRay, oIntersect, NULL );
}

BOOL CMesh::Intersect( CRay &oRay, CVector &oIntersect, CObjects *pIgnore )
{
    CVector oHit;
    BOOL    bHit = FALSE;

    if ( pIgnore == this || ! m_pGeometry || m_pGeometry->aoNodes.empty() || ! HitsBox( oRay ) )
        return FALSE;

    const SGeometry &oGeometry = *m_pGeometry;
        // A ray leaving one of this mesh's faces skips only that face.
    int     nSkip = pIgnore ? pIgnore->MeshFaceOf( this ) : -1;
    FLT     anOrigin[ 3 ] = { oRay.GetX(), oRay.GetY(), oRay.GetZ() };
    FLT     anDirection[ 3 ] = { oRay.GetI(), oRay.GetJ(), oRay.GetK() };
    FLT     nLimit = oRay.GetT();
    FLT     nBest = nLimit;
    int     nBestFace = -1;
    CVector oBestHit;
    int     anStack[ TREE_STACK ];
    FLT     anEntry[ TREE_STACK ];     // where the ray enters each node on the stack
    int     nStack = 0;
    FLT     nEntry;
    FLT     nT;

        // The closest hit, and of faces hit at exactly the same distance the
        // first in the mesh, which is the face testing every face in order
        // finds. Each face is tested against the ray's own limit, so its
        // distance doesn't depend on the order the tree visits faces in;
        // boxes beyond the closest hit so far are skipped. The nearer child
        // of a node is visited first, so the closest hit is found sooner and
        // more boxes are skipped.
    if ( ! NodeEntry( oGeometry.aoNodes[ 0 ], anOrigin, anDirection, nBest, nEntry ) )
        return FALSE;
    anStack[ nStack ] = 0;
    anEntry[ nStack++ ] = nEntry;
    while ( nStack > 0 )
    {
        nStack--;
        if ( anEntry[ nStack ] > nBest )
            continue;

        const SNode &oNode = oGeometry.aoNodes[ (size_t)anStack[ nStack ] ];

        if ( oNode.nCount == 0 )
        {
            FLT  nLeftEntry;
            FLT  nRightEntry;
            BOOL bLeft = NodeEntry( oGeometry.aoNodes[ (size_t)oNode.nLeft ], anOrigin, anDirection, nBest, nLeftEntry );
            BOOL bRight = NodeEntry( oGeometry.aoNodes[ (size_t)oNode.nRight ], anOrigin, anDirection, nBest, nRightEntry );

            if ( bLeft && bRight && nRightEntry < nLeftEntry )
            {
                anStack[ nStack ] = oNode.nLeft;
                anEntry[ nStack++ ] = nLeftEntry;
                anStack[ nStack ] = oNode.nRight;
                anEntry[ nStack++ ] = nRightEntry;
                continue;
            }
            if ( bRight )
            {
                anStack[ nStack ] = oNode.nRight;
                anEntry[ nStack++ ] = nRightEntry;
            }
            if ( bLeft )
            {
                anStack[ nStack ] = oNode.nLeft;
                anEntry[ nStack++ ] = nLeftEntry;
            }
            continue;
        }

        for ( int nIndex = oNode.nFirst; nIndex < oNode.nFirst + oNode.nCount; nIndex++ )
        {
            int nFace = oGeometry.anOrder[ (size_t)nIndex ];

            if ( nFace == nSkip || ! HitKeptFace( oGeometry, nFace, anOrigin, anDirection, nLimit, nT, oHit ) )
                continue;
            if ( nT < nBest || ( nT == nBest && nFace < nBestFace ) )
            {
                nBest = nT;
                nBestFace = nFace;
                oBestHit = oHit;
            }
        }
    }

    if ( nBestFace >= 0 )
    {
        bHit = TRUE;
        oRay.SetT( nBest );
        oIntersect = oBestHit;
        m_nHitFace = nBestFace;
    }
    return bHit;
}

BOOL CMesh::Occludes( CRay &oRay, CObjects *pIgnore )
{
    if ( pIgnore == this || ! m_pGeometry || m_pGeometry->aoNodes.empty() || ! HitsBox( oRay ) )
        return FALSE;

    const SGeometry &oGeometry = *m_pGeometry;
    int     nSkip = pIgnore ? pIgnore->MeshFaceOf( this ) : -1;
    FLT     anOrigin[ 3 ] = { oRay.GetX(), oRay.GetY(), oRay.GetZ() };
    FLT     anDirection[ 3 ] = { oRay.GetI(), oRay.GetJ(), oRay.GetK() };
    FLT     nLimit = oRay.GetT();
    int     anStack[ TREE_STACK ];
    int     nStack = 0;
    CVector oHit;
    FLT     nT;

    anStack[ nStack++ ] = 0;
    while ( nStack > 0 )
    {
        const SNode &oNode = oGeometry.aoNodes[ (size_t)anStack[ --nStack ] ];

        if ( ! HitsNode( oNode, anOrigin, anDirection, nLimit ) )
            continue;
        if ( oNode.nCount == 0 )
        {
            anStack[ nStack++ ] = oNode.nRight;
            anStack[ nStack++ ] = oNode.nLeft;
            continue;
        }
        for ( int nIndex = oNode.nFirst; nIndex < oNode.nFirst + oNode.nCount; nIndex++ )
        {
            int nFace = oGeometry.anOrder[ (size_t)nIndex ];

            if ( nFace != nSkip && HitKeptFace( oGeometry, nFace, anOrigin, anDirection, nLimit, nT, oHit ) )
                return TRUE;
        }
    }
    return FALSE;
}

BOOL CMesh::GetBounds( CVector &oMinimum, CVector &oMaximum )
{
    if ( ! m_pGeometry || m_pGeometry->FaceCount() == 0 )
        return FALSE;
    oMinimum = m_pGeometry->oMinimum;
    oMaximum = m_pGeometry->oMaximum;
    return TRUE;
}

BOOL CMesh::IntersectEachFace( CRay &oRay, CVector &oIntersect )
{
    CVector oHit;
    BOOL    bHit = FALSE;
    FLT     nT;

    if ( ! m_pGeometry )
        return FALSE;

    FLT     anOrigin[ 3 ] = { oRay.GetX(), oRay.GetY(), oRay.GetZ() };
    FLT     anDirection[ 3 ] = { oRay.GetI(), oRay.GetJ(), oRay.GetK() };

        // Each face only reports a hit closer than the ray's closest so far,
        // so the last face that reports one is the closest.
    for ( int nFace = 0; nFace < m_pGeometry->FaceCount(); nFace++ )
    {
        if ( HitKeptFace( *m_pGeometry, nFace, anOrigin, anDirection, oRay.GetT(), nT, oHit ) )
        {
            oRay.SetT( nT );
            bHit = TRUE;
            oIntersect = oHit;
        }
    }
    return bHit;
}

CColor CMesh::Shade( CScene &oScene, CRay &oRay, CVector &oIntersect )
{
        // The scene shades the object whose Intersect hit closest, straight
        // after asking, so m_nHitFace is that hit's face. Shading traces more
        // rays, which may hit this mesh again, so the face shades through a
        // stand-in of its own.
    if ( m_nHitFace < 0 || ! m_pGeometry )
        return CColor( 0, 0, 0 );

    CMeshFace oFace( this, m_nHitFace, *m_pGeometry, &m_oMaterials );

    oFace.SetBump( m_pBump );
    if ( m_pTextureFrame )
        oFace.SetTextureFrame( m_pTextureFrame );
    return oFace.Shade( oScene, oRay, oIntersect );
}

void CMesh::SetBump( const std::shared_ptr<const CBumpMap> &pBump )
{
    m_pBump = pBump;
    m_apLeafFaces.clear();
    m_anLeafFace.clear();
}

void CMesh::SetTextureFrame( const std::shared_ptr<const STextureFrame> &pFrame )
{
    m_pTextureFrame = pFrame;
    m_apLeafFaces.clear();
    m_anLeafFace.clear();
}

#ifdef RAY_GUI
void CMesh::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    HTREEITEM   hMesh;

    hMesh = pTree->AddObject( this, 24, "Mesh", hUnder );
    CObjects::MatToTree( pTree, hMesh );
}
#endif

void CMesh::UVMapToJson( IASLib::CObjectNode &oNode ) const
{
    if ( m_oUVMap.nType != UVMAP_NONE )
    {
        static const char *const astrTypes[] = { "none", "planar", "cylindrical", "spherical", "box" };
        static const char *const astrAxes[] = { "x", "y", "z" };
        IASLib::CObjectNode     *pMap = oNode.putObject( "uvMap" );

        pMap->put( "type", astrTypes[ m_oUVMap.nType ] );
        pMap->put( "axis", astrAxes[ m_oUVMap.nAxis ] );
        if ( m_oUVMap.bCenterGiven )
            RayJson::PutVector( *pMap, "center", m_oUVMap.oCenter );
        pMap->putArray( "scale" )->add( (double)m_oUVMap.anScale[ 0 ] )->add( (double)m_oUVMap.anScale[ 1 ] );
    }
}

void CMesh::ToJson( IASLib::CObjectNode &oNode )
{
    if ( IsFromFile() )
    {
        static const char *const astrAligns[] = { "none", "center", "bottom" };

        oNode.put( "file", RayJson::SceneFileName( m_strFile, m_strFilePath, m_pFileData ) );
        if ( m_pFileData && RayJson::EmbeddingData() )
        {
            std::vector<unsigned char> anData( m_pFileData->begin(), m_pFileData->end() );

            oNode.putBinary( "data", anData );
        }
        if ( m_nFitSize > 0.0 )
            RayJson::PutNumber( oNode, "fitSize", m_nFitSize );
        else
            RayJson::PutVector( oNode, "scale", m_oScale );
        RayJson::PutVector( oNode, "rotation", m_oRotation );
        oNode.put( "align", astrAligns[ m_nAlign ] );
        RayJson::PutVector( oNode, "translation", m_oTranslation );
        RayJson::PutBool( oNode, "smooth", m_bSmooth );
        RayJson::PutBool( oNode, "doubleSided", m_bDoubleSided );
        UVMapToJson( oNode );
        RayJson::PutColor( oNode, "color", m_oColor );
        m_oMaterials.ToJson( *oNode.putArray( "materials" ) );
        BumpToJson( oNode );
        return;
    }

    static const SGeometry  oNone = SGeometry();
    const SGeometry        &oGeometry = m_pGeometry ? *m_pGeometry : oNone;
    IASLib::CArrayNode     *pVertices = oNode.putArray( "vertices" );
    IASLib::CArrayNode     *pFaces = oNode.putArray( "faces" );

    RayJson::PutBool( oNode, "smooth", m_bSmooth );
    RayJson::PutBool( oNode, "doubleSided", m_bDoubleSided );
    if ( ! oGeometry.aoGivenNormals.empty() )
    {
        IASLib::CArrayNode *pNormals = oNode.putArray( "normals" );

        for ( size_t nIndex = 0; nIndex < oGeometry.aoGivenNormals.size(); nIndex++ )
            RayJson::AddVector( *pNormals, oGeometry.aoGivenNormals[ nIndex ] );
    }
    UVMapToJson( oNode );
    if ( ! oGeometry.anUVs.empty() )
    {
        IASLib::CArrayNode *pUVs = oNode.putArray( "uvs" );

        for ( size_t nIndex = 0; nIndex + 1 < oGeometry.anUVs.size(); nIndex += 2 )
            pUVs->addArray()->add( (double)oGeometry.anUVs[ nIndex ] )->add( (double)oGeometry.anUVs[ nIndex + 1 ] );
    }
    RayJson::PutColor( oNode, "color", m_oColor );
    m_oMaterials.ToJson( *oNode.putArray( "materials" ) );
    BumpToJson( oNode );

    for ( size_t nIndex = 0; nIndex < oGeometry.aoVertices.size(); nIndex++ )
        RayJson::AddVector( *pVertices, oGeometry.aoVertices[ nIndex ] );
    for ( size_t nIndex = 0; nIndex + 2 < oGeometry.anFaces.size(); nIndex += 3 )
        pFaces->addArray()->add( oGeometry.anFaces[ nIndex ] )->add( oGeometry.anFaces[ nIndex + 1 ] )->add( oGeometry.anFaces[ nIndex + 2 ] );
}

BOOL CMesh::ReadUVMap( const IASLib::CJsonNode &oNode, BOOL bUnused )
{
    (void)bUnused;
    const IASLib::CJsonNode *pMap = oNode.get( "uvMap" );

    if ( pMap )
    {
        static const char *const astrMapFields[] = { "axis", "center", "scale", NULL };
        static const char *const astrTypes[] = { "planar", "cylindrical", "spherical", "box" };
        RayJson::CWhere          oWhere( "uvMap" );
        std::string              strType;
        std::string              strAxis = "y";
        SUVMap                   oMap = m_oUVMap;

        if ( oNode.get( "uvs" ) )
            return RayJson::Fail( "a mesh takes \"uvs\" or a \"uvMap\", not both" );
        if ( ! RayJson::GetString( *pMap, "type", strType, TRUE ) || ! RayJson::OnlyFields( *pMap, astrMapFields ) ||
             ! RayJson::GetString( *pMap, "axis", strAxis, FALSE ) )
            return FALSE;

        oMap.nType = UVMAP_NONE;
        for ( int nType = 0; nType < 4; nType++ )
        {
            if ( strType == astrTypes[ nType ] )
                oMap.nType = nType + 1;
        }
        if ( oMap.nType == UVMAP_NONE )
            return RayJson::Fail( "unknown uvMap type \"" + strType + "\" (planar, cylindrical, spherical or box)" );
        if ( strAxis != "x" && strAxis != "y" && strAxis != "z" )
            return RayJson::Fail( "axis must be \"x\", \"y\" or \"z\"" );
        oMap.nAxis = strAxis[ 0 ] - 'x';

        oMap.bCenterGiven = ( pMap->get( "center" ) != NULL );
        if ( ! RayJson::GetVector( *pMap, "center", oMap.oCenter, FALSE ) )
            return FALSE;

        const IASLib::CJsonNode *pScale = pMap->get( "scale" );

        if ( pScale )
        {
            RayJson::CWhere oScaleWhere( "scale" );

            if ( ! pScale->isArray() || pScale->size() != 2 || ! pScale->get( 0 )->isNumber() || ! pScale->get( 1 )->isNumber() )
                return RayJson::Fail( "must be an array of two numbers, [u, v]" );
            oMap.anScale[ 0 ] = (FLT)pScale->get( 0 )->doubleValue();
            oMap.anScale[ 1 ] = (FLT)pScale->get( 1 )->doubleValue();
        }
        m_oUVMap = oMap;
    }

    return TRUE;
}

BOOL CMesh::ModelFromJson( const IASLib::CJsonNode &oNode, BOOL bSmooth, BOOL bDoubleSided )
{
    static const char *const astrAligns[] = { "none", "center", "bottom" };
    std::string              strAlign = astrAligns[ m_nAlign ];
    const IASLib::CJsonNode *pScale = oNode.get( "scale" );

    if ( ! RayJson::GetString( oNode, "file", m_strFile, TRUE ) ||
         ! RayJson::GetNumber( oNode, "fitSize", m_nFitSize, FALSE ) ||
         ! RayJson::GetVector( oNode, "rotation", m_oRotation, FALSE ) ||
         ! RayJson::GetString( oNode, "align", strAlign, FALSE ) ||
         ! RayJson::GetVector( oNode, "translation", m_oTranslation, FALSE ) )
        return FALSE;
    if ( m_strFile.empty() )
        return RayJson::Fail( "file can't be empty" );
    if ( oNode.get( "fitSize" ) && ! ( m_nFitSize > 0.0 ) )
        return RayJson::Fail( "fitSize must be more than 0" );
    if ( pScale )
    {
        RayJson::CWhere oWhere( "scale" );

        if ( oNode.get( "fitSize" ) )
            return RayJson::Fail( "a model takes a \"scale\" or a \"fitSize\", not both" );
        if ( pScale->isNumber() )
        {
            FLT nScale = (FLT)pScale->doubleValue();

            m_oScale.Set( nScale, nScale, nScale );
        }
        else if ( ! RayJson::ToVector( *pScale, m_oScale ) )
            return RayJson::Fail( "must be a number, or [x, y, z]" );
        if ( m_oScale.GetX() == 0.0 || m_oScale.GetY() == 0.0 || m_oScale.GetZ() == 0.0 )
            return RayJson::Fail( "can't be 0" );
    }

    m_nAlign = -1;
    for ( int nAlign = 0; nAlign < 3; nAlign++ )
    {
        if ( strAlign == astrAligns[ nAlign ] )
            m_nAlign = nAlign;
    }
    if ( m_nAlign < 0 )
    {
        m_nAlign = ALIGN_NONE;
        return RayJson::Fail( "unknown align \"" + strAlign + "\" (none, center or bottom)" );
    }

    std::string                 strData;
    std::string                 strError;
    const IASLib::CJsonNode    *pData = oNode.get( "data" );

    if ( pData )
    {
            // The file's bytes, carried in a copy of the scene sent to a task.
        std::vector<unsigned char> anData;

        if ( ! pData->binaryValue( anData ) )
            return RayJson::Fail( "data must be the model file's bytes, in base64" );
        strData.assign( anData.begin(), anData.end() );
        m_strFilePath.clear();
    }
    else if ( ! RayJson::ReadSceneFile( m_strFile, strData, m_strFilePath, strError ) )
        return RayJson::Fail( "can't open the model \"" + m_strFile + "\"" );

    m_bSmooth = bSmooth;
    m_bDoubleSided = bDoubleSided;
    if ( ! LoadModel( strData, m_strFile, strError ) )
        return RayJson::Fail( "the model \"" + m_strFile + "\" " + strError );
    m_pFileData = ShareFileData( strData );
    return TRUE;
}

BOOL CMesh::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "vertices", "faces", "normals", "uvs", "uvMap", "smooth", "doubleSided", "color", "materials", "bump",
                                              "file", "data", "scale", "fitSize", "rotation", "align", "translation", NULL };
    static const char *const astrModelOnly[] = { "data", "scale", "fitSize", "rotation", "align", "translation", NULL };
    static const char *const astrInlineOnly[] = { "vertices", "faces", "normals", "uvs", NULL };
    std::vector<CVector>    aoVertices;
    std::vector<int>        anFaces;
    std::vector<CVector>    aoNormals;
    BOOL                    bGivenNormals = oNode.isObject() && ( oNode.get( "normals" ) != NULL );
        // Given normals are what smooth shading blends, so they make the
        // mesh smooth; saying "smooth": false with them is an error.
    BOOL                    bSmooth = bGivenNormals ? TRUE : m_bSmooth;
    BOOL                    bDoubleSided = m_bDoubleSided;

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetBool( oNode, "smooth", bSmooth, FALSE ) ||
         ! RayJson::GetBool( oNode, "doubleSided", bDoubleSided, FALSE ) ||
         ! RayJson::GetColor( oNode, "color", m_oColor, FALSE ) ||
         ! m_oMaterials.FromJson( oNode, "materials" ) ||
         ! BumpFromJson( oNode ) )
        return FALSE;
    if ( bGivenNormals && ! bSmooth )
        return RayJson::Fail( "normals are only used by smooth shading, so \"smooth\" can't be false with them" );

    BOOL bFromFile = ( oNode.get( "file" ) != NULL );

    for ( int nField = 0; astrModelOnly[ nField ] && ! bFromFile; nField++ )
    {
        if ( oNode.get( astrModelOnly[ nField ] ) )
            return RayJson::Fail( std::string( "\"" ) + astrModelOnly[ nField ] + "\" places a model loaded from a \"file\"" );
    }
    for ( int nField = 0; astrInlineOnly[ nField ] && bFromFile; nField++ )
    {
        if ( oNode.get( astrInlineOnly[ nField ] ) )
            return RayJson::Fail( std::string( "a mesh takes its geometry from a \"file\" or from \"vertices\" and \"faces\", so not \"" ) +
                                  astrInlineOnly[ nField ] + "\" with a file" );
    }
    if ( ! ReadUVMap( oNode, FALSE ) )
        return FALSE;
    if ( bFromFile )
        return ModelFromJson( oNode, bSmooth, bDoubleSided );

    const IASLib::CJsonNode *pVertices = RayJson::Field( oNode, "vertices", TRUE );
    const IASLib::CJsonNode *pFaces = RayJson::Field( oNode, "faces", TRUE );

    if ( pVertices == NULL || pFaces == NULL )
        return FALSE;

    {
        RayJson::CWhere oWhere( "vertices" );

        if ( ! pVertices->isArray() )
            return RayJson::Fail( "must be an array of [x, y, z] arrays" );
        aoVertices.resize( (size_t)pVertices->size() );
        for ( int nIndex = 0; nIndex < pVertices->size(); nIndex++ )
        {
            RayJson::CWhere oElement( "", nIndex );

            if ( ! RayJson::ToVector( *pVertices->get( nIndex ), aoVertices[ (size_t)nIndex ] ) )
                return FALSE;
        }
    }

    {
        RayJson::CWhere oWhere( "faces" );

        if ( ! pFaces->isArray() )
            return RayJson::Fail( "must be an array of [a, b, c] vertex indices" );
        anFaces.reserve( (size_t)pFaces->size() * 3 );
        for ( int nIndex = 0; nIndex < pFaces->size(); nIndex++ )
        {
            RayJson::CWhere             oElement( "", nIndex );
            const IASLib::CJsonNode    &oFace = *pFaces->get( nIndex );

            if ( ! oFace.isArray() || oFace.size() != 3 )
                return RayJson::Fail( "must be an array of three vertex indices" );
            for ( int nCorner = 0; nCorner < 3; nCorner++ )
            {
                const IASLib::CJsonNode *pIndex = oFace.get( nCorner );
                double                   dIndex = ( pIndex && pIndex->isNumber() ) ? pIndex->doubleValue() : -1.0;

                if ( dIndex != floor( dIndex ) || dIndex < 0 || dIndex >= (double)aoVertices.size() )
                    return RayJson::Fail( "vertex indices must be whole numbers from 0 to " +
                                          std::to_string( (long)aoVertices.size() - 1 ) );
                anFaces.push_back( (int)dIndex );
            }
        }
    }

    if ( bGivenNormals )
    {
        const IASLib::CJsonNode *pNormals = oNode.get( "normals" );
        RayJson::CWhere          oWhere( "normals" );

        if ( ! pNormals->isArray() || pNormals->size() != (int)aoVertices.size() )
            return RayJson::Fail( "must be an array of [x, y, z] normals, one for each of the " +
                                  std::to_string( (long)aoVertices.size() ) + " vertices" );
        aoNormals.resize( aoVertices.size() );
        for ( int nIndex = 0; nIndex < pNormals->size(); nIndex++ )
        {
            RayJson::CWhere oElement( "", nIndex );

            if ( ! RayJson::ToVector( *pNormals->get( nIndex ), aoNormals[ (size_t)nIndex ] ) )
                return FALSE;
            if ( aoNormals[ (size_t)nIndex ].Magnitude() == 0.0 )
                return RayJson::Fail( "a normal can't be [0, 0, 0]" );
            aoNormals[ (size_t)nIndex ].Normalize();
        }
    }

    std::vector<FLT>            anUVs;
    const IASLib::CJsonNode    *pUVs = oNode.get( "uvs" );

    if ( pUVs )
    {
        RayJson::CWhere oWhere( "uvs" );

        if ( ! pUVs->isArray() || pUVs->size() != (int)aoVertices.size() )
            return RayJson::Fail( "must be an array of [u, v] pairs, one for each of the " +
                                  std::to_string( (long)aoVertices.size() ) + " vertices" );
        anUVs.reserve( aoVertices.size() * 2 );
        for ( int nIndex = 0; nIndex < pUVs->size(); nIndex++ )
        {
            const IASLib::CJsonNode *pPair = pUVs->get( nIndex );

            if ( ! pPair->isArray() || pPair->size() != 2 || ! pPair->get( 0 )->isNumber() || ! pPair->get( 1 )->isNumber() )
            {
                RayJson::CWhere oElement( "", nIndex );
                return RayJson::Fail( "must be a [u, v] pair" );
            }
            anUVs.push_back( (FLT)pPair->get( 0 )->doubleValue() );
            anUVs.push_back( (FLT)pPair->get( 1 )->doubleValue() );
        }
    }

    return SetGeometry( aoVertices, anFaces, bSmooth, bDoubleSided, aoNormals, anUVs );
}
