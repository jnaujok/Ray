#ifdef _WINDOWS
    #include "StdAfx.h"
#else
    #include "Globals.h"
    #include <string.h>
#endif

#include "PVMStream.h"
extern "C"
{
#include "pvm3.h"
}
// Include Greg Wolfe's Resource Monitor?

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPVMStream::CPVMStream( int TaskID, int nMessageTag, BOOL bSending )
{
    m_bSending = bSending;
    m_nTaskID = TaskID;
    m_nMessageTag = nMessageTag;
    m_anTasks = NULL;

    if ( bSending )
    {
        m_nBufID = pvm_initsend( PvmDataDefault );
    }
    else
    {
        m_nBufID = pvm_recv( m_nTaskID, m_nMessageTag ); 
    }

}

CPVMStream::CPVMStream( int *anTaskList, int nTasks, int nMessageTag )
{
    m_bSending = TRUE;
    m_nTaskID = nTasks;
    m_nMessageTag = nMessageTag;
    m_anTasks = anTaskList;
}


CPVMStream::~CPVMStream( void )
{
    Flush();
}

void CPVMStream::Write( char nChar )
{
    pvm_pkbyte( &nChar, 1, 1 );
}

void CPVMStream::Write( unsigned char nUChar )
{
    pvm_pkbyte( (char *)&nUChar, 1, 1 );
}

void CPVMStream::Write( short nShort )
{
    pvm_pkshort( &nShort, 1, 1 );
}

void CPVMStream::Write( unsigned short nUShort )
{
    pvm_pkshort( (short *)&nUShort, 1, 1 );
}

void CPVMStream::Write( unsigned long nULong )
{
    pvm_pklong( (long *)&nULong, 1, 1 );
}

void CPVMStream::Write( long nLong )
{
    pvm_pklong( &nLong, 1, 1 );
}

void CPVMStream::Write( float nFloat )
{
    pvm_pkfloat( &nFloat, 1, 1 );
}

void CPVMStream::Write( double nDouble )
{
    pvm_pkdouble( &nDouble, 1, 1 );
}

void CPVMStream::Write( LPCSTR strString )
{
    unsigned short  nLength;

    nLength = strlen( strString );
    pvm_pkshort( (short *)&nLength, 1, 1 );
    pvm_pkbyte( (char *)strString, nLength, 1 );
}

void CPVMStream::Read( char &nChar )
{
    pvm_upkbyte( &nChar, 1, 1 );
}

void CPVMStream::Read( unsigned char &nUChar )
{
    pvm_upkbyte( (char *)&nUChar, 1, 1 );
}

void CPVMStream::Read( short &nShort )
{
    pvm_upkshort( &nShort, 1, 1 );
}

void CPVMStream::Read( unsigned short &nUShort )
{
    pvm_upkshort( (short *)&nUShort, 1, 1 );
}

void CPVMStream::Read( unsigned long &nULong )
{
    pvm_upklong( (long *)&nULong, 1, 1 );
}

void CPVMStream::Read( long &nLong )
{
    pvm_upklong( &nLong, 1, 1 );
}

void CPVMStream::Read( float &nFloat )
{
    pvm_upkfloat( &nFloat, 1, 1 );
}

void CPVMStream::Read( double &nDouble )
{
    pvm_upkdouble( &nDouble, 1, 1 );
}

void CPVMStream::Read( CString &strString )
{
    short       nLength;
    char        chChar;

    pvm_upkshort( &nLength, 1, 1 );
    strString = "";
    while ( nLength )
    {
        pvm_upkbyte( &chChar, 1, 1 );
        strString += chChar;
        nLength--;
    }
}

void CPVMStream::Flush( void )
{
    int     nCount;

    if ( m_bSending )
    {
        if ( m_anTasks )
        {
                // We don't use multicast because it doesn't assure the order of reception
            nCount = 0;
                // Task ID is the number of tasks for a multi-cast message
            while ( nCount < m_nTaskID )
            {
                pvm_send( m_anTasks[ nCount ], m_nMessageTag );
                nCount++;
            }
        }
        else
            pvm_send( m_nTaskID, m_nMessageTag );
    }
}

BOOL CPVMStream::DataAvailable( void )
{
    return TRUE;
}

