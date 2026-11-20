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
    m_pWriter( NULL ),
    m_nChannels( 1 )
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

    // With channels apart, a ray's channel is in the top two bits of its
    // tag's y, which samplers never use.
#define CHANNEL_SHIFT       30
#define CHANNEL_TAG_MASK    0x3FFFFFFFu

void CTaskMaster::PutSample( int nX, int nY, const CColor &oColor )
{
    if ( ! m_pSampler || ! m_pWriter )
        return;

    if ( m_nChannels == 1 )
    {
        m_pSampler->PutSample( *m_pWriter, nX, nY, oColor );
        return;
    }

        // Keep the channel the ray was traced for; once a sample's three
        // channels are in, hand the whole color to the sampler.
    uint32_t        nTagY = (uint32_t)nY;
    int             nChannel = (int)( nTagY >> CHANNEL_SHIFT );
    int             nPlainY = (int)( nTagY & CHANNEL_TAG_MASK );
    uint64_t        nKey = ( (uint64_t)(uint32_t)nX << 32 ) | (uint32_t)nPlainY;
    CColor          oPart( oColor );
    SChannelParts  &oParts = m_mapParts[ nKey ];

    if ( nChannel < 1 || nChannel > 3 )
        return;
    oParts.anColor[ nChannel - 1 ] = ( nChannel == 1 ) ? oPart.GetR() : ( nChannel == 2 ) ? oPart.GetG() : oPart.GetB();
    if ( ++oParts.nParts == 3 )
    {
        CColor oWhole( oParts.anColor[ 0 ], oParts.anColor[ 1 ], oParts.anColor[ 2 ] );

        m_mapParts.erase( nKey );
        m_pSampler->PutSample( *m_pWriter, nX, nPlainY, oWhole );
    }
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
    m_nChannels = oScene.GetCamera().GetChannels();
    m_mapParts.clear();
    m_nRaysToTrace = oSampler.Begin( oSettings.GetImageWidth(), oSettings.GetImageHeight() ) * m_nChannels;

    m_bTracing = true;
    m_bStop = false;
    m_nStatus = 0;

    if ( ! SceneToTasks( oScene ) )
        bRetVal = FALSE;

    while ( bRetVal && oSampler.NextRay( oScene, oRay ) )
    {
        if ( m_nChannels == 1 )
            bRetVal = QueueRay( oRay, oRays, nCurrentTask );
        else
        {
                // A chromatic lens sends each wavelength its own way: remake
                // the sampler's ray once for each channel.
            CRay oChannelRay;

            for ( int nChannel = 1; bRetVal && nChannel <= 3; nChannel++ )
            {
                oChannelRay.CreateRayAt( oScene, oRay.GetImageX(), oRay.GetImageY(), oRay.GetPixelX(),
                                         (int)( (uint32_t)oRay.GetPixelY() | ( (uint32_t)nChannel << CHANNEL_SHIFT ) ),
                                         1.0, nChannel );
                bRetVal = QueueRay( oChannelRay, oRays, nCurrentTask );
            }
        }
    }

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
    m_mapParts.clear();
    m_pSampler = NULL;
    m_pWriter = NULL;
    m_bTracing = false;
    return bRetVal;
}
