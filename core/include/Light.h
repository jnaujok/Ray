/*************************************************************************
*   CLight header                                                        *
*                                                                        *
* This header defines the base light class for use in deriving other,    *
* more specific lights like point-lights and spot-lights.                *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __LIGHT_H__
#define __LIGHT_H__

#include "Objects.h"

class CLight : public CObjects
{
    protected:
        CColor          m_oColor;
        CVector         m_oPosition;
    public:
                        CLight( void );
                        CLight( FLT nRed, FLT nGreen, FLT nBlue, FLT nX = 0.0, FLT nY = 0.0, FLT nZ = 0.0 );
        virtual        ~CLight( void );

        virtual CVector GetPosition( void ) { return m_oPosition; }
        virtual CColor  GetColor( void ) { return m_oColor; }
        virtual CColor  GetShade( CVector oDirection ) = 0;
            /// How many points of the light a surface is shaded from: 0 for a
            /// point light (its position), or an area light's samples.
        virtual int     GetSampleCount( void ) { return 0; }
            /// The point of an area light that the point (nU, nV) of the unit
            /// square picks, as seen from oFrom. FALSE if oFrom gets no light
            /// from it (behind a one-sided light).
        virtual BOOL    SampleAt( CVector &oFrom, FLT nU, FLT nV, CVector &oPoint )
                        { (void)oFrom; (void)nU; (void)nV; oPoint = m_oPosition; return TRUE; }
        virtual BOOL    IsALight( void ) { return TRUE; }
#ifdef RAY_GUI
        virtual void    ToTree( CSceneTreeView *pTree, HTREEITEM hUnder ) = 0;
#endif
};

#endif // __LIGHT_H__
