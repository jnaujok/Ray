/*************************************************************************
*   CSphere                                                              *
*                                                                        *
*       This class defines the behavior of the basic sphere. It handles  *
* all of the sphere intersection and shading information.                *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __SPHERE_H__
#define __SPHERE_H__

#include "Objects.h"

class CSphere : public CObjects
{
    protected:
        CVector             m_oCenter;
        FLT                 m_nRadius;
        CColor              m_oColor;
    public:
                            DYNAMIC_OBJECTS( CSphere );

                            CSphere( void );
                            CSphere( FLT nX, FLT nY, FLT nZ, FLT nRadius );
                            CSphere( CVector oPosition, FLT nRadius );
        virtual            ~CSphere( void );

        virtual BOOL        Intersect( CRay &oRay, CVector &oIntersect );
        virtual BOOL        GetBounds( CVector &oMinimum, CVector &oMaximum );
        virtual BOOL        SurfaceNormal( CVector &oPoint, CVector &oNormal );
        virtual BOOL        IsClosed( void ) { return TRUE; }
            // Longitude and latitude: u runs once around, left to right as
            // seen from -z, and v from 0 at the bottom to 1 at the top.
        virtual BOOL        SurfaceCoords( CVector &oPoint, FLT &nU, FLT &nV );
        virtual CColor      Shade( CLight *pLight, CVector &oIntersect );
        virtual CColor      Shade( CScene &oScene, CRay &oRay, CVector &oIntersect );


#ifdef RAY_GUI
        virtual void    ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
        virtual void    SetProperties( void );
#endif
};

#endif // __SPHERE_H__
