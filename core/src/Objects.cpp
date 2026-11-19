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

#include <string.h>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CDynObject *CDynObject::m_pFirst = NULL;

static CDynObject   g_oBaseObject( NULL, "Base Object" );

    // The registry of object classes. It is a static object, so it exists
    // in every program (it used to be created only by the GUI), and it reads
    // the registrations when it is used, so the order in which static
    // objects are built does not matter.
static CDynList g_oDynamicList;
CDynList    *g_pDynamicList = &g_oDynamicList;

CObjects::CObjects( void )
{
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
    while ( nIndex < m_oMaterials.GetSize() )
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

CColor CObjects::BaseShade( CScene &oScene, CRay &oRay, CVector &oIntersect, CVector &oNormal )
{
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
        oToLight = pLight->GetPosition() - oIntersect;
        oLightRay.CreateRay( oToLight, oIntersect, 1.0, -1, -1 );
        if ( ! oScene.Intersect( oLightRay, this ) )
        {
            nMaterial = 0;
            while ( nMaterial < m_oMaterials.GetSize() )
            {
                pMaterial = m_oMaterials[ nMaterial ];
                if ( m_oMaterials.GetInfluence( nMaterial ) != 0.0 )
                {
                    oLight = pLight->GetShade( oToLight );
                    oColor = pMaterial->GetColor( oScene, oRay, oLight, oToLight, oIntersect, oNormal, this );
                    oReturn = oReturn + ( oColor * m_oMaterials.GetInfluence( nMaterial ) );
                }
                nMaterial++;
            }
        }
        nLight++;
        pLight = oScene.GetLight( nLight );
    }

        // Do the intersection/shadings that do NOT require light 
        // i.e. reflections, transparency, etc.

    nMaterial = 0;
    while ( nMaterial < m_oMaterials.GetSize() )
    {
        pMaterial = m_oMaterials[ nMaterial ];
        if ( m_oMaterials.GetInfluence( nMaterial ) != 0.0 )
        {
            oColor = pMaterial->GetColor( oScene, oRay, oIntersect, oNormal, this );
            oReturn = oReturn + ( oColor * m_oMaterials.GetInfluence( nMaterial ) );
        }
        nMaterial++;
    }

    oReturn = oReturn * oRay.GetInfluence();
    return oReturn;
}

CDynObject::CDynObject( CObjects *(CFUNC *lpFunction)(CRayStream &oStream), LPCSTR strKey )
{
    m_lpFunction = lpFunction;
    m_lpCreateEmpty = NULL;
    m_strJsonType = NULL;
    m_strObjectID = strKey;
    m_pNext = m_pFirst;
    m_pFirst = this;
}

CDynObject::CDynObject( CObjects *(CFUNC *lpFunction)(CRayStream &oStream), CObjects *(CFUNC *lpCreateEmpty)(void),
                        LPCSTR strKey, LPCSTR strJsonType )
{
    m_lpFunction = lpFunction;
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

CObjects *CDynObject::MakeMe( CRayStream &oStream )
{
    return (CObjects *)(*m_lpFunction)( oStream );
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

CObjects *CDynList::Instantiate( LPCSTR lpszName, CRayStream &oStream  )
{
    CDynObject     *pObject;
    CObjects       *pReturn = NULL;

    pObject = g_oBaseObject.GetFirst();
    while ( ( pObject ) && ( ! pObject->Match( lpszName ) ) )
    {
        pObject = pObject->GetNext();
    }

    if ( pObject )
    {
        pReturn = pObject->MakeMe( oStream );
    }

    return pReturn;
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

