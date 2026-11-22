#ifndef __SPECULAR_MATERIAL__
#define __SPECULAR_MATERIAL__

#include "Material.h"
#include "Globals.h"

class CSpecularMaterial : public CMaterial
{
    protected:
        CColorMap           m_oColor;
        CScalarMap          m_nPower;
        CScalarMap          m_nIOR;
    	BOOL                m_bMaterialHighlight;
    public:
                            DEFINE_MATERIAL( CSpecularMaterial );

                            CSpecularMaterial( CColor oColor, FLT nPower );


        virtual CColor      GetColor( CScene &oScene, CRay &oViewIncident, CColor &oLight, CVector &oToLight, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect );
        virtual CColor      GetColor( CScene &oScene, CRay &oViewIncident, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect );
        virtual void        SetColor( CColor &oColor ) { m_oColor = oColor; }
};

#endif
