#include "Globals.h"
#include "RenderJob.h"
#include "ViewCamera.h"

#include "ImageFile.h"
#include "Light.h"
#include "PixelSampler.h"
#include "Rays.h"
#include "Scene.h"
#include "SceneJson.h"
#include "ThreadTaskMaster.h"

#include <algorithm>
#include <chrono>

namespace RayEditor
{
    namespace
    {
            // An image kept in memory that hands each pixel to the job,
            // scaled up for the quick pass.
        class CProgressImage : public CImageFile
        {
            protected:
                CRenderJob     &m_oJob;
                int             m_nScale;

            public:
                                CProgressImage( CRenderJob &oJob, int nWidth, int nHeight, int nScale )
                                    : CImageFile( nWidth, nHeight ), m_oJob( oJob ), m_nScale( nScale ) {}

                virtual BOOL    PutRGB( int nX, int nY, BYTE nRed, BYTE nGreen, BYTE nBlue )
                {
                    m_oJob.PutPixel( nX, nY, m_nScale, nRed, nGreen, nBlue );
                    return TRUE;
                }
        };

            // The thread task master, with a way to stop a trace from
            // another thread that leaves its tasks running (EndTrace would
            // end them under the trace's feet if it hadn't started yet).
        class CStoppableMaster : public CThreadTaskMaster
        {
            public:
                void            RequestStop( void )
                {
                    SEvent oWake;

                    m_bStop = true;
                    oWake.nKind = SEvent::EVENT_WAKE;
                    PostEvent( std::move( oWake ) );
                }
        };

        IASLib::CArrayNode *Vector( const SVec3 &v )
        {
            IASLib::CArrayNode *pArray = new IASLib::CArrayNode;

            pArray->add( v.x )->add( v.y )->add( v.z );
            return pArray;
        }
    }

    CRenderJob::SRequest CRenderJob::ForView( const CViewCamera &oView, int nWidth, int nHeight )
    {
        SRequest oRequest;

        oRequest.oEye = oView.GetEye();
        oRequest.oTarget = oView.GetTarget();
        oRequest.oUp = oView.GetUp();
        oRequest.nFovY = oView.GetFovY();
        oRequest.nWidth = nWidth;
        oRequest.nHeight = nHeight;
        return oRequest;
    }

    CRenderJob::CRenderJob( void )
        : m_bStop( false ), m_bRunning( false ), m_bDone( false ), m_nPass( 0 ), m_nVersion( 0 ), m_pMaster( NULL )
    {
    }

    CRenderJob::~CRenderJob( void )
    {
        Stop();
    }

    std::string CRenderJob::GetError( void ) const
    {
        std::lock_guard<std::mutex> oLock( m_oPixelsLock );

        return m_strError;
    }

    void CRenderJob::CopyPixels( std::vector<unsigned char> &abOut ) const
    {
        std::lock_guard<std::mutex> oLock( m_oPixelsLock );

        abOut = m_abPixels;
    }

    void CRenderJob::PutPixel( int nX, int nY, int nScale, unsigned char nR, unsigned char nG, unsigned char nB )
    {
        std::lock_guard<std::mutex> oLock( m_oPixelsLock );
        int                         nWidth = m_oRequest.nWidth, nHeight = m_oRequest.nHeight;

        for ( int y = nY * nScale; y < std::min( nHeight, ( nY + 1 ) * nScale ); y++ )
        {
            for ( int x = nX * nScale; x < std::min( nWidth, ( nX + 1 ) * nScale ); x++ )
            {
                size_t nAt = ( ( (size_t)y * (size_t)nWidth ) + (size_t)x ) * 3;

                m_abPixels[ nAt ] = nR;
                m_abPixels[ nAt + 1 ] = nG;
                m_abPixels[ nAt + 2 ] = nB;
            }
        }
        m_nVersion++;
    }

    bool CRenderJob::Start( CScene &oScene, const SRequest &oRequest, std::string &strError )
    {
        Stop();
        if ( oRequest.nWidth <= 0 || oRequest.nHeight <= 0 )
        {
            strError = "the view has no pixels";
            return false;
        }
        m_strScene = oScene.ToJson( TRUE );
        m_oRequest = oRequest;
        {
            std::lock_guard<std::mutex> oLock( m_oPixelsLock );

            m_abPixels.assign( (size_t)oRequest.nWidth * (size_t)oRequest.nHeight * 3, 0 );
            m_strError.clear();
        }
        m_bStop = false;
        m_bDone = false;
        m_nPass = 0;
        m_nVersion++;
        m_bRunning = true;
        m_oThread = std::thread( &CRenderJob::Run, this );
        return true;
    }

    void CRenderJob::Stop( void )
    {
        m_bStop = true;
            // A trace resets its stop flag as it starts, so keep asking until
            // the thread has finished.
        while ( m_oThread.joinable() && m_bRunning )
        {
            {
                std::lock_guard<std::mutex> oLock( m_oMasterLock );

                if ( m_pMaster )
                    static_cast<CStoppableMaster *>( m_pMaster )->RequestStop();
            }
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        }
        if ( m_oThread.joinable() )
            m_oThread.join();
        m_bRunning = false;
    }

    bool CRenderJob::PassScene( CScene &oScene, int nWidth, int nHeight, bool bQuick, std::string &strError )
    {
        if ( oScene.FromJson( m_strScene ) != NO_SER_ERROR )
        {
            strError = oScene.GetLoadError();
            return false;
        }
        if ( ! m_oRequest.bSceneCamera )
        {
                // The editor's view replaces the camera's place and aim; its
                // sampler, color model and lens stay.
            std::unique_ptr<IASLib::CJsonNode>  pCamera( RayJson::Parse( oScene.PartToJson( "camera" ) ) );
            IASLib::CObjectNode                *pObject = ( pCamera && pCamera->isObject() ) ? static_cast<IASLib::CObjectNode *>( pCamera.get() ) : NULL;

            if ( ! pObject )
            {
                strError = "the scene's camera can't be read";
                return false;
            }
            pObject->set( "position", Vector( m_oRequest.oEye ) );
            pObject->set( "lookAt", Vector( m_oRequest.oTarget ) );
            pObject->set( "up", Vector( m_oRequest.oUp ) );
            pObject->put( "verticalAngle", m_oRequest.nFovY );
            pObject->remove( "horizontalAngle" );
            pObject->remove( "aspectRatio" );
            if ( oScene.EditPart( "camera", RayJson::Write( *pObject ), std::string() ) != NO_SER_ERROR )
            {
                strError = oScene.GetLoadError();
                return false;
            }
        }
        oScene.GetSettings().SetImageWidth( nWidth );
        oScene.GetSettings().SetImageHeight( nHeight );
        if ( bQuick )
            oScene.GetCamera().SetSampler( CPixelSampler::Create( "none" ) );
        return true;
    }

    void CRenderJob::Run( void )
    {
        int         nThreads = ( m_oRequest.nThreads > 0 ) ? m_oRequest.nThreads : (int)std::thread::hardware_concurrency();
        std::string strError;
        bool        bOK = true;

        if ( nThreads <= 0 )
            nThreads = 1;

        std::unique_ptr<CStoppableMaster> pMaster( new CStoppableMaster );

        pMaster->SpawnTasks( nThreads );
        {
            std::lock_guard<std::mutex> oLock( m_oMasterLock );

            m_pMaster = pMaster.get();
        }

        for ( int nPass = m_oRequest.bQuickPass ? 1 : 2; nPass <= 2 && bOK && ! m_bStop; nPass++ )
        {
            int     nScale = ( nPass == 1 ) ? 4 : 1;
            int     nWidth = std::max( 1, ( m_oRequest.nWidth + nScale - 1 ) / nScale );
            int     nHeight = std::max( 1, ( m_oRequest.nHeight + nScale - 1 ) / nScale );
            CScene  oScene;

            m_nPass = nPass;
            if ( ! PassScene( oScene, nWidth, nHeight, nPass == 1, strError ) )
            {
                bOK = false;
                break;
            }
            if ( m_bStop )
                break;

            CProgressImage oImage( *this, nWidth, nHeight, nScale );

            if ( ! pMaster->TraceScene( oScene, oImage ) )
            {
                if ( ! m_bStop )
                    strError = pMaster->GetLastError();
                bOK = false;
            }
        }

        {
            std::lock_guard<std::mutex> oLock( m_oMasterLock );

            m_pMaster = NULL;
        }
        pMaster->EndTasks();
        pMaster.reset();
        if ( ! strError.empty() )
        {
            std::lock_guard<std::mutex> oLock( m_oPixelsLock );

            m_strError = strError;
        }
        m_bDone = bOK && ! m_bStop;
        m_bRunning = false;
    }

    int PickObject( CScene &oScene, const CViewCamera &oView, double nX, double nY, int nWidth, int nHeight )
    {
        SVec3       oOrigin, oDirection;
        CRay        oRay;
        CVector     oIntersect;

        oView.PixelRay( nX, nY, nWidth, nHeight, oOrigin, oDirection );

            // The ray's length is how far it looks: as far as a sun.
        CVector     oO( oOrigin.x, oOrigin.y, oOrigin.z );
        CVector     oD( oDirection.x * RAY_FAR_AWAY, oDirection.y * RAY_FAR_AWAY, oDirection.z * RAY_FAR_AWAY );

        oRay.CreateRay( oD, oO, 1.0, 0, 0 );

        CObjects   *pHit = oScene.ClosestHit( oRay, oIntersect, NULL );

        return pHit ? oScene.IndexOfObject( pHit ) : -1;
    }
}
