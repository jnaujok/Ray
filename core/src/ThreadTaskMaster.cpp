#include "Globals.h"

#include "Scene.h"
#include "TaskSlave.h"
#include "ThreadTaskMaster.h"
#include "Threading/Thread.h"


namespace
{
        // What the master sends a worker.
    struct SInbox
    {
        CTaskSlave::EMessage                nKind = CTaskSlave::FROM_MASTER_STOP;
        std::shared_ptr<const std::string>  pScene;     // FROM_MASTER_SCENE: its JSON
        STile                               oTile;      // FROM_MASTER_TILE
        uint32_t                            nTrace = 0;
    };
}

class CThreadTaskMaster::CWorker : public IASLib::CThread, public CTaskSlave
{
    protected:
        int                 m_nIndex;
        CThreadTaskMaster  &m_oMaster;
        TWorkQueue<SInbox>  m_oInbox;

    public:
        CWorker( int nIndex, CThreadTaskMaster &oMaster ) :
            IASLib::CThread( "Ray tracing worker", true ),
            m_nIndex( nIndex ),
            m_oMaster( oMaster )
        {
            SetNoDelete( true );
            Resume();
        }

        void Send( SInbox oItem )
        {
            m_oInbox.Push( std::move( oItem ) );
        }

        virtual void *Run( void )
        {
                // RunEngine only returns early if something failed; then the
                // master hears that this task is gone.
            if ( ! RunEngine() )
            {
                CTaskMaster::SEvent oEvent;

                oEvent.nKind = CTaskMaster::SEvent::EVENT_LOST;
                oEvent.nTask = m_nIndex;
                m_oMaster.PostEvent( std::move( oEvent ) );
            }
            return NULL;
        }

        virtual EMessage FromMaster( std::unique_ptr<CScene> &pScene, STile &oTile, uint32_t &nTrace )
        {
            SInbox oItem = m_oInbox.Pop();

            if ( oItem.nKind == FROM_MASTER_SCENE )
            {
                pScene.reset( new CScene );
                if ( ! oItem.pScene || pScene->FromJson( *oItem.pScene ) != 0 )
                {
                    pScene.reset();
                    return FROM_MASTER_ERROR;
                }
            }
            else if ( oItem.nKind == FROM_MASTER_TILE )
            {
                oTile = oItem.oTile;
                nTrace = oItem.nTrace;
            }
            return oItem.nKind;
        }

        virtual BOOL RequestWork( void )
        {
            CTaskMaster::SEvent oEvent;

            oEvent.nKind = CTaskMaster::SEvent::EVENT_REQUEST;
            oEvent.nTask = m_nIndex;
            m_oMaster.PostEvent( std::move( oEvent ) );
            return TRUE;
        }

        virtual BOOL PixelsToMaster( uint32_t nTrace, const STile &oTile, const CRayCounts &oCounts, const std::vector<double> &anColors )
        {
            CTaskMaster::SEvent oEvent;

            oEvent.nKind = CTaskMaster::SEvent::EVENT_PIXELS;
            oEvent.nTask = m_nIndex;
            oEvent.nTrace = nTrace;
            oEvent.oTile = oTile;
            oEvent.oCounts = oCounts;
            oEvent.anColors = anColors;
            m_oMaster.PostEvent( std::move( oEvent ) );
            return TRUE;
        }
};

CThreadTaskMaster::CThreadTaskMaster( void ) :
    m_nSceneTextTrace( 0 )
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
    {
        m_apWorkers.push_back( new CWorker( nCount, *this ) );
        TaskStarted( nCount );
    }
    return TRUE;
}

BOOL CThreadTaskMaster::EndTasks( void )
{
    SInbox oStop;

    oStop.nKind = CTaskSlave::FROM_MASTER_STOP;
    for ( size_t nCount = 0; nCount < m_apWorkers.size(); nCount++ )
        m_apWorkers[ nCount ]->Send( oStop );
    for ( size_t nCount = 0; nCount < m_apWorkers.size(); nCount++ )
    {
        m_apWorkers[ nCount ]->Join();
        delete m_apWorkers[ nCount ];
    }
    m_apWorkers.clear();
    m_pSceneText.reset();
    m_nSceneTextTrace = 0;
    ForgetTasks();
    return TRUE;
}

BOOL CThreadTaskMaster::SceneToTask( int nTask, CScene &oScene )
{
    if ( ( nTask < 0 ) || ( nTask >= (int)m_apWorkers.size() ) )
        return FALSE;

        // Each worker gets its own copy of the scene, through its JSON text,
        // made once a trace.
    if ( ! m_pSceneText || m_nSceneTextTrace != m_nTrace )
    {
        m_pSceneText.reset( new std::string( oScene.ToJson( TRUE ) ) );
        m_nSceneTextTrace = m_nTrace;
    }

    SInbox oItem;

    oItem.nKind = CTaskSlave::FROM_MASTER_SCENE;
    oItem.pScene = m_pSceneText;
    m_apWorkers[ (size_t)nTask ]->Send( oItem );
    return TRUE;
}

BOOL CThreadTaskMaster::TileToTask( int nTask, uint32_t nTrace, const STile &oTile )
{
    if ( ( nTask < 0 ) || ( nTask >= (int)m_apWorkers.size() ) )
        return FALSE;

    SInbox oItem;

    oItem.nKind = CTaskSlave::FROM_MASTER_TILE;
    oItem.oTile = oTile;
    oItem.nTrace = nTrace;
    m_apWorkers[ (size_t)nTask ]->Send( oItem );
    return TRUE;
}
