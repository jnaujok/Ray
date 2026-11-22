/*************************************************************************
*   CExtrusion                                                           *
*                                                                        *
*   A flat shape pushed back into a solid: outlines of lines and curves  *
*   (see Profile.h), holes worked out from how they nest (see Outline.h) *
*   and swept straight back along +z by depth, as text's letters are     *
*   (the face at z = 0 facing -z). bevel and backBevel cut the front and *
*   back edges at 45 degrees; taper scales the shape toward the back and *
*   twist turns it (degrees, counterclockwise seen from the front), both *
*   about the outline's origin. Bevels can't be used with a taper or a   *
*   twist. Then the whole is turned by rotation (as a box's) and moved   *
*   to position.                                                         *
*                                                                        *
*   An extrusion is built into a closed mesh when it is loaded: curves   *
*   flattened to within 0.1% of the outline's size, the sides smooth     *
*   around curves and sharp at corners. It works in a csg and takes any  *
*   material.                                                            *
*                                                                        *
*   Surface coordinates: u and v are x and y of the outline, on every    *
*   face.                                                                *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __EXTRUSION_H__
#define __EXTRUSION_H__

#include "Mesh.h"
#include "Profile.h"

#include <string>
#include <vector>

class CExtrusion : public CMesh
{
    protected:
        std::vector<Profile::SPath> m_aoOutlines;
        BOOL                    m_bSingle;          // given as "outline" rather than "outlines"
        BOOL                    m_bSmooth;
        FLT                     m_nDepth;
        FLT                     m_nBevel;
        FLT                     m_nBackBevel;
        FLT                     m_nTaper;
        FLT                     m_nTwist;           // degrees
        CVector                 m_oPosition;
        CVector                 m_oShapeRotation;

    public:
                                DYNAMIC_OBJECTS( CExtrusion );

                                CExtrusion( void );
        virtual                ~CExtrusion( void );

            /// Builds the mesh from the fields above. FALSE, with a reason
            /// in strError, if the outlines can't be filled.
        BOOL                    Build( std::string &strError );

};

#endif // __EXTRUSION_H__
