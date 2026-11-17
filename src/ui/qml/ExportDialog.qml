import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import LumaTune

// P2.5/FR-3: format, JPEG quality, metadata (ICC) toggle, derived
// filename. EXIF is never offered here - Encoder.h's documented
// contract is that it's never re-embedded at all (decode-time pixels
// are already displayed-correct), so there's nothing a toggle could do.
Dialog {
    id: root
    title: qsTr("Export Image")
    modal: true
    standardButtons: Dialog.Cancel
    closePolicy: Popup.CloseOnEscape

    property PipelineController controller

    // The three formats Encoder.cpp actually branches on (FR-3) - BMP/
    // TIFF fall out of cv::imwrite's own extension sniffing but aren't
    // part of this dialog's offered set.
    readonly property var extensions: ["png", "jpg", "webp"]
    readonly property string extension: extensions[formatCombo.currentIndex]

    onOpened: {
        formatCombo.currentIndex = 0
        statusLabel.text = ""
        pathField.text = controller ? controller.derivedExportPath(root.extension) : ""
    }
    onExtensionChanged: pathField.text = controller ? controller.derivedExportPath(root.extension) : ""

    Connections {
        target: root.controller
        function onExportSucceeded(path) {
            statusLabel.text = qsTr("Exported: %1").arg(path)
            root.close()
        }
        function onExportFailed(message) {
            statusLabel.text = message
        }
    }

    FileDialog {
        id: saveDialog
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("PNG (*.png)"), qsTr("JPEG (*.jpg *.jpeg)"), qsTr("WebP (*.webp)")]
        onAccepted: pathField.text = root.controller.toLocalPath(selectedFile)
    }

    // ParamSlider is a value-only component (no onMoved side effect of
    // its own, Controls.qml's convention) - this is the one place that
    // mapping lives for jpegQuality, trivial here since it's already a
    // 1-100 int matching EncodeParams.jpegQuality directly.
    property int jpegQuality: 90

    Column {
        width: 360
        spacing: 10

        ComboBox {
            id: formatCombo
            width: parent.width
            model: [qsTr("PNG"), qsTr("JPEG"), qsTr("WebP")]
            Accessible.name: qsTr("Export format")
        }

        Row {
            width: parent.width
            spacing: 6

            TextField {
                id: pathField
                width: parent.width - browseButton.width - parent.spacing
                Accessible.name: qsTr("Destination path")
            }
            Button {
                id: browseButton
                text: qsTr("Browse…")
                onClicked: saveDialog.open()
            }
        }

        // JPEG/WebP only (Encoder.cpp applies jpegQuality to both);
        // 1:1 with EncodeParams.jpegQuality, no int<->float mapping.
        ParamSlider {
            width: parent.width
            visible: formatCombo.currentIndex !== 0
            label: qsTr("Quality")
            from: 1
            to: 100
            defaultValue: 90
            value: root.jpegQuality
            displayText: String(value)
            onMoved: root.jpegQuality = value
        }

        CheckBox {
            id: sixteenBitCheck
            visible: formatCombo.currentIndex === 0
            text: qsTr("16-bit PNG")
            Accessible.name: text
        }

        CheckBox {
            id: preserveIccCheck
            checked: true
            text: qsTr("Preserve ICC profile")
            Accessible.name: text
        }

        Label {
            id: statusLabel
            width: parent.width
            wrapMode: Text.WordWrap
        }

        Button {
            text: controller && controller.exporting ? qsTr("Exporting…") : qsTr("Export")
            enabled: controller && !controller.exporting && pathField.text.length > 0
            onClicked: controller.exportImage(pathField.text, root.jpegQuality,
                                               sixteenBitCheck.checked, preserveIccCheck.checked)
        }
    }
}
