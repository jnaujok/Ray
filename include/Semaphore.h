#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

class CSem
{
    protected:
        int        *m_pnCount;
        HANDLE      m_hMutex;
        CString     m_strName;
    public:
                    CSem( LPCSTR strName );
        virtual    ~CSem( void );

        BOOL        IsLocked( void );
        int         Value( void );
        BOOL        Wait( DWORD dwTimeout = 100000000 );
        int         Signal( void );
        LPCSTR      GetName( void ) { return (LPCSTR)m_strName; }

        void        Destroy( void );
};

#endif
