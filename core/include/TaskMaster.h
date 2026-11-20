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
#include <unordered_map>

class CScene;
class CImageFile;
class CRayArray;
class CRay;
class CColor;
class CPixelSampler;
class CPixelWriter;

class CTaskMaster
{
    protected:
        int                 m_nTasks;
        int                 m_nRaysPerPacket;
        std::atomic<bool>   m_bTracing;
        std::atomic<bool>   m_bStop;
        std::atomic<int>    m_nStatus;      // Fraction of the image traced, 0 to 10000.
        long long           m_nRaysTraced;
        long long           m_nRaysToTrace;
        CPixelSampler      *m_pSampler;     // The camera's sampler, during a trace.
        CPixelWriter       *m_pWriter;      // The image and color model, during a trace.
        int                 m_nChannels;    // 3: each sample is traced in red, green and blue apart.

            // With channels apart, the parts of each sample that have come
            // back, by tag, until all three are in.
        struct SChannelParts
        {
            FLT             anColor[ 3 ];
            int             nParts;
        };
        std::unordered_map<uint64_t, SChannelParts> m_mapParts;

            /// Hands a traced color to the camera's sampler, which puts it in
            /// the image. (nX, nY) is the tag the sampler gave the ray.
        void                PutSample( int nX, int nY, const CColor &oColor );

    private:
        BOOL                QueueRay( CRay &oRay, CRayArray &oRays, int &nCurrentTask );
        BOOL                CollectColors( BOOL bWait );

    public:
                            CTaskMaster( void );
        virtual            ~CTaskMaster( void ) {}

        virtual BOOL        SpawnTasks( int nTasksToSpawn ) = 0;
        virtual BOOL        EndTasks( void ) = 0;

            /// Traces the whole scene into the image, with the primary rays
            /// the camera's sampler makes. Returns FALSE if the trace was
            /// stopped or a task failed.
        virtual BOOL        TraceScene( CScene &oScene, CImageFile &oImageFile );

        virtual BOOL        SceneToTasks( CScene &oScene ) = 0;
        virtual BOOL        RaysToTask( CRayArray &oRays, int nTask ) = 0;
            /// Puts the colors the tasks have finished into the image, through
            /// PutSample. With bWait, blocks until at least one batch arrives.
            /// Returns the number of colors received, or -1 if a task failed.
        virtual int         ColorsFromTasks( BOOL bWait ) = 0;

        int                 GetTaskCount( void ) const { return m_nTasks; }
        void                SetRaysPerPacket( int nRays ) { if ( nRays > 0 ) m_nRaysPerPacket = nRays; }
            /// The number of rays cast from the camera by the last trace.
        long long           GetRaysTraced( void ) const { return m_nRaysTraced; }

        virtual int         Status( void ) { return m_nStatus; }
        virtual BOOL        IsTracing( void ) { return m_bTracing ? TRUE : FALSE; }
        virtual BOOL        EndTrace( void );
};

#endif
