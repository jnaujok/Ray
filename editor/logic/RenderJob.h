/*************************************************************************
*   CRenderJob                                                           *
*                                                                        *
*   The scene editor's Render button: ray-traces the editor's view (or   *
*   the scene's own camera) on worker threads with the tracer's own      *
*   task master, in two passes: a quick one, a ray a pixel at a quarter  *
*   of the size, then the scene's own sampler at full size. The window   *
*   copies the pixels as they arrive and shows them over the view.       *
*   Stopping it (when the view moves or the scene changes) ends the      *
*   trace within a tile.                                                 *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __RENDERJOB_H__
#define __RENDERJOB_H__

#include "EditorMath.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class CScene;
class CTaskMaster;

namespace RayEditor
{
    class CViewCamera;

    class CRenderJob
    {
        public:
            struct SRequest
            {
                bool        bSceneCamera = false;   // the scene's camera (and lens), not the view's
                SVec3       oEye, oTarget, oUp;     // the view, when not the scene's camera
                double      nFovY = 40.0;
                int         nWidth = 320;
                int         nHeight = 240;
                bool        bQuickPass = true;
                int         nThreads = 0;           // 0: one a processor
            };

                /// Fills a request for a view of a size.
            static SRequest ForView( const CViewCamera &oView, int nWidth, int nHeight );

        protected:
            std::string                 m_strScene;         // the scene as compact JSON, with its files' data
            SRequest                    m_oRequest;
            std::thread                 m_oThread;
            std::atomic<bool>           m_bStop;
            std::atomic<bool>           m_bRunning;
            std::atomic<bool>           m_bDone;
            std::atomic<int>            m_nPass;            // 0 before the first, 1 quick, 2 full
            std::atomic<unsigned>       m_nVersion;         // goes up with every pixel written

            mutable std::mutex          m_oPixelsLock;
            std::vector<unsigned char>  m_abPixels;         // the full image, three bytes a pixel
            std::string                 m_strError;

            std::mutex                  m_oMasterLock;
            CTaskMaster                *m_pMaster;          // while a trace runs

            void                        Run( void );
                // Builds the scene for a pass: the editor's view put in, the
                // image size set, and (for the quick pass) a ray a pixel.
            bool                        PassScene( CScene &oScene, int nWidth, int nHeight, bool bQuick, std::string &strError );

        public:
                                        CRenderJob( void );
                                       ~CRenderJob( void );

                /// Starts a render of oScene (copied now, so the editor can go
                /// on changing it). Stops any render already going.
            bool                        Start( CScene &oScene, const SRequest &oRequest, std::string &strError );
                /// Stops the render, waiting for its threads to finish.
            void                        Stop( void );

            bool                        IsRunning( void ) const { return m_bRunning; }
                /// Whether every pass finished.
            bool                        IsDone( void ) const { return m_bDone; }
            int                         GetPass( void ) const { return m_nPass; }
            unsigned                    GetVersion( void ) const { return m_nVersion; }
            int                         GetWidth( void ) const { return m_oRequest.nWidth; }
            int                         GetHeight( void ) const { return m_oRequest.nHeight; }
            std::string                 GetError( void ) const;
                /// A copy of the image so far, three bytes a pixel, rows from
                /// the top.
            void                        CopyPixels( std::vector<unsigned char> &abOut ) const;

                /// For the image file the passes write into.
            void                        PutPixel( int nX, int nY, int nScale, unsigned char nR, unsigned char nG, unsigned char nB );
    };

        /// The scene object (its number) under a pixel of the view, or -1:
        /// the tracer's own hit test, so it picks what a render shows there.
    int     PickObject( CScene &oScene, const CViewCamera &oView, double nX, double nY, int nWidth, int nHeight );
}

#endif // __RENDERJOB_H__
