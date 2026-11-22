#include "Globals.h"
#include "Tessellator.h"

#include "Mesh.h"
#include "LensElements.h"
#include "LocalShape.h"
#include "Objects.h"
#include "Scene.h"
#include "SceneJson.h"

#include <algorithm>
#include <memory>

namespace RayEditor
{
    namespace
    {
        const IASLib::CJsonNode *Get( const IASLib::CJsonNode &oNode, const char *strKey )
        {
            return oNode.isObject() ? oNode.get( strKey ) : NULL;
        }

        double Number( const IASLib::CJsonNode &oNode, const char *strKey, double nDefault )
        {
            const IASLib::CJsonNode *pValue = Get( oNode, strKey );

            return ( pValue && pValue->isNumber() ) ? pValue->asDouble( nDefault ) : nDefault;
        }

        bool Bool( const IASLib::CJsonNode &oNode, const char *strKey, bool bDefault )
        {
            const IASLib::CJsonNode *pValue = Get( oNode, strKey );

            return ( pValue && pValue->isBoolean() ) ? pValue->asBoolean( bDefault ) : bDefault;
        }

        std::string Text( const IASLib::CJsonNode &oNode, const char *strKey )
        {
            const IASLib::CJsonNode *pValue = Get( oNode, strKey );

            return ( pValue && pValue->isTextual() ) ? pValue->textString() : std::string();
        }

        void PutVec( std::vector<float> &anOut, const SVec3 &v )
        {
            anOut.push_back( (float)v.x );
            anOut.push_back( (float)v.y );
            anOut.push_back( (float)v.z );
        }

            // A triangle, with a normal at each corner.
        void Triangle( SDrawMesh &oMesh, const SVec3 &a, const SVec3 &b, const SVec3 &c,
                       const SVec3 &na, const SVec3 &nb, const SVec3 &nc )
        {
            PutVec( oMesh.anPositions, a );
            PutVec( oMesh.anPositions, b );
            PutVec( oMesh.anPositions, c );
            PutVec( oMesh.anNormals, na );
            PutVec( oMesh.anNormals, nb );
            PutVec( oMesh.anNormals, nc );
        }

            // A flat triangle, its normal from its corners.
        void FlatTriangle( SDrawMesh &oMesh, const SVec3 &a, const SVec3 &b, const SVec3 &c )
        {
            SVec3 n = ( b - a ).Cross( c - b ).Normalized();

            Triangle( oMesh, a, b, c, n, n, n );
        }

        void Quad( SDrawMesh &oMesh, const SVec3 &a, const SVec3 &b, const SVec3 &c, const SVec3 &d, const SVec3 &n )
        {
            Triangle( oMesh, a, b, c, n, n, n );
            Triangle( oMesh, a, c, d, n, n, n );
        }

        void AddLine( SDrawLines &oLines, const SVec3 &a, const SVec3 &b )
        {
            PutVec( oLines.anPoints, a );
            PutVec( oLines.anPoints, b );
        }

        void Circle( SDrawLines &oLines, const SVec3 &oCenter, const SVec3 &u, const SVec3 &v, double nRadius, int nSegments )
        {
            for ( int i = 0; i < nSegments; i++ )
            {
                double a0 = 2.0 * EDITOR_PI * i / nSegments, a1 = 2.0 * EDITOR_PI * ( i + 1 ) / nSegments;

                AddLine( oLines, oCenter + ( u * ( nRadius * std::cos( a0 ) ) ) + ( v * ( nRadius * std::sin( a0 ) ) ),
                                 oCenter + ( u * ( nRadius * std::cos( a1 ) ) ) + ( v * ( nRadius * std::sin( a1 ) ) ) );
            }
        }

        void Sphere( SDrawMesh &oMesh, const SVec3 &c, double r, int nSegments )
        {
            int nRings = (std::max)( 4, nSegments / 2 );

            for ( int j = 0; j < nRings; j++ )
            {
                double t0 = EDITOR_PI * j / nRings, t1 = EDITOR_PI * ( j + 1 ) / nRings;

                for ( int i = 0; i < nSegments; i++ )
                {
                    double p0 = 2.0 * EDITOR_PI * i / nSegments, p1 = 2.0 * EDITOR_PI * ( i + 1 ) / nSegments;
                    auto   At = [ & ]( double t, double p ) { return SVec3( std::sin( t ) * std::cos( p ), std::cos( t ), std::sin( t ) * std::sin( p ) ); };
                    SVec3  n00 = At( t0, p0 ), n01 = At( t0, p1 ), n10 = At( t1, p0 ), n11 = At( t1, p1 );

                    if ( j > 0 )
                        Triangle( oMesh, c + n00 * r, c + n01 * r, c + n11 * r, n00, n01, n11 );
                    if ( j < nRings - 1 )
                        Triangle( oMesh, c + n00 * r, c + n11 * r, c + n10 * r, n00, n11, n10 );
                }
            }
        }

            // A cone or cylinder from base to top, with each end's radius.
        void Cone( SDrawMesh &oMesh, const SVec3 &oBase, const SVec3 &oTop, double nBase, double nTop, bool bCapped, int nSegments )
        {
            SVec3   oAxis = oTop - oBase;
            double  nHeight = oAxis.Length();

            if ( nHeight <= 0.0 )
                return;
            oAxis = oAxis * ( 1.0 / nHeight );

            SVec3   u = Perpendicular( oAxis );
            SVec3   v = oAxis.Cross( u ).Normalized();
            double  nSlope = ( nBase - nTop ) / nHeight;       // how the side leans

            for ( int i = 0; i < nSegments; i++ )
            {
                double a0 = 2.0 * EDITOR_PI * i / nSegments, a1 = 2.0 * EDITOR_PI * ( i + 1 ) / nSegments;
                SVec3  d0 = ( u * std::cos( a0 ) ) + ( v * std::sin( a0 ) );
                SVec3  d1 = ( u * std::cos( a1 ) ) + ( v * std::sin( a1 ) );
                SVec3  n0 = ( d0 + ( oAxis * nSlope ) ).Normalized();
                SVec3  n1 = ( d1 + ( oAxis * nSlope ) ).Normalized();
                SVec3  b0 = oBase + d0 * nBase, b1 = oBase + d1 * nBase;
                SVec3  t0 = oTop + d0 * nTop, t1 = oTop + d1 * nTop;

                Triangle( oMesh, b0, b1, t1, n0, n1, n1 );
                Triangle( oMesh, b0, t1, t0, n0, n1, n0 );
                if ( bCapped )
                {
                    if ( nBase > 0.0 )
                        Triangle( oMesh, oBase, b1, b0, -oAxis, -oAxis, -oAxis );
                    if ( nTop > 0.0 )
                        Triangle( oMesh, oTop, t0, t1, oAxis, oAxis, oAxis );
                }
            }
        }

        void Box( SDrawMesh &oMesh, const SVec3 &c, const SVec3 &oSize, const SVec3 &oRotation )
        {
            SVec3 ax = RotateXYZ( SVec3( 1, 0, 0 ), oRotation.x, oRotation.y, oRotation.z ) * ( oSize.x * 0.5 );
            SVec3 ay = RotateXYZ( SVec3( 0, 1, 0 ), oRotation.x, oRotation.y, oRotation.z ) * ( oSize.y * 0.5 );
            SVec3 az = RotateXYZ( SVec3( 0, 0, 1 ), oRotation.x, oRotation.y, oRotation.z ) * ( oSize.z * 0.5 );
            SVec3 aoAxes[ 3 ] = { ax, ay, az };

            for ( int nAxis = 0; nAxis < 3; nAxis++ )
            {
                for ( int nSign = -1; nSign <= 1; nSign += 2 )
                {
                    SVec3 n = aoAxes[ nAxis ] * (double)nSign;
                    SVec3 a = aoAxes[ ( nAxis + 1 ) % 3 ], b = aoAxes[ ( nAxis + 2 ) % 3 ];
                    SVec3 f = c + n;

                    Quad( oMesh, f - a - b, f + a - b, f + a + b, f - a + b, n.Normalized() );
                }
            }
        }

        void BoundsBox( SDrawMesh &oMesh, CObjects &oObject )
        {
            CVector oMin, oMax;

            if ( ! oObject.GetBounds( oMin, oMax ) )
                return;

            SVec3 a( oMin.GetX(), oMin.GetY(), oMin.GetZ() ), b( oMax.GetX(), oMax.GetY(), oMax.GetZ() );

            Box( oMesh, ( a + b ) * 0.5, b - a, SVec3() );
        }

        void Average( const IASLib::CJsonNode *pColors, float anColor[ 4 ] )
        {
            if ( ! pColors || ! pColors->isArray() || pColors->size() == 0 )
                return;

            SVec3   oSum;
            int     nCount = 0;

            for ( int i = 0; i < pColors->size(); i++ )
            {
                SVec3 c;

                if ( JsonVector( pColors->get( i ), c ) )
                {
                    oSum = oSum + c;
                    nCount++;
                }
            }
            if ( nCount > 0 )
            {
                anColor[ 0 ] = (float)( oSum.x / nCount );
                anColor[ 1 ] = (float)( oSum.y / nCount );
                anColor[ 2 ] = (float)( oSum.z / nCount );
            }
        }
    }

    bool JsonVector( const IASLib::CJsonNode *pNode, SVec3 &oOut )
    {
        if ( ! pNode || ! pNode->isArray() || pNode->size() != 3 )
            return false;
        for ( int i = 0; i < 3; i++ )
        {
            if ( ! pNode->get( i ) || ! pNode->get( i )->isNumber() )
                return false;
        }
        oOut = SVec3( pNode->get( 0 )->asDouble(), pNode->get( 1 )->asDouble(), pNode->get( 2 )->asDouble() );
        return true;
    }

    void PreviewColor( const IASLib::CJsonNode &oObject, float anColor[ 4 ] )
    {
        anColor[ 0 ] = anColor[ 1 ] = anColor[ 2 ] = 0.7f;
        anColor[ 3 ] = 1.0f;

        const IASLib::CJsonNode *pMaterials = Get( oObject, "materials" );
        const IASLib::CJsonNode *pChosen = NULL;
        double                   nTransparent = 0.0, nReflective = 0.0;

        if ( pMaterials && pMaterials->isArray() )
        {
            for ( int i = 0; i < pMaterials->size(); i++ )
            {
                const IASLib::CJsonNode *pMaterial = pMaterials->get( i );
                std::string              strType = pMaterial ? Text( *pMaterial, "type" ) : std::string();

                if ( strType == "diffuse" && ! pChosen )
                    pChosen = pMaterial;
                else if ( strType == "transparent" )
                    nTransparent = (std::max)( nTransparent, Number( *pMaterial, "influence", 0.0 ) );
                else if ( strType == "reflective" )
                    nReflective = (std::max)( nReflective, Number( *pMaterial, "influence", 0.0 ) );
            }
            if ( ! pChosen && nTransparent <= 0.5 && nReflective <= 0.5 )
            {
                    // No diffuse color, glass or mirror: any material that
                    // isn't the ambient light.
                for ( int i = 0; i < pMaterials->size() && ! pChosen; i++ )
                {
                    const IASLib::CJsonNode *pMaterial = pMaterials->get( i );
                    std::string              strType = pMaterial ? Text( *pMaterial, "type" ) : std::string();

                    if ( strType != "ambient" && strType != "transparent" && strType != "reflective" )
                        pChosen = pMaterial;
                }
            }
        }

        SVec3 c;

        if ( pChosen && JsonVector( Get( *pChosen, "color" ), c ) )
        {
            anColor[ 0 ] = (float)c.x;
            anColor[ 1 ] = (float)c.y;
            anColor[ 2 ] = (float)c.z;
        }
        else if ( JsonVector( Get( oObject, "color" ), c ) )
        {
            anColor[ 0 ] = (float)c.x;
            anColor[ 1 ] = (float)c.y;
            anColor[ 2 ] = (float)c.z;
        }
        if ( pChosen )
        {
            const IASLib::CJsonNode *pTexture = Get( *pChosen, "texture" );

            if ( pTexture && pTexture->isObject() )
            {
                if ( Get( *pTexture, "colors" ) )
                    Average( Get( *pTexture, "colors" ), anColor );
                else if ( Text( *pTexture, "type" ) == "image" || Text( *pTexture, "type" ) == "faceColors" )
                    anColor[ 0 ] = anColor[ 1 ] = anColor[ 2 ] = 0.75f;
            }
        }
        if ( ! pChosen && nReflective > 0.5 )
        {
                // A mirror: a pale blue gray.
            anColor[ 0 ] = 0.72f;
            anColor[ 1 ] = 0.76f;
            anColor[ 2 ] = 0.82f;
        }
        if ( nTransparent > 0.5 )
        {
            if ( ! pChosen )
            {
                const IASLib::CJsonNode *pTint = NULL;

                for ( int i = 0; pMaterials && i < pMaterials->size(); i++ )
                {
                    if ( pMaterials->get( i ) && Text( *pMaterials->get( i ), "type" ) == "transparent" )
                    {
                        pTint = Get( *pMaterials->get( i ), "attenuationColor" );
                        if ( ! pTint )
                            pTint = Get( *pMaterials->get( i ), "color" );
                        break;
                    }
                }
                if ( JsonVector( pTint, c ) )
                {
                    anColor[ 0 ] = (float)c.x;
                    anColor[ 1 ] = (float)c.y;
                    anColor[ 2 ] = (float)c.z;
                }
            }
            anColor[ 3 ] = 0.35f;
        }
        for ( int i = 0; i < 3; i++ )
            anColor[ i ] = (std::min)( 1.0f, (std::max)( 0.0f, anColor[ i ] ) );
    }

        // A mesh's faces (its lean geometry), smooth where it has vertex normals.
    bool MeshTriangles( SDrawMesh &oMesh, CMesh &oSource )
    {
        if ( ! oSource.GetGeometry() )
            return false;

        const CMesh::SGeometry     &oGeometry = *oSource.GetGeometry();
        const std::vector<CVector> &aoVertices = oSource.GetVertices();
        bool                        bSmooth = ( oGeometry.aoVertexNormals.size() == aoVertices.size() );

        for ( size_t nFace = 0; nFace * 3 + 2 < oGeometry.anKept.size() && nFace < oGeometry.aoFaceNormals.size(); nFace++ )
        {
            SVec3 p[ 3 ], n[ 3 ];

            for ( int k = 0; k < 3; k++ )
            {
                int      nVertex = oGeometry.anKept[ nFace * 3 + (size_t)k ];
                CVector  oV( aoVertices[ (size_t)nVertex ] );
                CVector  oN( bSmooth ? oGeometry.aoVertexNormals[ (size_t)nVertex ] : oGeometry.aoFaceNormals[ nFace ] );

                p[ k ] = SVec3( oV.GetX(), oV.GetY(), oV.GetZ() );
                n[ k ] = SVec3( oN.GetX(), oN.GetY(), oN.GetZ() );
            }
            Triangle( oMesh, p[ 0 ], p[ 1 ], p[ 2 ], n[ 0 ], n[ 1 ], n[ 2 ] );
        }
        return true;
    }

    bool TessellateObject( const IASLib::CJsonNode &oObject, CObjects *pObject, int nObject, double nPlane,
                           std::vector<SDrawMesh> &aoOut, int nSegments )
    {
        std::string strType = Text( oObject, "type" );
        SDrawMesh   oMesh;
        SVec3       a, b, c;

        oMesh.nObject = nObject;
        PreviewColor( oObject, oMesh.anColor );
        if ( nSegments < 6 )
            nSegments = 6;

        if ( strType == "sphere" )
        {
            if ( ! JsonVector( Get( oObject, "center" ), c ) )
                return false;
            Sphere( oMesh, c, Number( oObject, "radius", 1.0 ), nSegments );
        }
        else if ( strType == "plane" )
        {
            SVec3 n;

            if ( ! JsonVector( Get( oObject, "position" ), c ) || ! JsonVector( Get( oObject, "normal" ), n ) )
                return false;
            n = n.Normalized();

            SVec3 u = Perpendicular( n ), v = n.Cross( u ).Normalized();

            u = u * nPlane;
            v = v * nPlane;
            Quad( oMesh, c - u - v, c - u + v, c + u + v, c + u - v, n );
            oMesh.bPlane = true;
        }
        else if ( strType == "box" )
        {
            SVec3 oSize( 1, 1, 1 ), oRotation;

            if ( ! JsonVector( Get( oObject, "center" ), c ) )
                return false;
            if ( Get( oObject, "size" ) && Get( oObject, "size" )->isNumber() )
            {
                double n = Number( oObject, "size", 1.0 );

                oSize = SVec3( n, n, n );
            }
            else
                JsonVector( Get( oObject, "size" ), oSize );
            JsonVector( Get( oObject, "rotation" ), oRotation );
            Box( oMesh, c, oSize, oRotation );
        }
        else if ( strType == "cylinder" || strType == "cone" )
        {
            if ( ! JsonVector( Get( oObject, "base" ), a ) || ! JsonVector( Get( oObject, "top" ), b ) )
                return false;

            double nBase = ( strType == "cylinder" ) ? Number( oObject, "radius", 1.0 ) : Number( oObject, "baseRadius", 1.0 );
            double nTop = ( strType == "cylinder" ) ? nBase : Number( oObject, "topRadius", 0.0 );

            Cone( oMesh, a, b, nBase, nTop, Bool( oObject, "capped", true ), nSegments );
        }
        else if ( strType == "triangle" )
        {
            const IASLib::CJsonNode *pCorners = Get( oObject, "corners" );

            if ( ! pCorners || ! pCorners->isArray() || pCorners->size() != 3 ||
                 ! JsonVector( pCorners->get( 0 ), a ) || ! JsonVector( pCorners->get( 1 ), b ) || ! JsonVector( pCorners->get( 2 ), c ) )
                return false;
            FlatTriangle( oMesh, a, b, c );
        }
        else if ( strType == "torus" || strType == "disc" || strType == "annulus" || strType == "paraboloid" ||
                  strType == "hyperboloid" || strType == "superquadric" || strType == "roundedBox" ||
                  strType == "bezier" || strType == "bspline" || strType == "lathe" ||
                  strType == "implicit" || strType == "blob" || strType == "fractal" )
        {
                // The core draws its own shapes (a csg part has no object of
                // its own here, so one is made from its JSON).
            std::unique_ptr<CObjects> pMade;
            CLocalShape              *pShape = dynamic_cast<CLocalShape *>( pObject );

            if ( ! pShape )
            {
                CDynList oTypes;

                pMade.reset( oTypes.Create( strType.c_str() ) );
                pShape = dynamic_cast<CLocalShape *>( pMade.get() );
                if ( ! pShape || ! pShape->FromJson( oObject ) )
                    return false;
            }

            std::vector<FLT> anPositions, anNormals;

            pShape->Triangles( nSegments, anPositions, anNormals );
            for ( size_t n = 0; n + 8 < anPositions.size(); n += 9 )
            {
                SVec3 p[ 3 ], q[ 3 ];

                for ( int k = 0; k < 3; k++ )
                {
                    p[ k ] = SVec3( anPositions[ n + 3 * k ], anPositions[ n + 3 * k + 1 ], anPositions[ n + 3 * k + 2 ] );
                    q[ k ] = SVec3( anNormals[ n + 3 * k ], anNormals[ n + 3 * k + 1 ], anNormals[ n + 3 * k + 2 ] );
                }
                Triangle( oMesh, p[ 0 ], p[ 1 ], p[ 2 ], q[ 0 ], q[ 1 ], q[ 2 ] );
            }
        }
        else if ( strType == "mesh" || strType == "text" || strType == "extrusion" || strType == "sweep" )
        {
            CMesh *pMesh = dynamic_cast<CMesh *>( pObject );

            if ( ! pMesh || ! pMesh->GetGeometry() )
                return false;

            const CMesh::SGeometry     &oGeometry = *pMesh->GetGeometry();
            const std::vector<CVector> &aoVertices = pMesh->GetVertices();
            bool                        bSmooth = ( oGeometry.aoVertexNormals.size() == aoVertices.size() );

            oMesh.anPositions.reserve( oGeometry.anKept.size() * 3 );
            oMesh.anNormals.reserve( oGeometry.anKept.size() * 3 );
            for ( size_t nFace = 0; nFace * 3 + 2 < oGeometry.anKept.size() && nFace < oGeometry.aoFaceNormals.size(); nFace++ )
            {
                SVec3 p[ 3 ], n[ 3 ];

                for ( int k = 0; k < 3; k++ )
                {
                    int      nVertex = oGeometry.anKept[ nFace * 3 + (size_t)k ];
                    CVector  oV( aoVertices[ (size_t)nVertex ] );
                    CVector  oN( bSmooth ? oGeometry.aoVertexNormals[ (size_t)nVertex ] : oGeometry.aoFaceNormals[ nFace ] );

                    p[ k ] = SVec3( oV.GetX(), oV.GetY(), oV.GetZ() );
                    n[ k ] = SVec3( oN.GetX(), oN.GetY(), oN.GetZ() );
                }
                Triangle( oMesh, p[ 0 ], p[ 1 ], p[ 2 ], n[ 0 ], n[ 1 ], n[ 2 ] );
            }
        }
        else if ( strType == "lensElements" )
        {
                // The ray tubes as they are; the glass, stop and barrel (csgs
                // of spheres) faintly, as their boxes.
            CLensElements *pLens = dynamic_cast<CLensElements *>( pObject );

            if ( ! pLens )
                return false;

            const std::vector<CObjects *> &apParts = pLens->GetParts();
            size_t                         nFirstRay = apParts.size() - (size_t)pLens->GetRayCount();
            bool                           bAny = false;

            for ( size_t nPart = 0; nPart < apParts.size(); nPart++ )
            {
                SDrawMesh oPart;

                oPart.nObject = oMesh.nObject;
                std::copy( oMesh.anColor, oMesh.anColor + 4, oPart.anColor );
                if ( nPart >= nFirstRay )
                {
                    CMesh *pTube = dynamic_cast<CMesh *>( apParts[ nPart ] );

                    if ( ! pTube || ! MeshTriangles( oPart, *pTube ) )
                        continue;
                }
                else
                {
                    BoundsBox( oPart, *apParts[ nPart ] );
                    oPart.bGhost = true;
                }
                if ( ! oPart.anPositions.empty() )
                {
                    aoOut.push_back( std::move( oPart ) );
                    bAny = true;
                }
            }
            return bAny;
        }
        else if ( strType == "csg" )
        {
            const IASLib::CJsonNode *pParts = Get( oObject, "objects" );
            bool                     bAny = false;

            if ( ! pParts || ! pParts->isArray() )
                return false;
            for ( int i = 0; i < pParts->size(); i++ )
            {
                std::vector<SDrawMesh> aoParts;

                if ( pParts->get( i ) && TessellateObject( *pParts->get( i ), NULL, nObject, nPlane, aoParts, nSegments ) )
                {
                    for ( SDrawMesh &oPart : aoParts )
                    {
                        std::copy( oMesh.anColor, oMesh.anColor + 4, oPart.anColor );
                        oPart.bGhost = true;
                        aoOut.push_back( std::move( oPart ) );
                        bAny = true;
                    }
                }
            }
            return bAny;
        }
        else if ( pObject )
            BoundsBox( oMesh, *pObject );

        if ( oMesh.anPositions.empty() )
            return false;
        aoOut.push_back( std::move( oMesh ) );
        return true;
    }

    bool LightIcon( const IASLib::CJsonNode &oLight, int nLight, const SVec3 &oCenter, double nSize, SDrawLines &oOut )
    {
        std::string strType = Text( oLight, "type" );
        SVec3       p, d, c;
        double      s = (std::max)( 1e-3, nSize * 0.08 );

        oOut.nLight = nLight;
        if ( JsonVector( Get( oLight, "color" ), c ) )
        {
            double nMax = (std::max)( c.x, (std::max)( c.y, c.z ) );

            if ( nMax > 0.0 )
            {
                oOut.anColor[ 0 ] = (float)( 0.4 + 0.6 * c.x / nMax );
                oOut.anColor[ 1 ] = (float)( 0.4 + 0.6 * c.y / nMax );
                oOut.anColor[ 2 ] = (float)( 0.4 + 0.6 * c.z / nMax );
            }
        }

        if ( strType == "directional" )
        {
            if ( ! JsonVector( Get( oLight, "direction" ), d ) )
                return false;
            d = d.Normalized();

            SVec3 u = Perpendicular( d ), v = d.Cross( u ).Normalized();
            SVec3 oStart = oCenter - ( d * ( nSize * 0.75 ) );

            for ( int i = -1; i <= 1; i++ )
            {
                SVec3 oFrom = oStart + ( u * ( i * nSize * 0.15 ) );
                SVec3 oTo = oFrom + ( d * ( nSize * 0.3 ) );

                AddLine( oOut, oFrom, oTo );
                AddLine( oOut, oTo, oTo - ( d * ( nSize * 0.06 ) ) + ( v * ( nSize * 0.04 ) ) );
                AddLine( oOut, oTo, oTo - ( d * ( nSize * 0.06 ) ) - ( v * ( nSize * 0.04 ) ) );
            }
            return true;
        }

        if ( ! JsonVector( Get( oLight, "position" ), p ) )
            return false;

            // Every light with a place has a small star there.
        AddLine( oOut, p - SVec3( s, 0, 0 ), p + SVec3( s, 0, 0 ) );
        AddLine( oOut, p - SVec3( 0, s, 0 ), p + SVec3( 0, s, 0 ) );
        AddLine( oOut, p - SVec3( 0, 0, s ), p + SVec3( 0, 0, s ) );

        if ( strType == "sphereLight" )
        {
            double r = Number( oLight, "radius", 1.0 );

            Circle( oOut, p, SVec3( 1, 0, 0 ), SVec3( 0, 1, 0 ), r, 32 );
            Circle( oOut, p, SVec3( 1, 0, 0 ), SVec3( 0, 0, 1 ), r, 32 );
            Circle( oOut, p, SVec3( 0, 1, 0 ), SVec3( 0, 0, 1 ), r, 32 );
        }
        else if ( strType == "rectangleLight" )
        {
            SVec3 e1, e2;

            if ( JsonVector( Get( oLight, "edge1" ), e1 ) && JsonVector( Get( oLight, "edge2" ), e2 ) )
            {
                SVec3 h1 = e1 * 0.5, h2 = e2 * 0.5;
                SVec3 q[ 4 ] = { p - h1 - h2, p + h1 - h2, p + h1 + h2, p - h1 + h2 };

                for ( int i = 0; i < 4; i++ )
                    AddLine( oOut, q[ i ], q[ ( i + 1 ) % 4 ] );
                    // Which way it shines.
                AddLine( oOut, p, p + ( e1.Cross( e2 ).Normalized() * ( s * 3.0 ) ) );
            }
        }
        else if ( strType == "spot" )
        {
            if ( JsonVector( Get( oLight, "direction" ), d ) )
            {
                d = d.Normalized();

                double nAngle = Number( oLight, "angle", 30.0 );
                double nLength = nSize * 0.3;
                double r = nLength * std::tan( (std::min)( 89.0, nAngle ) * EDITOR_PI / 180.0 );
                SVec3  u = Perpendicular( d ), v = d.Cross( u ).Normalized();
                SVec3  oEnd = p + ( d * nLength );

                Circle( oOut, oEnd, u, v, r, 32 );
                for ( int i = 0; i < 4; i++ )
                {
                    double a = EDITOR_PI * 0.5 * i;

                    AddLine( oOut, p, oEnd + ( u * ( r * std::cos( a ) ) ) + ( v * ( r * std::sin( a ) ) ) );
                }
            }
        }
        return true;
    }

    bool CameraIcon( const IASLib::CJsonNode &oCamera, double nAspect, double nLength, SDrawLines &oOut )
    {
        SVec3 p, t, up( 0, 1, 0 );

        if ( ! JsonVector( Get( oCamera, "position" ), p ) || ! JsonVector( Get( oCamera, "lookAt" ), t ) )
            return false;
        JsonVector( Get( oCamera, "up" ), up );

        double nFov = Number( oCamera, "verticalAngle", 30.0 );
        SVec3  f = ( t - p ).Normalized();

        if ( f.Length() <= 0.0 )
            return false;
        if ( std::fabs( f.Dot( up.Normalized() ) ) > 0.9999 )
            up = Perpendicular( f );

        SVec3  r = up.Cross( f ).Normalized(), u = f.Cross( r ).Normalized();
        double h = nLength * std::tan( nFov * EDITOR_PI / 360.0 ), w = h * ( ( nAspect > 0.0 ) ? nAspect : 4.0 / 3.0 );
        SVec3  m = p + ( f * nLength );
        SVec3  q[ 4 ] = { m - r * w - u * h, m + r * w - u * h, m + r * w + u * h, m - r * w + u * h };

        oOut.nLight = -1;
        oOut.anColor[ 0 ] = 0.6f;
        oOut.anColor[ 1 ] = 0.9f;
        oOut.anColor[ 2 ] = 1.0f;
        for ( int i = 0; i < 4; i++ )
        {
            AddLine( oOut, p, q[ i ] );
            AddLine( oOut, q[ i ], q[ ( i + 1 ) % 4 ] );
        }
            // A tick above the top edge shows which way is up.
        AddLine( oOut, m + u * h, m + u * ( h * 1.3 ) );
        return true;
    }

    void TessellateScene( CScene &oScene, SSceneDrawing &oOut, int nSegments )
    {
        oOut = SSceneDrawing();

            // The box around everything with bounds sets how large planes,
            // light icons and the camera's frustum are drawn.
        for ( int i = 0; i < oScene.GetObjectCount(); i++ )
        {
            CVector oMin, oMax;

            if ( oScene.GetObjectAt( i ) && oScene.GetObjectAt( i )->GetBounds( oMin, oMax ) )
            {
                SVec3 a( oMin.GetX(), oMin.GetY(), oMin.GetZ() ), b( oMax.GetX(), oMax.GetY(), oMax.GetZ() );

                if ( ! oOut.bHasBounds )
                {
                    oOut.oMinimum = a;
                    oOut.oMaximum = b;
                    oOut.bHasBounds = true;
                }
                else
                {
                    oOut.oMinimum = SVec3( (std::min)( oOut.oMinimum.x, a.x ), (std::min)( oOut.oMinimum.y, a.y ), (std::min)( oOut.oMinimum.z, a.z ) );
                    oOut.oMaximum = SVec3( (std::max)( oOut.oMaximum.x, b.x ), (std::max)( oOut.oMaximum.y, b.y ), (std::max)( oOut.oMaximum.z, b.z ) );
                }
            }
        }
        if ( ! oOut.bHasBounds )
        {
            oOut.oMinimum = SVec3( -5, -1, -5 );
            oOut.oMaximum = SVec3( 5, 5, 5 );
        }

        SVec3   oCenter = ( oOut.oMinimum + oOut.oMaximum ) * 0.5;
        double  nSize = (std::max)( 1.0, ( oOut.oMaximum - oOut.oMinimum ).Length() );

        for ( int i = 0; i < oScene.GetObjectCount(); i++ )
        {
            std::unique_ptr<IASLib::CJsonNode> pNode( RayJson::Parse( oScene.PartToJson( "objects/" + std::to_string( i ) ) ) );

            if ( pNode )
                TessellateObject( *pNode, oScene.GetObjectAt( i ), i, nSize * 2.0, oOut.aoMeshes, nSegments );
        }
        for ( int i = 0; i < oScene.GetLightCount(); i++ )
        {
            std::unique_ptr<IASLib::CJsonNode> pNode( RayJson::Parse( oScene.PartToJson( "lights/" + std::to_string( i ) ) ) );
            SDrawLines                         oLines;

            if ( pNode && LightIcon( *pNode, i, oCenter, nSize, oLines ) )
                oOut.aoLines.push_back( std::move( oLines ) );
        }

        std::unique_ptr<IASLib::CJsonNode> pCamera( RayJson::Parse( oScene.PartToJson( "camera" ) ) );
        SDrawLines                         oCamera;
        CSettings                         &oSettings = oScene.GetSettings();
        double                             nAspect = ( oSettings.GetImageHeight() > 0 ) ?
                                                     (double)oSettings.GetImageWidth() / (double)oSettings.GetImageHeight() : 4.0 / 3.0;

        if ( pCamera && CameraIcon( *pCamera, nAspect, nSize * 0.08, oCamera ) )
            oOut.aoLines.push_back( std::move( oCamera ) );
    }
}
