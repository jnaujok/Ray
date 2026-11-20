/*************************************************************************
*   CObjects                                                             *
*                                                                        *
*       This is the base class for all of the three-dimensional objects, *
* including objects like the camera and lights.                          *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Scene.h"
#include "Light.h"
#include "Sampling.h"
#include "BumpMap.h"
#include "SceneJson.h"

#include <string.h>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CDynObject *CDynObject::m_pFirst = NULL;

static CDynObject   g_oBaseObject( NULL, "Base Object", NULL );

    // The registry of object classes. It is a static object, so it exists
    // in every program (it used to be created only by the GUI), and it reads
    // the registrations when it is used, so the order in which static
    // objects are built does not matter.
static CDynList g_oDynamicList;
CDynList    *g_pDynamicList = &g_oDynamicList;

CObjects::CObjects( void )
{
    m_bOwnsMaterials = TRUE;
    m_pShadeMaterials = NULL;
#ifdef RAY_GUI
    m_bExpanded = FALSE;
    m_bSelected = FALSE;
    m_hObjectItem = NULL;
    m_hMaterialTree = NULL;
#endif
}

CObjects::~CObjects( void )
{
    CMaterial  *pMaterial;
    int         nIndex;

    nIndex = 0;
    while ( m_bOwnsMaterials && ( nIndex < m_oMaterials.GetSize() ) )
    {
        pMaterial = m_oMaterials[ nIndex ];
        delete pMaterial;
        nIndex++;
    }
    m_oMaterials.RemoveAll();
}

#ifdef RAY_GUI
#include "resource.h"
#include "MaterialsDlg.h"

void CObjects::MatToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    CMaterial      *pMaterial;
    int             nCount;

    m_poTree = pTree;
    m_hObjectItem = hUnder;

    if ( m_hMaterialTree )
        pTree->EmptyUnder( m_hMaterialTree );
    else
        m_hMaterialTree = pTree->AddObject( NULL, 14, "Materials", m_hObjectItem );

    nCount = 0;
    while ( nCount < m_oMaterials.GetSize() )
    {
        pMaterial = m_oMaterials[ nCount ];
        pMaterial->ToTree( pTree, m_hMaterialTree );
        nCount++;
    }
}

void CObjects::SetMaterials( void )
{
    CMaterialsDlg   oDlg;

    oDlg.m_oMaterials = m_oMaterials;

    oDlg.DoModal();

    m_oMaterials = oDlg.m_oMaterials;

    MatToTree( m_poTree, m_hObjectItem );
}

#endif

void CObjects::BumpToJson( IASLib::CObjectNode &oNode ) const
{
    if ( m_pBump )
        m_pBump->ToJson( *oNode.putObject( "bump" ) );
}

BOOL CObjects::BumpFromJson( const IASLib::CJsonNode &oNode )
{
    const IASLib::CJsonNode *pBump = oNode.isObject() ? oNode.get( "bump" ) : NULL;

    m_pBump.reset();
    if ( pBump == NULL )
        return TRUE;

    RayJson::CWhere           oWhere( "bump" );
    std::shared_ptr<CBumpMap> pNew( new CBumpMap );

    if ( ! pNew->FromJson( *pBump ) )
        return FALSE;
    m_pBump = pNew;
    return TRUE;
}

CColor CObjects::BaseShade( CScene &oScene, CRay &oRay, CVector &oIntersect, CVector &oSmoothNormal )
{
    CMaterialList  &oMaterials = m_pShadeMaterials ? *m_pShadeMaterials : m_oMaterials;
        // A bump map tilts the normal every material shades with. The smooth
        // normal is left as it is (a plane's is its own member).
    CVector     oNormal( oSmoothNormal );

    if ( m_pBump )
        m_pBump->Perturb( *this, oIntersect, oNormal, oRay );

    CColor      oReturn;
    CColor      oColor;
    CRay        oLightRay;
    CVector     oToLight;
    CLight     *pLight;
    int         nLight;
    CMaterial  *pMaterial;
    int         nMaterial;
    CColor      oBlack( 0, 0, 0 );
    CColor      oLight;

    oReturn.Set( 0, 0, 0 );

        // First we do the shadings that require light to be effective
        // i.e. diffuse, specular, etc.
    nLight = 0;
    pLight = oScene.GetLight( nLight );

    while ( pLight )
    {
        if ( pLight->GetSampleCount() > 0 )
        {
            AreaLightShade( oScene, oRay, oIntersect, oNormal, pLight, nLight, oReturn );
            nLight++;
            pLight = oScene.GetLight( nLight );
            continue;
        }

        oToLight = pLight->ToLight( oIntersect );
        oLightRay.CreateRay( oToLight, oIntersect, 1.0, -1, -1 );
        if ( ! oScene.Intersect( oLightRay, this ) )
        {
            nMaterial = 0;
            while ( nMaterial < oMaterials.GetSize() )
            {
                pMaterial = oMaterials[ nMaterial ];
                if ( oMaterials.GetInfluence( nMaterial ) != 0.0 )
                {
                        // Each material gets its own copies: materials adjust
                        // the normal and light vector (normalizing them, turning
                        // the normal toward the viewer), and those changes must
                        // not reach the next material, the next light, or the
                        // object (a plane's normal is its own member).
                    CVector oMaterialNormal( oNormal );
                    CVector oMaterialToLight( oToLight );
                    CVector oPoint( oIntersect );

                    oLight = pLight->GetShade( oToLight );
                    oColor = pMaterial->GetColor( oScene, oRay, oLight, oMaterialToLight, oPoint, oMaterialNormal, this );
                    oReturn = oReturn + ( oColor * oMaterials.GetInfluence( nMaterial ) );
                }
                nMaterial++;
            }
        }
        nLight++;
        pLight = oScene.GetLight( nLight );
    }

    const CEnvironment *pEnvironment = oScene.GetEnvironment();

    if ( pEnvironment && pEnvironment->GetSamples() > 0 )
        EnvironmentShade( oScene, oRay, oIntersect, oNormal, *pEnvironment, oReturn );

        // Do the intersection/shadings that do NOT require light 
        // i.e. reflections, transparency, etc.

    nMaterial = 0;
    while ( nMaterial < oMaterials.GetSize() )
    {
        pMaterial = oMaterials[ nMaterial ];
        if ( oMaterials.GetInfluence( nMaterial ) != 0.0 )
        {
                // The ray's influence on its pixel, scaled by this material's
                // share: a reflected or refracted ray made from it carries that
                // on, so depthThreshold can stop rays that no longer matter.
                // (The color is scaled by the material's influence here, once;
                // it is not scaled by the ray's influence as well, which would
                // count it again at every bounce.)
            CRay    oMaterialRay( oRay );
            CVector oMaterialNormal( oNormal );
            CVector oPoint( oIntersect );

            oMaterialRay.SetInfluence( oRay.GetInfluence() * ABS( oMaterials.GetInfluence( nMaterial ) ) );
            oColor = pMaterial->GetColor( oScene, oMaterialRay, oPoint, oMaterialNormal, this );
            oReturn = oReturn + ( oColor * oMaterials.GetInfluence( nMaterial ) );
        }
        nMaterial++;
    }

    return oReturn;
}

void CObjects::AreaLightShade( CScene &oScene, CRay &oRay, CVector &oIntersect, CVector &oNormal, CLight *pLight, int nLight,
                               CColor &oReturn )
{
    CMaterialList  &oMaterials = m_pShadeMaterials ? *m_pShadeMaterials : m_oMaterials;
        // An area light is shaded from several of its points, each with its
        // own shadow ray and an even share of its light. The points spread
        // from the one the ray's sample point picks, so over a pixel's rays
        // they cover the light evenly.
    int     nSamples = pLight->GetSampleCount();
    FLT     nShare = 1.0 / (FLT)nSamples;
    FLT     nFirstU;
    FLT     nFirstV;

    oRay.SamplePoint( (uint64_t)RaySampling::PURPOSE_LIGHT + (uint64_t)nLight, nFirstU, nFirstV );

    for ( int nSample = 0; nSample < nSamples; nSample++ )
    {
        FLT     nU;
        FLT     nV;
        CVector oPoint;
        CVector oFrom( oIntersect );
        CRay    oLightRay;

        RaySampling::Spread( nFirstU, nFirstV, nSample, nU, nV );
        if ( ! pLight->SampleAt( oFrom, nU, nV, oPoint ) )
            continue;

        CVector oToLight( oPoint - oIntersect );

        oLightRay.CreateRay( oToLight, oIntersect, 1.0, -1, -1 );
        if ( oScene.Intersect( oLightRay, this ) )
            continue;

        CColor  oLight( pLight->GetShade( oToLight ) * nShare );

        for ( int nMaterial = 0; nMaterial < oMaterials.GetSize(); nMaterial++ )
        {
            if ( oMaterials.GetInfluence( nMaterial ) == 0.0 )
                continue;

            CVector oMaterialNormal( oNormal );
            CVector oMaterialToLight( oToLight );
            CVector oMaterialPoint( oIntersect );
            CColor  oColor = oMaterials[ nMaterial ]->GetColor( oScene, oRay, oLight, oMaterialToLight, oMaterialPoint,
                                                                  oMaterialNormal, this );

            oReturn = oReturn + ( oColor * oMaterials.GetInfluence( nMaterial ) );
        }
    }
}

void CObjects::EnvironmentShade( CScene &oScene, CRay &oRay, CVector &oIntersect, CVector &oNormal,
                                 const CEnvironment &oEnvironment, CColor &oReturn )
{
    CMaterialList  &oMaterials = m_pShadeMaterials ? *m_pShadeMaterials : m_oMaterials;
        // The sky's light for the way the surface faces (worked out once,
        // smooth), scaled by the share of the sky the point can see: that
        // many shadow rays toward it, spread by the cosine over the half on
        // the side the ray came from. It goes to the diffuse materials as a
        // light straight along the normal.
    int     nSamples = oEnvironment.GetSamples();
    int     nOpen = 0;
    FLT     nFirstU;
    FLT     nFirstV;
    CVector oUp( oNormal );
    CVector oView( oIntersect.GetX() - oRay.GetX(), oIntersect.GetY() - oRay.GetY(), oIntersect.GetZ() - oRay.GetZ() );
    CVector oFirst;
    CVector oSecond;

    oUp.Normalize();
    if ( oUp.Dot( oView ) > 0.0 )
        oUp = oUp * -1.0;
    RaySampling::Basis( oUp, oFirst, oSecond );
    oRay.SamplePoint( (uint64_t)RaySampling::PURPOSE_ENVIRONMENT, nFirstU, nFirstV );

    for ( int nSample = 0; nSample < nSamples; nSample++ )
    {
        FLT     nU;
        FLT     nV;
        FLT     nX;
        FLT     nY;

        RaySampling::Spread( nFirstU, nFirstV, nSample, nU, nV );
        RaySampling::ConcentricDisc( nU, nV, nX, nY );

        FLT     nRest = 1.0 - ( nX * nX ) - ( nY * nY );
        FLT     nUp = ( nRest > 0.0 ) ? SQRT( nRest ) : 0.0;
        CVector oAcross1( oFirst * nX );
        CVector oAcross2( oSecond * nY );
        CVector oAlong( oUp * nUp );
        CVector oPartway( oAcross1 + oAcross2 );
        CVector oDirection( oPartway + oAlong );
        CVector oToSky( oDirection * RAY_FAR_AWAY );
        CRay    oSkyRay;

        oSkyRay.CreateRay( oToSky, oIntersect, 1.0, -1, -1 );
        if ( ! oScene.Intersect( oSkyRay, this ) )
            nOpen++;
    }
    if ( nOpen == 0 )
        return;

    CColor  oSky( oEnvironment.IrradianceIn( oUp ) );
    CColor  oLight( oSky * ( (FLT)nOpen / (FLT)nSamples ) );

    for ( int nMaterial = 0; nMaterial < oMaterials.GetSize(); nMaterial++ )
    {
        if ( oMaterials.GetInfluence( nMaterial ) == 0.0 || ! oMaterials[ nMaterial ]->LitBySky() )
            continue;

        CVector oMaterialNormal( oNormal );
        CVector oMaterialToLight( oUp );
        CVector oMaterialPoint( oIntersect );
        CColor  oColor = oMaterials[ nMaterial ]->GetColor( oScene, oRay, oLight, oMaterialToLight, oMaterialPoint,
                                                              oMaterialNormal, this );

        oReturn = oReturn + ( oColor * oMaterials.GetInfluence( nMaterial ) );
    }
}

CDynObject::CDynObject( CObjects *(CFUNC *lpCreateEmpty)(void), LPCSTR strKey, LPCSTR strJsonType )
{
    m_lpCreateEmpty = lpCreateEmpty;
    m_strJsonType = strJsonType;
    m_strObjectID = strKey;
    m_pNext = m_pFirst;
    m_pFirst = this;
}

CDynObject::~CDynObject( void )
{
    CDynObject  *pLast;
    CDynObject  *pTemp;

    pLast = NULL;
    pTemp = m_pFirst;

    while ( pTemp != this )
    {
        pLast = pTemp;
        pTemp = pTemp->m_pNext;
    }

    if ( pLast == NULL )
        m_pFirst = m_pNext;
    else
        pLast->m_pNext = m_pNext;
}

#ifdef RAY_GUI
#ifdef _DEBUG
void CObjects::Dump( CDumpContext& dc ) const
{
    dc << "CObjects[ No internal data ]";
}
#endif
#endif

int CDynObject::Compare( CDynObject *pCompare )
{
    return m_strObjectID.Compare( pCompare->m_strObjectID );
}

BOOL CDynObject::Match( LPCSTR lpszCompare )
{
    if ( m_strObjectID.Compare( lpszCompare ) == 0 )
        return TRUE;
    return FALSE;
}

CObjects *CDynObject::MakeEmpty( void )
{
    return m_lpCreateEmpty ? (*m_lpCreateEmpty)() : NULL;
}

CDynObject *CDynObject::GetFirst( void )
{
    return m_pFirst;
}

CDynObject *CDynObject::GetNext( void )
{
    return m_pNext;
}

CDynList::CDynList( void )
{
        // Retrieve the first node of the linked list
    m_pFirst = g_oBaseObject.GetFirst();
}

CDynList::~CDynList( void )
{
}

CObjects *CDynList::Create( LPCSTR strJsonType )
{
    for ( CDynObject *pObject = g_oBaseObject.GetFirst(); pObject; pObject = pObject->GetNext() )
    {
        if ( pObject->GetJsonType() && strJsonType && strcmp( pObject->GetJsonType(), strJsonType ) == 0 )
            return pObject->MakeEmpty();
    }
    return NULL;
}

LPCSTR CDynList::JsonTypeOf( LPCSTR strClassType )
{
    for ( CDynObject *pObject = g_oBaseObject.GetFirst(); pObject; pObject = pObject->GetNext() )
    {
        if ( pObject->Match( strClassType ) )
            return pObject->GetJsonType();
    }
    return NULL;
}

#ifdef __ENGINE__
#define ASSERT_ME
#endif

#ifdef __THROWER__
#define ASSERT_ME
#endif

#ifdef ASSERT_ME
#ifdef RAY_GUI
#include <windows.h>
void ASSERT_ShowError( LPCSTR lpszAssertion, LPCSTR lpszFileName, int nLineNumber )
{
    char        Buffer[ 512 ];

    wsprintf( Buffer, "Assertion Failed:\n%[%s]\nFile: %s\nLine: %d\n\nBreak into code?", lpszAssertion, lpszFileName, nLineNumber );
    if ( ::MessageBox( NULL, Buffer, "ASSERT FAILURE", MB_YESNO | MB_ICONSTOP ) == IDYES )
        DebugBreak();
}
#else
#include <stdio.h>
void ASSERT_ShowError( LPCSTR lpszAssertion, LPCSTR lpszFileName, int nLineNumber )
{
    printf( "Assertion Failed:\n%[%s]\nFile: %s\nLine: %d\n\nBreak into code?", lpszAssertion, lpszFileName, nLineNumber );
    exit(-1);
}
#endif
#endif

