#ifndef __TASKMASTER_H__
#define __TASKMASTER_H__

class CTaskMaster
{
    protected:
        int             m_nTasks;
        BOOL            m_bTracing;
        BOOL            m_bStop;
        int             m_nStatus;
    public:
                        CTaskMaster( void ) { m_nTasks = 0; m_nStatus = 0; }
        virtual        ~CTaskMaster( void ) {}

        virtual BOOL    SpawnTasks( int nTasksToSpawn ) = 0;
        virtual BOOL    EndTasks( void ) = 0;

        virtual BOOL    TraceScene( CScene &oScene, CImageFile &oImageFile );

        virtual BOOL    SceneToTasks( CScene &oScene ) = 0;
        virtual BOOL    RaysToTask( CRayArray &oRays, int nTask ) = 0;
        virtual BOOL    ColorsFromTask( CImageFile &oImageFile, int &nTask, int &nPixels ) = 0;

        virtual int     Status( void ) {return m_nStatus; };

        virtual BOOL    IsTracing( void ) { return m_bTracing; }
        virtual BOOL    EndTrace( void );
};

#endif

