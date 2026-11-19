#ifdef _WINDOWS
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "pvm3.h"

#include "Rays.h"
#include "Scene.h"
#include "ImageFile.h"
#include "PVMStream.h"
#include "RayArray.h"
#include "ColorArray.h"

#include "MsgTag.h"

#include "PVMTaskMaster.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CPVMTaskMaster::CPVMTaskMaster( void )
{
    m_nMyTaskID = pvm_mytid();
    if ( pvm_parent() != PvmNoParent )
    {
#ifdef _WINDOWS
//        ::MessageBox( NULL, "The PVM Task Master should be the parent object.\n\nPVM reports this process as having a parent.", "PVM Task Master Error", MB_OK | MB_ICONSTOP );
#endif
    }
    m_nChildTasks = 0;
    m_anChildren = NULL;
    m_nPercentCompleted = 0;
}

CPVMTaskMaster::~CPVMTaskMaster( void )
{
    if ( m_nChildTasks )
        delete [] m_anChildren;
    pvm_exit();
}

BOOL CPVMTaskMaster::SpawnTasks( int nTasksToSpawn )
{
    int     nTasksBuilt;
    BOOL    bRetVal;

    bRetVal = FALSE;

    m_nChildTasks = nTasksToSpawn;
    m_anChildren = new int[ m_nChildTasks ];

    nTasksBuilt = pvm_spawn( "rayengine", NULL, PvmTaskDefault, "", m_nChildTasks, m_anChildren );

    if ( nTasksBuilt > 0 )
    {
        bRetVal = TRUE;
        if ( nTasksBuilt < m_nChildTasks )
            m_nChildTasks = nTasksBuilt;
    }
    m_nTasks = m_nChildTasks;

    return bRetVal;
}

BOOL CPVMTaskMaster::EndTasks( void )
{
    int nCount;

    nCount = 0;
    while ( nCount < m_nChildTasks )
    {
        pvm_kill( m_anChildren[ nCount ] );
        nCount++;
    }

    return TRUE;
}

int CPVMTaskMaster::Status( void )
{
    return m_nPercentCompleted;
}

BOOL CPVMTaskMaster::SceneToTasks( CScene &oScene )
{
    CPVMStream      *poStream;

    poStream = new CPVMStream( m_anChildren, m_nTasks, MESSAGE_SCENE );

    oScene.Serialize( *poStream, FALSE );

    poStream->Flush();

    delete poStream;

    return TRUE;
}

BOOL CPVMTaskMaster::RaysToTask( CRayArray &oRays, int nTask )
{
    CPVMStream      *poStream;

    poStream = new CPVMStream( m_anChildren, nTask, MESSAGE_RAYS );

    *poStream << oRays;

    poStream->Flush();

    delete poStream;

    return TRUE;
}

BOOL CPVMTaskMaster::ColorsFromTask( CImageFile &oImageFile, int &nTask, int &nPixelsRet )
{
    DWORD           nTotalPixels = 0;
    DWORD           nCount = 0;
    CPVMStream     *poStream;
    CColorArray     oColors;

    while ( pvm_probe( m_anChildren[ nTask ], MESSAGE_COLOR ) )
    {
        poStream = new CPVMStream( m_anChildren[ nTask ], MESSAGE_COLOR, FALSE );

        *poStream >> oColors;

        nCount = 0;

        while ( nCount < (DWORD)oColors.GetSize() )
        {
            oImageFile.PutPoint( oColors.GetX( nCount ), oColors.GetY( nCount ), oColors[nCount] );
            nCount++;
        }
        delete poStream;
        nTotalPixels += oColors.GetSize();
    }
    nPixelsRet = (int)nTotalPixels;

    return ( nPixelsRet != 0 );
}
