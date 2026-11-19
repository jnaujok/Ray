/*************************************************************************
*   CTaskMaster                                                          *
*                                                                        *
*   Splits an image into batches of primary rays, deals them out to a    *
*   set of tracing tasks, and collects the colors they send back. The    *
*   tasks themselves (threads or networked rayengine processes) are      *
*   provided by a subclass.                                              *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __TASKMASTER_H__
#define __TASKMASTER_H__

#include "Globals.h"

#include <atomic>

class CScene;
class CImageFile;
class CRayArray;

class CTaskMaster
{
    protected:
        int                 m_nTasks;
        int                 m_nRaysPerPacket;
        std::atomic<bool>   m_bTracing;
        std::atomic<bool>   m_bStop;
        std::atomic<int>    m_nStatus;      // Fraction of the image traced, 0 to 10000.

    public:
                            CTaskMaster( void );
        virtual            ~CTaskMaster( void ) {}

        virtual BOOL        SpawnTasks( int nTasksToSpawn ) = 0;
        virtual BOOL        EndTasks( void ) = 0;

            /// Traces the whole scene into the image. Returns FALSE if the
            /// trace was stopped or a task failed.
        virtual BOOL        TraceScene( CScene &oScene, CImageFile &oImageFile );

        virtual BOOL        SceneToTasks( CScene &oScene ) = 0;
        virtual BOOL        RaysToTask( CRayArray &oRays, int nTask ) = 0;
            /// Puts the colors the tasks have finished into the image.
            /// With bWait, blocks until at least one batch arrives.
            /// Returns the number of pixels placed, or -1 if a task failed.
        virtual int         ColorsFromTasks( CImageFile &oImageFile, BOOL bWait ) = 0;

        int                 GetTaskCount( void ) const { return m_nTasks; }
        void                SetRaysPerPacket( int nRays ) { if ( nRays > 0 ) m_nRaysPerPacket = nRays; }

        virtual int         Status( void ) { return m_nStatus; }
        virtual BOOL        IsTracing( void ) { return m_bTracing ? TRUE : FALSE; }
        virtual BOOL        EndTrace( void );
};

#endif
