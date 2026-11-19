/*************************************************************************
*   CTriangle                                                            *
*                                                                        *
*       This class defines the behavior of the basic triangle. It handles*
* all of the triangle intersection information.                          *
*                                                                        *
* Copyright (c) 1997, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __TRIANGLE_H__
#define __TRIANGLE_H__

#include "Objects.h"

class CTriangle : public CObjects
{
    protected:
        CVector             m_oCorner1;
        CVector             m_oCorner2;
        CVector             m_oCorner3;

        CVector             m_oSurfaceNormal;

        CVector             m_oNormal1;
        CVector             m_oNormal2;
        CVector             m_oNormal3;

        CColor              m_oColor;
    public:
                            DYNAMIC_OBJECTS( CTriangle );

                            CTriangle( void );
                            CTriangle( CVector oVertex1,
                                       CVector oNormal1,
                                       CVector oVertex2,
                                       CVector oNormal2,
                                       CVector oVertex3,
                                       CVector oNormal3 );
        virtual            ~CTriangle( void );

        virtual BOOL        Intersect( CRay &oRay, CVector &oIntersect );
        virtual CColor      Shade( CLight *pLight, CVector &oIntersect );
        virtual CColor      Shade( CScene &oScene, CRay &oRay, CVector &oIntersect );

        friend CStream  &operator >>( CStream &oStream, CTriangle &oSphere );
        friend CStream  &operator <<( CStream &oStream, CTriangle &oSphere );

#ifdef _INC_WINDOWS
        virtual void    ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
        virtual void    SetProperties( void );
#endif
};

#endif // __TRIANGLE_H__
