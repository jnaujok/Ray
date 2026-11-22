    #include "Globals.h"

#include "Vector.h"

#include "DiffuseMaterial.h"
#include "SceneJson.h"


#define DIFFUSE_MATERIAL_VERSION    ((DWORD)1)

IMPLEMENT_MATERIAL( CDiffuseMaterial, Diffuse Material, "diffuse" );

CDiffuseMaterial::CDiffuseMaterial( void ) :
m_oColor( 0.5, 0.5, 0.5 )
{
    m_bDiffuseCast = FALSE;
}

CDiffuseMaterial::CDiffuseMaterial( CColor &oColor ) :
m_oColor( oColor )
{
    m_bDiffuseCast = FALSE;
}

CColor CDiffuseMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
        // Un-lit diffuse colors do nothing.
    CColor      oReturn( 0, 0, 0 );

    return oReturn;
}

CColor CDiffuseMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CColor &oLight, CVector &oToLight, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
    CVector     oToLt( oToLight );
    CColor      oLt( oLight );
    FLT         nIncidentCos;
    CColor      oDiffuse( 0, 0, 0 );
    CVector     oView( oIntersect.GetX() - oViewIncident.GetX(),
                       oIntersect.GetY() - oViewIncident.GetY(),
                       oIntersect.GetZ() - oViewIncident.GetZ() );

    if ( oLt != oDiffuse )
    {
        oToLt.Normalize();

        oNormal.Normalize();
    
            // Are we viewing from the wrong side?
        if ( oNormal.Dot( oView ) > 0.0 )
            oNormal = oNormal * -1.0;

        nIncidentCos = oNormal.Dot( oToLt );
  
        if ( nIncidentCos >= 0.0 )
        {
            oLt = oLt * nIncidentCos;
            oDiffuse = oLt * ColorAt( m_oColor, oIntersect, pIntersect );
        }
    }
    return oDiffuse;
}



void CDiffuseMaterial::ToJson( IASLib::CObjectNode &oNode )
{
    m_oColor.Write( oNode, "color" );
    RayJson::PutBool( oNode, "diffuseCast", m_bDiffuseCast );
}

BOOL CDiffuseMaterial::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "influence", "texture", "color", "diffuseCast", NULL };

    return RayJson::OnlyFields( oNode, astrFields ) &&
           m_oColor.Read( oNode, "color", FALSE ) &&
           RayJson::GetBool( oNode, "diffuseCast", m_bDiffuseCast, FALSE );
}
