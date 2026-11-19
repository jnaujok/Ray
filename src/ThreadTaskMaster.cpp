#ifdef _WINDOWS
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "Rays.h"
#include "Scene.h"
#include "ImageFile.h"
#include "PipeStream.h"
#include "RayArray.h"
#include "ColorArray.h"

#include "MsgTag.h"
#include "ThreadTaskSlave.h"

#include "ThreadTaskMaster.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


UINT ThreadSlaveRunner( LPVOID lParam )
{
    LPSTR               strPipeName = (LPSTR)lParam;
    CPipeStream         oStream( strPipeName, TRUE );
    CSem                oSemaphore( strPipeName );
    CThreadTaskSlave    oSlave( oStream, oSemaphore );

    delete strPipeName;
    
    oSlave.RunEngine();

    return 0;
}


CThreadTaskMaster::CThreadTaskMaster( void )
{
    m_nChildTasks = 0;
    m_anPipes = NULL;
    m_anReturnPipes = NULL;
    m_anLocks = NULL;
    m_anMasterLocks = NULL;
    m_nPercentCompleted = 0;
}

CThreadTaskMaster::~CThreadTaskMaster( void )
{
    if ( m_nChildTasks )
    {
        EndTasks();
    }
}

BOOL CThreadTaskMaster::SpawnTasks( int nTasksToSpawn )
{
    BOOL    bRetVal;
    int     nCount;
    CString strPipeName;
    char   *pBlock;
    CWinThread  *pThread;

    bRetVal = TRUE;

    m_nTasks = m_nChildTasks = nTasksToSpawn;
    m_anPipes = new LPPIPE[ m_nChildTasks ];
    m_anReturnPipes = new LPPIPE[ m_nChildTasks ];
    m_anLocks = new LPSEMAPHORE[ m_nChildTasks ];
    m_anMasterLocks = new LPSEMAPHORE[ m_nChildTasks ];

    nCount = 0;

    while ( nCount < nTasksToSpawn )
    {
        strPipeName.Format( "Thread_%d_Return", nCount );
        m_anReturnPipes[ nCount ] = new CPipeStream( strPipeName, FALSE );

        strPipeName.Format( "Master_%d", nCount );
        m_anMasterLocks[ nCount ] = new CSem( strPipeName );

        strPipeName.Format( "Thread_%d", nCount );
        m_anPipes[ nCount ] = new CPipeStream( strPipeName, FALSE );

        m_anLocks[ nCount ] = new CSem( strPipeName );
        pBlock = new char[ strPipeName.GetLength() + 1 ];
        strcpy( pBlock, strPipeName );
        pThread = AfxBeginThread( ThreadSlaveRunner, (LPVOID)(LPCSTR)pBlock );

        pThread->SetThreadPriority( THREAD_PRIORITY_HIGHEST );

        nCount++;
    }

    return bRetVal;
}

BOOL CThreadTaskMaster::EndTasks( void )
{
    int             nCount;
    CRayArray       oRays;

    if ( m_nChildTasks )
    {
        nCount = 0;
        while ( nCount < m_nChildTasks )
        {
            RaysToTask( oRays, nCount );
            nCount++;
        }

            // Allow time for the child threads to wake up and terminate.
        Sleep( 1000 );

        if ( m_nChildTasks )
        {
            nCount = 0;
            while ( nCount < m_nChildTasks )
            {
                delete m_anPipes[ nCount ];
                m_anPipes[nCount] = NULL;
                delete m_anReturnPipes[ nCount ];
                m_anReturnPipes[nCount] = NULL;
                delete m_anLocks[ nCount ];
                m_anLocks[nCount] = NULL;
                delete m_anMasterLocks[ nCount ];
                m_anLocks[nCount] = NULL;
                nCount ++;
            }
            delete m_anPipes;
            delete m_anReturnPipes;
            delete m_anLocks;
            delete m_anMasterLocks;
        }
        m_nChildTasks = 0;
    }
    return TRUE;
}

int CThreadTaskMaster::Status( void )
{
    return m_nStatus;
}

BOOL CThreadTaskMaster::SceneToTasks( CScene &oScene )
{
    int             nCount;

    nCount = 0;

    while ( nCount < m_nChildTasks )
    {
        oScene.Serialize( *(m_anPipes[ nCount ]), FALSE );
        m_anPipes[nCount]->Flush();
        m_anLocks[nCount]->Signal();
        nCount++;
    }

    return TRUE;
}

BOOL CThreadTaskMaster::RaysToTask( CRayArray &oRays, int nTask )
{
    *(m_anPipes[ nTask ]) << oRays;

    m_anPipes[nTask]->Flush();
    m_anLocks[nTask]->Signal();

//    if ( nTask == 0 )
//        Sleep( 100 );

    return TRUE;
}

BOOL CThreadTaskMaster::ColorsFromTask( CImageFile &oImageFile, int &nTask, int &nPixelsRet )
{
    DWORD           nTotalPixels = 0;
    DWORD           nCount = 0;
    CColorArray     oColors;


    while ( ! m_anMasterLocks[nTask]->IsLocked() )
    {
        m_anMasterLocks[nTask]->Wait();
        oColors.RemoveAll();

        *(m_anReturnPipes[nTask]) >> oColors;

        nCount = 0;

        while ( nCount < (DWORD)oColors.GetSize() )
        {
            oImageFile.PutPoint( oColors.GetX(nCount), oColors.GetY(nCount), oColors[nCount] );
            nCount++;
        }
        nTotalPixels += oColors.GetSize();
    }
    nPixelsRet = (int)nTotalPixels;

    return ( nPixelsRet != 0 );
}
