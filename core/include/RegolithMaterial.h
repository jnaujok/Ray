/*************************************************************************
*   CRegolithMaterial                                                    *
*                                                                        *
*   Lunar soil, after Hapke: the Lommel-Seeliger law, which scatters     *
*   light back toward where it came from and darkens no limb, times an   *
*   opposition surge that brightens it sharply where the viewer looks    *
*   along the light (the glow around an astronaut's own shadow):         *
*                                                                        *
*       light * color * 2 mu0 / (mu0 + mu) * (1 + B(g)) / (1 + surge)    *
*       B(g) = surge / (1 + tan(g / 2) / width)                          *
*                                                                        *
*   where mu0 and mu are the cosines of the light's and the viewer's     *
*   angles from the normal and g is the phase angle between them. Seen   *
*   along the light, at normal incidence, it gives exactly the color.    *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __REGOLITH_MATERIAL__
#define __REGOLITH_MATERIAL__

#include "Material.h"
#include "Globals.h"

class CRegolithMaterial : public CMaterial
{
    protected:
        CColorMap           m_oColor;
        FLT                 m_nSurge;       // Hapke's B0
        FLT                 m_nWidth;       // Hapke's h

    public:
                            DEFINE_MATERIAL( CRegolithMaterial );

        virtual BOOL        LitBySky( void ) { return TRUE; }

            /// The brightness factor for cosines mu0 (light) and mu (viewer)
            /// and phase angle nPhase (radians), before the color.
        FLT                 Factor( FLT nMu0, FLT nMu, FLT nPhase ) const;

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
