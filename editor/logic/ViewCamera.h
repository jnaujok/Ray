/*************************************************************************
*   CViewCamera                                                          *
*                                                                        *
*   The scene editor's view: an eye looking at a target, which the       *
*   mouse orbits, pans and zooms, or which is set from the scene's own   *
*   camera to look through it. It makes the OpenGL matrices for the      *
*   shaded view and the ray through a pixel for picking, both the same   *
*   way the tracer's camera aims its rays (CCamera::RayDirection), so a  *
*   render of the view lines up with the view.                           *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __VIEWCAMERA_H__
#define __VIEWCAMERA_H__

#include "EditorMath.h"

namespace RayEditor
{
    class CViewCamera
    {
        protected:
            SVec3       m_oEye;
            SVec3       m_oTarget;
            SVec3       m_oUp;
            double      m_nFovY;            // degrees, top to bottom

        public:
                        CViewCamera( void );

            const SVec3 &GetEye( void ) const { return m_oEye; }
            const SVec3 &GetTarget( void ) const { return m_oTarget; }
            const SVec3 &GetUp( void ) const { return m_oUp; }
            double      GetFovY( void ) const { return m_nFovY; }
            void        Set( const SVec3 &oEye, const SVec3 &oTarget, const SVec3 &oUp, double nFovY );

                /// Turns the eye about the target: nYaw degrees about the up
                /// direction, nPitch degrees up or down (kept short of the
                /// poles).
            void        Orbit( double nYaw, double nPitch );
                /// Slides eye and target across the view: nX and nY in
                /// fractions of the view's height at the target's distance.
            void        Pan( double nX, double nY );
                /// Moves the eye toward the target (nFactor < 1) or away.
            void        Zoom( double nFactor );
                /// Looks at a box from the current direction, close enough to
                /// see all of it.
            void        Frame( const SVec3 &oMinimum, const SVec3 &oMaximum );

                /// The view's unit vectors: where it looks, and its image's
                /// right and top edges.
            void        Basis( SVec3 &oForward, SVec3 &oRight, SVec3 &oTop ) const;
                /// The direction of the ray through a point on the image:
                /// nScreenX from -1 (left) to 1 (right), nScreenY from -1
                /// (bottom) to 1 (top), nAspect width over height, as
                /// CCamera::RayDirection.
            SVec3       RayDirection( double nScreenX, double nScreenY, double nAspect ) const;
                /// The ray through pixel (nX, nY) of a nWidth by nHeight view,
                /// counted from the top left; through the pixel's center.
            void        PixelRay( double nX, double nY, int nWidth, int nHeight, SVec3 &oOrigin, SVec3 &oDirection ) const;

                /// OpenGL's projection and view matrices for a view of this
                /// shape, with the near and far planes given.
            void        Projection( double nAspect, double nNear, double nFar, double anOut[ 16 ] ) const;
            void        View( double anOut[ 16 ] ) const;
    };
}

#endif // __VIEWCAMERA_H__
