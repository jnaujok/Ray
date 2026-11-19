#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "Scene.h"
#include "ImageFile.h"
#include "RayArray.h"
#include "ColorArray.h"
#include "SocketTaskMaster.h"
#include "WireProtocol.h"

#include "Exceptions/Exception.h"
#include "Sockets/ServerSocket.h"
#include "Sockets/Socket.h"
#include "Streams/SocketStream.h"
#include "Threading/Thread.h"

    // One connected rayengine. The master thread writes to it; a reader
    // thread takes the colors it sends back and queues them. Each direction
    // is its own SJSON stream, so each has its own encoder or decoder, used
    // by one thread only.
class CSocketTaskMaster::CConnection : public IASLib::CThread
{
    protected:
        IASLib::CSocket            *m_pSocket;
        IASLib::CSocketStream       m_oOut;     // owns the socket
        IASLib::CSocketStream       m_oIn;      // shares it
        TWorkQueue<CColorArray *>  &m_oResults;
        std::atomic<bool>          &m_bClosing;
        RayWire::CEncoder           m_oEncoder;     // the master thread's
        RayWire::CDecoder           m_oDecoder;     // the reader thread's

    public:
        CConnection( IASLib::CSocket *pSocket, TWorkQueue<CColorArray *> &oResults, std::atomic<bool> &bClosing ) :
            IASLib::CThread( "rayengine connection", true ),
            m_pSocket( pSocket ),
            m_oOut( pSocket ),
            m_oIn( pSocket ),
            m_oResults( oResults ),
            m_bClosing( bClosing ),
            m_oEncoder( RAY_WIRE_MASTER_ID ),
            m_oDecoder( RAY_WIRE_MASTER_ID, RAY_WIRE_ENGINE_ID )
        {
            m_oIn.SetNoDelete();
            SetNoDelete( true );
        }

        virtual ~CConnection( void ) {}

            /// Reads the engine's hello. Call before Start().
        bool Handshake( void )
        {
            uint8_t     nTag;
            std::string strPayload;

            if ( ! RayWire::ReceiveFrame( m_oIn, nTag, strPayload ) )
                return false;
            return ( nTag == MESSAGE_HELLO ) && RayWire::DecodeHello( strPayload );
        }

        void SendScene( CScene &oScene )
        {
            RayWire::SendFrame( m_oOut, MESSAGE_SCENE, m_oEncoder.Scene( oScene ) );
        }

            /// An empty batch tells the engine to stop.
        void SendRays( CRayArray &oRays )
        {
            RayWire::SendFrame( m_oOut, MESSAGE_RAYS, m_oEncoder.Rays( oRays ) );
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
                    if ( nTag == MESSAGE_COLOR )
                    {
                        CColorArray *pColors = new CColorArray;

                        if ( ! m_oDecoder.Colors( strPayload, *pColors ) )
                        {
                            delete pColors;
                            break;      // A damaged stream can't be followed.
                        }
                        m_oResults.Push( pColors );
                    }
                }
            }
            catch ( ... )
            {
            }

            if ( ! m_bClosing )
                m_oResults.Push( NULL );    // The engine went away during a trace.
            return NULL;
        }
};

CSocketTaskMaster::CSocketTaskMaster( int nPort, int nAcceptTimeoutSeconds ) :
    m_nPort( ( nPort > 0 ) ? nPort : 0 ),
    m_nAcceptTimeoutMs( nAcceptTimeoutSeconds * 1000 ),
    m_pServer( NULL ),
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
        m_strLastError = oException.GetMessage();
        delete m_pServer;
        m_pServer = NULL;
        return FALSE;
    }
    return TRUE;
}

BOOL CSocketTaskMaster::SpawnTasks( int nTasksToSpawn )
{
    EndTasks();

    if ( ! Listen() )
        return FALSE;

    m_bClosing = false;

    while ( (int)m_apConnections.size() < nTasksToSpawn )
    {
        IASLib::CSocket *pSocket = NULL;

        try
        {
            pSocket = m_pServer->Accept( m_nAcceptTimeoutMs );
        }
        catch ( IASLib::CException &oException )
        {
            m_strLastError = oException.GetMessage();
            return FALSE;
        }

        if ( pSocket == NULL )
        {
            m_strLastError.Format( "Timed out waiting for engine %d of %d to connect",
                                   (int)m_apConnections.size() + 1, nTasksToSpawn );
            return FALSE;
        }

        CConnection *pConnection = new CConnection( pSocket, m_oResults, m_bClosing );
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
            continue;
        }

        pConnection->Start();
        m_apConnections.push_back( pConnection );
    }

    m_nTasks = (int)m_apConnections.size();
    return TRUE;
}

BOOL CSocketTaskMaster::EndTasks( void )
{
    CRayArray       oStop;
    CColorArray    *pColors;

    m_bClosing = true;

    for ( size_t nCount = 0; nCount < m_apConnections.size(); nCount++ )
    {
        try
        {
            m_apConnections[ nCount ]->SendRays( oStop );
        }
        catch ( IASLib::CException & )
        {
        }
        m_apConnections[ nCount ]->Shutdown();
    }

    for ( size_t nCount = 0; nCount < m_apConnections.size(); nCount++ )
    {
        m_apConnections[ nCount ]->Join();
        delete m_apConnections[ nCount ];
    }
    m_apConnections.clear();
    m_nTasks = 0;

    while ( m_oResults.TryPop( pColors ) )
        delete pColors;

    return TRUE;
}

BOOL CSocketTaskMaster::SceneToTasks( CScene &oScene )
{
    try
    {
            // Each connection has its own SJSON stream, so the scene is
            // encoded for each.
        for ( size_t nCount = 0; nCount < m_apConnections.size(); nCount++ )
            m_apConnections[ nCount ]->SendScene( oScene );
    }
    catch ( IASLib::CException &oException )
    {
        m_strLastError = oException.GetMessage();
        return FALSE;
    }
    return TRUE;
}

BOOL CSocketTaskMaster::RaysToTask( CRayArray &oRays, int nTask )
{
    if ( ( nTask < 0 ) || ( nTask >= (int)m_apConnections.size() ) )
        return FALSE;

    try
    {
        m_apConnections[ (size_t)nTask ]->SendRays( oRays );
    }
    catch ( IASLib::CException &oException )
    {
        m_strLastError = oException.GetMessage();
        return FALSE;
    }
    return TRUE;
}

int CSocketTaskMaster::ColorsFromTasks( BOOL bWait )
{
    CColorArray    *pColors;
    int             nPixels = 0;
    bool            bHaveOne;

    if ( bWait )
    {
        pColors = m_oResults.Pop();
        bHaveOne = true;
    }
    else
        bHaveOne = m_oResults.TryPop( pColors );

    while ( bHaveOne )
    {
        if ( pColors == NULL )
        {
            m_strLastError = "A rayengine disconnected during the trace";
            return -1;
        }

        for ( int nCount = 0; nCount < pColors->GetSize(); nCount++ )
            PutSample( pColors->GetX( nCount ), pColors->GetY( nCount ), (*pColors)[ nCount ] );

        nPixels += pColors->GetSize();
        delete pColors;

        bHaveOne = m_oResults.TryPop( pColors );
    }

    return nPixels;
}
