/*************************************************************************
*   CTaskMaster                                                          *
*                                                                        *
*   Splits an image into tiles and hands them to tracing tasks as they   *
*   ask for work: each task gets the scene, asks for a tile, traces it   *
*   (camera rays, sampling and all), sends the pixels back and asks for  *
*   the next. Faster tasks simply do more tiles. A task that goes away   *
*   has its unfinished tiles handed to the others, and a task can join   *
*   in the middle of a trace. The tasks themselves (threads, or          *
*   networked rayengine processes) are provided by a subclass.           *
*                                                                        *
*   Everything here runs on the thread that calls TraceScene; the tasks  *
*   reach it through PostEvent.                                          *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __TASKMASTER_H__
#define __TASKMASTER_H__

#include "Globals.h"
#include "TileTracer.h"
#include "WorkQueue.h"

#include <atomic>
#include <stdint.h>
#include <string>
#include <vector>

class CScene;
class CImageFile;

    // Tiles are this many pixels square, unless set otherwise.
#define RAY_DEFAULT_TILE_SIZE   32

class CTaskMaster
{
    public:
            // What a task tells the master.
        struct SEvent
        {
            enum EKind
            {
                EVENT_REQUEST,      // ready for a tile
                EVENT_PIXELS,       // a traced tile
                EVENT_LOST,         // the task went away
                EVENT_JOINED,       // a new task is ready for the scene
                EVENT_WAKE          // nothing; makes the master look at m_bStop
            };

            EKind               nKind = EVENT_WAKE;
            int                 nTask = -1;
            uint32_t            nTrace = 0;
            STile               oTile;
            long long           nRays = 0;
            std::vector<double> anColors;
        };

    protected:
        int                     m_nTasks;           // tasks started (some may be gone)
        int                     m_nTileSize;
        std::atomic<bool>       m_bTracing;
        std::atomic<bool>       m_bStop;
        std::atomic<int>        m_nStatus;          // fraction of the image traced, 0 to 10000
        long long               m_nRaysTraced;
        uint32_t                m_nTrace;           // which trace this is, from 1
        std::string             m_strLastError;
        TWorkQueue<SEvent>      m_oEvents;
        std::vector<char>       m_abLive;           // by task
        std::vector<uint32_t>   m_anSceneTrace;     // the trace whose scene each task has

            /// For the tasks' threads: tells the master something.
        void                    PostEvent( SEvent &&oEvent ) { m_oEvents.Push( std::move( oEvent ) ); }
            /// For subclasses, on the master's thread: task nTask is ready.
        void                    TaskStarted( int nTask );
            /// For subclasses' EndTasks: forgets every task and anything
            /// they sent.
        void                    ForgetTasks( void );

            /// Sends the scene to a task. FALSE if it can't be reached.
        virtual BOOL            SceneToTask( int nTask, CScene &oScene ) = 0;
            /// Sends a tile to a task. FALSE if it can't be reached.
        virtual BOOL            TileToTask( int nTask, uint32_t nTrace, const STile &oTile ) = 0;
            /// How long a trace with no tasks left waits for a new one to
            /// join before it fails; 0 (the default) fails at once.
        virtual int             JoinWaitMs( void ) const { return 0; }

    public:
                                CTaskMaster( void );
        virtual                ~CTaskMaster( void ) {}

        virtual BOOL            SpawnTasks( int nTasksToSpawn ) = 0;
        virtual BOOL            EndTasks( void ) = 0;

            /// Traces the whole scene into the image. Returns FALSE if the
            /// trace was stopped or every task was lost (GetLastError says).
        virtual BOOL            TraceScene( CScene &oScene, CImageFile &oImageFile );

            /// The tasks that are still there.
        int                     GetTaskCount( void ) const;
        void                    SetTileSize( int nPixels ) { if ( nPixels > 0 ) m_nTileSize = nPixels; }
        int                     GetTileSize( void ) const { return m_nTileSize; }
            /// The number of rays cast from the camera by the last trace.
        long long               GetRaysTraced( void ) const { return m_nRaysTraced; }
        const char             *GetLastError( void ) const { return m_strLastError.c_str(); }

        virtual int             Status( void ) { return m_nStatus; }
        virtual BOOL            IsTracing( void ) { return m_bTracing ? TRUE : FALSE; }
        virtual BOOL            EndTrace( void );
};

#endif
