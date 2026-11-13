import QtQuick
import QtQuick.Controls
// Self-import: a C++ type registered via this same module's SOURCES
// (RecentFilesModel) isn't visible to this module's own QML_FILES
// without it, even though the QML-defined Theme singleton is (verified
// by a real CI failure: "RecentFilesModel is not a type").
import LumaTune

ApplicationWindow {
    id: window
    width: 1280
    height: 800
    visible: true
    title: qsTr("LumaTune")
    color: Theme.background

    // Set from main.cpp via QQmlApplicationEngine::setInitialProperties()
    // when a path is passed on the command line (AGENTS.md §4's
    // documented `lumatune tests/golden/fixtures/warm_cast.png` form) -
    // empty otherwise.
    property string initialPath: ""

    RecentFilesModel {
        id: recentFiles
    }

    // Not "document" - that's PipelineController's own property name, and
    // `document: document` inside its block would resolve the RHS to
    // that same property (shadowing this id) rather than to this object.
    ImageDocument {
        id: doc
    }

    PipelineController {
        id: pipeline
        document: doc
    }

    // Opens a path and, only on success, records it as recent - a failed
    // open (e.g. a stale recent entry whose file moved) shouldn't push
    // itself back to the top of its own list.
    function openPath(path) {
        if (doc.openPath(path)) {
            recentFiles.addPath(path)
        }
    }

    Component.onCompleted: {
        if (initialPath.length > 0) {
            openPath(initialPath)
        }
    }

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")

            Menu {
                id: recentMenu
                title: qsTr("Open &Recent")
                enabled: recentFiles.paths.length > 0

                Instantiator {
                    model: recentFiles.paths
                    MenuItem {
                        required property string modelData
                        text: modelData
                        onTriggered: window.openPath(modelData)
                    }
                    onObjectAdded: (index, object) => recentMenu.insertItem(index, object)
                    onObjectRemoved: (index, object) => recentMenu.removeItem(object)
                }
            }

            MenuSeparator {}

            MenuItem {
                text: qsTr("E&xit")
                onTriggered: Qt.quit()
            }
        }
    }

    PreviewItem {
        id: preview
        anchors.fill: parent
        controller: pipeline
    }

    // Drop target for opening a document. Only the first dropped URL
    // becomes the open document (batch import is BatchDialog's job,
    // P2.5+) - a non-local URL just fails doc.open() silently for now
    // (P2.6 wires up error dialogs).
    DropArea {
        anchors.fill: parent
        onDropped: (drop) => {
            if (drop.hasUrls && drop.urls.length > 0) {
                const url = drop.urls[0]
                if (doc.open(url)) {
                    recentFiles.addUrl(url)
                }
            }
        }
    }
}
