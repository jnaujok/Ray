/*************************************************************************
*   CSocketTaskMaster                                                    *
*                                                                        *
*   Traces with rayengine processes that connect over TCP, on this       *
*   machine or any other. This takes the place of the original PVM       *
*   master: instead of pvm_spawn, the engines are started separately     *
*   (raythrower --spawn starts local ones) and connect to the port.      *
*                                                                        *
*   SpawnTasks waits for the first engines; after that the master keeps  *
*   listening, and an engine that connects later gets the scene and      *
*   starts asking for tiles, even in the middle of a trace. An engine    *
*   that drops out has its unfinished tiles handed to the others.        *
*                                                                        *
*************************************************************************/

#ifndef __SOCKETTASKMASTER_H__
#define __SOCKETTASKMASTER_H__

#include "TaskMaster.h"
#include "WireProtocol.h"
#include "Threading/Mutex.h"

#include <atomic>
#include <string>
#include <vector>

namespace IASLib
{
    class CServerSocket;
    class CSocket;
}

class CSocketTaskMaster : public CTaskMaster
{
    public:
        class CConnection;
        class CAcceptor;

    protected:
        int                         m_nPort;
        int                         m_nAcceptTimeoutMs;
        IASLib::CServerSocket      *m_pServer;
        IASLib::CMutex              m_mutexConnections;
        std::vector<CConnection *>  m_apConnections;    // by task; only ever added to during a run
        CAcceptor                  *m_pAcceptor;
        std::atomic<bool>           m_bClosing;
            // The scene as an SJSON packet, encoded once a trace from a
            // fresh stream (sequence 0, so a receiver starts afresh too) and
            // sent to every engine alike.
        std::string                 m_strScenePacket;
        uint32_t                    m_nScenePacketTrace = 0;

            /// Reads a new engine's hello and adds it. Returns its task
            /// number, or -1 if it isn't a rayengine of this version.
        int             AddConnection( IASLib::CSocket *pSocket );
        CConnection    *GetConnection( int nTask );

        virtual BOOL    SceneToTask( int nTask, CScene &oScene );
        virtual BOOL    TileToTask( int nTask, uint32_t nTrace, const STile &oTile );
            /// Engines can join late, so a trace waits for one as long as
            /// SpawnTasks waits for each.
        virtual int     JoinWaitMs( void ) const { return m_nAcceptTimeoutMs; }

    public:
            /// nPort 0 picks any free port (GetPort() says which once
            /// listening). nAcceptTimeoutSeconds is how long SpawnTasks
            /// waits for each engine to connect.
                        CSocketTaskMaster( int nPort = RAY_WIRE_DEFAULT_PORT, int nAcceptTimeoutSeconds = 60 );
        virtual        ~CSocketTaskMaster( void );

            /// Opens the listening socket. SpawnTasks does this if needed;
            /// call it first to start engines only once the port is open.
        BOOL            Listen( void );
        int             GetPort( void ) const { return m_nPort; }

            /// Waits for nTasksToSpawn engines to connect and say hello, then
            /// keeps accepting more in the background.
        virtual BOOL    SpawnTasks( int nTasksToSpawn );
        virtual BOOL    EndTasks( void );

    private:
                        CSocketTaskMaster( const CSocketTaskMaster & );
        CSocketTaskMaster &operator =( const CSocketTaskMaster & );
};

#endif
