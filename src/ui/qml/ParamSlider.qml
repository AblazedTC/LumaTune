import QtQuick
import QtQuick.Controls

// One labelled, integer-stepped slider with a numeric readout and
// double-click-to-reset (FR-11) - shared by Controls.qml's four
// V1-parity sliders (AGENTS.md §8: "Sliders are integer-valued in
// QML"). The int<->float Params mapping deliberately does NOT live here
// - it lives beside each slider's instantiation in Controls.qml, next
// to its mirrored inverse (AGENTS.md §8: "the int→float mapping lives
// in one place per parameter... mirror the inverse mapping right beside
// it"), since this component has no notion of which PipelineController
// property it's driving.
Column {
    id: root
    spacing: 2

    property string label: ""
    property int from: 0
    property int to: 100
    property int value: 0
    property int defaultValue: 0
    // Cosmetic only - what the readout shows for the current `value`
    // (e.g. "0.5 %"); supplied by the instantiating site.
    property string displayText: String(value)

    signal moved(int value)

    Label {
        text: root.label + ": " + root.displayText
        Accessible.name: root.label

        MouseArea {
            anchors.fill: parent
            // Setting slider.value directly doesn't fire Slider's own
            // moved() signal (that only follows actual user drag/touch/
            // keyboard interaction) - root.moved() has to be emitted
            // explicitly here, or the reset would move the handle
            // without ever telling Controls.qml to update the Params
            // value behind it.
            onDoubleClicked: {
                slider.value = root.defaultValue
                root.moved(root.defaultValue)
            }
        }
    }

    Slider {
        id: slider
        width: root.width
        from: root.from
        to: root.to
        stepSize: 1
        value: root.value
        Accessible.name: root.label
        onMoved: root.moved(value)
    }
}
