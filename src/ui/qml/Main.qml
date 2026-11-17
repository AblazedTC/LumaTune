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

    DiagnosticsModel {
        id: diagnostics
        controller: pipeline
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

    // P2.6/FR-16's status-message half, V1-parity wording
    // (legacy/ui/main_window.py's statusBar().showMessage() call sites).
    // basename() has no QML builtin - this one-liner is the whole
    // reason it isn't worth a C++ helper (AGENTS.md §8: move JS longer
    // than ~10 lines, not a single expression).
    property string statusMessage: qsTr("No image open")
    function basename(path) {
        return path.split(/[\\/]/).pop()
    }

    Connections {
        target: doc
        function onPathChanged() {
            if (doc.loaded) {
                window.statusMessage = qsTr("Loaded: %1").arg(window.basename(doc.path))
            }
        }
        function onErrorOccurred(message) {
            // FR-16: modal, with path and reason - both already baked
            // into `message` by ImageDocument::toMessage().
            errorDialog.message = message
            errorDialog.open()
            window.statusMessage = message
        }
    }

    Connections {
        target: pipeline
        function onPreviewFailed(message) {
            // Not a file-IO error (ImageDocument's errorOccurred covers
            // those) - a colour-space failure mid-pipeline. V1 only ever
            // surfaced this in its status bar, never a modal, so this
            // doesn't either.
            window.statusMessage = qsTr("Pipeline error: %1").arg(message)
        }
    }

    ErrorDialog {
        id: errorDialog
        anchors.centerIn: parent
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
                text: qsTr("&Export…")
                enabled: doc.loaded
                onTriggered: exportDialog.open()
            }

            MenuSeparator {}

            MenuItem {
                text: qsTr("E&xit")
                onTriggered: Qt.quit()
            }
        }
    }

    footer: ToolBar {
        Label {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 8
            text: window.statusMessage
        }
    }

    ExportDialog {
        id: exportDialog
        anchors.centerIn: parent
        controller: pipeline
    }

    PreviewItem {
        id: preview
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: controlsPane.left
        controller: pipeline
    }

    ScrollView {
        id: controlsPane
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 280
        clip: true

        Column {
            width: controlsPane.width
            spacing: 12

            Controls {
                width: parent.width
                controller: pipeline
            }

            // P2.4: luma-only before/after histogram (PLAN.md §8.1).
            // RGB overlay, clipping indicators, and the log-scale toggle
            // are P3.6.
            HistogramItem {
                width: parent.width
                height: 80
                controller: pipeline
            }

            // Per-stage readout (FR-10) - no dedicated Diagnostics.qml
            // wrapper yet (same as HistogramItem/PreviewItem above):
            // there's nothing beyond plain text to lay out until a
            // later phase adds real styling/controls around it.
            Label {
                width: parent.width
                wrapMode: Text.WordWrap
                text: qsTr("Black/white point: %1 / %2 (0-255)\nTotal: %3 ms%4\n%5")
                    .arg(diagnostics.blackPoint.toFixed(1))
                    .arg(diagnostics.whitePoint.toFixed(1))
                    .arg(diagnostics.totalMs.toFixed(1))
                    .arg(diagnostics.anyGpu ? qsTr(" (GPU)") : "")
                    .arg(diagnostics.wbExplanation)
            }
        }
    }

    // Drop target for opening a document. Only the first dropped URL
    // becomes the open document (batch import is BatchDialog's job,
    // P2.5+) - a non-local URL now surfaces through the errorOccurred
    // Connections above (P2.6) rather than failing silently.
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
