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

#ifdef RAY_GUI
#include "StdAfx.h"
#include "resource.h"
#else
#include "Globals.h"
#endif

#include "RaySettings.h"
#include "Scene.h"
#include "RayStream.h"
#include "SceneJson.h"

#include "Exceptions/Exception.h"
#include "Files/File.h"
#include "Streams/FileStream.h"
#include "Streams/StringStream.h"

#include <stdio.h>
#include <memory>
#include <string>
#include <vector>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define MAGIC_NUMBER    (DWORD)0x122771F3
#define CURRENT_VERSION ((DWORD)0x02)
#define VERSION_ONE     ((DWORD)0x01)

    // JSON scene files say what they are, and their version follows on from
    // the two binary versions.
#define JSON_FORMAT     "Ray scene"
#define JSON_VERSION    3

CScene::CScene( void )
{
    m_oObjects.RemoveAll();
    m_oLights.RemoveAll();
    m_oBackgroundColor.Set( 1, 1, 1 );
#ifdef RAY_GUI
    m_hObjects = NULL;
    m_hLights = NULL;
    m_hCamera = NULL;
    m_poSceneTree = NULL;
#endif
}

CScene::~CScene( void )
{
    m_oObjects.RemoveAll();
    m_oLights.RemoveAll();
}

#ifdef RAY_GUI
void CScene::ToTree( CSceneTreeView *poSceneTree )
{
	CLight     *pLight;
    CObjects   *pObject;
    int         nCount;

    m_poSceneTree = poSceneTree;

    if ( m_hCamera == NULL )
    {
        m_hCamera = poSceneTree->AddObject( &m_oCamera, 12, "Camera" );
        m_oCamera.ToTree( poSceneTree, m_hCamera );
    }
    
        // Add Base Light Tree
    if ( m_hLights == NULL )
    {
        m_hLights = poSceneTree->AddObject( NULL, 2, "Lights" );
        nCount = 0;

        while ( nCount < m_oLights.GetSize() )
        {
            pLight = m_oLights[nCount ];
            pLight->ToTree( poSceneTree, m_hLights );
            nCount++;
        }
    }

        // Add Base Object Tree
    if ( m_hObjects == NULL )
    {
        m_hObjects = poSceneTree->AddObject( NULL, 4, "Objects" );

        nCount = 0;

        while ( nCount < m_oObjects.GetSize() )
        {
            pObject = m_oObjects[nCount ];
            pObject->ToTree( poSceneTree, m_hObjects );
            nCount++;
        }
    }
}
#endif

void CScene::AddLight( CLight *pLight )
{
    m_oLights.Add( pLight );
#ifdef RAY_GUI
    if ( m_hLights )
        pLight->ToTree( m_poSceneTree, m_hLights );
#endif
}

void CScene::AddObject( CObjects *pObject )
{
    m_oObjects.Add( pObject );
#ifdef RAY_GUI
    if ( m_hObjects )
        pObject->ToTree( m_poSceneTree, m_hObjects );
#endif
}

#ifdef RAY_GUI
BOOL CScene::TraceScene( CDC *pDC, CRect oRect, BOOL *pbStop, int nQuarter )
{
        // Current Pixel Color
    CColor  oPixel;

        // Pixel Iterators
    int     nX;
    int     nY;

        // Object Iterator
    int     nCount;

        // Object pointer
    CObjects    *pObject;

        // The closest intersecting object
    CObjects    *pIntersection;

        // The Ray we're casting
    CRay    oRay;

        // The intersection point of the ray with the object
    CVector oIntersect;

        // Light's for shading/shadow ray
//    int         nLight;
//    CLight     *pLight;

        // Objects for Shadow testing
//    int         nShadowObject;
//    CObjects   *pShadowObject;

        // Vector and rays for shadow casting
    CRay        oShadowRay;
    CVector     oLightVector;
    CVector     oShadowIntersect;

        // Camera information
    CVector oCamPos = m_oCamera.GetPosition();
    CVector oCamPoint = m_oCamera.PointAt() - oCamPos;

    int         nYStop = oRect.bottom + 1;
    int         nXStart = oRect.left;
    int         nXStop = oRect.right + 1;
    int         nYStart = oRect.top;

    switch( nQuarter )
    {
        case 1:
            break;

        case 2:
            return TRUE;

        case 3:
            return TRUE;

        case 4:
            return TRUE;

        default:
            return TRUE;
    }

    nY = nYStart;
    while ( nY < nYStop )
    {
        nX = nXStart;
        while ( nX < nXStop )
        {
                // Check if the trace has been interrupted
            if ( *pbStop )
                return FALSE;

            oRay.CreateRay( *this, nX, nY );

            nCount = 0;
            oPixel = RGB( 0, 0, 0 );
            pIntersection = NULL;

            while ( nCount < m_oObjects.GetSize() )
            {
                pObject = m_oObjects[ nCount ];
                if ( pObject->Intersect( oRay, oIntersect ) )
                    pIntersection = pObject;
                nCount++;
            }

            if ( pIntersection )
            {
                oPixel = pIntersection->Shade( *this, oRay, oIntersect );
            }
            else
            {
                oPixel = m_oBackgroundColor;
#ifdef _DEBUG
                    // This should set the scene to a funky gradient
                oPixel.Set( oPixel.GetR() * ABS(oRay.GetI()), oPixel.GetG() * ABS(oRay.GetJ()), oPixel.GetB() * ABS(oRay.GetK()) );
#endif
            }

            oPixel.Clip();
            pDC->SetPixel( nX, nY, (COLORREF)oPixel );

            nX++;
        }
        nY++;
    }
    return TRUE;
}
#endif

DWORD CScene::Serialize( CRayStream &oStream, BOOL bLoading )
{
    DWORD       nLength;
    DWORD       nObjects;
    IASLib::CString     strClassType;
    CObjects   *pObject;
    LPCSTR      pName;
    int         nCount;
        

    if ( bLoading )
    {

        oStream >> nLength;
        if ( nLength != MAGIC_NUMBER )
            return BAD_FILE;

        m_oObjects.RemoveAll();
        m_oLights.RemoveAll();

        oStream >> nLength;
        switch ( nLength )
        {
            case VERSION_ONE:
                    // Version one files all predate text-encoded doubles.
                oStream.SetLegacyDoubles( TRUE );
                oStream >> m_oSettings;

                while ( oStream.DataAvailable() )
                {
                    oStream >> strClassType;

                    pObject = g_pDynamicList->Instantiate( strClassType, oStream );

                        // An unknown class leaves the stream in the middle of
                        // its data, so nothing after it can be read.
                    if ( pObject == NULL )
                        return BAD_FILE;

                    if ( pObject )
                    {
                        if ( pObject->IsCamera() )
                        {
                            m_oCamera = *((CCamera *)pObject);
                            delete pObject;     // the camera is copied, not kept
                        }
                        else
                        {
                            if ( pObject->IsALight() )
                            {
                                AddLight( (CLight *)pObject );
                            }
                            else
                            {
                                AddObject( pObject );
                            }
                        }
                    }
                }
                break;

            case CURRENT_VERSION:
                oStream >> m_oSettings;

                oStream >> nObjects;

                while ( nObjects )
                {
                    oStream >> strClassType;

                    pObject = g_pDynamicList->Instantiate( strClassType, oStream );

                        // An unknown class leaves the stream in the middle of
                        // its data, so nothing after it can be read.
                    if ( pObject == NULL )
                        return BAD_FILE;

                    if ( pObject )
                    {
                        if ( pObject->IsCamera() )
                        {
                            m_oCamera = *((CCamera *)pObject);
                            delete pObject;     // the camera is copied, not kept
                        }
                        else
                        {
                            if ( pObject->IsALight() )
                            {
                                AddLight( (CLight *)pObject );
                            }
                            else
                            {
                                AddObject( pObject );
                            }
                        }
                    }
                    nObjects--;
                }
                break;

            default:
                return WRONG_VERSION;
        }
    }
    else
    {
        nLength = MAGIC_NUMBER;
        oStream << nLength;

        nLength = CURRENT_VERSION;
        oStream << nLength;

        oStream << m_oSettings;

        nObjects = 1 + m_oObjects.GetSize() + m_oLights.GetSize();

        oStream << nObjects;

        pName = m_oCamera.ClassType();
        oStream << pName;

        oStream << m_oCamera;
        
        nCount = 0;
        while ( nCount < m_oObjects.GetSize() )
        {
            pObject = m_oObjects[ nCount ];
            
            pName = pObject->ClassType();
            oStream << pName;

            pObject->Serialize( oStream, FALSE );

            nCount++;
        }

        nCount = 0;
        while ( nCount < m_oLights.GetSize() )
        {
            pObject = m_oLights[ nCount ];
            
            pName = pObject->ClassType();

            oStream << pName;

            pObject->Serialize( oStream, FALSE );

            nCount++;
        }
    }

    return 0;
}

BOOL CScene::Intersect( CRay &oRay, CObjects *pIgnore )
{
    int         nObject;
    CObjects   *pObject;
    CVector     oIntersect;

    nObject = 0;
    while ( nObject < m_oObjects.GetSize() )
    {
        pObject = m_oObjects[ nObject ];

        if ( pObject != pIgnore )
        {
            if ( pObject->Intersect( oRay, oIntersect ) )
                return TRUE;
        }
        nObject++;
    }
    return FALSE;
}


BOOL CScene::TraceRay( CRay &oRay, CColor &oResult, CObjects *pDeny )
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

    nCount = 0;
    pIntersection = NULL;

    while ( nCount < m_oObjects.GetSize() )
    {
        pObject = m_oObjects[ nCount ];
        if ( pObject != pDeny )
        {
            if ( pObject->Intersect( oRay, oIntersect ) )
                pIntersection = pObject;
        }
        nCount++;
    }

    if ( pIntersection )
    {
        oPixel = pIntersection->Shade( *this, oRay, oIntersect );
        bRetVal = TRUE;
    }
    else
    {
        oPixel = m_oBackgroundColor;
#ifdef _DEBUG
            // This should set the scene to a funky gradient
        oPixel.Set( oPixel.GetR() * ABS(oRay.GetI()), oPixel.GetG() * ABS(oRay.GetJ()), oPixel.GetB() * ABS(oRay.GetK()) );
#endif
        bRetVal = FALSE;
    }
    oResult = oPixel;
    return bRetVal;
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

        // A JSON scene starts with "{", perhaps after white space or a UTF-8
        // byte order mark. A binary one starts with its magic number.
    size_t nStart = ( strData.compare( 0, 3, "\xEF\xBB\xBF" ) == 0 ) ? 3 : 0;

    while ( nStart < strData.size() && ( strData[ nStart ] == ' ' || strData[ nStart ] == '\t' ||
                                         strData[ nStart ] == '\r' || strData[ nStart ] == '\n' ) )
        nStart++;
    if ( nStart < strData.size() && strData[ nStart ] == '{' )
        return FromJson( strData.substr( nStart ) );

    m_strLoadError.clear();

        // Files saved in the weeks when doubles were still binary but the
        // header already said version two can only be told apart by trying.
    for ( int nAttempt = 0; nAttempt < 2; nAttempt++ )
    {
        IASLib::CStringStream   oBuffer( strData.data(), strData.size() );
        CRayStream              oStream( oBuffer );
        DWORD                   nResult;

        oStream.SetLegacyDoubles( nAttempt == 1 );
        try
        {
            nResult = Serialize( oStream, TRUE );
        }
        catch ( IASLib::CException & )
        {
            nResult = BAD_FILE;
        }

        if ( nResult == 0 )
            return 0;
        if ( nResult == WRONG_VERSION )
        {
            m_strLoadError = "it was saved by a newer version of the ray tracer";
            return nResult;
        }
    }

    m_oObjects.RemoveAll();
    m_oLights.RemoveAll();
    m_strLoadError = "it is not a scene file this program can read";
    return BAD_FILE;
}

DWORD CScene::Save( const char *strFileName )
{
    std::string strData = ToJson( FALSE );

    FILE *pFile = fopen( strFileName, "wb" );
    if ( pFile == NULL )
        return BAD_FILE;

    bool bWritten = fwrite( strData.data(), 1, strData.size(), pFile ) == strData.size();

    if ( fclose( pFile ) != 0 || ! bWritten )
        return BAD_FILE;
    return 0;
}

std::string CScene::ToJson( BOOL bCompact )
{
    IASLib::CObjectNode     oRoot;

    oRoot.put( "format", JSON_FORMAT );
    oRoot.put( "version", JSON_VERSION );
    m_oSettings.ToJson( *oRoot.putObject( "settings" ) );
    RayJson::PutColor( oRoot, "background", m_oBackgroundColor );
    m_oCamera.ToJson( *oRoot.putObject( "camera" ) );

    IASLib::CArrayNode *apArrays[ 2 ] = { oRoot.putArray( "objects" ), oRoot.putArray( "lights" ) };

    for ( int nList = 0; nList < 2; nList++ )
    {
        int nCount = ( nList == 0 ) ? m_oObjects.GetSize() : m_oLights.GetSize();

        for ( int nIndex = 0; nIndex < nCount; nIndex++ )
        {
            CObjects               *pObject = ( nList == 0 ) ? m_oObjects[ nIndex ] : (CObjects *)m_oLights[ nIndex ];
            IASLib::CObjectNode    *pNode = apArrays[ nList ]->addObject();
            LPCSTR                  strType = g_pDynamicList->JsonTypeOf( pObject->ClassType() );

            pNode->put( "type", strType ? strType : pObject->ClassType() );
            pObject->ToJson( *pNode );
        }
    }

    return bCompact ? oRoot.toJson( -1 ) : RayJson::Write( oRoot );
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
        const IASLib::CJsonNode    &oNode = *pArray->get( nIndex );
        std::string                 strType;

        if ( ! RayJson::GetString( oNode, "type", strType, TRUE ) )
            return FALSE;

        CObjects *pObject = g_pDynamicList->Create( strType.c_str() );

        if ( pObject == NULL || pObject->IsCamera() || ( pObject->IsALight() ? TRUE : FALSE ) != bLights )
        {
            delete pObject;
            return RayJson::Fail( std::string( bLights ? "unknown light type \"" : "unknown object type \"" ) + strType + "\"" );
        }

        apRead.push_back( pObject );
        if ( ! pObject->FromJson( oNode ) )
            return FALSE;
    }
    return TRUE;
}

DWORD CScene::FromJson( const std::string &strText )
{
    static const char *const astrFields[] = { "format", "version", "settings", "background", "camera", "objects", "lights", NULL };
    std::unique_ptr<IASLib::CJsonNode>  pRoot;
    std::vector<CObjects *>             apObjects;
    std::vector<CObjects *>             apLights;
    CSettings                           oSettings;
    CCamera                             oCamera;
    CColor                              oBackground( 1, 1, 1 );
    std::string                         strFormat;
    int                                 nVersion = 0;
    BOOL                                bRead = FALSE;

    RayJson::ClearError();
    pRoot.reset( RayJson::Parse( strText ) );

    if ( pRoot )
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

            bRead = bSettings && bCamera &&
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
    m_oSettings = oSettings;
    m_oCamera = oCamera;
    m_oBackgroundColor = oBackground;
    for ( size_t nIndex = 0; nIndex < apObjects.size(); nIndex++ )
        AddObject( apObjects[ nIndex ] );
    for ( size_t nIndex = 0; nIndex < apLights.size(); nIndex++ )
        AddLight( (CLight *)apLights[ nIndex ] );
    m_strLoadError.clear();
    return 0;
}
