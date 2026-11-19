#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "ObjectsArray.h"
#include "Objects.h"

CObjectsArray::CObjectsArray( int nSize )
{
    if ( nSize > 0 )
        m_apoItems.Reserve( (size_t)nSize );
}

int CObjectsArray::Add( CObjects *pObject )
{
    return (int)m_apoItems.Push( pObject );
}

BOOL CObjectsArray::Delete( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= GetSize() ) )
        return FALSE;
    m_apoItems.Remove( (size_t)nIndex );
    return TRUE;
}

int CObjectsArray::Insert( CObjects *pObject, int nIndex )
{
    if ( nIndex < 0 )
        nIndex = 0;
    if ( nIndex > GetSize() )
        nIndex = GetSize();
    return (int)m_apoItems.Insert( (size_t)nIndex, pObject );
}

CObjects *CObjectsArray::GetAt( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= GetSize() ) )
        return NULL;
    return m_apoItems[ (size_t)nIndex ];
}

void CObjectsArray::RemoveAll( void )
{
    m_apoItems.DeleteAll();
}
