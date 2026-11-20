/*************************************************************************
*   editor_test                                                          *
*                                                                        *
*   Checks the scene editor's logic, which needs no display: editing a   *
*   scene's parts as JSON with undo and redo, the new-object templates,  *
*   tessellation for the shaded view, the view's camera math against the *
*   tracer's own camera, picking, model import, saving, the progressive  *
*   render, and the in-memory image file it renders into.                *
*                                                                        *
*       editor_test <scenes directory> <scratch directory>               *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#include "Globals.h"
#include "EditorDocument.h"
#include "RenderJob.h"
#include "Tessellator.h"
#include "ViewCamera.h"

#include "Camera.h"
#include "Color.h"
#include "ImageFile.h"
#include "Mesh.h"
#include "Scene.h"
#include "SceneJson.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <thread>

using namespace RayEditor;

static int  g_nFailures = 0;
static int  g_nChecks = 0;

#define CHECK( x )  do { g_nChecks++; if ( ! ( x ) ) { g_nFailures++; printf( "FAILED: %s (line %d)\n", #x, __LINE__ ); } } while ( 0 )

static int CountParts( CEditorDocument &oDoc, const char *strPrefix )
{
    int n = 0;

    for ( const SPart &oPart : oDoc.Parts() )
    {
        if ( oPart.strPart.compare( 0, strlen( strPrefix ), strPrefix ) == 0 )
            n++;
    }
    return n;
}

static void TestDocument( void )
{
    CEditorDocument oDoc;
    std::string     strError, strPart;

        // A new scene: settings, camera, a floor, a ball and a light.
    CHECK( oDoc.Scene().GetObjectCount() == 2 );
    CHECK( oDoc.Scene().GetLightCount() == 1 );
    CHECK( CountParts( oDoc, "objects/" ) == 2 );
    CHECK( oDoc.Parts().size() == 5 );
    CHECK( oDoc.Parts()[ 2 ].strType == "plane" );
    CHECK( oDoc.Parts()[ 3 ].astrMaterials.size() == 3 );
    CHECK( ! oDoc.IsDirty() && ! oDoc.CanUndo() );

        // Every template is a part the scene accepts.
    for ( const std::string &strType : CEditorDocument::ObjectTypes() )
    {
        std::string strList, strText = CEditorDocument::NewPartJson( strType, strList );

        CHECK( strList == "objects" );
        CHECK( oDoc.Add( strList, strText, strPart, strError ) );
        if ( ! strError.empty() )
            printf( "  %s: %s\n", strType.c_str(), strError.c_str() );
        strError.clear();
    }
    for ( const std::string &strType : CEditorDocument::LightTypes() )
    {
        std::string strList, strText = CEditorDocument::NewPartJson( strType, strList );

        CHECK( strList == "lights" );
        CHECK( oDoc.Add( strList, strText, strPart, strError ) );
        if ( ! strError.empty() )
            printf( "  %s: %s\n", strType.c_str(), strError.c_str() );
        strError.clear();
    }
    CHECK( oDoc.Scene().GetObjectCount() == 2 + (int)CEditorDocument::ObjectTypes().size() );
    CHECK( oDoc.Scene().GetLightCount() == 1 + (int)CEditorDocument::LightTypes().size() );
    CHECK( strPart == "lights/" + std::to_string( oDoc.Scene().GetLightCount() - 1 ) );
    CHECK( oDoc.IsDirty() && oDoc.CanUndo() );

        // A bad edit is refused, says why, and changes nothing.
    unsigned    nRevision = oDoc.Revision();
    std::string strBefore = oDoc.PartJson( "objects/1" );

    CHECK( ! oDoc.SetPart( "objects/1", "{ \"type\": \"sphere\", \"center\": [0, 1, 0], \"radius\": 1, \"colour\": [1, 0, 0] }", strError ) );
    CHECK( ! strError.empty() );
    CHECK( oDoc.Revision() == nRevision );
    CHECK( oDoc.PartJson( "objects/1" ) == strBefore );
    CHECK( ! oDoc.SetPart( "objects/1", "{ not json", strError ) );

        // A good edit takes, and undo and redo walk it back and forth.
    std::string strText = strBefore;
    size_t      nAt = strText.find( "\"radius\": 1.0" );

    CHECK( nAt != std::string::npos );
    strText.replace( nAt, 13, "\"radius\": 2.5" );
    CHECK( oDoc.SetPart( "objects/1", strText, strError ) );
    CHECK( oDoc.PartJson( "objects/1" ).find( "\"radius\": 2.5" ) != std::string::npos );
    CHECK( oDoc.Undo() );
    CHECK( oDoc.PartJson( "objects/1" ) == strBefore );
    CHECK( oDoc.CanRedo() );
    CHECK( oDoc.Redo() );
    CHECK( oDoc.PartJson( "objects/1" ).find( "\"radius\": 2.5" ) != std::string::npos );

        // Duplicate and delete.
    int nObjects = oDoc.Scene().GetObjectCount();

    CHECK( oDoc.Duplicate( "objects/1", strPart, strError ) );
    CHECK( oDoc.Scene().GetObjectCount() == nObjects + 1 );
    CHECK( oDoc.PartJson( strPart ) == oDoc.PartJson( "objects/1" ) );
    CHECK( oDoc.Delete( strPart, strError ) );
    CHECK( oDoc.Scene().GetObjectCount() == nObjects );
    CHECK( ! oDoc.Delete( "camera", strError ) );
    CHECK( oDoc.Undo() );
    CHECK( oDoc.Scene().GetObjectCount() == nObjects + 1 );

        // Undo all the way back to the new scene.
    while ( oDoc.CanUndo() )
        CHECK( oDoc.Undo() );
    CHECK( oDoc.Scene().GetObjectCount() == 2 && oDoc.Scene().GetLightCount() == 1 );
    CHECK( ! oDoc.Undo() );

    std::string strList;

    CHECK( CEditorDocument::NewPartJson( "teapot", strList ).empty() && strList.empty() );
}

static void TestFiles( const std::string &strScenes, const std::string &strScratch )
{
    CEditorDocument oDoc;
    std::string     strError, strPart;

        // A model file imports as a mesh with faces.
    CHECK( oDoc.ImportModel( strScenes + "/models/ring.obj", strPart, strError ) );
    if ( ! strError.empty() )
        printf( "  import: %s\n", strError.c_str() );

    CMesh *pMesh = dynamic_cast<CMesh *>( oDoc.Scene().GetObjectAt( oDoc.Scene().GetObjectCount() - 1 ) );

    CHECK( pMesh != NULL && pMesh->GetFaceCount() > 0 );
    CHECK( ! oDoc.ImportModel( strScenes + "/models/no_such_model.stl", strPart, strError ) );

        // Saved as an archive and as JSON, it reads back the same.
    std::string strArchive = strScratch + "/editor_test_scene.ray";
    std::string strJson = strScratch + "/editor_test_scene.json";

    CHECK( oDoc.Save( strArchive, strError ) );
    CHECK( ! oDoc.IsDirty() && oDoc.Path() == strArchive );
    CHECK( oDoc.Save( strJson, strError ) );

    for ( const std::string &strFile : { strArchive, strJson } )
    {
        CEditorDocument oBack;

        CHECK( oBack.Load( strFile, strError ) );
        CHECK( oBack.Scene().GetObjectCount() == oDoc.Scene().GetObjectCount() );
        CHECK( oBack.Scene().GetLightCount() == oDoc.Scene().GetLightCount() );

        CMesh *pBack = dynamic_cast<CMesh *>( oBack.Scene().GetObjectAt( oBack.Scene().GetObjectCount() - 1 ) );

        CHECK( pBack != NULL && pMesh != NULL && pBack->GetFaceCount() == pMesh->GetFaceCount() );

            // Editing a loaded archive scene still finds its model.
        std::string strText = oBack.PartJson( "objects/2" );

        CHECK( oBack.SetPart( "objects/2", strText, strError ) );
    }

    CEditorDocument oMissing;

    CHECK( ! oMissing.Load( strScratch + "/no_such_scene.ray", strError ) && ! strError.empty() );

        // The torture test loads, and every object in it draws.
    CEditorDocument oTorture;
    SSceneDrawing   oDrawing;

    CHECK( oTorture.Load( strScenes + "/torture_test.ray", strError ) );
    TessellateScene( oTorture.Scene(), oDrawing );

    std::vector<bool> abDrawn( (size_t)oTorture.Scene().GetObjectCount(), false );

    for ( const SDrawMesh &oMesh : oDrawing.aoMeshes )
    {
        if ( oMesh.nObject >= 0 && (size_t)oMesh.nObject < abDrawn.size() )
            abDrawn[ (size_t)oMesh.nObject ] = true;
    }
    for ( size_t i = 0; i < abDrawn.size(); i++ )
    {
        CHECK( abDrawn[ i ] );
        if ( ! abDrawn[ i ] )
            printf( "  torture object %d not drawn\n", (int)i );
    }
        // Every light and the camera have icons.
    CHECK( (int)oDrawing.aoLines.size() == oTorture.Scene().GetLightCount() + 1 );
    CHECK( oDrawing.bHasBounds );
}

static void TestTessellation( void )
{
    std::vector<SDrawMesh>              aoMeshes;
    std::unique_ptr<IASLib::CJsonNode>  pSphere( RayJson::Parse( "{ \"type\": \"sphere\", \"center\": [1, 2, 3], \"radius\": 2, "
                                                                 "\"materials\": [ { \"type\": \"diffuse\", \"influence\": 1, \"color\": [0.2, 0.4, 0.6] } ] }" ) );

    CHECK( TessellateObject( *pSphere, NULL, 7, 10.0, aoMeshes, 16 ) );
    CHECK( aoMeshes.size() == 1 && aoMeshes[ 0 ].nObject == 7 );
    CHECK( std::fabs( aoMeshes[ 0 ].anColor[ 2 ] - 0.6f ) < 1e-6 );

    bool bOnSphere = true, bUnit = true;

    for ( size_t i = 0; i + 2 < aoMeshes[ 0 ].anPositions.size(); i += 3 )
    {
        SVec3 p( aoMeshes[ 0 ].anPositions[ i ], aoMeshes[ 0 ].anPositions[ i + 1 ], aoMeshes[ 0 ].anPositions[ i + 2 ] );
        SVec3 n( aoMeshes[ 0 ].anNormals[ i ], aoMeshes[ 0 ].anNormals[ i + 1 ], aoMeshes[ 0 ].anNormals[ i + 2 ] );

        if ( std::fabs( ( p - SVec3( 1, 2, 3 ) ).Length() - 2.0 ) > 1e-5 )
            bOnSphere = false;
        if ( std::fabs( n.Length() - 1.0 ) > 1e-5 || ( p - SVec3( 1, 2, 3 ) ).Normalized().Dot( n ) < 0.999 )
            bUnit = false;
    }
    CHECK( bOnSphere && bUnit );

        // A box has twelve triangles, its corners turned as the tracer turns them.
    aoMeshes.clear();

    std::unique_ptr<IASLib::CJsonNode> pBox( RayJson::Parse( "{ \"type\": \"box\", \"center\": [0, 0, 0], \"size\": [2, 4, 6], \"rotation\": [0, 90, 0] }" ) );

    CHECK( TessellateObject( *pBox, NULL, 0, 10.0, aoMeshes ) );
    CHECK( aoMeshes.size() == 1 && aoMeshes[ 0 ].TriangleCount() == 12 );

    double nMaxX = 0.0, nMaxZ = 0.0;

    for ( size_t i = 0; i + 2 < aoMeshes[ 0 ].anPositions.size(); i += 3 )
    {
        nMaxX = std::max( nMaxX, (double)aoMeshes[ 0 ].anPositions[ i ] );
        nMaxZ = std::max( nMaxZ, (double)aoMeshes[ 0 ].anPositions[ i + 2 ] );
    }
        // Turned 90 degrees about y, the box's 6 deep side lies along x.
    CHECK( std::fabs( nMaxX - 3.0 ) < 1e-6 && std::fabs( nMaxZ - 1.0 ) < 1e-6 );

        // Glass is translucent, and a csg's parts are ghosts.
    aoMeshes.clear();

    std::unique_ptr<IASLib::CJsonNode> pCsg( RayJson::Parse( "{ \"type\": \"csg\", \"operation\": \"difference\", \"objects\": ["
        "{ \"type\": \"box\", \"center\": [0, 0, 0], \"size\": 1 }, { \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 0.6 } ],"
        "\"materials\": [ { \"type\": \"transparent\", \"influence\": 0.9, \"color\": [1, 1, 1], \"attenuationColor\": [0.2, 0.8, 0.3] } ] }" ) );

    CHECK( TessellateObject( *pCsg, NULL, 3, 10.0, aoMeshes ) );
    CHECK( aoMeshes.size() == 2 && aoMeshes[ 0 ].bGhost && aoMeshes[ 1 ].bGhost );
    CHECK( aoMeshes[ 0 ].anColor[ 3 ] < 0.5f && std::fabs( aoMeshes[ 0 ].anColor[ 1 ] - 0.8f ) < 1e-6 );

        // A plane is a large square; cones and cylinders close their ends.
    aoMeshes.clear();

    std::unique_ptr<IASLib::CJsonNode> pPlane( RayJson::Parse( "{ \"type\": \"plane\", \"position\": [0, -1, 0], \"normal\": [0, 1, 0] }" ) );
    std::unique_ptr<IASLib::CJsonNode> pCone( RayJson::Parse( "{ \"type\": \"cone\", \"base\": [0, 0, 0], \"top\": [0, 2, 0], \"baseRadius\": 1, \"topRadius\": 0 }" ) );
    std::unique_ptr<IASLib::CJsonNode> pUnknown( RayJson::Parse( "{ \"type\": \"teapot\" }" ) );

    CHECK( TessellateObject( *pPlane, NULL, 0, 50.0, aoMeshes ) && aoMeshes.back().bPlane && aoMeshes.back().TriangleCount() == 2 );
    CHECK( TessellateObject( *pCone, NULL, 0, 50.0, aoMeshes, 8 ) && aoMeshes.back().TriangleCount() == 8 * 3 );
    CHECK( ! TessellateObject( *pUnknown, NULL, 0, 50.0, aoMeshes ) );

        // Light icons.
    SDrawLines                          oLines;
    std::unique_ptr<IASLib::CJsonNode>  pSun( RayJson::Parse( "{ \"type\": \"directional\", \"direction\": [0, -1, 0], \"color\": [1, 1, 1] }" ) );

    CHECK( LightIcon( *pSun, 2, SVec3(), 10.0, oLines ) && oLines.nLight == 2 && oLines.LineCount() == 9 );
}

static void TestCamera( void )
{
        // The view aims rays as the tracer's camera does.
    std::unique_ptr<IASLib::CJsonNode> pCamera( RayJson::Parse(
        "{ \"position\": [1, 2, -10], \"lookAt\": [0.5, 1, 0], \"up\": [0, 1, 0], \"verticalAngle\": 35 }" ) );
    CCamera     oCamera;
    CViewCamera oView;

    CHECK( oCamera.FromJson( *pCamera ) );
    oCamera.CalcRotate();
    oView.Set( SVec3( 1, 2, -10 ), SVec3( 0.5, 1, 0 ), SVec3( 0, 1, 0 ), 35.0 );

    double nWorst = 0.0;

    for ( double x = -1.0; x <= 1.0; x += 0.5 )
    {
        for ( double y = -1.0; y <= 1.0; y += 0.5 )
        {
            CVector oTracer = oCamera.RayDirection( x, y, 1.5 );
            SVec3   oMine = oView.RayDirection( x, y, 1.5 );

            nWorst = std::max( nWorst, ( SVec3( oTracer.GetX(), oTracer.GetY(), oTracer.GetZ() ) - oMine ).Length() );
        }
    }
    CHECK( nWorst < 1e-9 );

        // The OpenGL matrices put a point where the ray through it lands.
    double  anProjection[ 16 ], anView[ 16 ];
    SVec3   oDirection = oView.RayDirection( 0.5, -0.25, 1.5 );
    SVec3   oPoint = oView.GetEye() + ( oDirection * 7.0 );
    double  anEye[ 4 ], anClip[ 4 ];

    oView.View( anView );
    oView.Projection( 1.5, 0.1, 100.0, anProjection );
    for ( int r = 0; r < 4; r++ )
        anEye[ r ] = ( anView[ r ] * oPoint.x ) + ( anView[ 4 + r ] * oPoint.y ) + ( anView[ 8 + r ] * oPoint.z ) + anView[ 12 + r ];
    for ( int r = 0; r < 4; r++ )
        anClip[ r ] = ( anProjection[ r ] * anEye[ 0 ] ) + ( anProjection[ 4 + r ] * anEye[ 1 ] ) + ( anProjection[ 8 + r ] * anEye[ 2 ] ) + ( anProjection[ 12 + r ] * anEye[ 3 ] );
    CHECK( std::fabs( anClip[ 0 ] / anClip[ 3 ] - 0.5 ) < 1e-9 );
    CHECK( std::fabs( anClip[ 1 ] / anClip[ 3 ] + 0.25 ) < 1e-9 );

        // Orbiting keeps the distance to the target; zoom and pan work.
    double nDistance = ( oView.GetEye() - oView.GetTarget() ).Length();

    oView.Orbit( 40.0, 20.0 );
    CHECK( std::fabs( ( oView.GetEye() - oView.GetTarget() ).Length() - nDistance ) < 1e-9 );
    oView.Orbit( 0.0, 200.0 );     // stops short of straight up
    CHECK( ( oView.GetEye() - oView.GetTarget() ).Normalized().y < 0.9999 );
    oView.Zoom( 0.5 );
    CHECK( std::fabs( ( oView.GetEye() - oView.GetTarget() ).Length() - nDistance * 0.5 ) < 1e-9 );

    SVec3 oTarget = oView.GetTarget();

    oView.Pan( 0.1, 0.0 );
    CHECK( ( oView.GetTarget() - oTarget ).Length() > 0.0 );
    oView.Frame( SVec3( -1, -1, -1 ), SVec3( 1, 1, 1 ) );
    CHECK( ( oView.GetTarget() - SVec3( 0, 0, 0 ) ).Length() < 1e-12 );
}

static void TestPicking( void )
{
    CEditorDocument oDoc;
    CViewCamera     oView;

        // Looking at the new scene's ball from the front.
    oView.Set( SVec3( 0, 1, -8 ), SVec3( 0, 1, 0 ), SVec3( 0, 1, 0 ), 30.0 );
    CHECK( PickObject( oDoc.Scene(), oView, 100, 75, 200, 150 ) == 1 );      // the ball, in the middle
    CHECK( PickObject( oDoc.Scene(), oView, 100, 148, 200, 150 ) == 0 );     // the floor, at the bottom
    CHECK( PickObject( oDoc.Scene(), oView, 100, 1, 200, 150 ) == -1 );      // the sky, at the top
}

static void TestRender( void )
{
    CEditorDocument         oDoc;
    CViewCamera             oView;
    CRenderJob              oJob;
    std::string             strError;
    std::vector<unsigned char> abPixels;

    oView.Set( SVec3( 0, 1, -8 ), SVec3( 0, 1, 0 ), SVec3( 0, 1, 0 ), 30.0 );

    CRenderJob::SRequest oRequest = CRenderJob::ForView( oView, 40, 30 );

    oRequest.nThreads = 2;
    CHECK( oJob.Start( oDoc.Scene(), oRequest, strError ) );
    for ( int i = 0; i < 6000 && oJob.IsRunning(); i++ )
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    CHECK( ! oJob.IsRunning() && oJob.IsDone() && oJob.GetPass() == 2 );
    CHECK( oJob.GetError().empty() );
    oJob.CopyPixels( abPixels );
    CHECK( abPixels.size() == 40 * 30 * 3 );

        // The ball is red in the middle; the sky is the background color.
    size_t nMiddle = ( ( 15 * 40 ) + 20 ) * 3, nTop = ( ( 0 * 40 ) + 20 ) * 3;

    CHECK( abPixels[ nMiddle ] > abPixels[ nMiddle + 1 ] + 40 );
    CHECK( abPixels[ nTop + 2 ] > 200 && abPixels[ nTop ] < 160 );

        // The full pass overwrites every pixel of the quick one: rendering
        // without the quick pass gives the same image.
    {
        CRenderJob::SRequest oTracer = oRequest;

        oTracer.bQuickPass = false;
        oTracer.bSceneCamera = false;
        CHECK( oJob.Start( oDoc.Scene(), oTracer, strError ) );
        while ( oJob.IsRunning() )
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

        std::vector<unsigned char> abFull;

        oJob.CopyPixels( abFull );
        CHECK( abFull == abPixels );
    }

        // Stopping straight away leaves no thread behind and no finished image.
    oRequest.nWidth = 400;
    oRequest.nHeight = 300;
    CHECK( oJob.Start( oDoc.Scene(), oRequest, strError ) );
    oJob.Stop();
    CHECK( ! oJob.IsRunning() && ! oJob.IsDone() );
    CHECK( ! oJob.Start( oDoc.Scene(), CRenderJob::ForView( oView, 0, 10 ), strError ) );

        // The in-memory image file.
    CImageFile oImage( 3, 2 );

    CHECK( oImage.IsOpen() && oImage.GetMemory().size() == 18 );
    CHECK( oImage.PutRGB( 2, 1, 255, 0, 51 ) );
    CHECK( ! oImage.PutRGB( 3, 1, 1, 2, 3 ) );

    CColor oColor = oImage.GetPoint( 2, 1 );

    CHECK( std::fabs( oColor.GetR() - 1.0 ) < 1e-9 && std::fabs( oColor.GetB() - 0.2 ) < 1e-9 );
    CHECK( oImage.GetMemory()[ 15 ] == 255 );
}

int main( int argc, char *argv[] )
{
    std::string strScenes = ( argc > 1 ) ? argv[ 1 ] : "scenes";
    std::string strScratch = ( argc > 2 ) ? argv[ 2 ] : ".";

    TestDocument();
    TestTessellation();
    TestCamera();
    TestPicking();
    TestFiles( strScenes, strScratch );
    TestRender();

    printf( "%d checks, %d failed\n", g_nChecks, g_nFailures );
    return ( g_nFailures == 0 ) ? 0 : 1;
}
