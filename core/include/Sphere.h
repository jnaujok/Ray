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
        virtual CColor      Shade( CLight *pLight, CVector &oIntersect );
        virtual CColor      Shade( CScene &oScene, CRay &oRay, CVector &oIntersect );

        friend CRayStream  &operator >>( CRayStream &oStream, CSphere &oSphere );
        friend CRayStream  &operator <<( CRayStream &oStream, CSphere &oSphere );

#ifdef RAY_GUI
        virtual void    ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
        virtual void    SetProperties( void );
#endif
};

#endif // __SPHERE_H__
