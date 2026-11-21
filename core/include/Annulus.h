/*************************************************************************
*   CAnnulus                                                             *
*                                                                        *
*   A flat ring: a disc (see Disc.h) whose innerRadius must be given and *
*   be more than 0.                                                      *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __ANNULUS_H__
#define __ANNULUS_H__

#include "Disc.h"

class CAnnulus : public CDisc
{
    protected:
        virtual BOOL        ShapeFromJson( const IASLib::CJsonNode &oNode );

    public:
                            DYNAMIC_OBJECTS( CAnnulus );

                            CAnnulus( void );
        virtual            ~CAnnulus( void );
};

#endif // __ANNULUS_H__
