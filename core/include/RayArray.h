#ifndef __RAYARRAY_H__
#define __RAYARRAY_H__

#include "Rays.h"
#include "Templates/TArray.h"

class CRayStream;

class CRayArray
{
    protected:
        IASLib::TArray<CRay>    m_aoItems;
        CRay                    m_oEmpty;

    public:
        explicit        CRayArray( int nSize = 16 );
        virtual        ~CRayArray( void ) {}

        int             Add( const CRay &oRay );
        BOOL            Delete( int nIndex );
        int             Insert( const CRay &oRay, int nIndex );
        CRay           &GetAt( int nIndex );
        CRay           &operator[] ( int nIndex ) { return GetAt( nIndex ); }
        void            RemoveAll( void ) { m_aoItems.Clear(); }
        int             GetSize( void ) const { return (int)m_aoItems.Length(); }

        friend CRayStream  &operator >>( CRayStream &oStream, CRayArray &oRay );
        friend CRayStream  &operator <<( CRayStream &oStream, CRayArray &oRay );
};

#endif // __RAYARRAY_H__
