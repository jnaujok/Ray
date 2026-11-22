/*************************************************************************
*   CPointLight                                                          *
*                                                                        *
*       This class defines the basic point light source which is ever so *
* popular in graphics. All light from a point source is produced from a  *
* single non-dimensional point in space. Light from the point source is  *
* evenly distributed at all angles and does not fall off with distance.  *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __PTLIGHT_H__
#define __PTLIGHT_H__

#include "Light.h"

class CPointLight : public CLight
{
    public:
                        DYNAMIC_OBJECTS( CPointLight );

                        CPointLight( void );
                        CPointLight( FLT nRed, FLT nGreen, FLT nBlue, FLT nX = 0.0, FLT nY = 0.0, FLT nZ = 0.0 );
                        CPointLight( CVector oColor, CVector oPosition );
        virtual        ~CPointLight( void );

        virtual CColor  GetShade( CVector oVector );


};

#endif // __PTLIGHT_H__
