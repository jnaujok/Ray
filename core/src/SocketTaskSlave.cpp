#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "Scene.h"
#include "RayArray.h"
#include "ColorArray.h"
#include "SocketTaskSlave.h"
#include "WireProtocol.h"

#include "Exceptions/Exception.h"
#include "Sockets/ClientSocket.h"
#include "Streams/SocketStream.h"
#include "Threading/Thread.h"

#include <errno.h>
#include <string.h>
#include <time.h>

CSocketTaskSlave::CSocketTaskSlave( void ) :
    m_pStream( NULL ),
    m_oEncoder( RAY_WIRE_ENGINE_ID ),
    m_oDecoder( RAY_WIRE_ENGINE_ID, RAY_WIRE_MASTER_ID ),
    m_nRaysTraced( 0 )
{
}

CSocketTaskSlave::~CSocketTaskSlave( void )
{
    delete m_pStream;       // closes and deletes the socket
}

BOOL CSocketTaskSlave::Connect( const char *strHost, int nPort, int nRetrySeconds )
{
    time_t  nGiveUp = time( NULL ) + nRetrySeconds;

    delete m_pStream;
    m_pStream = NULL;

    for ( ;; )
    {
        IASLib::CSocket *pSocket = NULL;

        try
        {
            pSocket = new IASLib::CClientSocket( strHost, nPort );
        }
        catch ( IASLib::CException &oException )
        {
            m_strLastError = oException.GetMessage();
            pSocket = NULL;
        }

        if ( pSocket && pSocket->IsConnected() )
        {
            m_pStream = new IASLib::CSocketStream( pSocket );
            break;
        }

        int nError = errno;

        delete pSocket;
        if ( time( NULL ) >= nGiveUp )
        {
            m_strLastError.Format( "Could not connect to %s:%d (%s)", strHost, nPort, strerror( nError ) );
            return FALSE;
        }
        IASLib::CThread::Millisleep( 250 );
    }

    try
    {
        RayWire::SendFrame( *m_pStream, MESSAGE_HELLO, RayWire::EncodeHello() );
    }
    catch ( IASLib::CException &oException )
    {
        m_strLastError = oException.GetMessage();
        return FALSE;
    }
    return TRUE;
}

BOOL CSocketTaskSlave::Receive( uint8_t nExpected, std::string &strPayload )
{
    uint8_t nTag;

    if ( m_pStream == NULL )
        return FALSE;

    try
    {
        if ( ! RayWire::ReceiveFrame( *m_pStream, nTag, strPayload ) )
        {
            m_strLastError = "The master closed the connection";
            return FALSE;
        }
    }
    catch ( IASLib::CException &oException )
    {
        m_strLastError = oException.GetMessage();
        return FALSE;
    }

    if ( nTag != nExpected )
    {
        m_strLastError.Format( "Expected message %d from the master, got %d", (int)nExpected, (int)nTag );
        return FALSE;
    }
    return TRUE;
}

BOOL CSocketTaskSlave::SceneFromMaster( CScene &oScene )
{
    std::string strPayload;

    if ( ! Receive( MESSAGE_SCENE, strPayload ) )
        return FALSE;

    if ( ! m_oDecoder.Scene( strPayload, oScene ) )
    {
        m_strLastError.Format( "The scene from the master could not be read: %s", m_oDecoder.GetError().c_str() );
        return FALSE;
    }
    return TRUE;
}

BOOL CSocketTaskSlave::RaysFromMaster( CRayArray &oRays )
{
    std::string strPayload;

    oRays.RemoveAll();
    if ( ! Receive( MESSAGE_RAYS, strPayload ) )
        return FALSE;

    if ( ! m_oDecoder.Rays( strPayload, oRays ) )
    {
        m_strLastError.Format( "Rays from the master could not be read: %s", m_oDecoder.GetError().c_str() );
        return FALSE;
    }
    return ( oRays.GetSize() != 0 );
}

BOOL CSocketTaskSlave::ColorsToMaster( CColorArray &oColors )
{
    try
    {
        RayWire::SendFrame( *m_pStream, MESSAGE_COLOR, m_oEncoder.Colors( oColors ) );
    }
    catch ( IASLib::CException &oException )
    {
        m_strLastError = oException.GetMessage();
        return FALSE;
    }
    m_nRaysTraced += (unsigned long)oColors.GetSize();
    return TRUE;
}
