#ifndef __TRANSPARENT_MATERIAL__
#define __TRANSPARENT_MATERIAL__

#include "Material.h"
#include "Globals.h"

class CTransparentMaterial : public CMaterial
{
    protected:
        CColor              m_oColor;
        FLT                 m_nIndexOfRefraction;
            // Absorption inside: light that has gone m_nAttenuationDistance
            // through the object is left m_oAttenuationColor (Beer-Lambert,
            // so twice as far is that color squared). 0 absorbs nothing.
        CColor              m_oAttenuationColor;
        FLT                 m_nAttenuationDistance;
            // GGX roughness: 0 is clear glass; more blurs what is seen
            // through it, as frosted glass does.
        FLT                 m_nRoughness;
            // The glass's Abbe number: 0 doesn't disperse. Otherwise a ray in
            // every channel splits into red, green and blue rays here, each
            // bent by its own index (CLens::DispersedIndex).
        FLT                 m_nAbbe;

    public:
                            DEFINE_MATERIAL( CTransparentMaterial );

                            CTransparentMaterial( CColor oColor, FLT nIOR );


        virtual CColor      GetColor( CScene &oScene, CRay &oViewIncident, CColor &oLight, CVector &oToLight, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect );
        virtual CColor      GetColor( CScene &oScene, CRay &oViewIncident, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect );
        virtual void        SetColor( CColor &oColor ) { m_oColor = oColor; }

            // The direction a ray going along oView takes through a surface
            // with normal oNormal (either way round) into or out of glass of
            // index nIndex. FALSE for total internal reflection, when oTransmit
            // is the reflected direction instead.
            // What is left of light that has gone nDistance through the
            // object: m_oAttenuationColor to the power nDistance /
            // m_nAttenuationDistance, per channel.
        CColor              Attenuation( FLT nDistance ) const;
            // The index a ray in nChannel (CLens::EChannel) is bent by.
        FLT                 IndexFor( int nChannel ) const;
        static BOOL         Refract( CVector oView, CVector oNormal, FLT nIndex, CVector &oTransmit );
#ifdef RAY_GUI
        virtual void        Properties( void );
        virtual void        ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
        virtual void        ToList( CListCtrl &oList, int nIndex, FLT nInfluence );
#endif
};

#endif
