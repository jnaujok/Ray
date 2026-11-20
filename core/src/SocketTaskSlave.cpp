#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "Scene.h"
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
    m_oDecoder( RAY_WIRE_ENGINE_ID, RAY_WIRE_MASTER_ID )
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

CTaskSlave::EMessage CSocketTaskSlave::FromMaster( std::unique_ptr<CScene> &pScene, STile &oTile, uint32_t &nTrace )
{
    uint8_t     nTag;
    std::string strPayload;

    if ( m_pStream == NULL )
        return FROM_MASTER_ERROR;

    try
    {
        if ( ! RayWire::ReceiveFrame( *m_pStream, nTag, strPayload ) )
        {
            m_strLastError = "The master closed the connection";
            return FROM_MASTER_ERROR;
        }
    }
    catch ( IASLib::CException &oException )
    {
        m_strLastError = oException.GetMessage();
        return FROM_MASTER_ERROR;
    }

    switch ( nTag )
    {
        case MESSAGE_SCENE:
            pScene.reset( new CScene );
            if ( ! m_oDecoder.Scene( strPayload, *pScene ) )
            {
                m_strLastError.Format( "The scene from the master could not be read: %s", m_oDecoder.GetError().c_str() );
                pScene.reset();
                return FROM_MASTER_ERROR;
            }
            return FROM_MASTER_SCENE;

        case MESSAGE_TILE:
            if ( ! RayWire::DecodeTile( strPayload, nTrace, oTile ) )
            {
                m_strLastError = "A tile from the master could not be read";
                return FROM_MASTER_ERROR;
            }
            return FROM_MASTER_TILE;

        case MESSAGE_STOP:
            return FROM_MASTER_STOP;

        default:
            m_strLastError.Format( "Unexpected message %d from the master", (int)nTag );
            return FROM_MASTER_ERROR;
    }
}

BOOL CSocketTaskSlave::Send( uint8_t nTag, const std::string &strPayload )
{
    if ( m_pStream == NULL )
        return FALSE;
    try
    {
        RayWire::SendFrame( *m_pStream, nTag, strPayload );
    }
    catch ( IASLib::CException &oException )
    {
        m_strLastError = oException.GetMessage();
        return FALSE;
    }
    return TRUE;
}

BOOL CSocketTaskSlave::RequestWork( void )
{
    return Send( MESSAGE_REQUEST, std::string() );
}

BOOL CSocketTaskSlave::PixelsToMaster( uint32_t nTrace, const STile &oTile, long long nRays, const std::vector<double> &anColors )
{
    return Send( MESSAGE_PIXELS, RayWire::EncodePixels( nTrace, oTile, (uint64_t)nRays, anColors ) );
}
