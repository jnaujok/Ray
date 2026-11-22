/*************************************************************************
*   CInclude                                                             *
*                                                                        *
*   Brings the objects of another scene file into this one: a prefab,    *
*   such as an object written by raythrower --save-object or the         *
*   editor's Export Object. The file is a scene (an archive or JSON);    *
*   its objects come in (its camera, settings and lights don't), placed  *
*   again by the include's "scale", "rotation" (degrees about x, then y, *
*   then z) and "position", which apply on top of each object's own      *
*   placement. Only meshes can be placed so; an include that moves      *
*   anything else is an error.                                          *
*                                                                        *
*   The objects it brings in go into the scene's list after it, marked  *
*   as included, so the scene saves the include (and the file goes into *
*   an archive with it), not the objects. The include itself is never   *
*   hit. A task's copy of the scene carries the objects inline           *
*   ("objects"), so engines need no file.                                *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __INCLUDE_H__
#define __INCLUDE_H__

#include "Objects.h"

#include <memory>
#include <string>
#include <vector>

class CInclude : public CObjects
{
    protected:
        std::string                         m_strFile;          // as the scene names it
        std::string                         m_strPath;          // where on the disk it came from (empty: an archive)
        std::shared_ptr<const std::string>  m_pFileData;
        FLT                                 m_nScale;
        CVector                             m_oRotation;
        CVector                             m_oPosition;
            // The objects read, until the scene takes them.
        std::vector<CObjects *>             m_apObjects;
            // What the scene holds of them, to write a task's copy.
        std::vector<CObjects *>             m_apIncluded;

            // Reads the objects of a scene's JSON.
        BOOL                                ReadObjects( const IASLib::CJsonNode &oArray );

    public:
                                            DYNAMIC_OBJECTS( CInclude );

                                            CInclude( void );
        virtual                            ~CInclude( void );

        virtual BOOL                        Intersect( CRay &oRay, CVector &oIntersect ) { (void)oRay; (void)oIntersect; return FALSE; }
        virtual CColor                      Shade( CScene &oScene, CRay &oRay, CVector &oIntersect )
                                            { (void)oScene; (void)oRay; (void)oIntersect; return CColor( 0, 0, 0 ); }

            /// Hands the objects read to the scene (appended to apInto), each
            /// marked as included by this one.
        void                                TakeObjects( std::vector<CObjects *> &apInto );
        const std::string                  &GetFile( void ) const { return m_strFile; }
};

#endif // __INCLUDE_H__
