#ifdef _WINDOWS
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "PipeStream.h"
#include "RayArray.h"
#include "ColorArray.h"
#include "Scene.h"
#include "MsgTag.h"

#include "ThreadTaskSlave.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CThreadTaskSlave::CThreadTaskSlave( CPipeStream &oStream, CSem &oSemaphore ) :
m_oStream( oStream ), m_oSemaphore( oSemaphore )
{
    CString strLongName;
    CString strName;

    TRACE( "[%s] Creating Thread Slave\n", m_oSemaphore.GetName() );
    strLongName = m_oSemaphore.GetName();
    strLongName += "_Return";
    m_oToMaster.Attach( strLongName, TRUE );
    strName = m_oSemaphore.GetName();
    strLongName = "Master";
    strLongName += strName.Right( strName.GetLength() - 6 );
    m_poMasterLock = new CSem( strLongName );
}

CThreadTaskSlave::~CThreadTaskSlave( void )
{
    TRACE( "[%s] Destroying Thread Slave\n", m_oSemaphore.GetName() );
    delete m_poMasterLock;
}

BOOL CThreadTaskSlave::SceneFromMaster( CScene &oScene )
{
    BOOL bRetVal;

//    m_oSemaphore.Wait();
    if ( oScene.Serialize( m_oStream, TRUE ) != 0 )
        bRetVal = FALSE;
    else
        bRetVal = TRUE;

    if ( bRetVal )
        TRACE( "[%s] Retrieved Scene\n", m_oSemaphore.GetName() );
    else
        TRACE( "[%s] Scene was not retrieved correctly.\n", m_oSemaphore.GetName() );

    return bRetVal;
}

BOOL CThreadTaskSlave::RaysFromMaster( CRayArray &oRays )
{
//    m_oSemaphore.Wait();
    m_oStream >> oRays;

//  TRACE( "[%s] Retrieved %d Rays\n", m_oSemaphore.GetName(), oRays.GetSize() );
    return (oRays.GetSize() != 0 );
}

BOOL CThreadTaskSlave::ColorsToMaster( CColorArray &oColors )
{
//  TRACE( "[%s]Sending %d traced rays to the master process\n", m_oSemaphore.GetName(), oColors.GetSize() );
    m_oToMaster << oColors;

    m_poMasterLock->Signal();

    return TRUE;
}
