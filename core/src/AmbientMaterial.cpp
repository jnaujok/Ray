    #include "Globals.h"

#include "Vector.h"

#include "AmbientMaterial.h"
#include "SceneJson.h"



#define AMBIENT_MATERIAL_VERSION    ((DWORD)1)

IMPLEMENT_MATERIAL( CAmbientMaterial, Ambient Material, "ambient" );

CAmbientMaterial::CAmbientMaterial( void ) :
m_oColor( .3, .3, .3 )
{
}

CAmbientMaterial::CAmbientMaterial( CColor &oColor ) :
m_oColor( oColor )
{
}

CColor CAmbientMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
        // Simply return the ambient color and let the influence adjust the light. 
        // That's what ambient means...
    return ColorAt( m_oColor, oIntersect, pIntersect );
}

CColor CAmbientMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CColor &oLight, CVector &oToLight, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
        // Ambient color does not depend on light sources
    CColor      oAmbient( 0, 0, 0 );

    return oAmbient;
}



void CAmbientMaterial::ToJson( IASLib::CObjectNode &oNode )
{
    m_oColor.Write( oNode, "color" );
}

BOOL CAmbientMaterial::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "influence", "texture", "color", NULL };

    return RayJson::OnlyFields( oNode, astrFields ) &&
           m_oColor.Read( oNode, "color", FALSE );
}
