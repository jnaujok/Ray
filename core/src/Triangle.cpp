/*************************************************************************
*   CTriangle                                                              *
*                                                                        *
*       This class defines the behavior of the basic sphere. It handles  *
* all of the sphere intersection and shading information.                *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
    #include "resource.h"
//    #include "SpherePropertiesDlg.h"
#else
    #include "Globals.h"
#endif

#include "Triangle.h"
#include "Light.h"
#include "SceneJson.h"

#ifdef RAY_GUI
#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

CTriangle::CTriangle( void )
{
    CMaterial   *pMaterial;

    pMaterial = new CDiffuseMaterial;
    m_oMaterials.Add( pMaterial, 1.0 );
    m_oColor.Set( 0.5, 0.5, 0.5 );
    m_oCorner1.Set( 0.0, 1.0, 0.0 );
    m_oCorner2.Set( 1.0, 0.0, 0.0 );
    m_oCorner3.Set( -1.0, 0.0, 0.0 );
    m_bDoubleSided = FALSE;
    m_bSmooth = FALSE;
    m_bGivenNormals = FALSE;
    m_bHasUVs = FALSE;
    m_bHasFaceColor = FALSE;
    CalcNormal();
}

CTriangle::CTriangle( const CVector &oCorner1, const CVector &oCorner2, const CVector &oCorner3,
                      BOOL bDoubleSided, BOOL bSmooth, CMaterialList *pMaterials ) :
    m_oCorner1( oCorner1 ),
    m_oCorner2( oCorner2 ),
    m_oCorner3( oCorner3 ),
    m_bDoubleSided( bDoubleSided ),
    m_bSmooth( bSmooth ),
    m_bGivenNormals( FALSE ),
    m_bHasUVs( FALSE ),
    m_bHasFaceColor( FALSE )
{
    m_oColor.Set( 0.5, 0.5, 0.5 );
    if ( pMaterials )
    {
        m_bOwnsMaterials = FALSE;
        for ( int nIndex = 0; nIndex < pMaterials->GetSize(); nIndex++ )
            m_oMaterials.Add( pMaterials->GetAt( nIndex ), pMaterials->GetInfluence( nIndex ) );
    }
    CalcNormal();
}

void CTriangle::CalcNormal( void )
{
    CVector     oTemp1;
    CVector     oTemp2;

    oTemp1 = m_oCorner2 - m_oCorner1;
    oTemp2 = m_oCorner3 - m_oCorner2;

        // The same rule OBJ and most other model formats are made for, so
        // their faces come out facing outward.
    m_oSurfaceNormal = oTemp1.Cross( oTemp2 );
    m_oSurfaceNormal.Normalize();

    m_oNormal1 = m_oSurfaceNormal;
    m_oNormal2 = m_oSurfaceNormal;
    m_oNormal3 = m_oSurfaceNormal;
}

void CTriangle::SetCornerNormals( const CVector &oNormal1, const CVector &oNormal2, const CVector &oNormal3 )
{
    m_oNormal1 = oNormal1;
    m_oNormal2 = oNormal2;
    m_oNormal3 = oNormal3;
}

CTriangle::~CTriangle( void )
{
}


#ifdef RAY_GUI
void CTriangle::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    HTREEITEM   hTriangle;

    hTriangle = pTree->AddObject( this, 24, "Triangle", hUnder );
    CObjects::MatToTree( pTree, hTriangle );
}

void CTriangle::SetProperties( void )
{
/*    CTrianglePropertiesDlg    oProp( &m_oCenter, &m_nRadius );
    int                     nCount;
    CMaterial              *pMaterial;

    oProp.m_oColor = m_oColor;
    if ( ( oProp.DoModal() == IDOK ) && ( oProp.m_bColorChanged ) )
    {
        m_oColor = oProp.m_oColor;
        nCount = 0;
        while ( nCount < m_oMaterials.GetSize() )
        {
            pMaterial = m_oMaterials[ nCount ];
            pMaterial->SetColor( m_oColor );
            nCount++;
        }
    }
*/
}
#endif

DYNAMIC_IMPLEMENTS( CTriangle, "triangle" );


BOOL CTriangle::Intersect( CRay &oRay, CVector &oIntersect )
{
    FLT     nA;
    FLT     nB;
    FLT     nC;
    FLT     nD;
    FLT     nDenom;
    FLT     nNumer;
    FLT     nT;
    CVector oLine1, oLine2, oLine3;
    FLT     nAngle;
    CVector oPoint;

        // Here we develop the parametric equation for a plane
    nA = m_oSurfaceNormal.GetX();
    nB = m_oSurfaceNormal.GetY();
    nC = m_oSurfaceNormal.GetZ();
    nD = -( (m_oSurfaceNormal.GetX() * m_oCorner1.GetX() ) + 
            (m_oCorner1.GetY() * m_oSurfaceNormal.GetY() ) +
            (m_oCorner1.GetZ() * m_oSurfaceNormal.GetZ() ) );

        // The solution is given by t= -(ax+by+cz+d)/(ai+bj+ck)

    nNumer = - ( (nA * oRay.GetX()) + ( nB * oRay.GetY() ) + ( nC * oRay.GetZ() ) + nD );
    nDenom = ( nA * oRay.GetI()) + ( nB * oRay.GetJ() ) + ( nC * oRay.GetK() );

    if ( nDenom == 0.0 )
        return FALSE;

        // A single-sided triangle can't be hit from behind (a ray that
        // travels the way its normal points).
    if ( ! m_bDoubleSided && ( nDenom > 0.0 ) )
        return FALSE;

    nT = nNumer / nDenom;

    if ( ( nT > 0.0 ) && ( nT < oRay.GetT() ) )
    {
            // We know that we intersect the plane of the triangle, now we limit it to within
            // the boundaries of the triangle.
        oPoint.Set( (oRay.GetI() * nT) + oRay.GetX(), 
                    (oRay.GetJ() * nT) + oRay.GetY(),
                    (oRay.GetK() * nT) + oRay.GetZ() );
        nAngle = 0.0;
        oLine1 = m_oCorner1 - oPoint;
        oLine2 = m_oCorner2 - oPoint;
        oLine3 = m_oCorner3 - oPoint;
        oLine1.Normalize();
        oLine2.Normalize();
        oLine3.Normalize();

        nAngle = ACOS( oLine1.Dot( oLine2 ) );
        nAngle += ACOS( oLine2.Dot( oLine3 ) );
        nAngle += ACOS( oLine3.Dot( oLine1 ) );

        nAngle = ABS( nAngle - ( 2 * PI ) );

        if ( nAngle < 1e-6 )
        {
            oRay.SetT( nT );
            oIntersect = oPoint;
            return TRUE;
        }
    }

        // No intersection
    return FALSE;
}

CColor CTriangle::Shade( CLight *pLight, CVector &oIntersect ) // CArray <CLight *, CLight *> *pLight, CVector &oIntersect )
{
    CColor  oColor;
    return oColor;
}

    // Barycentric weights of a point: the share of the triangle facing each
    // corner. On an edge the far corner's weight is 0, so faces that share the
    // edge agree along it.
BOOL CTriangle::Weights( CVector &oPoint, FLT &nWeight1, FLT &nWeight2, FLT &nWeight3 )
{
    CVector oEdge1 = m_oCorner2 - m_oCorner1;
    CVector oEdge2 = m_oCorner3 - m_oCorner1;
    CVector oToHit = oPoint - m_oCorner1;
    FLT     n11 = oEdge1.Dot( oEdge1 );
    FLT     n12 = oEdge1.Dot( oEdge2 );
    FLT     n22 = oEdge2.Dot( oEdge2 );
    FLT     nH1 = oToHit.Dot( oEdge1 );
    FLT     nH2 = oToHit.Dot( oEdge2 );
    FLT     nDenom = ( n11 * n22 ) - ( n12 * n12 );

    if ( nDenom == 0.0 )
        return FALSE;
    nWeight2 = ( ( n22 * nH1 ) - ( n12 * nH2 ) ) / nDenom;
    nWeight3 = ( ( n11 * nH2 ) - ( n12 * nH1 ) ) / nDenom;
    nWeight1 = 1.0 - nWeight2 - nWeight3;
    return TRUE;
}

BOOL CTriangle::SurfaceCoords( CVector &oPoint, FLT &nU, FLT &nV )
{
    FLT nWeight1, nWeight2, nWeight3;

    nU = nV = 0.0;
    if ( ! m_bHasUVs || ! Weights( oPoint, nWeight1, nWeight2, nWeight3 ) )
        return FALSE;
    nU = ( nWeight1 * m_anUVs[ 0 ] ) + ( nWeight2 * m_anUVs[ 2 ] ) + ( nWeight3 * m_anUVs[ 4 ] );
    nV = ( nWeight1 * m_anUVs[ 1 ] ) + ( nWeight2 * m_anUVs[ 3 ] ) + ( nWeight3 * m_anUVs[ 5 ] );
    return TRUE;
}

void CTriangle::SetCornerUVs( const FLT anUVs[ 6 ] )
{
    for ( int nIndex = 0; nIndex < 6; nIndex++ )
        m_anUVs[ nIndex ] = anUVs[ nIndex ];
    m_bHasUVs = TRUE;
}

CVector CTriangle::NormalAt( CVector &oPoint )
{
    CVector oNormal( m_oSurfaceNormal );
    FLT     nWeight1, nWeight2, nWeight3;

    if ( m_bSmooth )
    {
        if ( Weights( oPoint, nWeight1, nWeight2, nWeight3 ) )
        {
            CVector oPart1 = m_oNormal1 * nWeight1;
            CVector oPart2 = m_oNormal2 * nWeight2;
            CVector oPart3 = m_oNormal3 * nWeight3;

            oNormal = oPart1 + oPart2;
            oNormal = oNormal + oPart3;
            oNormal.Normalize();
        }
    }
    return oNormal;
}

CColor CTriangle::Shade( CScene &oScene, CRay &oRay, CVector &oIntersect )
{
    CVector     oNormal = NormalAt( oIntersect );
    CVector     oDirection( oRay.GetI(), oRay.GetJ(), oRay.GetK() );

        // Seen from behind, a double-sided triangle shows its back face.
    if ( m_bDoubleSided && ( m_oSurfaceNormal.Dot( oDirection ) > 0.0 ) )
        oNormal = oNormal * -1.0;

    return BaseShade( oScene, oRay, oIntersect, oNormal );
}

    // Reads an array of three [x, y, z] vectors (with bNormals, each made a
    // unit vector, and none may be zero).
static BOOL ReadThree( const IASLib::CJsonNode &oNode, const char *strKey, CVector &oFirst, CVector &oSecond, CVector &oThird,
                       BOOL bNormals = FALSE )
{
    const IASLib::CJsonNode *pArray = RayJson::Field( oNode, strKey, TRUE );
    RayJson::CWhere          oWhere( strKey );

    if ( pArray == NULL )
        return FALSE;
    if ( ! pArray->isArray() || pArray->size() != 3 )
        return RayJson::Fail( "must be an array of three [x, y, z] arrays" );

    CVector *apoVectors[ 3 ] = { &oFirst, &oSecond, &oThird };

    for ( int nIndex = 0; nIndex < 3; nIndex++ )
    {
        RayJson::CWhere oElement( "", nIndex );

        if ( ! RayJson::ToVector( *pArray->get( nIndex ), *apoVectors[ nIndex ] ) )
            return FALSE;
        if ( bNormals )
        {
            if ( apoVectors[ nIndex ]->Magnitude() == 0.0 )
                return RayJson::Fail( "a normal can't be [0, 0, 0]" );
            apoVectors[ nIndex ]->Normalize();
        }
    }
    return TRUE;
}

void CTriangle::ToJson( IASLib::CObjectNode &oNode )
{
    IASLib::CArrayNode *pCorners = oNode.putArray( "corners" );
    RayJson::AddVector( *pCorners, m_oCorner1 );
    RayJson::AddVector( *pCorners, m_oCorner2 );
    RayJson::AddVector( *pCorners, m_oCorner3 );
    RayJson::PutBool( oNode, "doubleSided", m_bDoubleSided );
    RayJson::PutBool( oNode, "smooth", m_bSmooth );
    if ( m_bGivenNormals )
    {
        IASLib::CArrayNode *pNormals = oNode.putArray( "normals" );
        RayJson::AddVector( *pNormals, m_oNormal1 );
        RayJson::AddVector( *pNormals, m_oNormal2 );
        RayJson::AddVector( *pNormals, m_oNormal3 );
    }
    if ( m_bHasUVs )
    {
        IASLib::CArrayNode *pUVs = oNode.putArray( "uvs" );

        for ( int nCorner = 0; nCorner < 3; nCorner++ )
            pUVs->addArray()->add( (double)m_anUVs[ nCorner * 2 ] )->add( (double)m_anUVs[ ( nCorner * 2 ) + 1 ] );
    }
    RayJson::PutColor( oNode, "color", m_oColor );
    m_oMaterials.ToJson( *oNode.putArray( "materials" ) );
    BumpToJson( oNode );
}

BOOL CTriangle::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "corners", "doubleSided", "smooth", "normals", "uvs", "color", "materials", "bump", NULL };
    BOOL    bNormals = oNode.isObject() && ( oNode.get( "normals" ) != NULL );

        // Given corner normals are what smooth shading blends, so they make
        // the triangle smooth; saying "smooth": false with them is an error.
    m_bSmooth = bNormals;
    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! ReadThree( oNode, "corners", m_oCorner1, m_oCorner2, m_oCorner3 ) ||
         ! RayJson::GetBool( oNode, "doubleSided", m_bDoubleSided, FALSE ) ||
         ! RayJson::GetBool( oNode, "smooth", m_bSmooth, FALSE ) )
        return FALSE;
    if ( bNormals && ! m_bSmooth )
        return RayJson::Fail( "normals are only used by smooth shading, so \"smooth\" can't be false with them" );

    CalcNormal();
    if ( m_oSurfaceNormal.Magnitude() == 0.0 )
        return RayJson::Fail( "the corners are in a line, so the triangle has no surface" );

    m_bGivenNormals = bNormals;
    if ( bNormals && ! ReadThree( oNode, "normals", m_oNormal1, m_oNormal2, m_oNormal3, TRUE ) )
        return FALSE;

    const IASLib::CJsonNode *pUVs = oNode.get( "uvs" );

    m_bHasUVs = FALSE;
    if ( pUVs )
    {
        RayJson::CWhere oWhere( "uvs" );

        if ( ! pUVs->isArray() || pUVs->size() != 3 )
            return RayJson::Fail( "must be an array of three [u, v] pairs, one for each corner" );
        for ( int nCorner = 0; nCorner < 3; nCorner++ )
        {
            const IASLib::CJsonNode *pPair = pUVs->get( nCorner );

            if ( ! pPair->isArray() || pPair->size() != 2 || ! pPair->get( 0 )->isNumber() || ! pPair->get( 1 )->isNumber() )
            {
                RayJson::CWhere oElement( "", nCorner );
                return RayJson::Fail( "must be a [u, v] pair" );
            }
            m_anUVs[ nCorner * 2 ] = (FLT)pPair->get( 0 )->doubleValue();
            m_anUVs[ ( nCorner * 2 ) + 1 ] = (FLT)pPair->get( 1 )->doubleValue();
        }
        m_bHasUVs = TRUE;
    }

    return RayJson::GetColor( oNode, "color", m_oColor, FALSE ) &&
           m_oMaterials.FromJson( oNode, "materials" ) &&
           BumpFromJson( oNode );
}

    // A box a little larger than [anLow, anHigh], so hits a hair outside the
    // exact shape (as some tests allow) are inside it.
static void PadBounds( FLT anLow[ 3 ], FLT anHigh[ 3 ], CVector &oMinimum, CVector &oMaximum )
{
    FLT nMargin = 1e-6 * ( 1.0 + ( anHigh[ 0 ] - anLow[ 0 ] ) + ( anHigh[ 1 ] - anLow[ 1 ] ) + ( anHigh[ 2 ] - anLow[ 2 ] ) );

    oMinimum.Set( anLow[ 0 ] - nMargin, anLow[ 1 ] - nMargin, anLow[ 2 ] - nMargin );
    oMaximum.Set( anHigh[ 0 ] + nMargin, anHigh[ 1 ] + nMargin, anHigh[ 2 ] + nMargin );
}

BOOL CTriangle::GetBounds( CVector &oMinimum, CVector &oMaximum )
{
    FLT anLow[ 3 ] = { 1e300, 1e300, 1e300 };
    FLT anHigh[ 3 ] = { -1e300, -1e300, -1e300 };

    for ( int nCorner = 0; nCorner < 3; nCorner++ )
    {
        CVector &oCorner = GetCorner( nCorner );
        FLT      anValue[ 3 ] = { oCorner.GetX(), oCorner.GetY(), oCorner.GetZ() };

        for ( int nAxis = 0; nAxis < 3; nAxis++ )
        {
            anLow[ nAxis ] = ( anValue[ nAxis ] < anLow[ nAxis ] ) ? anValue[ nAxis ] : anLow[ nAxis ];
            anHigh[ nAxis ] = ( anValue[ nAxis ] > anHigh[ nAxis ] ) ? anValue[ nAxis ] : anHigh[ nAxis ];
        }
    }
        // A triangle's hit test allows a point slightly outside its edges.
    FLT nSlack = 1e-3 * ( ( anHigh[ 0 ] - anLow[ 0 ] ) + ( anHigh[ 1 ] - anLow[ 1 ] ) + ( anHigh[ 2 ] - anLow[ 2 ] ) );

    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        anLow[ nAxis ] -= nSlack;
        anHigh[ nAxis ] += nSlack;
    }
    PadBounds( anLow, anHigh, oMinimum, oMaximum );
    return TRUE;
}
