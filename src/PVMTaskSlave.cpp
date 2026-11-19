#ifdef _WINDOWS
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "PVMStream.h"
#include "pvm3.h"
#include "RayArray.h"
#include "ColorArray.h"
#include "Scene.h"
#include "MsgTag.h"

#include "PVMTaskSlave.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPVMTaskSlave::CPVMTaskSlave( void )
{
    m_nTaskID = pvm_mytid();
    m_nParentID = pvm_parent();

    if ( m_nParentID == PvmNoParent )
    {
        // TO DO: Show some kind of error message. A Slave task
        //        requires a parent.
    }
}

CPVMTaskSlave::~CPVMTaskSlave( void )
{
}

BOOL CPVMTaskSlave::SceneFromMaster( CScene &oScene )
{
    CPVMStream      oStream( m_nTaskID, MESSAGE_SCENE, FALSE );

    oScene.Serialize( oStream, TRUE );

    return TRUE;
}

BOOL CPVMTaskSlave::RaysFromMaster( CRayArray &oRays )
{
    CPVMStream      oStream( m_nTaskID, MESSAGE_RAYS, FALSE );

    oStream >> oRays;

    return TRUE;
}

BOOL CPVMTaskSlave::ColorsToMaster( CColorArray &oColors )
{
    CPVMStream      oStream( m_nParentID, MESSAGE_COLOR, TRUE );

    oStream << oColors;

    return TRUE;
}
