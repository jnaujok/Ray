/*************************************************************************
*   CMaterial                                                            *
*                                                                        *
*       This is the base class for all of the object materials, which    *
* include things like diffuse materials, reflective materials, texture   *
* maps, bump maps and the like.                                          *
*                                                                        *
* Copyright (c) 1997, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef _WINDOWS
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Material.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDynMaterialList    *g_pMaterialList = NULL;

CDynMaterial *CDynMaterial::m_pFirst = NULL;

static CDynMaterial   g_oBaseMaterial( NULL, NULL, "Base Material", "Base Material" );

CDynMaterial::CDynMaterial( CMaterial *(CFUNC *lpFunction)(CStream &oStream), LPADDFUNC lpAddFunc, LPCSTR strKey, LPCSTR strName )
{
    m_lpFunction = lpFunction;
    m_lpAddFunc = lpAddFunc;
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

CMaterial *CDynMaterial::MakeMe( CStream &oStream )
{
    return (CMaterial *)(*m_lpFunction)( oStream );
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

CMaterial *CDynMaterialList::Instantiate( LPCSTR lpszName, CStream &oStream  )
{
    CDynMaterial   *pObject;
    CMaterial      *pReturn = NULL;

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

#ifdef _INC_WINDOWS
void CDynMaterialList::LoadAddList( CListCtrl &oList )
{
    int             nIndex;
    CDynMaterial   *pObject;

    nIndex = 0;
    pObject = m_pFirst;
    while ( pObject )
    {
        if ( pObject != &g_oBaseMaterial )
        {
            oList.InsertItem( nIndex, pObject->GetAddName() );
            oList.SetItemData( nIndex, (LPARAM)pObject->GetAddFunc() );
            nIndex++;
        }
        pObject = pObject->GetNext();
    }
}
#endif
