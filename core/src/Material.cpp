/*************************************************************************
*   CMaterial                                                            *
*                                                                        *
*       This is the base class for all of the object materials, which    *
* include things like diffuse materials, reflective materials, texture   *
* maps, bump maps and the like.                                          *
*                                                                        *
* Copyright (c) 1997, Jeffrey R. Naujok                                  *
*************************************************************************/

    #include "Globals.h"

#include "Material.h"
#include "Objects.h"
#include "SceneJson.h"

#include <string.h>


CDynMaterialList    *g_pMaterialList = NULL;

CDynMaterial *CDynMaterial::m_pFirst = NULL;

static CDynMaterial   g_oBaseMaterial( NULL, "Base Material", "Base Material", NULL );

CDynMaterial::CDynMaterial( LPADDFUNC lpAddFunc, LPCSTR strKey, LPCSTR strName, LPCSTR strJsonType )
{
    m_lpAddFunc = lpAddFunc;
    m_strJsonType = strJsonType;
    m_strObjectID = strKey;
    m_strAddName = strName;
    m_pNext = m_pFirst;
    m_pFirst = this;
}

CDynMaterial::~CDynMaterial( void )
{
    CDynMaterial  *pLast;
    CDynMaterial  *pTemp;

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

int CDynMaterial::Compare( CDynMaterial *pCompare )
{
    return m_strObjectID.Compare( pCompare->m_strObjectID );
}

BOOL CDynMaterial::Match( LPCSTR lpszCompare )
{
    if ( m_strObjectID.Compare( lpszCompare ) == 0 )
        return TRUE;
    return FALSE;
}

CDynMaterial *CDynMaterial::GetFirst( void )
{
    return m_pFirst;
}

CDynMaterial *CDynMaterial::GetNext( void )
{
    return m_pNext;
}

CDynMaterialList::CDynMaterialList( void )
{
        // Retrieve the first node of the linked list
    m_pFirst = g_oBaseMaterial.GetFirst();
}

CDynMaterialList::~CDynMaterialList( void )
{
}

CMaterial *CDynMaterialList::Create( LPCSTR strJsonType )
{
    for ( CDynMaterial *pObject = m_pFirst; pObject; pObject = pObject->GetNext() )
    {
        if ( pObject->GetJsonType() && strJsonType && strcmp( pObject->GetJsonType(), strJsonType ) == 0 )
            return pObject->GetAddFunc() ? (*pObject->GetAddFunc())() : NULL;
    }
    return NULL;
}

LPCSTR CDynMaterialList::JsonTypeOf( LPCSTR strClassType )
{
    for ( CDynMaterial *pObject = m_pFirst; pObject; pObject = pObject->GetNext() )
    {
        if ( pObject->Match( strClassType ) )
            return pObject->GetJsonType();
    }
    return NULL;
}


CColor CMaterial::ColorAt( const CColor &oColor, CVector &oPoint, CObjects *pObject )
{
    if ( ! m_pTexture )
        return oColor;

    STexturePoint oAt;

    if ( pObject )
        pObject->TextureAt( oPoint, oAt );
    else
    {
        oAt.oPoint = oPoint;
        oAt.nU = oAt.nV = 0.0;
        oAt.bHasUV = FALSE;
        oAt.bHasColor = FALSE;
    }
    return m_pTexture->ColorAt( oAt );
}

CColor CMaterial::ColorAt( const CColorMap &oColor, CVector &oPoint, CObjects *pObject )
{
    if ( m_pTexture || ! oColor.IsMapped() )
        return ColorAt( oColor.Get(), oPoint, pObject );

    CMapPoint oAt( oPoint, pObject );

    return oColor.At( oAt );
}

void CMaterial::TextureToJson( IASLib::CObjectNode &oNode ) const
{
    if ( m_pTexture )
        m_pTexture->Write( oNode, "texture" );
}

BOOL CMaterial::TextureFromJson( const IASLib::CJsonNode &oNode )
{
    const IASLib::CJsonNode *pTexture = oNode.isObject() ? oNode.get( "texture" ) : NULL;

    if ( pTexture == NULL )
        return TRUE;

    RayJson::CWhere oWhere( "texture" );
    CTexture       *pNew = CTexture::Read( *pTexture );

    if ( pNew == NULL )
        return FALSE;
    m_pTexture.reset( pNew );
    return TRUE;
}
