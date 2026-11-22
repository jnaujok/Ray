/*************************************************************************
*   CScene                                                               *
*                                                                        *
*       This class is the heart of the raytracer engine. It contains all *
* of the objects within the 3-space that comprise the scene. It also     *
* performs all of the actual ray-tracing. The trace routine is totally   *
* thread-safe and reentrant.                                             *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#include "Globals.h"

#include "RaySettings.h"
#include "Scene.h"

#include <algorithm>
#include <cmath>
#include "Globals.h"
#include "SceneJson.h"
#include "Include.h"

#include "Exceptions/Exception.h"
#include "Files/File.h"
#include "Streams/FileStream.h"
#include "Streams/StringStream.h"
#include "Resources/Resource.h"
#include "Resources/ResourceFile.h"

#include <filesystem>

#include <stdio.h>
#include <memory>
#include <string>
#include <vector>



    // The binary scene files of 1996-1999 began with this.
#define MAGIC_NUMBER    "\x12\x27\x71\xF3"

    // JSON scene files say what they are, and their version follows on from
    // the two binary versions.
    // Saved scenes are IASLib resource archives: the scene's JSON, and the
    // files it uses, each under the name the scene gives it.
#define ARCHIVE_MAGIC       "IASRES\r\n"
#define FOLDER_SCENE        1
#define FOLDER_FILES        2
#define RESOURCE_SCENE      1
#define TYPE_SCENE          1
#define TYPE_FILE           2
#define SCENE_RESOURCE_NAME "scene.json"

#define JSON_FORMAT     "Ray scene"
#define JSON_VERSION    3

CScene::CScene( void ) :
    m_nOnlyObject( -1 ),
    m_bTreeBuilt( FALSE ),
    m_pLastOccluder( NULL )
{
    m_oObjects.RemoveAll();
    m_oLights.RemoveAll();
    m_oBackgroundColor.Set( 1, 1, 1 );
}

CScene::~CScene( void )
{
    m_oObjects.RemoveAll();
    m_oLights.RemoveAll();
}


void CScene::AddLight( CLight *pLight )
{
    m_oLights.Add( pLight );
}

void CScene::AddObject( CObjects *pObject )
{
    m_oObjects.Add( pObject );
    ForgetTree();
}


BOOL CScene::Intersect( CRay &oRay, CObjects *pIgnore )
{
    if ( ! m_bTreeBuilt )
        BuildTree();

        // Any hit will do. What blocked the last shadow ray is the likeliest
        // to block this one (the next point along, toward the same light).
    if ( m_pLastOccluder && m_pLastOccluder->Occludes( oRay, pIgnore ) )
        return TRUE;

    for ( size_t nIndex = 0; nIndex < m_anUnbounded.size(); nIndex++ )
    {
        CObjects *pObject = m_oObjects[ m_anUnbounded[ nIndex ] ];

        if ( pObject != m_pLastOccluder && pObject->Occludes( oRay, pIgnore ) )
        {
            m_pLastOccluder = pObject;
            return TRUE;
        }
    }

    if ( m_aoTree.empty() )
        return FALSE;

    FLT anOrigin[ 3 ] = { oRay.GetX(), oRay.GetY(), oRay.GetZ() };
    FLT anDirection[ 3 ] = { oRay.GetI(), oRay.GetJ(), oRay.GetK() };
    FLT nLimit = oRay.GetT();
    int anStack[ 128 ];
    int nStack = 0;

    anStack[ nStack++ ] = 0;
    while ( nStack > 0 )
    {
        const STreeNode &oNode = m_aoTree[ (size_t)anStack[ --nStack ] ];

        if ( ! RayHitsBox( oNode.anMin, oNode.anMax, anOrigin, anDirection, nLimit ) )
            continue;
        if ( oNode.nCount == 0 )
        {
            anStack[ nStack++ ] = oNode.nRight;
            anStack[ nStack++ ] = oNode.nLeft;
            continue;
        }
        for ( int nIndex = oNode.nFirst; nIndex < oNode.nFirst + oNode.nCount; nIndex++ )
        {
            CObjects *pObject = m_oObjects[ m_anTreeObjects[ (size_t)nIndex ] ];

            if ( pObject != m_pLastOccluder && pObject->Occludes( oRay, pIgnore ) )
            {
                m_pLastOccluder = pObject;
                return TRUE;
            }
        }
    }
    return FALSE;
}

CObjects *CScene::ClosestHit( CRay &oRay, CVector &oIntersect, CObjects *pDeny )
{
    if ( ! m_bTreeBuilt )
        BuildTree();

        // The closest hit, and of objects hit at exactly the same distance
        // the first in the scene, which is what testing every object in
        // order finds. Each object is tested against a copy of the ray
        // limited to just past the closest hit so far, so a tie still shows.
    FLT     nBest = oRay.GetT();
    int     nWinner = -1;
    auto    Try = [ & ]( int nObject )
    {
        CRay    oTry( oRay );
        CVector oHit;

        if ( nWinner >= 0 )
            oTry.SetT( std::nextafter( nBest, nBest * 2.0 + 1.0 ) );
        if ( m_oObjects[ nObject ]->Intersect( oTry, oHit, pDeny ) )
        {
            FLT nT = oTry.GetT();

            if ( nWinner < 0 || nT < nBest || ( nT == nBest && nObject < nWinner ) )
            {
                nBest = nT;
                nWinner = nObject;
                oIntersect = oHit;
            }
        }
    };

    for ( size_t nIndex = 0; nIndex < m_anUnbounded.size(); nIndex++ )
        Try( m_anUnbounded[ nIndex ] );

    if ( ! m_aoTree.empty() )
    {
        FLT anOrigin[ 3 ] = { oRay.GetX(), oRay.GetY(), oRay.GetZ() };
        FLT anDirection[ 3 ] = { oRay.GetI(), oRay.GetJ(), oRay.GetK() };
        int anStack[ 128 ];
        int nStack = 0;

        anStack[ nStack++ ] = 0;
        while ( nStack > 0 )
        {
            const STreeNode &oNode = m_aoTree[ (size_t)anStack[ --nStack ] ];

            if ( ! RayHitsBox( oNode.anMin, oNode.anMax, anOrigin, anDirection, nBest ) )
                continue;
            if ( oNode.nCount == 0 )
            {
                anStack[ nStack++ ] = oNode.nRight;
                anStack[ nStack++ ] = oNode.nLeft;
                continue;
            }
            for ( int nIndex = oNode.nFirst; nIndex < oNode.nFirst + oNode.nCount; nIndex++ )
                Try( m_anTreeObjects[ (size_t)nIndex ] );
        }
    }

    if ( nWinner < 0 )
        return NULL;
    oRay.SetT( nBest );
    return m_oObjects[ nWinner ];
}

void CScene::Nearby( const CVector &oPoint, FLT nDistance, std::vector<int> &anObjects )
{
    if ( ! m_bTreeBuilt )
        BuildTree();

    CVector oAt( oPoint );
    FLT     anAt[ 3 ] = { oAt.GetX(), oAt.GetY(), oAt.GetZ() };
    FLT     nLimit = nDistance * nDistance;
    auto    Within = [ & ]( const FLT anMin[ 3 ], const FLT anMax[ 3 ] )
    {
        FLT nSum = 0.0;

        for ( int nAxis = 0; nAxis < 3; nAxis++ )
        {
            FLT nOut = ( anAt[ nAxis ] < anMin[ nAxis ] ) ? anMin[ nAxis ] - anAt[ nAxis ] :
                       ( anAt[ nAxis ] > anMax[ nAxis ] ) ? anAt[ nAxis ] - anMax[ nAxis ] : 0.0;

            nSum += nOut * nOut;
        }
        return nSum <= nLimit;
    };

    anObjects.clear();
    for ( size_t nIndex = 0; nIndex < m_anUnbounded.size(); nIndex++ )
        anObjects.push_back( m_anUnbounded[ nIndex ] );
    if ( ! m_aoTree.empty() )
    {
        int anStack[ 128 ];
        int nStack = 0;

        anStack[ nStack++ ] = 0;
        while ( nStack > 0 )
        {
            const STreeNode &oNode = m_aoTree[ (size_t)anStack[ --nStack ] ];

            if ( ! Within( oNode.anMin, oNode.anMax ) )
                continue;
            if ( oNode.nCount == 0 )
            {
                anStack[ nStack++ ] = oNode.nRight;
                anStack[ nStack++ ] = oNode.nLeft;
                continue;
            }
            for ( int nIndex = oNode.nFirst; nIndex < oNode.nFirst + oNode.nCount; nIndex++ )
            {
                int nObject = m_anTreeObjects[ (size_t)nIndex ];

                if ( Within( &m_anObjectBounds[ (size_t)nObject * 6 ], &m_anObjectBounds[ (size_t)nObject * 6 + 3 ] ) )
                    anObjects.push_back( nObject );
            }
        }
    }
        // In scene order, whatever order the tree gave them in.
    std::sort( anObjects.begin(), anObjects.end() );
}

CObjects *CScene::ClosestHitByEachObject( CRay &oRay, CVector &oIntersect, CObjects *pDeny )
{
    CObjects *pIntersection = NULL;

    for ( int nObject = 0; nObject < m_oObjects.GetSize(); nObject++ )
    {
        if ( m_oObjects[ nObject ]->Intersect( oRay, oIntersect, pDeny ) )
            pIntersection = m_oObjects[ nObject ];
    }
    return pIntersection;
}

BOOL CScene::IntersectEachObject( CRay &oRay, CObjects *pIgnore )
{
    CVector oIntersect;

    for ( int nObject = 0; nObject < m_oObjects.GetSize(); nObject++ )
    {
        if ( m_oObjects[ nObject ]->Intersect( oRay, oIntersect, pIgnore ) )
            return TRUE;
    }
    return FALSE;
}

void CScene::ForgetTree( void )
{
    m_aoTree.clear();
    m_anTreeObjects.clear();
    m_anUnbounded.clear();
    m_anObjectBounds.clear();
    m_bTreeBuilt = FALSE;
    m_pLastOccluder = NULL;
}

BOOL CScene::RayHitsBox( const FLT anMin[ 3 ], const FLT anMax[ 3 ], const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLimit )
{
    FLT nNear = 0.0;
    FLT nFar = nLimit;

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

void CScene::BuildTree( void )
{
    ForgetTree();
    m_anObjectBounds.assign( (size_t)m_oObjects.GetSize() * 6, 0.0 );
    for ( int nObject = 0; nObject < m_oObjects.GetSize(); nObject++ )
    {
        CVector oMinimum, oMaximum;

        if ( m_oObjects[ nObject ]->GetBounds( oMinimum, oMaximum ) )
        {
            FLT *pnBounds = &m_anObjectBounds[ (size_t)nObject * 6 ];

            pnBounds[ 0 ] = oMinimum.GetX();
            pnBounds[ 1 ] = oMinimum.GetY();
            pnBounds[ 2 ] = oMinimum.GetZ();
            pnBounds[ 3 ] = oMaximum.GetX();
            pnBounds[ 4 ] = oMaximum.GetY();
            pnBounds[ 5 ] = oMaximum.GetZ();
            m_anTreeObjects.push_back( nObject );
        }
        else
            m_anUnbounded.push_back( nObject );
    }
    if ( ! m_anTreeObjects.empty() )
        BuildTreeNode( 0, (int)m_anTreeObjects.size() );
    m_bTreeBuilt = TRUE;
}

int CScene::BuildTreeNode( int nFirst, int nCount )
{
    int         nNode = (int)m_aoTree.size();
    STreeNode   oNode;
    FLT         anLow[ 3 ] = { 1e300, 1e300, 1e300 };       // the objects' centers' extent
    FLT         anHigh[ 3 ] = { -1e300, -1e300, -1e300 };

    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        oNode.anMin[ nAxis ] = 1e300;
        oNode.anMax[ nAxis ] = -1e300;
    }
    for ( int nIndex = nFirst; nIndex < nFirst + nCount; nIndex++ )
    {
        const FLT *pnBounds = &m_anObjectBounds[ (size_t)m_anTreeObjects[ (size_t)nIndex ] * 6 ];

        for ( int nAxis = 0; nAxis < 3; nAxis++ )
        {
            FLT nCenter = ( pnBounds[ nAxis ] + pnBounds[ nAxis + 3 ] ) / 2.0;

            oNode.anMin[ nAxis ] = ( pnBounds[ nAxis ] < oNode.anMin[ nAxis ] ) ? pnBounds[ nAxis ] : oNode.anMin[ nAxis ];
            oNode.anMax[ nAxis ] = ( pnBounds[ nAxis + 3 ] > oNode.anMax[ nAxis ] ) ? pnBounds[ nAxis + 3 ] : oNode.anMax[ nAxis ];
            anLow[ nAxis ] = ( nCenter < anLow[ nAxis ] ) ? nCenter : anLow[ nAxis ];
            anHigh[ nAxis ] = ( nCenter > anHigh[ nAxis ] ) ? nCenter : anHigh[ nAxis ];
        }
    }
    oNode.nFirst = nFirst;
    oNode.nCount = nCount;
    oNode.nLeft = oNode.nRight = -1;
    m_aoTree.push_back( oNode );
    if ( nCount <= 2 )
        return nNode;

        // Split at the median center along the axis the centers spread
        // furthest on.
    int nSplitAxis = 0;

    for ( int nAxis = 1; nAxis < 3; nAxis++ )
    {
        if ( anHigh[ nAxis ] - anLow[ nAxis ] > anHigh[ nSplitAxis ] - anLow[ nSplitAxis ] )
            nSplitAxis = nAxis;
    }

    int nHalf = nCount / 2;

    std::nth_element( m_anTreeObjects.begin() + nFirst, m_anTreeObjects.begin() + nFirst + nHalf,
                      m_anTreeObjects.begin() + nFirst + nCount, [ & ]( int nA, int nB )
                      {
                          FLT nCenterA = m_anObjectBounds[ (size_t)nA * 6 + nSplitAxis ] + m_anObjectBounds[ (size_t)nA * 6 + nSplitAxis + 3 ];
                          FLT nCenterB = m_anObjectBounds[ (size_t)nB * 6 + nSplitAxis ] + m_anObjectBounds[ (size_t)nB * 6 + nSplitAxis + 3 ];

                          return ( nCenterA != nCenterB ) ? ( nCenterA < nCenterB ) : ( nA < nB );
                      } );

    int nLeft = BuildTreeNode( nFirst, nHalf );
    int nRight = BuildTreeNode( nFirst + nHalf, nCount - nHalf );

    m_aoTree[ (size_t)nNode ].nLeft = nLeft;
    m_aoTree[ (size_t)nNode ].nRight = nRight;
    m_aoTree[ (size_t)nNode ].nCount = 0;
    return nNode;
}


BOOL CScene::TraceRay( CRay &oRay, CColor &oResult, CObjects *pDeny, int nKind )
{
    int     nCount;
        // Object pointer
    CObjects    *pObject;

        // The closest intersecting object
    CObjects    *pIntersection;

        // The intersection point of the ray with the object
    CVector oIntersect;

    CColor      oPixel( 0, 0, 0 );

    BOOL        bRetVal;

    oRay.Increment();

    if ( ( oRay.GetInfluence() < m_oSettings.GetDepthThreshold() ) || ( oRay.GetDepth() > m_oSettings.GetMaximumDepth() ) )
    {
        oResult = oPixel;
        return TRUE;
    }
    m_oRayCounts.Count( nKind );

    (void)nCount;
    (void)pObject;
    pIntersection = ClosestHit( oRay, oIntersect, pDeny );

    if ( pIntersection )
    {
        oPixel = pIntersection->Shade( *this, oRay, oIntersect );
        bRetVal = TRUE;
    }
    else
    {
        CVector oDirection( oRay.GetI(), oRay.GetJ(), oRay.GetK() );

        oPixel = Escaped( oDirection );
#ifdef _DEBUG
            // This should set the scene to a funky gradient
        if ( ! m_pEnvironment )
            oPixel.Set( oPixel.GetR() * ABS(oRay.GetI()), oPixel.GetG() * ABS(oRay.GetJ()), oPixel.GetB() * ABS(oRay.GetK()) );
#endif
        bRetVal = FALSE;
    }
    if ( m_pHaze )
    {
            // What the ray saw, through the haze between: as far as the hit,
            // or RAY_FAR_AWAY for a ray that left the scene.
        CVector oOrigin( oRay.GetX(), oRay.GetY(), oRay.GetZ() );
        CVector oDirection( oRay.GetI(), oRay.GetJ(), oRay.GetK() );
        FLT     nLength = oDirection.Magnitude();
        FLT     nDistance = RAY_FAR_AWAY;

        if ( nLength > 0.0 )
        {
            oDirection = oDirection * ( 1.0 / nLength );
            if ( pIntersection )
            {
                CVector oPath( oIntersect - oOrigin );

                nDistance = oPath.Magnitude();
            }
            oPixel = m_pHaze->Through( oPixel, m_pHaze->Transmittance( oOrigin, oDirection, nDistance ) );
        }
    }
    oResult = oPixel;
    return bRetVal;
}


CColor CScene::Escaped( CVector &oDirection )
{
    if ( m_pEnvironment )
        return m_pEnvironment->ColorIn( oDirection );
    return m_oBackgroundColor;
}

FLT CScene::LightThrough( const CVector &oPoint, const CVector &oToLight )
{
    if ( ! m_pHaze )
        return 1.0;

    CVector oPath( oToLight );
    FLT     nDistance = oPath.Magnitude();

    if ( ! ( nDistance > 0.0 ) )
        return 1.0;
        // A directional light's vector is RAY_FAR_AWAY long: sunlight. In
        // haze that is the same everywhere its path never ends, so it isn't
        // dimmed (see Haze.h).
    if ( nDistance >= RAY_FAR_AWAY * 0.5 && m_pHaze->GetHeightFalloff() <= 0.0 )
        return 1.0;
    oPath = oPath * ( 1.0 / nDistance );
    return m_pHaze->Transmittance( oPoint, oPath, nDistance );
}

CLight  *CScene::GetLight( int nIndex )
{
    if ( ( nIndex >= 0 ) && ( nIndex < m_oLights.GetSize() ) )
    {
        return m_oLights[ nIndex ];
    }
    return NULL;
}


DWORD CScene::Load( const char *strFileName )
{
    std::string strData;

    try
    {
            // Binary: in text mode Windows would translate line breaks and
            // stop at the first 0x1A byte.
        IASLib::CFileStream oFile( new IASLib::CFile( strFileName, IASLib::CFile::READ | IASLib::CFile::BINARY ) );
        char                achBuffer[ 8192 ];
        int                 nRead;

        if ( ! oFile.IsOpen() )
        {
            m_strLoadError = "the file can't be opened";
            return BAD_FILE;
        }
        while ( ( nRead = oFile.GetBuffer( achBuffer, (int)sizeof( achBuffer ) ) ) > 0 )
            strData.append( achBuffer, (size_t)nRead );
    }
    catch ( IASLib::CException & )
    {
            // IASLib's CFile throws when the file isn't there.
        m_strLoadError = "the file can't be opened";
        return BAD_FILE;
    }

        // A JSON scene starts with "{", perhaps after white space or a UTF-8
        // byte order mark. A binary one starts with its magic number.
    size_t nStart = ( strData.compare( 0, 3, "\xEF\xBB\xBF" ) == 0 ) ? 3 : 0;

    while ( nStart < strData.size() && ( strData[ nStart ] == ' ' || strData[ nStart ] == '\t' ||
                                         strData[ nStart ] == '\r' || strData[ nStart ] == '\n' ) )
        nStart++;
    if ( nStart < strData.size() && strData[ nStart ] == '{' )
    {
            // Files the scene names are relative to the scene file.
        std::string             strName( strFileName );
        size_t                  nSlash = strName.find_last_of( "/\\" );
        RayJson::CBaseDirectory oBase( ( nSlash == std::string::npos ) ? std::string() : strName.substr( 0, nSlash ) );

        return FromJson( strData.substr( nStart ) );
    }

    if ( strData.compare( 0, 8, ARCHIVE_MAGIC, 8 ) == 0 )
    {
        IASLib::CResourceFile oArchive;

        if ( ! oArchive.Open( strFileName, IASLib::CResourceFile::READ_ONLY ) )
        {
            m_strLoadError = std::string( "the scene archive can't be read: " ) + (const char *)oArchive.GetLastError();
            return BAD_FILE;
        }

        std::shared_ptr<const IASLib::CResource> pScene = oArchive.Get( FOLDER_SCENE, RESOURCE_SCENE );

        if ( ! pScene || pScene->GetResourceType() != TYPE_SCENE )
        {
            m_strLoadError = "the archive has no scene in it";
            return BAD_FILE;
        }

            // Files the scene names come from the archive (or else the disk,
            // relative to the archive).
        std::string             strName( strFileName );
        size_t                  nSlash = strName.find_last_of( "/\\" );
        RayJson::CBaseDirectory oBase( ( nSlash == std::string::npos ) ? std::string() : strName.substr( 0, nSlash ) );
        RayJson::CArchiveSource oSource( oArchive );

        return FromJson( std::string( (const char *)pScene->GetData(), pScene->GetLength() ) );
    }

    if ( strData.compare( 0, 4, MAGIC_NUMBER, 4 ) == 0 )
        m_strLoadError = "it is a binary scene from 1996-1999, which is no longer read; "
                         "the sample scenes have JSON versions in scenes/";
    else
        m_strLoadError = "it is not a scene file (a scene archive or JSON)";
    return BAD_FILE;
}

DWORD CScene::Save( const char *strFileName )
{
    RayJson::CFileCollector oFiles;
    std::string             strScene = ToJson( FALSE );
    std::string             strTemporary = std::string( strFileName ) + ".saving";

    m_strLoadError.clear();

        // Written beside the file and then put in its place, so a failed save
        // leaves the old file as it was.
    {
        IASLib::CResourceFile oArchive;
        IASLib::CResource     oScene( FOLDER_SCENE, RESOURCE_SCENE );
        bool                  bWritten;

        if ( ! oArchive.Open( strTemporary.c_str(), IASLib::CResourceFile::CREATE ) )
        {
            m_strLoadError = (const char *)oArchive.GetLastError();
            return BAD_FILE;
        }
        oArchive.SetAutoFlush( false );

        oScene.SetName( SCENE_RESOURCE_NAME );
        oScene.SetModifiedBy( "Ray" );
        oScene.SetResourceType( TYPE_SCENE );
        oScene.SetData( strScene.data(), strScene.size() );
        bWritten = oArchive.Put( oScene );

        for ( size_t nIndex = 0; bWritten && nIndex < oFiles.Files().size(); nIndex++ )
        {
            IASLib::CResource oFile( FOLDER_FILES, (short)( nIndex + 1 ) );

            oFile.SetName( oFiles.Files()[ nIndex ].first.c_str() );
            oFile.SetModifiedBy( "Ray" );
            oFile.SetResourceType( TYPE_FILE );
            oFile.SetData( oFiles.Files()[ nIndex ].second->data(), oFiles.Files()[ nIndex ].second->size() );
            bWritten = oArchive.Put( oFile );
        }
        if ( oFiles.Files().size() > 32767 )
            bWritten = false;
        if ( ! bWritten || ! oArchive.Flush() || ! oArchive.Close() )
        {
            m_strLoadError = (const char *)oArchive.GetLastError();
            oArchive.Close();
            remove( strTemporary.c_str() );
            return BAD_FILE;
        }
    }

    std::error_code oError;

    std::filesystem::rename( strTemporary, strFileName, oError );
    if ( oError )
    {
        m_strLoadError = oError.message();
        remove( strTemporary.c_str() );
        return BAD_FILE;
    }
    return 0;
}

DWORD CScene::SaveJson( const char *strFileName )
{
    RayJson::CFileCollector oFiles;
    std::string             strData;
    std::filesystem::path   oDirectory = std::filesystem::path( strFileName ).parent_path();

    m_strLoadError.clear();
    strData = ToJson( FALSE );

        // The files the scene uses go beside it, under the names it gives them.
    for ( size_t nIndex = 0; nIndex < oFiles.Files().size(); nIndex++ )
    {
        std::filesystem::path   oPath = oDirectory / oFiles.Files()[ nIndex ].first;
        std::error_code         oError;

        if ( oPath.has_parent_path() )
            std::filesystem::create_directories( oPath.parent_path(), oError );

        FILE *pFile = fopen( oPath.string().c_str(), "wb" );

        if ( pFile == NULL ||
             fwrite( oFiles.Files()[ nIndex ].second->data(), 1, oFiles.Files()[ nIndex ].second->size(), pFile ) !=
                 oFiles.Files()[ nIndex ].second->size() )
        {
            if ( pFile )
                fclose( pFile );
            m_strLoadError = "can't write " + oPath.string();
            return BAD_FILE;
        }
        fclose( pFile );
    }

    FILE *pFile = fopen( strFileName, "wb" );
    if ( pFile == NULL )
        return BAD_FILE;

    bool bWritten = fwrite( strData.data(), 1, strData.size(), pFile ) == strData.size();

    if ( fclose( pFile ) != 0 || ! bWritten )
        return BAD_FILE;
    return 0;
}

int CScene::FindObject( const std::string &strName )
{
    for ( int nIndex = 0; nIndex < m_oObjects.GetSize(); nIndex++ )
    {
        if ( ! m_oObjects[ nIndex ]->GetName().empty() && m_oObjects[ nIndex ]->GetName() == strName )
            return nIndex;
    }

    char  *pEnd = NULL;
    long   nNumber = strtol( strName.c_str(), &pEnd, 10 );

    if ( ! strName.empty() && pEnd && *pEnd == '\0' && nNumber >= 1 && nNumber <= m_oObjects.GetSize() )
        return (int)nNumber - 1;
    return -1;
}

DWORD CScene::SaveObject( int nIndex, const char *strFileName, BOOL bJson )
{
    if ( nIndex < 0 || nIndex >= m_oObjects.GetSize() )
    {
        m_strLoadError = "there is no such object";
        return BAD_FILE;
    }
    if ( m_oObjects[ nIndex ]->IncludedBy() )
    {
        m_strLoadError = "that object came in with an include; save the include's file instead";
        return BAD_FILE;
    }
    m_nOnlyObject = nIndex;

    DWORD nResult = bJson ? SaveJson( strFileName ) : Save( strFileName );

    m_nOnlyObject = -1;
    return nResult;
}

std::string CScene::ToJson( BOOL bCompact )
{
        // A compact copy goes to a task, which may not have the scene's
        // files, so it carries their data.
    std::unique_ptr<RayJson::CEmbedData>    pEmbed( bCompact ? new RayJson::CEmbedData : NULL );
    std::unique_ptr<IASLib::CObjectNode>    pRoot( ToJsonTree() );

    return bCompact ? pRoot->toJson( -1 ) : RayJson::Write( *pRoot );
}

    // An object or light as its JSON node: its type, its name (if any) and
    // its fields.
static void ObjectToJson( CObjects *pObject, IASLib::CObjectNode &oNode )
{
    LPCSTR strType = g_pDynamicList->JsonTypeOf( pObject->ClassType() );

    oNode.put( "type", strType ? strType : pObject->ClassType() );
    if ( ! pObject->GetName().empty() )
        oNode.put( "name", pObject->GetName().c_str() );
    pObject->ToJson( oNode );
}

IASLib::CObjectNode *CScene::ToJsonTree( void )
{
    IASLib::CObjectNode    *pRoot = new IASLib::CObjectNode;
    IASLib::CObjectNode    &oRoot = *pRoot;

    oRoot.put( "format", JSON_FORMAT );
    oRoot.put( "version", JSON_VERSION );
    m_oSettings.ToJson( *oRoot.putObject( "settings" ) );
    RayJson::PutColor( oRoot, "background", m_oBackgroundColor );
    if ( m_pEnvironment )
        m_pEnvironment->ToJson( *oRoot.putObject( "environment" ) );
    if ( m_pHaze )
        m_pHaze->ToJson( *oRoot.putObject( "haze" ) );
    m_oCamera.ToJson( *oRoot.putObject( "camera" ) );

    IASLib::CArrayNode *apArrays[ 2 ] = { oRoot.putArray( "objects" ), oRoot.putArray( "lights" ) };

    for ( int nList = 0; nList < 2; nList++ )
    {
        int nCount = ( nList == 0 ) ? m_oObjects.GetSize() : m_oLights.GetSize();

        for ( int nIndex = 0; nIndex < nCount; nIndex++ )
        {
            CObjects               *pObject = ( nList == 0 ) ? m_oObjects[ nIndex ] : (CObjects *)m_oLights[ nIndex ];

                // What an include brought in is saved as the include; with
                // m_nOnlyObject set, only that object is (--save-object).
            if ( nList == 0 && ( pObject->IncludedBy() || ( m_nOnlyObject >= 0 && nIndex != m_nOnlyObject ) ) )
                continue;

            ObjectToJson( pObject, *apArrays[ nList ]->addObject() );
        }
    }

    return pRoot;
}

    // Reads the array oRoot[ strKey ] of objects (bLights FALSE) or lights.
static BOOL ReadObjects( const IASLib::CJsonNode &oRoot, const char *strKey, BOOL bLights, std::vector<CObjects *> &apRead )
{
    const IASLib::CJsonNode *pArray = RayJson::Field( oRoot, strKey, FALSE );

    if ( pArray == NULL )
        return TRUE;

    RayJson::CWhere oWhere( strKey );

    if ( ! pArray->isArray() )
        return RayJson::Fail( bLights ? "must be an array of lights" : "must be an array of objects" );

    for ( int nIndex = 0; nIndex < pArray->size(); nIndex++ )
    {
        RayJson::CWhere             oElement( "", nIndex );
        CObjects                   *pObject = CObjects::ReadJson( *pArray->get( nIndex ), bLights );

        if ( pObject == NULL )
            return FALSE;
        apRead.push_back( pObject );

        CInclude *pInclude = dynamic_cast<CInclude *>( pObject );

        if ( pInclude )
            pInclude->TakeObjects( apRead );
    }
    return TRUE;
}

    // Splits "objects/3" into "objects" and 3 ("+" is -1). FALSE if the part
    // isn't a list entry.
static BOOL SplitPart( const std::string &strPart, std::string &strList, int &nIndex )
{
    size_t nSlash = strPart.find( '/' );

    if ( nSlash == std::string::npos )
        return FALSE;
    strList = strPart.substr( 0, nSlash );
    if ( strList != "objects" && strList != "lights" )
        return FALSE;

    std::string strIndex = strPart.substr( nSlash + 1 );

    if ( strIndex == "+" )
    {
        nIndex = -1;
        return TRUE;
    }
    if ( strIndex.empty() || strIndex.find_first_not_of( "0123456789" ) != std::string::npos || strIndex.size() > 9 )
        return FALSE;
    nIndex = atoi( strIndex.c_str() );
    return TRUE;
}

int CScene::RemoveIncluded( CObjects *pInclude, int nBefore )
{
    int nRemovedBefore = 0;

    for ( int nIndex = m_oObjects.GetSize() - 1; nIndex >= 0; nIndex-- )
    {
        CObjects *pObject = m_oObjects[ nIndex ];

        if ( pObject->IncludedBy() == pInclude )
        {
            m_oObjects.Delete( nIndex );
            delete pObject;
            if ( nIndex < nBefore )
                nRemovedBefore++;
        }
    }
    return nRemovedBefore;
}

std::string CScene::PartToJson( const std::string &strPart )
{
    std::unique_ptr<IASLib::CObjectNode>    pRoot( ToJsonTree() );
    const IASLib::CJsonNode                *pNode = NULL;
    std::string                             strList;
    int                                     nIndex;

    if ( strPart == "settings" || strPart == "camera" || strPart == "environment" || strPart == "haze" )
        pNode = pRoot->get( strPart.c_str() );
    else if ( SplitPart( strPart, strList, nIndex ) && nIndex >= 0 )
    {
            // By the scene's own lists, which hold what includes brought in
            // (the saved tree leaves those out).
        BOOL bLights = ( strList == "lights" );

        if ( nIndex < ( bLights ? m_oLights.GetSize() : m_oObjects.GetSize() ) )
        {
            IASLib::CObjectNode oNode;

            ObjectToJson( bLights ? (CObjects *)m_oLights[ nIndex ] : m_oObjects[ nIndex ], oNode );
            return RayJson::Write( oNode );
        }
    }
    return pNode ? RayJson::Write( *pNode ) : std::string();
}

int CScene::IndexOfObject( CObjects *pObject )
{
    for ( int nIndex = 0; nIndex < m_oObjects.GetSize(); nIndex++ )
    {
        if ( m_oObjects[ nIndex ] == pObject )
            return nIndex;
    }
    return -1;
}

int CScene::IndexOfLight( CObjects *pLight )
{
    for ( int nIndex = 0; nIndex < m_oLights.GetSize(); nIndex++ )
    {
        if ( (CObjects *)m_oLights[ nIndex ] == pLight )
            return nIndex;
    }
    return -1;
}

DWORD CScene::EditPart( const std::string &strPart, const std::string &strText, const std::string &strDirectory,
                        const std::string &strArchive )
{
    std::string strList;
    int         nIndex = 0;
    BOOL        bList = SplitPart( strPart, strList, nIndex );
    BOOL        bLights = bList && strList == "lights";
    int         nCount = bLights ? m_oLights.GetSize() : m_oObjects.GetSize();
    BOOL        bEmpty = ( strText.find_first_not_of( " \t\r\n" ) == std::string::npos );

    RayJson::ClearError();
    m_strLoadError.clear();
    if ( ! bList && strPart != "settings" && strPart != "camera" && strPart != "environment" && strPart != "haze" )
    {
        m_strLoadError = "there is no part of a scene called \"" + strPart + "\"";
        return BAD_FILE;
    }
    if ( bList && nIndex >= nCount )
    {
        m_strLoadError = "there is no " + strPart;
        return BAD_FILE;
    }
    if ( bList && ! bLights && nIndex >= 0 && m_oObjects[ nIndex ]->IncludedBy() )
    {
        m_strLoadError = "that object came in with an include; edit the include, or its file";
        return BAD_FILE;
    }

        // Empty text removes the part, where it can be removed.
    if ( bEmpty )
    {
        if ( strPart == "environment" )
        {
            m_pEnvironment.reset();
            return 0;
        }
        if ( strPart == "haze" )
        {
            m_pHaze.reset();
            return 0;
        }
        if ( bList && nIndex >= 0 )
        {
            if ( bLights )
            {
                CLight *pOld = m_oLights[ nIndex ];

                m_oLights.Delete( nIndex );
                delete pOld;
            }
            else
            {
                CObjects *pOld = m_oObjects[ nIndex ];

                RemoveIncluded( pOld );
                m_oObjects.Delete( nIndex );
                delete pOld;
                ForgetTree();
            }
            return 0;
        }
        m_strLoadError = "the " + strPart + " can't be left out";
        return BAD_FILE;
    }

    std::unique_ptr<IASLib::CJsonNode> pNode( RayJson::Parse( strText ) );

    if ( ! pNode )
    {
        m_strLoadError = RayJson::GetError();
        RayJson::ClearError();
        return BAD_FILE;
    }

        // Files are read from the archive, if it has them, else the disk.
    RayJson::CBaseDirectory                 oBase( strDirectory );
    IASLib::CResourceFile                   oArchive;
    std::unique_ptr<RayJson::CArchiveSource> pSource;
    BOOL                                    bRead = FALSE;

    if ( ! strArchive.empty() && oArchive.Open( strArchive.c_str(), IASLib::CResourceFile::READ_ONLY ) )
        pSource.reset( new RayJson::CArchiveSource( oArchive ) );

    if ( strPart == "settings" )
    {
        CSettings       oSettings;
        RayJson::CWhere oWhere( "settings" );

        if ( oSettings.FromJson( *pNode ) )
        {
            m_oSettings = oSettings;
            bRead = TRUE;
        }
    }
    else if ( strPart == "camera" )
    {
        CCamera         oCamera;
        RayJson::CWhere oWhere( "camera" );

        if ( oCamera.FromJson( *pNode ) )
        {
            m_oCamera = oCamera;
            bRead = TRUE;
        }
    }
    else if ( strPart == "environment" )
    {
        std::shared_ptr<CEnvironment>   pEnvironment( new CEnvironment );
        RayJson::CWhere                 oWhere( "environment" );

        if ( pEnvironment->FromJson( *pNode ) )
        {
            m_pEnvironment = pEnvironment;
            bRead = TRUE;
        }
    }
    else if ( strPart == "haze" )
    {
        std::shared_ptr<CHaze>  pHaze( new CHaze );
        RayJson::CWhere         oWhere( "haze" );

        if ( pHaze->FromJson( *pNode ) )
        {
            m_pHaze = pHaze;
            bRead = TRUE;
        }
    }
    else
    {
            // Read as the scene reads its lists, from a list of one.
        IASLib::CObjectNode     oRoot;
        std::vector<CObjects *> apRead;

        oRoot.putArray( strList.c_str() )->add( pNode.release() );
            // An include reads as itself and then what it brings in.
        if ( ReadObjects( oRoot, strList.c_str(), bLights, apRead ) &&
             ( apRead.size() == 1 || ( ! bLights && dynamic_cast<CInclude *>( apRead[ 0 ] ) ) ) )
        {
            if ( bLights )
            {
                if ( nIndex < 0 )
                    m_oLights.Add( (CLight *)apRead[ 0 ] );
                else
                {
                    CLight *pOld = m_oLights[ nIndex ];

                    m_oLights.Delete( nIndex );
                    m_oLights.Insert( (CLight *)apRead[ 0 ], nIndex );
                    delete pOld;
                }
            }
            else
            {
                if ( nIndex < 0 )
                {
                    for ( CObjects *pRead : apRead )
                        m_oObjects.Add( pRead );
                }
                else
                {
                    CObjects *pOld = m_oObjects[ nIndex ];

                    nIndex -= RemoveIncluded( pOld, nIndex );
                    m_oObjects.Delete( nIndex );
                    for ( size_t nRead = 0; nRead < apRead.size(); nRead++ )
                        m_oObjects.Insert( apRead[ nRead ], nIndex + (int)nRead );
                    delete pOld;
                }
                ForgetTree();
            }
            apRead.clear();
            bRead = TRUE;
        }
        for ( size_t nRead = 0; nRead < apRead.size(); nRead++ )
            delete apRead[ nRead ];
    }

    if ( ! bRead )
    {
        m_strLoadError = RayJson::GetError();
        RayJson::ClearError();
        return BAD_FILE;
    }
    return 0;
}

DWORD CScene::FromJson( const std::string &strText )
{
    RayJson::ClearError();

    std::unique_ptr<IASLib::CJsonNode> pRoot( RayJson::Parse( strText ) );

    if ( ! pRoot )
    {
        m_strLoadError = RayJson::GetError();
        RayJson::ClearError();
        return BAD_FILE;
    }
    return FromJsonTree( *pRoot );
}

DWORD CScene::FromJsonTree( const IASLib::CJsonNode &oRoot )
{
    static const char *const astrFields[] = { "format", "version", "settings", "background", "environment", "haze", "camera", "objects", "lights", NULL };
    const IASLib::CJsonNode            *pRoot = &oRoot;
    std::vector<CObjects *>             apObjects;
    std::vector<CObjects *>             apLights;
    CSettings                           oSettings;
    CCamera                             oCamera;
    CColor                              oBackground( 1, 1, 1 );
    std::shared_ptr<CEnvironment>       pEnvironment;
    std::shared_ptr<CHaze>              pHaze;
    std::string                         strFormat;
    int                                 nVersion = 0;
    BOOL                                bRead = FALSE;

    RayJson::ClearError();

    {
        if ( ! RayJson::GetString( *pRoot, "format", strFormat, TRUE ) )
            ;
        else if ( strFormat != JSON_FORMAT )
            RayJson::Fail( "\"format\" must be \"" JSON_FORMAT "\"" );
        else if ( ! RayJson::GetInt( *pRoot, "version", nVersion, 1, 1000000, TRUE ) )
            ;
        else if ( nVersion > JSON_VERSION )
        {
            m_strLoadError = "it was saved by a newer version of the ray tracer (scene version " +
                             std::to_string( nVersion ) + ")";
            return WRONG_VERSION;
        }
        else if ( nVersion < JSON_VERSION )
            RayJson::Fail( "JSON scenes start at version " + std::to_string( JSON_VERSION ) );
        else if ( RayJson::OnlyFields( *pRoot, astrFields ) )
        {
            const IASLib::CJsonNode *pSettings = pRoot->get( "settings" );
            const IASLib::CJsonNode *pCamera = RayJson::Field( *pRoot, "camera", TRUE );
            BOOL                     bSettings = TRUE;
            BOOL                     bCamera = FALSE;

            if ( pSettings )
            {
                RayJson::CWhere oWhere( "settings" );
                bSettings = oSettings.FromJson( *pSettings );
            }
            if ( bSettings && pCamera )
            {
                RayJson::CWhere oWhere( "camera" );
                bCamera = oCamera.FromJson( *pCamera );
            }

            const IASLib::CJsonNode *pEnvironmentNode = pRoot->get( "environment" );
            BOOL                     bEnvironment = TRUE;

            if ( bSettings && bCamera && pEnvironmentNode )
            {
                RayJson::CWhere oWhere( "environment" );

                pEnvironment.reset( new CEnvironment );
                bEnvironment = pEnvironment->FromJson( *pEnvironmentNode );
            }

            const IASLib::CJsonNode *pHazeNode = pRoot->get( "haze" );
            BOOL                     bHaze = TRUE;

            if ( bSettings && bCamera && bEnvironment && pHazeNode )
            {
                RayJson::CWhere oWhere( "haze" );

                pHaze.reset( new CHaze );
                bHaze = pHaze->FromJson( *pHazeNode );
            }

            bRead = bSettings && bCamera && bEnvironment && bHaze &&
                    RayJson::GetColor( *pRoot, "background", oBackground, FALSE ) &&
                    ReadObjects( *pRoot, "objects", FALSE, apObjects ) &&
                    ReadObjects( *pRoot, "lights", TRUE, apLights );
        }
    }

    if ( ! bRead )
    {
        for ( size_t nIndex = 0; nIndex < apObjects.size(); nIndex++ )
            delete apObjects[ nIndex ];
        for ( size_t nIndex = 0; nIndex < apLights.size(); nIndex++ )
            delete apLights[ nIndex ];
        m_strLoadError = RayJson::GetError();
        RayJson::ClearError();
        return BAD_FILE;
    }

    m_oObjects.RemoveAll();
    m_oLights.RemoveAll();
    ForgetTree();
    m_oSettings = oSettings;
    m_oCamera = oCamera;
    m_oBackgroundColor = oBackground;
    m_pEnvironment = pEnvironment;
    m_pHaze = pHaze;
    for ( size_t nIndex = 0; nIndex < apObjects.size(); nIndex++ )
        AddObject( apObjects[ nIndex ] );
    for ( size_t nIndex = 0; nIndex < apLights.size(); nIndex++ )
        AddLight( (CLight *)apLights[ nIndex ] );
    m_strLoadError.clear();
    return 0;
}
