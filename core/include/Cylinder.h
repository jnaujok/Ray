/*************************************************************************
*   CCylinder                                                            *
*                                                                        *
*   A cylinder about the line from its base to its top: a cone whose     *
*   ends have the same radius. Capped (the default) it is a solid; open, *
*   a tube. Surface coordinates are as on a cone (see Cone.h).           *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __CYLINDER_H__
#define __CYLINDER_H__

#include "Cone.h"

class CCylinder : public CCone
{
    public:
                            DYNAMIC_OBJECTS( CCylinder );

                            CCylinder( void );
        virtual            ~CCylinder( void );

#ifdef RAY_GUI
        virtual void        ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
#endif
};

#endif // __CYLINDER_H__
