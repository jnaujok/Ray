#include "StdAfx.h"
#include "Semaphore.h"

class CSemaphoreData
{
    public:
        CString             strSemName;
        CSem               *poSemaphore;
        CSemaphoreData     *pNext;
        static CSemaphoreData   *pFirst;
        int                 nReferences;

                            CSemaphoreData( LPCSTR strName, CSem *poSem ) :
                            poSemaphore( poSem )
                            {
                                strSemName = strName;
                                pNext = pFirst;
                                pFirst = this;
                                nReferences = 1;
                            }

                           ~CSemaphoreData( void )
                            {
                               poSemaphore->Destroy();
                            }

        static CSem        *Find( LPCSTR strName )
                            {
                                CSemaphoreData *pData;

                                pData = pFirst;
                                while ( pData )
                                {
                                    if ( pData->strSemName == strName )
                                    {
                                        pData->AddRef();
                                        return pData->poSemaphore;
                                    }
                                    pData = pData->pNext;
                                }
                                return NULL;
                            }

        void                AddRef( void )
                            {
                                nReferences++;
                            }

        static void         Release( LPCSTR strName )
                            {
                                CSemaphoreData *pData;
                                CSemaphoreData *pLast = NULL;

                                pData = pFirst;
                                while ( pData )
                                {
                                    if ( pData->strSemName == strName )
                                    {
                                        pData->nReferences--;
                                        if ( pData->nReferences == 0 )
                                        {
                                            if ( pLast )
                                            {
                                                pLast->pNext = pData->pNext;
                                            }
                                            else
                                            {
                                                pFirst = pData->pNext;
                                            }
                                            delete pData;
                                            return;
                                        }
                                    }
                                    pLast = pData;
                                    pData = pData->pNext;
                                }
                            }
};

CSemaphoreData *CSemaphoreData::pFirst = NULL;



CSem::CSem( LPCSTR strName )
{
    CSem        *pSem;

    m_strName = strName;

    pSem = CSemaphoreData::Find( strName );

    if ( pSem )
    {
        m_hMutex = pSem->m_hMutex;
        m_pnCount = pSem->m_pnCount;
    }
    else
    {
        m_hMutex = CreateMutex( NULL, TRUE, strName );
        m_pnCount = new int( 0 );
        new CSemaphoreData( strName, this );
        ReleaseMutex( m_hMutex );
    }
}

CSem::~CSem( void )
{
    CSemaphoreData::Release( m_strName );
}

void CSem::Destroy( void )
{
    CloseHandle( m_hMutex );
    delete m_pnCount;
}

BOOL CSem::IsLocked( void )
{
    BOOL bRetVal;
    if ( WaitForSingleObject( m_hMutex, 1000 ) != WAIT_FAILED )
    {
        bRetVal = ( *m_pnCount == 0 );
        ReleaseMutex( m_hMutex );
    }
    else
        bRetVal = ( *m_pnCount == 0 );
    return bRetVal;
}

int CSem::Value( void )
{
    int nRetVal;
    if ( WaitForSingleObject( m_hMutex, 1000 ) != WAIT_FAILED )
    {
        nRetVal = *m_pnCount;
        ReleaseMutex( m_hMutex );
    }
    else
        nRetVal = *m_pnCount;
    return nRetVal;
}


BOOL CSem::Wait( DWORD dwTimeout )
{
    DWORD   dwStart = GetTickCount();
    BOOL    bDone = FALSE;

    do
    {
        while ( ( *m_pnCount == 0 ) && ( (GetTickCount() - dwStart ) < dwTimeout ) )
            Sleep(1);
        if ( ( GetTickCount() - dwStart ) > dwTimeout )
            return FALSE;

        if ( WaitForSingleObject( m_hMutex, dwTimeout - ( GetTickCount() - dwStart ) ) == WAIT_FAILED )
            return FALSE;

        if ( *m_pnCount )
        {
            (*m_pnCount)--;
            bDone = TRUE;
        }

        ReleaseMutex( m_hMutex );
    } while ( ! bDone );

    return bDone;
}

int CSem::Signal( void )
{
    int nRet;
    WaitForSingleObject( m_hMutex, INFINITE );
    (*m_pnCount)++;
    nRet = *m_pnCount;
    ReleaseMutex( m_hMutex );
    return nRet;
}

