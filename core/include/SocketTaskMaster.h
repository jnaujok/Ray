/*************************************************************************
*   CSocketTaskMaster                                                    *
*                                                                        *
*   Traces with rayengine processes that connect over TCP, on this       *
*   machine or any other. This takes the place of the original PVM       *
*   master: instead of pvm_spawn, the engines are started separately     *
*   (raythrower --spawn starts local ones) and connect to the port.      *
*                                                                        *
*************************************************************************/
#ifndef __SOCKETTASKMASTER_H__
#define __SOCKETTASKMASTER_H__

#include "TaskMaster.h"
#include "WorkQueue.h"
#include "WireProtocol.h"

#include <atomic>
#include <string>
#include <vector>

class CColorArray;

namespace IASLib
{
    class CServerSocket;
}

class CSocketTaskMaster : public CTaskMaster
{
    public:
        class CConnection;

    protected:
        int                         m_nPort;
        int                         m_nAcceptTimeoutMs;
        IASLib::CServerSocket      *m_pServer;
        std::vector<CConnection *>  m_apConnections;
        TWorkQueue<CColorArray *>   m_oResults;     // NULL: an engine went away.
        std::atomic<bool>           m_bClosing;
        IASLib::CString             m_strLastError;

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
        const char     *GetLastError( void ) const { return (const char *)m_strLastError; }

            /// Waits for nTasksToSpawn engines to connect and say hello.
        virtual BOOL    SpawnTasks( int nTasksToSpawn );
        virtual BOOL    EndTasks( void );

        virtual BOOL    SceneToTasks( CScene &oScene );
        virtual BOOL    RaysToTask( CRayArray &oRays, int nTask );
        virtual int     ColorsFromTasks( CImageFile &oImageFile, BOOL bWait );

    private:
                        CSocketTaskMaster( const CSocketTaskMaster & );
        CSocketTaskMaster &operator =( const CSocketTaskMaster & );
};

#endif
