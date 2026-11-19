#ifndef __REFLECTIVE_MATERIAL__
#define __REFLECTIVE_MATERIAL__

#include "Material.h"
#include "RayStream.h"

class CReflectiveMaterial : public CMaterial
{
    protected:
        CColor              m_oColor;
        FLT                 m_nDiffuseReflectivity;
        BOOL                m_bFilterReflection;

    public:
                            DEFINE_MATERIAL( CReflectiveMaterial );

                            CReflectiveMaterial( CColor oColor, FLT nReflect );

        friend CRayStream     &operator <<( CRayStream &oAr, CReflectiveMaterial &oMaterial );
        friend CRayStream     &operator >>( CRayStream &oAr, CReflectiveMaterial &oMaterial );

        virtual CColor      GetColor( CScene &oScene, CRay &oViewIncident, CColor &oLight, CVector &oToLight, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect );
        virtual CColor      GetColor( CScene &oScene, CRay &oViewIncident, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect );
        virtual void        SetColor( CColor &oColor ) { m_oColor = oColor; }
#ifdef RAY_GUI
        virtual void        Properties( void );
        virtual void        ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
        virtual void        ToList( CListCtrl &oList, int nIndex, FLT nInfluence );
#endif
};

#endif
