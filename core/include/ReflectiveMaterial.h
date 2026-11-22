#ifndef __REFLECTIVE_MATERIAL__
#define __REFLECTIVE_MATERIAL__

#include "Material.h"
#include "Globals.h"

class CReflectiveMaterial : public CMaterial
{
    protected:
        CColorMap           m_oColor;
        CScalarMap          m_nDiffuseReflectivity;
        BOOL                m_bFilterReflection;
            // GGX roughness: 0 is a perfect mirror; more blurs the
            // reflection, as of brushed or satin metal.
        CScalarMap          m_nRoughness;

    public:
                            DEFINE_MATERIAL( CReflectiveMaterial );

                            CReflectiveMaterial( CColor oColor, FLT nReflect );


        virtual CColor      GetColor( CScene &oScene, CRay &oViewIncident, CColor &oLight, CVector &oToLight, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect );
        virtual CColor      GetColor( CScene &oScene, CRay &oViewIncident, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect );
        virtual void        SetColor( CColor &oColor ) { m_oColor = oColor; }
};

#endif
