#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "Scene.h"
#include "ImageFile.h"
#include "RayArray.h"
#include "ColorArray.h"
#include "TaskSlave.h"
#include "ThreadTaskMaster.h"
#include "WireProtocol.h"

#include "Threading/Thread.h"
#include "Threading/Semaphore.h"

    // How many batches of rays may wait for one worker.
#define WORKER_QUEUE_DEPTH      16

namespace
{
        // What the master sends a worker: the scene (once), then batches of
        // rays. An empty batch tells the worker to stop.
    struct SInbox
    {
        std::shared_ptr<const std::string>  pScene;
        std::unique_ptr<CRayArray>          pRays;
    };
}

class CThreadTaskMaster::CWorker : public IASLib::CThread, public CTaskSlave
{
    protected:
        TWorkQueue<SInbox>          m_oInbox;
        IASLib::CSemaphore          m_semFreeSlots;
        TWorkQueue<CColorArray *>  &m_oResults;

    public:
        CWorker( int nIndex, TWorkQueue<CColorArray *> &oResults ) :
            IASLib::CThread( "Ray tracing worker", true ),
            m_semFreeSlots( WORKER_QUEUE_DEPTH ),
            m_oResults( oResults )
        {
            (void)nIndex;
            SetNoDelete( true );
            Resume();
        }

        void SendScene( std::shared_ptr<const std::string> pScene )
        {
            SInbox oItem;
            oItem.pScene = pScene;
            m_oInbox.Push( std::move( oItem ) );
        }

        void SendRays( CRayArray &oRays )
        {
            SInbox oItem;

            m_semFreeSlots.Wait();
            oItem.pRays.reset( new CRayArray( oRays ) );
            m_oInbox.Push( std::move( oItem ) );
        }

            // Tells the worker to stop. Takes no queue slot, so it cannot
            // block even if the worker has already ended.
        void SendStop( void )
        {
            SInbox oItem;
            oItem.pRays.reset( new CRayArray );
            m_oInbox.Push( std::move( oItem ) );
        }

        virtual void *Run( void )
        {
            RunEngine();
                // RunEngine only returns when it was told to stop, or could not
                // read the scene. Either way the master hears about it.
            m_oResults.Push( NULL );
            return NULL;
        }

        virtual BOOL SceneFromMaster( CScene &oScene )
        {
            SInbox oItem = m_oInbox.Pop();

            if ( ! oItem.pScene )
                return FALSE;
            return RayWire::DecodeScene( *oItem.pScene, oScene ) ? TRUE : FALSE;
        }

        virtual BOOL RaysFromMaster( CRayArray &oRays )
        {
            SInbox oItem = m_oInbox.Pop();

            m_semFreeSlots.Post();
            oRays.RemoveAll();
            if ( ! oItem.pRays )
                return FALSE;
            oRays = *oItem.pRays;
            return ( oRays.GetSize() != 0 );
        }

        virtual BOOL ColorsToMaster( CColorArray &oColors )
        {
            m_oResults.Push( new CColorArray( oColors ) );
            return TRUE;
        }
};

CThreadTaskMaster::CThreadTaskMaster( void )
{
}

CThreadTaskMaster::~CThreadTaskMaster( void )
{
    EndTasks();
}

BOOL CThreadTaskMaster::SpawnTasks( int nTasksToSpawn )
{
    EndTasks();

    if ( nTasksToSpawn <= 0 )
        nTasksToSpawn = 1;

    for ( int nCount = 0; nCount < nTasksToSpawn; nCount++ )
        m_apWorkers.push_back( new CWorker( nCount, m_oResults ) );

    m_nTasks = (int)m_apWorkers.size();
    return TRUE;
}

BOOL CThreadTaskMaster::EndTasks( void )
{
    CColorArray    *pColors;

    for ( size_t nCount = 0; nCount < m_apWorkers.size(); nCount++ )
        m_apWorkers[ nCount ]->SendStop();

    for ( size_t nCount = 0; nCount < m_apWorkers.size(); nCount++ )
    {
        m_apWorkers[ nCount ]->Join();
        delete m_apWorkers[ nCount ];
    }
    m_apWorkers.clear();
    m_nTasks = 0;

        // Throw away anything the workers sent after the trace ended.
    while ( m_oResults.TryPop( pColors ) )
        delete pColors;

    return TRUE;
}

BOOL CThreadTaskMaster::SceneToTasks( CScene &oScene )
{
    std::shared_ptr<const std::string> pScene( new std::string( RayWire::EncodeScene( oScene ) ) );

    for ( size_t nCount = 0; nCount < m_apWorkers.size(); nCount++ )
        m_apWorkers[ nCount ]->SendScene( pScene );

    return TRUE;
}

BOOL CThreadTaskMaster::RaysToTask( CRayArray &oRays, int nTask )
{
    if ( ( nTask < 0 ) || ( nTask >= (int)m_apWorkers.size() ) )
        return FALSE;

    m_apWorkers[ (size_t)nTask ]->SendRays( oRays );
    return TRUE;
}

int CThreadTaskMaster::ColorsFromTasks( CImageFile &oImageFile, BOOL bWait )
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
            return -1;      // A worker stopped in the middle of the trace.

        for ( int nCount = 0; nCount < pColors->GetSize(); nCount++ )
            oImageFile.PutPoint( pColors->GetX( nCount ), pColors->GetY( nCount ), (*pColors)[ nCount ] );

        nPixels += pColors->GetSize();
        delete pColors;

        bHaveOne = m_oResults.TryPop( pColors );
    }

    return nPixels;
}
