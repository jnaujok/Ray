#ifndef __PLANE_H__
#define __PLANE_H__

#include "Objects.h"
#ifdef RAY_GUI
#include "SceneTreeView.h"
#endif

class CPlane : public CObjects
{
    protected:
        CVector     m_oPosition;
        CVector     m_oNormal;
        CColor      m_oColor;
    public:
                    DYNAMIC_OBJECTS( CPlane )

                    CPlane( void );
                    CPlane( FLT nX, FLT nY, FLT nZ, CVector oNormal );
                    CPlane( CVector oPosition, CVector oNormal );
        virtual    ~CPlane( void );

        virtual BOOL        Intersect( CRay &oRay, CVector &oIntersect );
            // Distances from the plane's position along two axes across it,
            // in scene units: on a floor, u along +x and v along +z; on a
            // wall facing -z, u along +x and v up.
        virtual BOOL        SurfaceCoords( CVector &oPoint, FLT &nU, FLT &nV );
        virtual CColor      Shade( CLight *pLight, CVector &oIntersect );
        virtual CColor      Shade( CScene &oScene, CRay &oRay, CVector &oIntersect );


#ifdef RAY_GUI
        virtual void        ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
        virtual void        SetProperties( void );
#endif
};

#endif // __PLANE_H__
