#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "Scene.h"
#include "ImageFile.h"
#include "RayArray.h"
#include "TaskMaster.h"
#include "Rays.h"
#include "Color.h"
#include "PixelSampler.h"
#include "PixelWriter.h"

#include "Threading/Thread.h"

#define RAYS_PER_PACKET    127

CTaskMaster::CTaskMaster( void ) :
    m_nTasks( 0 ),
    m_nRaysPerPacket( RAYS_PER_PACKET ),
    m_bTracing( false ),
    m_bStop( false ),
    m_nStatus( 0 ),
    m_nRaysTraced( 0 ),
    m_nRaysToTrace( 0 ),
    m_pSampler( NULL ),
    m_pWriter( NULL )
{
}

BOOL CTaskMaster::EndTrace( void )
{
    int     nCount = 0;

    m_bStop = true;

    while ( ( m_bTracing ) && ( nCount < 100 ) )
    {
        nCount++;
        IASLib::CThread::Millisleep( 100 );
    }

    EndTasks();
    m_bTracing = false;

    return ( nCount < 100 );
}

void CTaskMaster::PutSample( int nX, int nY, const CColor &oColor )
{
    if ( m_pSampler && m_pWriter )
        m_pSampler->PutSample( *m_pWriter, nX, nY, oColor );
}

    // Collects whatever colors the tasks have finished. Returns FALSE if a
    // task failed.
BOOL CTaskMaster::CollectColors( BOOL bWait )
{
    int nColors = ColorsFromTasks( bWait );

    if ( nColors < 0 )
        return FALSE;

    m_nRaysTraced += nColors;
    if ( m_nRaysToTrace > 0 )
        m_nStatus = (int)( ( m_nRaysTraced * 10000 ) / m_nRaysToTrace );
    return TRUE;
}

    // Adds a ray to the batch being built, and sends the batch off when it is
    // full. Returns FALSE if the trace was stopped or a task failed.
BOOL CTaskMaster::QueueRay( CRay &oRay, CRayArray &oRays, int &nCurrentTask )
{
    if ( m_bStop )
        return FALSE;

    oRays.Add( oRay );

    if ( oRays.GetSize() > m_nRaysPerPacket )
    {
        if ( RaysToTask( oRays, nCurrentTask ) == FALSE )
            return FALSE;

        if ( ! CollectColors( FALSE ) )
            return FALSE;

        nCurrentTask++;
        nCurrentTask = nCurrentTask % m_nTasks;
        oRays.RemoveAll();
    }
    return TRUE;
}

BOOL CTaskMaster::TraceScene( CScene &oScene, CImageFile &oImageFile )
{
    CSettings      &oSettings = oScene.GetSettings();
    CPixelSampler  &oSampler = oScene.GetCamera().GetSampler();
    CPixelWriter    oWriter( oImageFile, oScene.GetCamera().GetColorModel(), oScene.GetCamera().GetExposure() );
    CRayArray       oRays;
    CRay            oRay;
    int             nCurrentTask = 0;
    BOOL            bRetVal = TRUE;

    m_nRaysTraced = 0;

    if ( ( m_nTasks <= 0 ) || ( oSettings.GetImageWidth() <= 0 ) || ( oSettings.GetImageHeight() <= 0 ) )
        return FALSE;

    m_pSampler = &oSampler;
    m_pWriter = &oWriter;
    m_nRaysToTrace = oSampler.Begin( oSettings.GetImageWidth(), oSettings.GetImageHeight() );

    m_bTracing = true;
    m_bStop = false;
    m_nStatus = 0;

    if ( ! SceneToTasks( oScene ) )
        bRetVal = FALSE;

    while ( bRetVal && oSampler.NextRay( oScene, oRay ) )
        bRetVal = QueueRay( oRay, oRays, nCurrentTask );

    if ( bRetVal && oRays.GetSize() )
        bRetVal = RaysToTask( oRays, nCurrentTask );

    while ( bRetVal && ( m_nRaysTraced < m_nRaysToTrace ) )
    {
        if ( m_bStop )
            bRetVal = FALSE;
        else
            bRetVal = CollectColors( TRUE );
    }

    oSampler.End();
    m_pSampler = NULL;
    m_pWriter = NULL;
    m_bTracing = false;
    return bRetVal;
}
