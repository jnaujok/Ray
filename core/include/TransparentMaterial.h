#ifndef __TRANSPARENT_MATERIAL__
#define __TRANSPARENT_MATERIAL__

#include "Material.h"
#include "RayStream.h"

class CTransparentMaterial : public CMaterial
{
    protected:
        CColor              m_oColor;
        FLT                 m_nIndexOfRefraction;

    public:
                            DEFINE_MATERIAL( CTransparentMaterial );

                            CTransparentMaterial( CColor oColor, FLT nIOR );

        friend CRayStream     &operator <<( CRayStream &oAr, CTransparentMaterial &oMaterial );
        friend CRayStream     &operator >>( CRayStream &oAr, CTransparentMaterial &oMaterial );

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
