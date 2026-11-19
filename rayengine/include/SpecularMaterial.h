#ifndef __SPECULAR_MATERIAL__
#define __SPECULAR_MATERIAL__

#include "Material.h"
#include "Stream.h"

class CSpecularMaterial : public CMaterial
{
    protected:
        CColor              m_oColor;
        FLT                 m_nPower;
        FLT                 m_nIOR;
    	BOOL                m_bMaterialHighlight;
    public:
                            DEFINE_MATERIAL( CSpecularMaterial );

                            CSpecularMaterial( CColor oColor, FLT nPower );

        friend CStream     &operator <<( CStream &oAr, CSpecularMaterial &oMaterial );
        friend CStream     &operator >>( CStream &oAr, CSpecularMaterial &oMaterial );

        virtual CColor      GetColor( CScene &oScene, CRay &oViewIncident, CColor &oLight, CVector &oToLight, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect );
        virtual CColor      GetColor( CScene &oScene, CRay &oViewIncident, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect );
        virtual void        SetColor( CColor &oColor ) { m_oColor = oColor; }
#ifdef _INC_WINDOWS
        virtual void        Properties( void );
        virtual void        ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
        virtual void        ToList( CListCtrl &oList, int nIndex, FLT nInfluence );
#endif
};

#endif
