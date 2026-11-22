/*************************************************************************
*   CSweep                                                               *
*                                                                        *
*   A tube: a circle of radius (or any small outline, see Profile.h)     *
*   carried along a path through the scene, for pipes, rails, springs    *
*   and neon tubes. The path is a list of points [x, y, z]; smooth (the  *
*   default) makes it a curve through them (Catmull-Rom), otherwise it   *
*   runs straight from point to point. A closed path joins its last      *
*   point back to its first. The cross-section is turned along the path  *
*   as little as it can be (a rotation-minimizing frame, by double       *
*   reflection), so the tube doesn't twist; a closed path's frame is     *
*   evened out so it meets itself.                                       *
*                                                                        *
*   A sweep is built into a smooth mesh when it is loaded: steps pieces  *
*   along each span of the path and segments around. An open path's     *
*   ends are closed with flat caps unless capped is false.               *
*                                                                        *
*   Surface coordinates: u runs around the cross-section (0 to 1), v     *
*   along the path (0 to 1 by length).                                   *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __SWEEP_H__
#define __SWEEP_H__

#include "Mesh.h"
#include "Profile.h"

#include <string>
#include <vector>

class CSweep : public CMesh
{
    protected:
        std::vector<CVector>    m_aoPath;
        BOOL                    m_bSmooth;
        BOOL                    m_bClosedPath;
        FLT                     m_nRadius;
        Profile::SPath          m_oOutline;         // empty: a circle of m_nRadius
        BOOL                    m_bOutline;
        int                     m_nSegments;
        int                     m_nSteps;
        BOOL                    m_bCapped;

    public:
                                DYNAMIC_OBJECTS( CSweep );

                                CSweep( void );
        virtual                ~CSweep( void );

            /// Builds the mesh from the fields above. FALSE, with a reason
            /// in strError, if it can't be made.
        BOOL                    Build( std::string &strError );

};

#endif // __SWEEP_H__
