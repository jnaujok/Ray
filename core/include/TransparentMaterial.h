#ifndef __TRANSPARENT_MATERIAL__
#define __TRANSPARENT_MATERIAL__

#include "Material.h"
#include "Globals.h"

class CTransparentMaterial : public CMaterial
{
    protected:
        CColorMap           m_oColor;
        CScalarMap          m_nIndexOfRefraction;
            // Absorption inside: light that has gone m_nAttenuationDistance
            // through the object is left m_oAttenuationColor (Beer-Lambert,
            // so twice as far is that color squared). 0 absorbs nothing.
            // Either may be a texture: then the absorption is added up along
            // the ray's path inside, a step (m_nAttenuationStep) at a time, so
            // a solid texture colors the glass from within.
        CColorMap           m_oAttenuationColor;
        CScalarMap          m_nAttenuationDistance;
        FLT                 m_nAttenuationStep;
            // GGX roughness: 0 is clear glass; more blurs what is seen
            // through it, as frosted glass does.
        CScalarMap          m_nRoughness;
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
            // Whether the absorption inside varies (a texture drives it).
        BOOL                IsVolume( void ) const { return ( m_oAttenuationColor.IsMapped() || m_nAttenuationDistance.IsMapped() ) ? TRUE : FALSE; }
            // What is left of light that has crossed the object from oFrom
            // for nLength along the unit vector oDirection, adding up the
            // absorption the textures give a step at a time. nJitter (0 to 1)
            // shifts where in each step it is read.
        CColor              VolumeTransmittance( const CVector &oFrom, const CVector &oDirection, FLT nLength,
                                                 const CObjects *pObject, FLT nJitter ) const;
        FLT                 GetAttenuationStep( void ) const { return m_nAttenuationStep; }
            // The index a ray in nChannel (CLens::EChannel) is bent by, for
            // glass of index nBase at the d line (the constant if not given).
        FLT                 IndexFor( int nChannel ) const { return IndexFor( nChannel, m_nIndexOfRefraction ); }
        FLT                 IndexFor( int nChannel, FLT nBase ) const;
        static BOOL         Refract( CVector oView, CVector oNormal, FLT nIndex, CVector &oTransmit );
#ifdef RAY_GUI
        virtual void        Properties( void );
        virtual void        ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
        virtual void        ToList( CListCtrl &oList, int nIndex, FLT nInfluence );
#endif
};

#endif
