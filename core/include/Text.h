/*************************************************************************
*   CText                                                                *
*                                                                        *
*   Solid letters: a string set in a TrueType font (see FontFile.h) and  *
*   extruded into a closed mesh, so it takes any material, works in a    *
*   csg, and casts and catches shadows like any solid.                   *
*                                                                        *
*   The text lies in its own x-y plane with y up, the first line's       *
*   baseline starting at the position (or centered on it, or ending at   *
*   it, by align); each line below the last by the font's line height    *
*   times lineSpacing. The face is at z = 0, facing -z, and the letters  *
*   run depth back along +z; then the whole is turned by rotation (as a  *
*   box's) and moved to position. size is the font's em in scene units   *
*   (a capital is about 0.7 of it).                                      *
*                                                                        *
*   Each glyph's curves are flattened to within 0.15% of the em, holes   *
*   found from how its outlines nest, and the face cut into triangles    *
*   (see Outline.h). The sides are smooth around curves and sharp at     *
*   corners (more than 35 degrees). bevel cuts the front edges at 45     *
*   degrees, bevel deep. Kerning comes from the font (its kern table or  *
*   GPOS pair kerning) unless kerning is false.                          *
*                                                                        *
*   The font file goes into a saved scene's archive like a model file,   *
*   and to networked engines inside the scene.                           *
*                                                                        *
*   Surface coordinates: u and v are x and y in the text's own plane,    *
*   one unit per em, on every face.                                      *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __TEXT_H__
#define __TEXT_H__

#include "Mesh.h"

#include <memory>
#include <string>

class CText : public CMesh
{
    protected:
        std::string             m_strString;        // UTF-8
        std::string             m_strFont;          // its name in the scene
        std::string             m_strFontPath;      // where on the disk it came from (empty: the archive or the scene)
        std::shared_ptr<const std::string> m_pFontData;
        FLT                     m_nSize;
        FLT                     m_nDepth;
        FLT                     m_nBevel;
        int                     m_nAlign;           // 0 left, 1 center, 2 right
        FLT                     m_nLineSpacing;
        BOOL                    m_bKerning;
        CVector                 m_oPosition;
        CVector                 m_oTextRotation;

    public:
                                DYNAMIC_OBJECTS( CText );

                                CText( void );
        virtual                ~CText( void );

            /// Sets the text from a font's bytes and builds the mesh. FALSE,
            /// with a reason in strError, if the font can't be used.
        BOOL                    SetText( const std::string &strString, const std::string &strFontData, FLT nSize, FLT nDepth,
                                         FLT nBevel, std::string &strError );
            /// Builds the mesh from the fields above.
        BOOL                    Build( std::string &strError );

};

#endif // __TEXT_H__
