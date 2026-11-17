import QtQuick
import QtQuick.Controls

// FR-16: "Modal error dialogs for unreadable... files with path and
// reason" - the message text itself already carries both (see
// ImageDocument's toMessage()), so this is just the generic modal
// shell Main.qml raises it through; one instance reused for every
// source rather than a dialog type per error kind.
Dialog {
    id: root
    title: qsTr("Error")
    modal: true
    standardButtons: Dialog.Ok
    closePolicy: Popup.CloseOnEscape

    property alias message: messageLabel.text

    Label {
        id: messageLabel
        width: 320
        wrapMode: Text.WordWrap
        Accessible.name: text
    }
}
