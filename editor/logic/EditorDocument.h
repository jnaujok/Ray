/*************************************************************************
*   CEditorDocument                                                      *
*                                                                        *
*   The scene the editor works on, and every way the editor changes it:  *
*   loading and saving, editing a part as JSON (checked by the scene's   *
*   own reader, which says what is wrong), adding, deleting and          *
*   duplicating objects and lights, importing a model file, and undo     *
*   and redo. It knows nothing of any window toolkit, so its tests run   *
*   without a display.                                                   *
*                                                                        *
*   Parts are named as CScene::PartToJson names them: "settings",        *
*   "camera", "environment", "objects/N" and "lights/N".                 *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __EDITORDOCUMENT_H__
#define __EDITORDOCUMENT_H__

#include "Scene.h"

#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace RayEditor
{
        // A line in the editor's scene tree.
    struct SPart
    {
        std::string                 strPart;        // its name, for PartToJson and EditPart
        std::string                 strLabel;       // what the tree shows
        std::string                 strType;        // its JSON "type", if it has one
        std::vector<std::string>    astrMaterials;  // its materials' types, for an object
    };

    class CEditorDocument
    {
        protected:
            std::unique_ptr<CScene>     m_pScene;
            std::string                 m_strPath;          // the file, or empty for a new scene
            bool                        m_bArchive;         // whether m_strPath is a scene archive
            bool                        m_bDirty;
            unsigned                    m_nRevision;        // goes up with every change
                // Whole scenes, as compact JSON with their files' data, to go
                // back to; the newest last.
            std::deque< std::shared_ptr<const std::string> > m_apUndo;
            std::deque< std::shared_ptr<const std::string> > m_apRedo;
            size_t                      m_nUndoBytes;

            std::shared_ptr<const std::string> Snapshot( void );
            bool                        Restore( const std::string &strScene );
                // Remembers the scene as it was before a change.
            void                        PushUndo( const std::shared_ptr<const std::string> &pBefore );
            void                        Changed( void );

        public:
                                        CEditorDocument( void );

                /// A new scene: a floor, a ball, a light and a camera.
            void                        New( void );
                /// Reads a scene file (archive or JSON). FALSE, with the
                /// reason, if it can't.
            bool                        Load( const std::string &strPath, std::string &strError );
                /// Saves as a scene archive, or as plain JSON with its files
                /// beside it if the name ends in ".json".
            bool                        Save( const std::string &strPath, std::string &strError );

            CScene                     &Scene( void ) { return *m_pScene; }
            const std::string          &Path( void ) const { return m_strPath; }
            bool                        IsDirty( void ) const { return m_bDirty; }
            unsigned                    Revision( void ) const { return m_nRevision; }
                /// The folder files the scene names are read from.
            std::string                 Directory( void ) const;

                /// What the tree shows: the settings, the camera, the
                /// environment (if any), each object, each light.
            std::vector<SPart>          Parts( void );
                /// A part as JSON text, indented (empty if there is none).
            std::string                 PartJson( const std::string &strPart );
                /// Replaces a part with JSON text. FALSE, with the scene's
                /// reason, if the text is wrong; the scene is then unchanged.
            bool                        SetPart( const std::string &strPart, const std::string &strText, std::string &strError );
                /// Adds an object or light ("objects" or "lights") from JSON
                /// text; strPart gets its name.
            bool                        Add( const std::string &strList, const std::string &strText, std::string &strPart, std::string &strError );
            bool                        Delete( const std::string &strPart, std::string &strError );
                /// Copies an object or light to the end of its list.
            bool                        Duplicate( const std::string &strPart, std::string &strNew, std::string &strError );
                /// Adds a mesh that loads a model file (STL or OBJ), fitted
                /// to two units and standing on the floor at the origin.
            bool                        ImportModel( const std::string &strFile, std::string &strPart, std::string &strError );

            bool                        CanUndo( void ) const { return ! m_apUndo.empty(); }
            bool                        CanRedo( void ) const { return ! m_apRedo.empty(); }
            bool                        Undo( void );
            bool                        Redo( void );

                /// JSON text for a new object or light of a type ("sphere",
                /// "box", "plane", "cylinder", "cone", "triangle", "point",
                /// "spot", "directional", "sphereLight", "rectangleLight"), or
                /// empty. strList gets "objects" or "lights".
            static std::string          NewPartJson( const std::string &strType, std::string &strList );
                /// The types NewPartJson knows, objects then lights.
            static std::vector<std::string> ObjectTypes( void );
            static std::vector<std::string> LightTypes( void );
    };
}

#endif // __EDITORDOCUMENT_H__
