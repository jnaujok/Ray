#ifdef _WINDOWS
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "Objects.h"
#include "PipeStream.h"
#include <string.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class CPipeData
{
    public:
        CString             strPipeName;
        HANDLE              hServerInput;
        HANDLE              hServerOutput;
        CPipeData          *pNext;
        static CPipeData   *pFirst;
        int                 nReferences;

                            CPipeData( LPCSTR strName, HANDLE hInput, HANDLE hOutput )
                            {
                                strPipeName = strName;
                                hServerInput = hInput;
                                hServerOutput = hOutput;
                                pNext = pFirst;
                                pFirst = this;
                                nReferences = 1;
                            }

                           ~CPipeData( void )
                            {
                               CloseHandle( hServerInput );
                               CloseHandle( hServerOutput );
                            }

        static CPipeData   *Find( LPCSTR strName )
                            {
                                CPipeData *pData;

                                pData = pFirst;
                                while ( pData )
                                {
                                    if ( pData->strPipeName == strName )
                                        return pData;
                                    pData = pData->pNext;
                                }
                                return pData;
                            }

        void                AddRef( void )
                            {
                                nReferences++;
                            }

        static void         Release( LPCSTR strName )
                            {
                                CPipeData *pData;
                                CPipeData *pLast = NULL;

                                pData = pFirst;
                                while ( pData )
                                {
                                    if ( pData->strPipeName == strName )
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

CPipeData *CPipeData::pFirst = NULL;

CPipeStream::CPipeStream( void )
{
    m_strPipeName = "";
    m_hInputPipe = NULL;
    m_hOutputPipe = NULL;
}

CPipeStream::CPipeStream( LPCSTR strPipeName, BOOL bClient )
{
    CPipeData  *pPipeData;

    m_strPipeName = strPipeName;

    pPipeData = CPipeData::Find( strPipeName );

    if ( ! pPipeData )
    {
        HANDLE hInput;
        HANDLE hOutput;

        CreatePipe( &hInput, &hOutput, NULL, 65536 );
        pPipeData = new CPipeData( strPipeName, hInput, hOutput );
    }
    else
        pPipeData->AddRef();

    m_hInputPipe = pPipeData->hServerInput;
    m_hOutputPipe = pPipeData->hServerOutput;
}

CPipeStream::~CPipeStream( void )
{
    CPipeData::Release( m_strPipeName );
}

void CPipeStream::Attach( LPCSTR strPipeName, BOOL bClient )
{
    CPipeData  *pPipeData;

    m_strPipeName = strPipeName;

    pPipeData = CPipeData::Find( strPipeName );

    if ( ! pPipeData )
    {
        HANDLE hInput;
        HANDLE hOutput;

        CreatePipe( &hInput, &hOutput, NULL, 65536 );
        pPipeData = new CPipeData( strPipeName, hInput, hOutput );
    }
    else
        pPipeData->AddRef();

    m_hInputPipe = pPipeData->hServerInput;
    m_hOutputPipe = pPipeData->hServerOutput;
}



void CPipeStream::Write( char nChar )
{
    DWORD   dwTemp;

    WriteFile( m_hOutputPipe, &nChar, (DWORD)sizeof( char ), &dwTemp, NULL );
}

void CPipeStream::Write( unsigned char nUChar )
{
    DWORD   dwTemp;

    WriteFile( m_hOutputPipe, &nUChar, (DWORD)sizeof( unsigned char ), &dwTemp, NULL );
}

void CPipeStream::Write( short nShort )
{
    DWORD   dwTemp;

    WriteFile( m_hOutputPipe, &nShort, (DWORD)sizeof( short ), &dwTemp, NULL );
}

void CPipeStream::Write( unsigned short nUShort )
{
    DWORD   dwTemp;

    WriteFile( m_hOutputPipe, &nUShort, (DWORD)sizeof( unsigned short ), &dwTemp, NULL );
}

void CPipeStream::Write( unsigned long nULong )
{
    DWORD   dwTemp;

    WriteFile( m_hOutputPipe, &nULong, (DWORD)sizeof( unsigned long ), &dwTemp, NULL );
}

void CPipeStream::Write( long nLong )
{
    DWORD   dwTemp;

    WriteFile( m_hOutputPipe, &nLong, (DWORD)sizeof( long ), &dwTemp, NULL );
}

void CPipeStream::Write( float nFloat )
{
    DWORD   dwTemp;

    WriteFile( m_hOutputPipe, &nFloat, (DWORD)sizeof( float ), &dwTemp, NULL );
}

void CPipeStream::Write( double nDouble )
{
/*
    char            Buffer[ 64 ];

    sprintf( Buffer, "%.18e", nDouble );

    Write( Buffer );
*/
    DWORD   dwTemp;

    WriteFile( m_hOutputPipe, &nDouble, (DWORD)sizeof( double ), &dwTemp, NULL );
}

void CPipeStream::Write( LPCSTR strString )
{
    unsigned short  nLength;
    int             nCount;

    nLength = strlen( strString );
    Write( nLength );

    nCount = 0;
    while ( nCount < nLength )
    {
        Write( strString[ nCount ] );
        nCount++;
    }
}

void CPipeStream::Read( char &nChar )
{
    DWORD       dwTemp;

    ReadFile( m_hInputPipe, &nChar, (DWORD)sizeof( char ), &dwTemp, NULL );
}

void CPipeStream::Read( unsigned char &nUChar )
{
    DWORD       dwTemp;

    ReadFile( m_hInputPipe, &nUChar, (DWORD)sizeof( unsigned char ), &dwTemp, NULL );
}

void CPipeStream::Read( short &nShort )
{
    DWORD       dwTemp;

    ReadFile( m_hInputPipe, &nShort, (DWORD)sizeof( short ), &dwTemp, NULL );
}

void CPipeStream::Read( unsigned short &nUShort )
{
    DWORD       dwTemp;

    ReadFile( m_hInputPipe, &nUShort, (DWORD)sizeof( unsigned short ), &dwTemp, NULL );
}

void CPipeStream::Read( unsigned long &nULong )
{
    DWORD       dwTemp;

    ReadFile( m_hInputPipe, &nULong, (DWORD)sizeof( unsigned long ), &dwTemp, NULL );
}

void CPipeStream::Read( long &nLong )
{
    DWORD       dwTemp;

    ReadFile( m_hInputPipe, &nLong, (DWORD)sizeof( long ), &dwTemp, NULL );
}

void CPipeStream::Read( float &nFloat )
{
    DWORD       dwTemp;

    ReadFile( m_hInputPipe, &nFloat, (DWORD)sizeof( float ), &dwTemp, NULL );
}

void CPipeStream::Read( double &nDouble )
{
/*
    CString     strValue;
    char        *stop;

    Read( strValue );

    nDouble = strtod( strValue, &stop );
*/
    DWORD       dwTemp;

    ReadFile( m_hInputPipe, &nDouble, (DWORD)sizeof( double ), &dwTemp, NULL );
}

void CPipeStream::Read( CString &strString )
{
    unsigned short  nLength;
    int             nCount;
    char            chTemp;

    strString = "";
    Read( nLength );

    nCount = 0;
    while ( nCount < nLength )
    {
        Read( chTemp );
        strString += chTemp;
        nCount++;
    }
}

void CPipeStream::Flush( void )
{
    FlushFileBuffers( m_hOutputPipe );
}

BOOL CPipeStream::DataAvailable( void )
{
    return TRUE;
}


