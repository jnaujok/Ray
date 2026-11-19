#ifndef __TASKSLAVE_H__
#define __TASKSLAVE_H__

#include "Globals.h"

class CScene;
class CRayArray;
class CColorArray;

    // A tracing task: gets the scene, then traces batches of rays until it
    // is sent an empty batch.
class CTaskSlave
{
    public:
                        CTaskSlave( void ) {}
        virtual        ~CTaskSlave( void ) {}

        virtual BOOL    RunEngine( void );

        virtual BOOL    SceneFromMaster( CScene &oScene ) = 0;
        virtual BOOL    RaysFromMaster( CRayArray &oRays ) = 0;
        virtual BOOL    ColorsToMaster( CColorArray &oColors ) = 0;
};

#endif
