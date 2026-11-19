/*************************************************************************
*   CObjects                                                             *
*                                                                        *
*       This is the base class for all of the three-dimensional objects, *
* including objects like the camera and lights.                          *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef _WINDOWS
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Scene.h"
#include "Light.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDynList    *g_pDynamicList = NULL;

CDynObject *CDynObject::m_pFirst = NULL;

static CDynObject   g_oBaseObject( NULL, "Base Object" );

CObjects::CObjects( void )
{
#ifdef _WINDOWS
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

#ifdef _WINDOWS
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
                    oColor = pMaterial->GetColor( oScene, oRay, pLight->GetShade( oToLight ), oToLight, oIntersect, oNormal, this );
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

CDynObject::CDynObject( CObjects *(CFUNC *lpFunction)(CStream &oStream), LPCSTR strKey )
{
    m_lpFunction = lpFunction;
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

#ifdef _WINDOWS
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

CObjects *CDynObject::MakeMe( CStream &oStream )
{
    return (CObjects *)(*m_lpFunction)( oStream );
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

CObjects *CDynList::Instantiate( LPCSTR lpszName, CStream &oStream  )
{
    CDynObject     *pObject;
    CObjects       *pReturn = NULL;

    pObject = m_pFirst;
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

#ifdef __ENGINE__
#define ASSERT_ME
#endif

#ifdef __THROWER__
#define ASSERT_ME
#endif

#ifdef ASSERT_ME
#ifdef _WINDOWS
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

