#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "Scene.h"
#include "SocketTaskMaster.h"
#include "WireProtocol.h"

#include "Exceptions/Exception.h"
#include "Sockets/ServerSocket.h"
#include "Sockets/Socket.h"
#include "Streams/SocketStream.h"
#include "Threading/Thread.h"

#include <stdio.h>

    // How often the background acceptor looks up from waiting to see if it
    // should stop.
#define ACCEPT_POLL_MS      250

    // One connected rayengine. The master thread writes to it; a reader
    // thread takes what it sends back (requests and pixels) and posts it to
    // the master. Each direction is its own SJSON stream, so each has its
    // own encoder or decoder, used by one thread only.
class CSocketTaskMaster::CConnection : public IASLib::CThread
{
    protected:
        CSocketTaskMaster          &m_oMaster;
        int                         m_nTask;
        IASLib::CSocket            *m_pSocket;
        IASLib::CSocketStream       m_oOut;     // owns the socket
        IASLib::CSocketStream       m_oIn;      // shares it

    public:
        CConnection( CSocketTaskMaster &oMaster, IASLib::CSocket *pSocket ) :
            IASLib::CThread( "rayengine connection", true ),
            m_oMaster( oMaster ),
            m_nTask( -1 ),
            m_pSocket( pSocket ),
            m_oOut( pSocket ),
            m_oIn( pSocket )
        {
            m_oIn.SetNoDelete();
            SetNoDelete( true );
        }

        virtual ~CConnection( void ) {}

        void SetTask( int nTask ) { m_nTask = nTask; }

            /// Reads the engine's hello. Call before Start().
        bool Handshake( void )
        {
            uint8_t     nTag;
            std::string strPayload;

            if ( ! RayWire::ReceiveFrame( m_oIn, nTag, strPayload ) )
                return false;
            return ( nTag == MESSAGE_HELLO ) && RayWire::DecodeHello( strPayload );
        }

        void SendScene( const std::string &strPacket )
        {
            RayWire::SendFrame( m_oOut, MESSAGE_SCENE, strPacket );
        }

        void SendTile( uint32_t nTrace, const STile &oTile )
        {
            RayWire::SendFrame( m_oOut, MESSAGE_TILE, RayWire::EncodeTile( nTrace, oTile ) );
        }

        void SendStop( void )
        {
            RayWire::SendFrame( m_oOut, MESSAGE_STOP, std::string() );
        }

            /// Stops the reader by closing the connection under it.
        void Shutdown( void )
        {
            if ( m_pSocket )
                m_pSocket->Shutdown();
        }

        virtual void *Run( void )
        {
            uint8_t     nTag;
            std::string strPayload;

            try
            {
                while ( RayWire::ReceiveFrame( m_oIn, nTag, strPayload ) )
                {
                    CTaskMaster::SEvent oEvent;

                    oEvent.nTask = m_nTask;
                    if ( nTag == MESSAGE_REQUEST )
                        oEvent.nKind = CTaskMaster::SEvent::EVENT_REQUEST;
                    else if ( nTag == MESSAGE_PIXELS )
                    {

                        oEvent.nKind = CTaskMaster::SEvent::EVENT_PIXELS;
                        if ( ! RayWire::DecodePixels( strPayload, oEvent.nTrace, oEvent.oTile, oEvent.oCounts, oEvent.anColors ) )
                            break;      // A damaged stream can't be followed.
                    }
                    else
                        continue;
                    m_oMaster.PostEvent( std::move( oEvent ) );
                }
            }
            catch ( ... )
            {
            }

            if ( ! m_oMaster.m_bClosing )
            {
                CTaskMaster::SEvent oLost;

                oLost.nKind = CTaskMaster::SEvent::EVENT_LOST;
                oLost.nTask = m_nTask;
                m_oMaster.PostEvent( std::move( oLost ) );
            }
            return NULL;
        }
};

    // Accepts engines that connect after SpawnTasks, and tells the master.
class CSocketTaskMaster::CAcceptor : public IASLib::CThread
{
    protected:
        CSocketTaskMaster  &m_oMaster;

    public:
        explicit CAcceptor( CSocketTaskMaster &oMaster ) :
            IASLib::CThread( "rayengine acceptor", true ),
            m_oMaster( oMaster )
        {
            SetNoDelete( true );
        }

        virtual void *Run( void )
        {
            while ( ! m_oMaster.m_bClosing )
            {
                IASLib::CSocket *pSocket = m_oMaster.m_pServer->Accept( ACCEPT_POLL_MS );

                if ( pSocket == NULL )
                    continue;

                int nTask = m_oMaster.AddConnection( pSocket );

                if ( nTask >= 0 )
                {
                    CTaskMaster::SEvent oJoined;

                    oJoined.nKind = CTaskMaster::SEvent::EVENT_JOINED;
                    oJoined.nTask = nTask;
                    m_oMaster.PostEvent( std::move( oJoined ) );
                }
            }
            return NULL;
        }
};

CSocketTaskMaster::CSocketTaskMaster( int nPort, int nAcceptTimeoutSeconds ) :
    m_nPort( ( nPort > 0 ) ? nPort : 0 ),
    m_nAcceptTimeoutMs( nAcceptTimeoutSeconds * 1000 ),
    m_pServer( NULL ),
    m_pAcceptor( NULL ),
    m_bClosing( false )
{
}

CSocketTaskMaster::~CSocketTaskMaster( void )
{
    EndTasks();
    delete m_pServer;
}

BOOL CSocketTaskMaster::Listen( void )
{
    if ( m_pServer )
        return TRUE;

    try
    {
        m_pServer = new IASLib::CServerSocket( m_nPort );
        if ( m_nPort == 0 )
            m_nPort = m_pServer->GetLocalPort();
    }
    catch ( IASLib::CException &oException )
    {
        m_strLastError = (const char *)oException.GetMessage();
        delete m_pServer;
        m_pServer = NULL;
        return FALSE;
    }
    return TRUE;
}

int CSocketTaskMaster::AddConnection( IASLib::CSocket *pSocket )
{
    CConnection *pConnection = new CConnection( *this, pSocket );
    bool         bAccepted = false;

    try
    {
        bAccepted = pConnection->Handshake();
    }
    catch ( IASLib::CException & )
    {
    }

    if ( ! bAccepted )
    {
            // Not a rayengine (or a different version of one): drop it.
        delete pConnection;
        return -1;
    }

    int nTask;

    {
        IASLib::CMutexLock oLock( m_mutexConnections );

        nTask = (int)m_apConnections.size();
        m_apConnections.push_back( pConnection );
    }
    pConnection->SetTask( nTask );
    pConnection->Start();
    return nTask;
}

CSocketTaskMaster::CConnection *CSocketTaskMaster::GetConnection( int nTask )
{
    IASLib::CMutexLock oLock( m_mutexConnections );

    if ( ( nTask < 0 ) || ( nTask >= (int)m_apConnections.size() ) )
        return NULL;
    return m_apConnections[ (size_t)nTask ];
}

BOOL CSocketTaskMaster::SpawnTasks( int nTasksToSpawn )
{
    EndTasks();
    if ( ! Listen() )
        return FALSE;

    m_bClosing = false;
    while ( GetTaskCount() < nTasksToSpawn )
    {
        IASLib::CSocket *pSocket = NULL;

        try
        {
            pSocket = m_pServer->Accept( m_nAcceptTimeoutMs );
        }
        catch ( IASLib::CException &oException )
        {
            m_strLastError = (const char *)oException.GetMessage();
            return FALSE;
        }

        if ( pSocket == NULL )
        {
            char achMessage[ 128 ];

            snprintf( achMessage, sizeof( achMessage ), "Timed out waiting for engine %d of %d to connect",
                      GetTaskCount() + 1, nTasksToSpawn );
            m_strLastError = achMessage;
            return FALSE;
        }

        int nTask = AddConnection( pSocket );

        if ( nTask >= 0 )
            TaskStarted( nTask );
    }

        // Engines that come later join as they connect.
    m_pAcceptor = new CAcceptor( *this );
    m_pAcceptor->Start();
    return TRUE;
}

BOOL CSocketTaskMaster::EndTasks( void )
{
    std::vector<CConnection *> apConnections;

    m_bClosing = true;
    if ( m_pAcceptor )
    {
        m_pAcceptor->Join();
        delete m_pAcceptor;
        m_pAcceptor = NULL;
    }

    {
        IASLib::CMutexLock oLock( m_mutexConnections );

        apConnections.swap( m_apConnections );
    }
    for ( size_t nCount = 0; nCount < apConnections.size(); nCount++ )
    {
        try
        {
            apConnections[ nCount ]->SendStop();
        }
        catch ( IASLib::CException & )
        {
        }
        apConnections[ nCount ]->Shutdown();
    }
    for ( size_t nCount = 0; nCount < apConnections.size(); nCount++ )
    {
        apConnections[ nCount ]->Join();
        delete apConnections[ nCount ];
    }
    ForgetTasks();
    return TRUE;
}

BOOL CSocketTaskMaster::SceneToTask( int nTask, CScene &oScene )
{
    CConnection *pConnection = GetConnection( nTask );

    if ( pConnection == NULL )
        return FALSE;
    try
    {
            // Each connection is its own SJSON stream, but a scene starts its
            // stream afresh (a new encoder's first packet, sequence 0), so the
            // packet depends only on the scene: encoded once a trace, it goes
            // to every engine. Tiles and pixels aren't SJSON, so nothing
            // else on the stream depends on it.
        if ( m_nScenePacketTrace != m_nTrace || m_strScenePacket.empty() )
        {
            RayWire::CEncoder oEncoder( RAY_WIRE_MASTER_ID );

            m_strScenePacket = oEncoder.Scene( oScene );
            m_nScenePacketTrace = m_nTrace;
        }
        pConnection->SendScene( m_strScenePacket );
    }
    catch ( IASLib::CException &oException )
    {
        m_strLastError = (const char *)oException.GetMessage();
        return FALSE;
    }
    return TRUE;
}

BOOL CSocketTaskMaster::TileToTask( int nTask, uint32_t nTrace, const STile &oTile )
{
    CConnection *pConnection = GetConnection( nTask );

    if ( pConnection == NULL )
        return FALSE;
    try
    {
        pConnection->SendTile( nTrace, oTile );
    }
    catch ( IASLib::CException &oException )
    {
        m_strLastError = (const char *)oException.GetMessage();
        return FALSE;
    }
    return TRUE;
}
