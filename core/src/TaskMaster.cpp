#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "Scene.h"
#include "ImageFile.h"
#include "RayArray.h"
#include "TaskMaster.h"

#include "Threading/Thread.h"

#define RAYS_PER_PACKET    127

CTaskMaster::CTaskMaster( void ) :
    m_nTasks( 0 ),
    m_nRaysPerPacket( RAYS_PER_PACKET ),
    m_bTracing( false ),
    m_bStop( false ),
    m_nStatus( 0 )
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

BOOL CTaskMaster::TraceScene( CScene &oScene, CImageFile &oImageFile )
{
    CSettings      &oSettings = oScene.GetSettings();
    CRayArray       oRays;

        // Pixel Iterators
    int     nX;
    int     nY;

        // The Ray we're casting
    CRay    oRay;

    int         nYStop = oSettings.GetImageHeight();
    int         nXStart = 0;
    int         nXStop = oSettings.GetImageWidth();
    int         nYStart = 0;
    int         nPixels = 0;
    long long   nTotalPixels = (long long)oSettings.GetImageHeight() * (long long)oSettings.GetImageWidth();
    long long   nPixelCount = 0;
    int         nCurrentTask = 0;

    if ( ( m_nTasks <= 0 ) || ( nTotalPixels <= 0 ) )
        return FALSE;

    m_bTracing = true;
    m_bStop = false;
    m_nStatus = 0;

    if ( ! SceneToTasks( oScene ) )
    {
        m_bTracing = false;
        return FALSE;
    }

    nY = nYStart;
    while ( nY < nYStop )
    {
        nX = nXStart;
        while ( nX < nXStop )
        {
            if ( m_bStop )
            {
                m_bTracing = false;
                return FALSE;
            }

            oRay.CreateRay( oScene, nX, nY );
            oRays.Add( oRay );

            if ( oRays.GetSize() > m_nRaysPerPacket )
            {
                if ( RaysToTask( oRays, nCurrentTask ) == FALSE )
                {
                    m_bTracing = false;
                    return FALSE;
                }

                nPixels = ColorsFromTasks( oImageFile, FALSE );
                if ( nPixels < 0 )
                {
                    m_bTracing = false;
                    return FALSE;
                }
                nPixelCount += nPixels;

                nCurrentTask++;
                nCurrentTask = nCurrentTask % m_nTasks;
                oRays.RemoveAll();
            }

            nX++;
        }
        nY++;
        m_nStatus = (int)( ( nPixelCount * 10000 ) / nTotalPixels );
    }

    if ( oRays.GetSize() )
    {
        if ( RaysToTask( oRays, nCurrentTask ) == FALSE )
        {
            m_bTracing = false;
            return FALSE;
        }
    }

    while ( nPixelCount < nTotalPixels )
    {
        if ( m_bStop )
        {
            m_bTracing = false;
            return FALSE;
        }

        nPixels = ColorsFromTasks( oImageFile, TRUE );
        if ( nPixels < 0 )
        {
            m_bTracing = false;
            return FALSE;
        }
        nPixelCount += nPixels;
        m_nStatus = (int)( ( nPixelCount * 10000 ) / nTotalPixels );
    }

    m_bTracing = false;
    return TRUE;
}
