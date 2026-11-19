#include "StdAfx.h"

#include "ImageFile.h"
#include "RayArray.h"

#include "TaskMaster.h"

#define RAYS_PER_PACKET    127

BOOL CTaskMaster::EndTrace( void )
{
    int     nCount = 0;

    m_bStop = TRUE;
    while ( ( m_bTracing ) && ( nCount < 100 ) )
    {
        nCount++;
        Sleep( 100 );
    }
    EndTasks();
    m_bTracing = FALSE;
    return ( nCount < 100 );
}

BOOL CTaskMaster::TraceScene( CScene &oScene, CImageFile &oImageFile )
{
    CSettings      &oSettings = oScene.GetSettings();
    CCamera        &oCamera = oScene.GetCamera();

    CRayArray       oRays;

    FLT nWidth = (FLT)oSettings.GetImageWidth();
    FLT nHeight = (FLT)oSettings.GetImageHeight();
    FLT nXCenter = nWidth / 2;
    FLT nYCenter = nHeight / 2;

        // Current Trace Point;
    FLT nCurrentX = -nXCenter;
    FLT nCurrentY = nYCenter;
    FLT nXRestart;

        // Angular step per pixel/line
    FLT nXStep = oCamera.GetHorizontalAngle() / nWidth;
    FLT nYStep = oCamera.GetVerticalAngle() / nHeight;

        // Current Angular Position for 0, 0 position
    FLT nXAngle;
    FLT nYAngle;

        // Pixel Iterators
    int     nX;
    int     nY;

        // The Ray we're casting
    CRay    oRay;

        // Camera information
    CVector oCamPos = oCamera.GetPosition();
    CVector oCamPoint = oCamera.PointAt() - oCamPos;

    int         nYStop = oSettings.GetImageHeight();
    int         nXStart = 0;
    int         nXStop = oSettings.GetImageWidth();
    int         nYStart = 0;

    int         nPixels = 0;
    int         nTotalPixels = oSettings.GetImageHeight() * oSettings.GetImageWidth();
    int         nPixelCount = 0;

    int         nCurrentTask = 0;

    m_bTracing = TRUE;
    m_bStop = FALSE;

    SceneToTasks( oScene );

    nXRestart = nCurrentX;

    nY = nYStart;
    while ( nY < nYStop )
    {
        nX = nXStart;
        while ( nX < nXStop )
        {
            if ( m_bStop )
            {
                m_bTracing = FALSE;
                return FALSE;
            }

            nXAngle = nCurrentX * nXStep;
            nYAngle = nCurrentY * nYStep;

            oRay.CreateRay( oScene, nXAngle, nYAngle, 1.0, nX, nY );
            oRays.Add( oRay );

            if ( oRays.GetSize() > RAYS_PER_PACKET )
            {
                if ( RaysToTask( oRays, nCurrentTask ) == FALSE )
                    return FALSE;

                if ( ColorsFromTask( oImageFile, nCurrentTask, nPixels ) )
                    nPixelCount += nPixels;

                nCurrentTask++;
                nCurrentTask = nCurrentTask % m_nTasks;

                oRays.RemoveAll();
            }

            nCurrentX += 1.0;
            nX++;
        }
        nCurrentX = nXRestart;
        nCurrentY -= 1.0;
        nY++;

        m_nStatus = ( nPixelCount * 10000 ) / nTotalPixels;
    }

    if ( oRays.GetSize() )
        RaysToTask( oRays, nCurrentTask );

    while ( nPixelCount < nTotalPixels )
    {
        if ( ColorsFromTask( oImageFile, nCurrentTask, nPixels ) )
            nPixelCount += nPixels;
        nCurrentTask++;
        nCurrentTask = nCurrentTask % m_nTasks;
    }

    return TRUE;
}
