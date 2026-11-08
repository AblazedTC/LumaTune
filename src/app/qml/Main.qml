import QtQuick
import QtQuick.Controls
import LumaTune
import LumaTune.Ui

ApplicationWindow {
    id: window
    width: 1280
    height: 800
    visible: true
    title: qsTr("LumaTune")
    color: Theme.background

    RecentFilesModel {
        id: recentFiles
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
                        text: modelData
                        onTriggered: recentFiles.addPath(modelData)
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

    // Drop target for opening a document (ImageDocument wiring is P2.2);
    // for now this just records the dropped path(s) as recent.
    DropArea {
        anchors.fill: parent
        onDropped: (drop) => {
            if (drop.hasUrls) {
                for (const url of drop.urls) {
                    recentFiles.addPath(url.toString())
                }
            }
        }
    }
}
