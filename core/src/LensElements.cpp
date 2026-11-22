#include "Globals.h"

#include "LensElements.h"
#include "Lens.h"
#include "LocalShape.h"
#include "Rays.h"
#include "SceneJson.h"
#include "JSON/ObjectNode.h"
#include "JSON/ArrayNode.h"

#include <cmath>
#include <cstdio>
#include <memory>

DYNAMIC_IMPLEMENTS( CLensElements, "lensElements" );

namespace
{
        // A flat surface, or the cutaway's cut, is a sphere this large (mm):
        // across a 60 mm element it bows by under 0.01 mm.
    const FLT BIG_RADIUS = 100000.0;

    std::string Number( FLT nValue )
    {
        char achBuffer[ 40 ];

        snprintf( achBuffer, sizeof( achBuffer ), "%.17g", (double)nValue );
        return achBuffer;
    }

    std::string Point( CVector oPoint )
    {
        return "[" + Number( oPoint.GetX() ) + ", " + Number( oPoint.GetY() ) + ", " + Number( oPoint.GetZ() ) + "]";
    }

    std::string Rgb( FLT nR, FLT nG, FLT nB )
    {
        return "[" + Number( nR ) + ", " + Number( nG ) + ", " + Number( nB ) + "]";
    }

        // The surface's sag at a height: where it is on the axis, relative to
        // its vertex, at that distance from the axis.
    FLT Sag( FLT nRadius, FLT nHeight )
    {
        if ( nRadius == 0.0 )
            return 0.0;

        FLT nReach = (std::min)( nHeight, std::fabs( nRadius ) * 0.999999 );

        return nRadius - ( ( nRadius > 0.0 ) ? 1.0 : -1.0 ) * SQRT( ( nRadius * nRadius ) - ( nReach * nReach ) );
    }

    const char *const s_strMetal =
        "[{\"type\": \"ambient\", \"influence\": 1, \"color\": [0.03, 0.03, 0.03]},"
        " {\"type\": \"diffuse\", \"influence\": 0.5, \"color\": [0.16, 0.16, 0.17], \"diffuseCast\": false},"
        " {\"type\": \"specular\", \"influence\": 0.2, \"color\": [1, 1, 1], \"power\": 0.3, \"indexOfRefraction\": 1.5,"
        " \"materialHighlight\": false}]";

        // The colors of the fans after the first (on-axis) one.
    const FLT s_aanFanColors[][ 3 ] =
    {
        { 1.0, 0.62, 0.1 }, { 1.0, 0.25, 0.85 }, { 0.15, 0.9, 1.0 }, { 1.0, 1.0, 0.3 }, { 0.6, 1.0, 0.4 }
    };
}

CLensElements::CLensElements( void ) :
    m_nFNumber( 0.0 ),
    m_nScale( 0.1 ),
    m_oPosition( 0.0, 0.0, 0.0 ),
    m_oRotation( 0.0, 0.0, 0.0 ),
    m_bCutaway( TRUE ),
    m_bBarrel( FALSE ),
    m_bFans( TRUE ),
    m_nRays( 7 ),
    m_nTubeRadius( 0.25 ),
    m_nFanOpacity( 0.5 ),
    m_nLead( 0.0 ),
    m_nSensorWidth( 0.0 ),
    m_nElements( 0 ),
    m_nRayCount( 0 ),
    m_bHasBounds( FALSE ),
    m_pHitPart( NULL ),
    m_pShadePart( NULL )
{
    m_anFields.push_back( 0.0 );
    m_anFields.push_back( 0.7 );
}

CLensElements::~CLensElements( void )
{
    Clear();
}

void CLensElements::Clear( void )
{
    for ( size_t nIndex = 0; nIndex < m_apParts.size(); nIndex++ )
        delete m_apParts[ nIndex ];
    m_apParts.clear();
    m_aaoRayPaths.clear();
    m_nElements = 0;
    m_nRayCount = 0;
    m_bHasBounds = FALSE;
    m_pHitPart = NULL;
    m_pShadePart = NULL;
}

CVector CLensElements::ToScene( FLT nX, FLT nY, FLT nZ ) const
{
    CVector oRotation( m_oRotation );
    CVector oPosition( m_oPosition );
    FLT nToRadians = PI / 180.0;
    FLT anPoint[ 3 ] = { nX * m_nScale, nY * m_nScale, nZ * m_nScale };

    CLocalShape::RotateXYZ( anPoint, oRotation.GetX() * nToRadians, oRotation.GetY() * nToRadians, oRotation.GetZ() * nToRadians );
    return CVector( anPoint[ 0 ] + oPosition.GetX(), anPoint[ 1 ] + oPosition.GetY(), anPoint[ 2 ] + oPosition.GetZ() );
}

BOOL CLensElements::Build( std::string &strError )
{
    CLens oLens;

    Clear();
    if ( ! oLens.SetPrescription( m_strPrescription.c_str() ) )
    {
        int                         nCount = 0;
        const CLens::SPrescription *aoPrescriptions = CLens::GetPrescriptions( nCount );
        std::string                 strNames;

        for ( int nIndex = 0; nIndex < nCount; nIndex++ )
            strNames += std::string( nIndex ? ", " : "" ) + aoPrescriptions[ nIndex ].strName;
        strError = "has no built-in lens \"" + m_strPrescription + "\" (there are " + strNames + ")";
        return FALSE;
    }
    if ( m_nFNumber > 0.0 )
        oLens.SetFNumber( m_nFNumber );
    if ( m_nSensorWidth > 0.0 )
        oLens.SetSensorWidth( m_nSensorWidth );
    oLens.SetFocusDistance( 1e9 );

    std::string strLens;

    if ( ! oLens.Prepare( 1e9, strLens ) )
    {
        strError = "lens can't be prepared: " + strLens;
        return FALSE;
    }

    const std::vector<CLens::SSurface> &aoSurfaces = oLens.GetSurfaces();
    int                                 nSurfaces = (int)aoSurfaces.size();
    FLT                                 nScaledBig = BIG_RADIUS * m_nScale;
    FLT                                 nWidest = 0.0;
    std::vector<std::string>            astrParts;

    for ( int nIndex = 0; nIndex < nSurfaces; nIndex++ )
        nWidest = (std::max)( nWidest, aoSurfaces[ (size_t)nIndex ].nAperture );

        // Pieces of JSON, in scene coordinates.
    auto Sphere = [ & ]( FLT nX, FLT nY, FLT nZ, FLT nRadius )
    {
        return "{\"type\": \"sphere\", \"center\": " + Point( ToScene( nX, nY, nZ ) ) + ", \"radius\": " +
               Number( nRadius * m_nScale ) + "}";
    };
    auto Cylinder = [ & ]( FLT nZ0, FLT nZ1, FLT nRadius )
    {
        return "{\"type\": \"cylinder\", \"base\": " + Point( ToScene( 0.0, 0.0, nZ0 ) ) + ", \"top\": " +
               Point( ToScene( 0.0, 0.0, nZ1 ) ) + ", \"radius\": " + Number( nRadius * m_nScale ) + ", \"capped\": true}";
    };
    auto Csg = []( const char *strOperation, const std::string &strA, const std::string &strB, const std::string &strTail )
    {
        return std::string( "{\"type\": \"csg\", \"operation\": \"" ) + strOperation + "\", \"objects\": [" + strA + ", " + strB +
               "]" + strTail + "}";
    };
    auto Cut = [ & ]( const std::string &strSolid, const std::string &strTail )
    {
            // Keeps the lens's +x half: inside a huge sphere touching x = 0.
        if ( ! m_bCutaway )
        {
            if ( strTail.empty() )
                return strSolid;
                // Give the solid its materials: put them after its last field.
            return strSolid.substr( 0, strSolid.size() - 1 ) + strTail + "}";
        }
        std::string strCut = "{\"type\": \"sphere\", \"center\": " + Point( ToScene( BIG_RADIUS, 0.0, 0.0 ) ) + ", \"radius\": " +
                             Number( nScaledBig ) + "}";

        return Csg( "intersection", strSolid, strCut, strTail );
    };

        // The glass: each run of glass between two surfaces.
    for ( int nIndex = 0; nIndex + 1 < nSurfaces; nIndex++ )
    {
        FLT nIndexD, nAbbe;

        oLens.GlassAfter( nIndex, nIndexD, nAbbe );
        if ( ! ( nIndexD > 1.0001 ) )
            continue;

        const CLens::SSurface &oFront = aoSurfaces[ (size_t)nIndex ];
        const CLens::SSurface &oBack = aoSurfaces[ (size_t)nIndex + 1 ];
        FLT                    nFrontZ = oLens.GetVertexZ( nIndex );
        FLT                    nBackZ = oLens.GetVertexZ( nIndex + 1 );
        FLT                    nRadius = (std::min)( oFront.nAperture, oBack.nAperture );
        FLT                    nLow = (std::min)( nFrontZ, nFrontZ + Sag( oFront.nRadius, nRadius ) ) - 1.0;
        FLT                    nHigh = (std::max)( nBackZ, nBackZ + Sag( oBack.nRadius, nRadius ) ) + 1.0;
        std::string            strSolid = Cylinder( nLow, nHigh, nRadius );

            // Behind the front surface: inside its sphere when the center is
            // behind it, outside when in front.
        if ( oFront.nRadius == 0.0 )
            strSolid = Csg( "intersection", strSolid, Sphere( 0.0, 0.0, nFrontZ + BIG_RADIUS, BIG_RADIUS ), "" );
        else
            strSolid = Csg( ( oFront.nRadius > 0.0 ) ? "intersection" : "difference", strSolid,
                            Sphere( 0.0, 0.0, nFrontZ + oFront.nRadius, std::fabs( oFront.nRadius ) ), "" );
            // In front of the back surface: the other way round.
        if ( oBack.nRadius == 0.0 )
            strSolid = Csg( "intersection", strSolid, Sphere( 0.0, 0.0, nBackZ - BIG_RADIUS, BIG_RADIUS ), "" );
        else
            strSolid = Csg( ( oBack.nRadius > 0.0 ) ? "difference" : "intersection", strSolid,
                            Sphere( 0.0, 0.0, nBackZ + oBack.nRadius, std::fabs( oBack.nRadius ) ), "" );

            // Glass that reads as solid in a diagram: a pale blue tint and a
            // little diffuse light on top of the clear glass.
        std::string strGlass =
            ", \"color\": [1, 1, 1], \"materials\": ["
            "{\"type\": \"ambient\", \"influence\": 1, \"color\": [0.03, 0.05, 0.08]},"
            " {\"type\": \"diffuse\", \"influence\": 0.22, \"color\": [0.55, 0.75, 1.0], \"diffuseCast\": false},"
            " {\"type\": \"reflective\", \"influence\": 0.08, \"color\": [1, 1, 1], \"diffuseReflectivity\": 0,"
            " \"filterReflection\": false},"
            " {\"type\": \"transparent\", \"influence\": 0.7, \"color\": [0.7, 0.85, 1.0], \"indexOfRefraction\": " +
            Number( nIndexD ) + ( ( nAbbe >= 5.0 ) ? ", \"abbe\": " + Number( nAbbe ) : std::string() ) + "},"
            " {\"type\": \"specular\", \"influence\": 0.35, \"color\": [1, 1, 1], \"power\": 0.08, \"indexOfRefraction\": " +
            Number( nIndexD ) + ", \"materialHighlight\": false}]";

        astrParts.push_back( Cut( strSolid, strGlass ) );
        m_nElements++;
    }

        // The stop: a thin ring, opened to the f-number.
    for ( int nIndex = 0; nIndex < nSurfaces; nIndex++ )
    {
        if ( ! aoSurfaces[ (size_t)nIndex ].bStop )
            continue;

        FLT         nZ = oLens.GetVertexZ( nIndex );
        FLT         nOpen = oLens.GetStopRadius();
        FLT         nOuter = (std::max)( nOpen * 1.6, aoSurfaces[ (size_t)nIndex ].nAperture * 1.3 );
        std::string strRing = Csg( "difference", Cylinder( nZ - 0.3, nZ + 0.3, nOuter ), Cylinder( nZ - 1.0, nZ + 1.0, nOpen ), "" );

        astrParts.push_back( Cut( strRing, std::string( ", \"color\": [1, 1, 1], \"materials\": " ) + s_strMetal ) );
    }

        // The barrel: a tube around the glass.
    if ( m_bBarrel && nSurfaces > 0 )
    {
        FLT         nFront = -1.5;
        FLT         nBack = oLens.GetVertexZ( nSurfaces - 1 ) + 1.5;
        std::string strTube = Csg( "difference", Cylinder( nFront, nBack, nWidest * 1.22 ),
                                   Cylinder( nFront - 1.0, nBack + 1.0, nWidest * 1.08 ), "" );

        astrParts.push_back( Cut( strTube, std::string( ", \"color\": [1, 1, 1], \"materials\": " ) + s_strMetal ) );
    }

        // The ray fans.
    if ( m_bFans )
    {
        FLT nFocal = oLens.GetFocalLength();
        FLT nWidth = ( m_nSensorWidth > 0.0 ) ? m_nSensorWidth : 36.0;
        FLT nHalfDiagonal = 0.5 * SQRT( ( nWidth * nWidth ) + ( nWidth * nWidth * 4.0 / 9.0 ) );
        FLT nLength = oLens.GetVertexZ( nSurfaces - 1 );
        FLT nLead = ( m_nLead > 0.0 ) ? m_nLead : (std::max)( 0.5 * nLength, 0.4 * nWidest );
        FLT nFront = aoSurfaces[ 0 ].nAperture;
        FLT nOffsetX = m_bCutaway ? -2.0 * m_nTubeRadius : 0.0;

        for ( size_t nField = 0; nField < m_anFields.size(); nField++ )
        {
            FLT nTan = m_anFields[ nField ] * nHalfDiagonal / nFocal;
            FLT nAngle = std::atan( nTan );
            FLT anDirection[ 3 ] = { 0.0, SIN( nAngle ), COS( nAngle ) };

                // Where on the front a ray at this angle gets through.
            auto Passes = [ & ]( FLT nHeight, int nChannel, std::vector<FLT> &anPath )
            {
                FLT anOrigin[ 3 ] = { 0.0, nHeight - ( nLead * nTan ), -nLead };

                return oLens.TracePath( anOrigin, anDirection, nChannel, anPath );
            };

            std::vector<FLT>    anPath;
            FLT                 nLowest = 1e30;
            FLT                 nHighest = -1e30;
            const int           nScan = 801;

            for ( int nStep = 0; nStep < nScan; nStep++ )
            {
                FLT nHeight = -1.2 * nFront + ( 2.4 * nFront * nStep / ( nScan - 1 ) );

                if ( Passes( nHeight, CLens::CHANNEL_ALL, anPath ) )
                {
                    nLowest = (std::min)( nLowest, nHeight );
                    nHighest = (std::max)( nHighest, nHeight );
                }
            }
            if ( ! ( nHighest > nLowest ) )
                continue;

                // The on-axis fan in the three colors; the others in one.
            std::vector<int> anChannels;
            std::vector<CVector> aoColors;

            if ( nField == 0 && m_anFields[ nField ] == 0.0 )
            {
                anChannels = { CLens::CHANNEL_RED, CLens::CHANNEL_GREEN, CLens::CHANNEL_BLUE };
                aoColors = { CVector( 1.0, 0.15, 0.1 ), CVector( 0.2, 1.0, 0.25 ), CVector( 0.25, 0.45, 1.0 ) };
            }
            else
            {
                size_t     nColors = sizeof( s_aanFanColors ) / sizeof( s_aanFanColors[ 0 ] );
                const FLT *anColor = s_aanFanColors[ ( nField + nColors - 1 ) % nColors ];

                anChannels = { CLens::CHANNEL_ALL };
                aoColors = { CVector( anColor[ 0 ], anColor[ 1 ], anColor[ 2 ] ) };
            }

            for ( size_t nColor = 0; nColor < anChannels.size(); nColor++ )
            {
                for ( int nRay = 0; nRay < m_nRays; nRay++ )
                {
                    FLT nShare = ( m_nRays > 1 ) ? ( 0.02 + ( 0.96 * nRay / ( m_nRays - 1 ) ) ) : 0.5;
                    FLT nHeight = nLowest + ( nShare * ( nHighest - nLowest ) );

                    if ( ! Passes( nHeight, anChannels[ nColor ], anPath ) )
                        continue;

                    std::vector<CVector> aoPath;
                    std::string          strPath;

                    aoPath.push_back( ToScene( nOffsetX, nHeight - ( nLead * nTan ), -nLead ) );
                    for ( size_t nPoint = 0; nPoint + 2 < anPath.size(); nPoint += 3 )
                        aoPath.push_back( ToScene( nOffsetX, anPath[ nPoint + 1 ], anPath[ nPoint + 2 ] ) );
                    for ( size_t nPoint = 0; nPoint < aoPath.size(); nPoint++ )
                        strPath += ( nPoint ? ", " : "" ) + Point( aoPath[ nPoint ] );

                    CVector        oColor( aoColors[ nColor ] );
                    std::string    strColor = Rgb( oColor.GetX(), oColor.GetY(), oColor.GetZ() );

                    std::string strMaterials = "[{\"type\": \"ambient\", \"influence\": " + Number( m_nFanOpacity ) +
                                               ", \"color\": " + strColor + "}";

                    if ( m_nFanOpacity < 1.0 )
                        strMaterials += ", {\"type\": \"transparent\", \"influence\": " + Number( 1.0 - m_nFanOpacity ) +
                                        ", \"color\": [1, 1, 1], \"indexOfRefraction\": 1}";
                    strMaterials += "]";
                    astrParts.push_back( "{\"type\": \"sweep\", \"path\": [" + strPath + "], \"radius\": " +
                                         Number( m_nTubeRadius * m_nScale ) +
                                         ", \"segments\": 8, \"smooth\": false, \"capped\": true, \"color\": " + strColor +
                                         ", \"materials\": " + strMaterials + "}" );
                    m_aaoRayPaths.push_back( aoPath );
                    m_nRayCount++;
                }
            }
        }
    }

        // The parts, read as any scene object is.
    for ( size_t nPart = 0; nPart < astrParts.size(); nPart++ )
    {
        std::unique_ptr<IASLib::CJsonNode> pNode( RayJson::Parse( astrParts[ nPart ] ) );
        std::string                        strType;

        if ( ! pNode || ! RayJson::GetString( *pNode, "type", strType, TRUE ) )
        {
            strError = "couldn't build part " + std::to_string( nPart );
            Clear();
            return FALSE;
        }

        CObjects *pPart = g_pDynamicList->Create( strType.c_str() );

        if ( pPart == NULL || ! pPart->FromJson( *pNode ) )
        {
            delete pPart;
            strError = "couldn't build part " + std::to_string( nPart ) + " (" + strType + ")";
            Clear();
            return FALSE;
        }
        m_apParts.push_back( pPart );

        CVector oMinimum, oMaximum;

        if ( pPart->GetBounds( oMinimum, oMaximum ) )
        {
            if ( ! m_bHasBounds )
            {
                m_oMinimum = oMinimum;
                m_oMaximum = oMaximum;
                m_bHasBounds = TRUE;
            }
            else
            {
                m_oMinimum.Set( (std::min)( m_oMinimum.GetX(), oMinimum.GetX() ), (std::min)( m_oMinimum.GetY(), oMinimum.GetY() ),
                                (std::min)( m_oMinimum.GetZ(), oMinimum.GetZ() ) );
                m_oMaximum.Set( (std::max)( m_oMaximum.GetX(), oMaximum.GetX() ), (std::max)( m_oMaximum.GetY(), oMaximum.GetY() ),
                                (std::max)( m_oMaximum.GetZ(), oMaximum.GetZ() ) );
            }
        }
    }
    if ( m_apParts.empty() )
    {
        strError = "made nothing (no glass in the prescription)";
        return FALSE;
    }
    return TRUE;
}

BOOL CLensElements::Intersect( CRay &oRay, CVector &oIntersect )
{
    return Intersect( oRay, oIntersect, NULL );
}

BOOL CLensElements::Intersect( CRay &oRay, CVector &oIntersect, CObjects *pIgnore )
{
        // The part a ray leaves from: the one being shaded, when the scene
        // names this object, or the part itself when a part names itself.
    CObjects   *pFrom = ( pIgnore == this ) ? m_pShadePart : pIgnore;
    FLT         nBest = oRay.GetT();
    CObjects   *pWinner = NULL;

    for ( size_t nIndex = 0; nIndex < m_apParts.size(); nIndex++ )
    {
        CObjects   *pPart = m_apParts[ nIndex ];
        CRay        oTry( oRay );
        CVector     oHit;

        if ( pWinner )
            oTry.SetT( nBest );
        if ( pPart->Intersect( oTry, oHit, ( pPart == pFrom ) ? pPart : NULL ) && ( ! pWinner || oTry.GetT() < nBest ) )
        {
            nBest = oTry.GetT();
            pWinner = pPart;
            oIntersect = oHit;
        }
    }
    if ( ! pWinner )
        return FALSE;
    m_pHitPart = pWinner;
    oRay.SetT( nBest );
    return TRUE;
}

BOOL CLensElements::Occludes( CRay &oRay, CObjects *pIgnore )
{
    CObjects *pFrom = ( pIgnore == this ) ? m_pShadePart : pIgnore;

        // The ray tubes stand for light, so they cast no shadows.
    for ( size_t nIndex = 0; nIndex + (size_t)m_nRayCount < m_apParts.size(); nIndex++ )
    {
        CObjects *pPart = m_apParts[ nIndex ];

        if ( pPart->Occludes( oRay, ( pPart == pFrom ) ? pPart : NULL ) )
            return TRUE;
    }
    return FALSE;
}

BOOL CLensElements::GetBounds( CVector &oMinimum, CVector &oMaximum )
{
    if ( ! m_bHasBounds )
        return FALSE;
    oMinimum = m_oMinimum;
    oMaximum = m_oMaximum;
    return TRUE;
}

BOOL CLensElements::SurfaceNormal( CVector &oPoint, CVector &oNormal )
{
    CObjects *pPart = m_pShadePart ? m_pShadePart : m_pHitPart;

    return pPart ? pPart->SurfaceNormal( oPoint, oNormal ) : FALSE;
}

BOOL CLensElements::SurfaceCoords( CVector &oPoint, FLT &nU, FLT &nV )
{
    CObjects *pPart = m_pShadePart ? m_pShadePart : m_pHitPart;

    if ( ! pPart )
    {
        nU = nV = 0.0;
        return FALSE;
    }
    return pPart->SurfaceCoords( oPoint, nU, nV );
}

CObjects *CLensElements::HitPart( BOOL &bFlip )
{
    bFlip = FALSE;
    return m_pHitPart ? m_pHitPart->HitPart( bFlip ) : this;
}

CColor CLensElements::Shade( CScene &oScene, CRay &oRay, CVector &oIntersect )
{
    CObjects *pPart = m_pHitPart;

    if ( ! pPart )
        return CColor( 0, 0, 0 );

        // The part shades itself; its own rays (which name it) may meet it
        // or the other parts again, and put back what was being shaded.
    CObjects   *pOld = m_pShadePart;

    m_pShadePart = pPart;

    CColor oColor = pPart->Shade( oScene, oRay, oIntersect );

    m_pShadePart = pOld;
    return oColor;
}

void CLensElements::ToJson( IASLib::CObjectNode &oNode )
{
    oNode.put( "prescription", m_strPrescription.c_str() );
    if ( m_nFNumber > 0.0 )
        RayJson::PutNumber( oNode, "fNumber", m_nFNumber );
    RayJson::PutNumber( oNode, "scale", m_nScale );
    RayJson::PutVector( oNode, "position", m_oPosition );
    RayJson::PutVector( oNode, "rotation", m_oRotation );
    RayJson::PutBool( oNode, "cutaway", m_bCutaway );
    RayJson::PutBool( oNode, "barrel", m_bBarrel );
    RayJson::PutBool( oNode, "fans", m_bFans );
    oNode.put( "rays", m_nRays );

    IASLib::CArrayNode *pFields = oNode.putArray( "fields" );

    for ( size_t nIndex = 0; nIndex < m_anFields.size(); nIndex++ )
        pFields->add( (double)m_anFields[ nIndex ] );
    RayJson::PutNumber( oNode, "tubeRadius", m_nTubeRadius );
    RayJson::PutNumber( oNode, "fanOpacity", m_nFanOpacity );
    if ( m_nLead > 0.0 )
        RayJson::PutNumber( oNode, "lead", m_nLead );
    if ( m_nSensorWidth > 0.0 )
        RayJson::PutNumber( oNode, "sensorWidth", m_nSensorWidth );
}

BOOL CLensElements::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "prescription", "fNumber", "scale", "position", "rotation", "cutaway", "barrel",
                                              "fans", "rays", "fields", "tubeRadius", "fanOpacity", "lead", "sensorWidth", NULL };

    Clear();
    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetString( oNode, "prescription", m_strPrescription, TRUE ) ||
         ! RayJson::GetNumber( oNode, "fNumber", m_nFNumber, FALSE ) ||
         ! RayJson::GetNumber( oNode, "scale", m_nScale, FALSE ) ||
         ! RayJson::GetVector( oNode, "position", m_oPosition, FALSE ) ||
         ! RayJson::GetVector( oNode, "rotation", m_oRotation, FALSE ) ||
         ! RayJson::GetBool( oNode, "cutaway", m_bCutaway, FALSE ) ||
         ! RayJson::GetBool( oNode, "barrel", m_bBarrel, FALSE ) ||
         ! RayJson::GetBool( oNode, "fans", m_bFans, FALSE ) ||
         ! RayJson::GetInt( oNode, "rays", m_nRays, 1, 64, FALSE ) ||
         ! RayJson::GetNumber( oNode, "tubeRadius", m_nTubeRadius, FALSE ) ||
         ! RayJson::GetNumber( oNode, "fanOpacity", m_nFanOpacity, FALSE ) ||
         ! RayJson::GetNumber( oNode, "lead", m_nLead, FALSE ) ||
         ! RayJson::GetNumber( oNode, "sensorWidth", m_nSensorWidth, FALSE ) )
        return FALSE;
    if ( m_nFNumber < 0.0 )
        return RayJson::Fail( "fNumber can't be less than 0" );
    if ( ! ( m_nScale > 0.0 ) )
        return RayJson::Fail( "scale must be more than 0" );
    if ( ! ( m_nTubeRadius > 0.0 ) )
        return RayJson::Fail( "tubeRadius must be more than 0" );
    if ( ! ( m_nFanOpacity > 0.0 ) || m_nFanOpacity > 1.0 )
        return RayJson::Fail( "fanOpacity must be more than 0 and at most 1" );
    if ( m_nLead < 0.0 || m_nSensorWidth < 0.0 )
        return RayJson::Fail( "lead and sensorWidth can't be less than 0" );

    const IASLib::CJsonNode *pFields = oNode.get( "fields" );

    if ( pFields )
    {
        RayJson::CWhere oWhere( "fields" );

        if ( ! pFields->isArray() || pFields->size() < 1 || pFields->size() > 6 )
            return RayJson::Fail( "must be a list of 1 to 6 field fractions, each from 0 to 1" );
        m_anFields.clear();
        for ( int nIndex = 0; nIndex < pFields->size(); nIndex++ )
        {
            RayJson::CWhere          oElement( "", nIndex );
            const IASLib::CJsonNode *pField = pFields->get( nIndex );

            if ( ! pField || ! pField->isNumber() )
                return RayJson::Fail( "must be a number from 0 to 1" );

            FLT nField = (FLT)pField->doubleValue();

            if ( ! ( nField >= 0.0 && nField <= 1.0 ) )
                return RayJson::Fail( "must be from 0 to 1" );
            m_anFields.push_back( nField );
        }
    }

    std::string strError;

    if ( ! Build( strError ) )
        return RayJson::Fail( "the lens " + strError );
    return TRUE;
}
