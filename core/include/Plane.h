#ifndef __PLANE_H__
#define __PLANE_H__

#include "Objects.h"

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
        virtual BOOL        PlaneOf( CVector &oPoint, CVector &oNormal ) { oPoint = m_oPosition; oNormal = m_oNormal; return TRUE; }
            // Distances from the plane's position along two axes across it,
            // in scene units: on a floor, u along +x and v along +z; on a
            // wall facing -z, u along +x and v up.
        virtual BOOL        SurfaceCoords( CVector &oPoint, FLT &nU, FLT &nV );
        virtual CColor      Shade( CLight *pLight, CVector &oIntersect );
        virtual CColor      Shade( CScene &oScene, CRay &oRay, CVector &oIntersect );


};

#endif // __PLANE_H__
