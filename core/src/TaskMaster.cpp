#include "Globals.h"

#include "Scene.h"
#include "ImageFile.h"
#include "ImageWriter.h"
#include "ColorModel.h"
#include "ImageFile.h"
#include "TaskMaster.h"
#include "Color.h"
#include "Threading/Thread.h"

#include <algorithm>
#include <deque>

CTaskMaster::CTaskMaster( void ) :
    m_nTasks( 0 ),
    m_nTileSize( RAY_DEFAULT_TILE_SIZE ),
    m_bTracing( false ),
    m_bStop( false ),
    m_nStatus( 0 ),
    m_nRaysTraced( 0 ),
    m_nTrace( 0 )
{
}

void CTaskMaster::TaskStarted( int nTask )
{
    if ( nTask < 0 )
        return;
    if ( (size_t)nTask >= m_abLive.size() )
    {
        m_abLive.resize( (size_t)nTask + 1, 0 );
        m_anSceneTrace.resize( (size_t)nTask + 1, 0 );
    }
    m_abLive[ (size_t)nTask ] = 1;
    m_anSceneTrace[ (size_t)nTask ] = 0;
    if ( nTask >= m_nTasks )
        m_nTasks = nTask + 1;
}

void CTaskMaster::ForgetTasks( void )
{
    SEvent oEvent;

    m_abLive.clear();
    m_anSceneTrace.clear();
    m_nTasks = 0;
    while ( m_oEvents.TryPop( oEvent ) )
        ;
}

int CTaskMaster::GetTaskCount( void ) const
{
    return (int)std::count( m_abLive.begin(), m_abLive.end(), (char)1 );
}

BOOL CTaskMaster::EndTrace( void )
{
    int     nCount = 0;
    SEvent  oWake;

    m_bStop = true;
    oWake.nKind = SEvent::EVENT_WAKE;
    oWake.nTask = -1;
    PostEvent( std::move( oWake ) );

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
    CSettings                       &oSettings = oScene.GetSettings();
    int                             nWidth = oSettings.GetImageWidth();
    int                             nHeight = oSettings.GetImageHeight();
    CImageWriter                    oWriter( oImageFile, oScene.GetCamera().GetColorModel(), oScene.GetCamera().GetExposure() );
    std::deque<STile>               aoTiles;            // not handed out yet
    std::vector<std::vector<STile>> aaoOut;             // handed out, by task
    std::deque<int>                 anIdle;             // tasks waiting for a tile
    long long                       nPixelsLeft = (long long)nWidth * (long long)nHeight;
    long long                       nPixels = nPixelsLeft;
    uint32_t                        nTrace = ++m_nTrace;
    BOOL                            bRetVal = TRUE;

    m_nRaysTraced = 0;
    m_oRayCounts.Clear();
    m_strLastError.clear();
        // A color model that works on the whole image (film) needs every
        // pixel's linear color.
    if ( oScene.GetCamera().GetColorModel().NeedsWholeImage() && ! oImageFile.KeepsLinear() )
        oImageFile.KeepLinear();
    if ( ( GetTaskCount() <= 0 ) || ( nWidth <= 0 ) || ( nHeight <= 0 ) )
    {
        m_strLastError = ( GetTaskCount() <= 0 ) ? "There are no tasks to trace with" : "The image has no pixels";
        return FALSE;
    }

        // Tiles in reading order, so the image fills from the top.
    for ( int nTop = 0; nTop < nHeight; nTop += m_nTileSize )
    {
        for ( int nLeft = 0; nLeft < nWidth; nLeft += m_nTileSize )
        {
            STile oTile = { nLeft, nTop, (std::min)( m_nTileSize, nWidth - nLeft ), (std::min)( m_nTileSize, nHeight - nTop ) };

            aoTiles.push_back( oTile );
        }
    }

    m_bTracing = true;
    m_bStop = false;
    m_nStatus = 0;

    auto Grow = [ & ]( int nTask )
    {
        if ( nTask >= 0 && (size_t)nTask >= aaoOut.size() )
            aaoOut.resize( (size_t)nTask + 1 );
    };

        // A task is gone: its unfinished tiles go back at the front.
    auto Lose = [ & ]( int nTask )
    {
        if ( nTask < 0 || (size_t)nTask >= m_abLive.size() || ! m_abLive[ (size_t)nTask ] )
            return;
        Grow( nTask );
        m_abLive[ (size_t)nTask ] = 0;
        std::vector<STile> &aoOut = aaoOut[ (size_t)nTask ];
        for ( size_t nIndex = aoOut.size(); nIndex > 0; nIndex-- )
            aoTiles.push_front( aoOut[ nIndex - 1 ] );
        aoOut.clear();
        anIdle.erase( std::remove( anIdle.begin(), anIdle.end(), nTask ), anIdle.end() );
    };

        // Gives a task the next tile, or lets it wait for one.
    auto Give = [ & ]( int nTask )
    {
        if ( nTask < 0 || (size_t)nTask >= m_abLive.size() || ! m_abLive[ (size_t)nTask ] )
            return;
        if ( aoTiles.empty() )
        {
            anIdle.push_back( nTask );
            return;
        }

            // Near the end, the last big tiles would leave tasks idle while
            // one or two finish, so the tiles still waiting are split into
            // quarters (down to 8 pixels) as they're handed out. A pixel
            // comes out the same in any tile, so the image doesn't change.
        int nLive = (int)std::count( m_abLive.begin(), m_abLive.end(), (char)1 );

        while ( (int)aoTiles.size() < 2 * nLive && ( aoTiles.front().nWidth > 8 || aoTiles.front().nHeight > 8 ) )
        {
            STile oBig = aoTiles.front();
            int   nW1 = ( oBig.nWidth > 8 ) ? oBig.nWidth / 2 : oBig.nWidth;
            int   nH1 = ( oBig.nHeight > 8 ) ? oBig.nHeight / 2 : oBig.nHeight;

            aoTiles.pop_front();
            if ( oBig.nHeight > nH1 && oBig.nWidth > nW1 )
                aoTiles.push_front( { oBig.nLeft + nW1, oBig.nTop + nH1, oBig.nWidth - nW1, oBig.nHeight - nH1 } );
            if ( oBig.nHeight > nH1 )
                aoTiles.push_front( { oBig.nLeft, oBig.nTop + nH1, nW1, oBig.nHeight - nH1 } );
            if ( oBig.nWidth > nW1 )
                aoTiles.push_front( { oBig.nLeft + nW1, oBig.nTop, oBig.nWidth - nW1, nH1 } );
            aoTiles.push_front( { oBig.nLeft, oBig.nTop, nW1, nH1 } );
        }

        STile oTile = aoTiles.front();

        aoTiles.pop_front();
        Grow( nTask );
        aaoOut[ (size_t)nTask ].push_back( oTile );
        if ( ! TileToTask( nTask, nTrace, oTile ) )
            Lose( nTask );
    };

    auto SendScene = [ & ]( int nTask )
    {
        if ( m_anSceneTrace[ (size_t)nTask ] == nTrace )
            return;
        if ( SceneToTask( nTask, oScene ) )
            m_anSceneTrace[ (size_t)nTask ] = nTrace;
        else
            Lose( nTask );
    };

    for ( int nTask = 0; nTask < (int)m_abLive.size(); nTask++ )
    {
        if ( m_abLive[ (size_t)nTask ] )
            SendScene( nTask );
    }

    while ( nPixelsLeft > 0 )
    {
        if ( m_bStop )
        {
            m_strLastError = "The trace was stopped";
            bRetVal = FALSE;
            break;
        }
        if ( GetTaskCount() == 0 && JoinWaitMs() <= 0 )
        {
            m_strLastError = "Every task was lost";
            bRetVal = FALSE;
            break;
        }

        while ( ! anIdle.empty() && ! aoTiles.empty() )
        {
            int nTask = anIdle.front();

            anIdle.pop_front();
            Give( nTask );
        }

        SEvent oEvent;

        if ( GetTaskCount() > 0 )
            oEvent = m_oEvents.Pop();
        else if ( ! m_oEvents.PopFor( oEvent, JoinWaitMs() ) )
        {
            m_strLastError = "Every task was lost, and none joined in time";
            bRetVal = FALSE;
            break;
        }

        switch ( oEvent.nKind )
        {
            case SEvent::EVENT_REQUEST:
                Give( oEvent.nTask );
                break;

            case SEvent::EVENT_PIXELS:
            {
                if ( oEvent.nTrace != nTrace || oEvent.nTask < 0 || (size_t)oEvent.nTask >= aaoOut.size() )
                    break;      // from a stopped trace

                std::vector<STile> &aoOut = aaoOut[ (size_t)oEvent.nTask ];
                auto itTile = std::find_if( aoOut.begin(), aoOut.end(), [ & ]( const STile &oTile )
                {
                    return oTile.nLeft == oEvent.oTile.nLeft && oTile.nTop == oEvent.oTile.nTop &&
                           oTile.nWidth == oEvent.oTile.nWidth && oTile.nHeight == oEvent.oTile.nHeight;
                } );

                if ( itTile == aoOut.end() ||
                     oEvent.anColors.size() != (size_t)oEvent.oTile.nWidth * (size_t)oEvent.oTile.nHeight * 3 )
                    break;
                aoOut.erase( itTile );

                const STile &oTile = oEvent.oTile;
                size_t       nAt = 0;

                for ( int nRow = 0; nRow < oTile.nHeight; nRow++ )
                {
                    for ( int nColumn = 0; nColumn < oTile.nWidth; nColumn++, nAt += 3 )
                        oWriter.PutPixel( oTile.nLeft + nColumn, oTile.nTop + nRow,
                                          CColor( (FLT)oEvent.anColors[ nAt ], (FLT)oEvent.anColors[ nAt + 1 ],
                                                  (FLT)oEvent.anColors[ nAt + 2 ] ) );
                }
                nPixelsLeft -= (long long)oTile.nWidth * (long long)oTile.nHeight;
                m_nRaysTraced += (long long)oEvent.oCounts.anCounts[ CRayCounts::CAMERA ];
                m_oRayCounts.Add( oEvent.oCounts );
                m_nStatus = (int)( ( ( nPixels - nPixelsLeft ) * 10000 ) / nPixels );
                break;
            }

            case SEvent::EVENT_LOST:
                Lose( oEvent.nTask );
                break;

            case SEvent::EVENT_JOINED:
                TaskStarted( oEvent.nTask );
                Grow( oEvent.nTask );
                SendScene( oEvent.nTask );
                break;

            default:
                break;
        }
    }

    m_bTracing = false;
    if ( bRetVal && oScene.GetCamera().GetColorModel().NeedsWholeImage() )
        oScene.GetCamera().GetColorModel().Finish( oImageFile );
    return bRetVal;
}
