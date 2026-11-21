/*************************************************************************
*   CCellsTexture                                                        *
*                                                                        *
*   Worley's cellular noise: space split into cells around points        *
*   scattered one a unit cube, for granite's crystals, terrazzo chips,   *
*   cracked mud or scales. "mode" picks what is looked up in the ramp:   *
*                                                                        *
*       "flecks"    (the default) a value from 0 to 1 that is the same   *
*                   all over each cell, so every cell is one chip of a   *
*                   color picked from the ramp                           *
*       "distance"  the distance to the nearest point, about 0 to 1:     *
*                   round spots                                          *
*       "edges"     the gap between the nearest point and the second     *
*                   nearest, 0 on the boundaries between cells           *
*                                                                        *
*   With turbulence, the point is moved by noise before the cells are    *
*   found, so the cells' sides wander.                                   *
*                                                                        *
*   JSON settings: those of CSolidTexture, and "mode". Default colors:   *
*   black to white.                                                      *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __CELLSTEXTURE_H__
#define __CELLSTEXTURE_H__

#include "SolidTexture.h"

class CCellsTexture : public CSolidTexture
{
    public:
        enum EMode { MODE_FLECKS = 0, MODE_DISTANCE, MODE_EDGES };

    protected:
        int                     m_nMode;

        virtual FLT             Pattern( FLT nX, FLT nY, FLT nZ ) const;
        virtual void            ExtraToJson( IASLib::CObjectNode &oNode ) const;
        virtual BOOL            ExtraFromJson( const IASLib::CJsonNode &oNode );
        virtual const char *const *ExtraFields( void ) const;

    public:
                                CCellsTexture( void );

        virtual CTexture       *Clone( void ) const { return new CCellsTexture( *this ); }
        virtual const char     *GetName( void ) const { return "cells"; }
};

#endif // __CELLSTEXTURE_H__
