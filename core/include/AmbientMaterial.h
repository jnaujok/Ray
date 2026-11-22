#ifndef __AMBIENT_MATERIAL__
#define __AMBIENT_MATERIAL__

#include "Material.h"
#include "Globals.h"

class CAmbientMaterial : public CMaterial
{
    protected:
        CColorMap           m_oColor;
    public:
                            DEFINE_MATERIAL( CAmbientMaterial );

                            CAmbientMaterial( CColor &oColor );


        virtual CColor      GetColor( CScene &oScene, CRay &oViewIncident, CColor &oLight, CVector &oToLight, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect );
        virtual CColor      GetColor( CScene &oScene, CRay &oViewIncident, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect );
        virtual void        SetColor( CColor &oColor ) { m_oColor = oColor; }
        virtual BOOL        Glows( void ) { return TRUE; }
};

#endif
