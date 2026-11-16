import QtQuick
import LumaTune

// V1-parity controls (PLAN.md P2.3): one StageCard per bypassable
// classical stage, each holding that stage's sliders. The int<->float
// mapping for each V1-parity slider lives here, right next to its
// mirrored inverse (AGENTS.md §8) - the exact mappings are legacy/ui/
// main_window.py's own (quoted in each slider's comment below), kept
// for V1 parity.
Column {
    id: root
    property PipelineController controller
    spacing: 12

    StageCard {
        title: qsTr("Levels")
        width: root.width
        stageEnabled: !root.controller.levelsBypass
        onStageEnabledChanged: root.controller.levelsBypass = !stageEnabled

        ParamSlider {
            width: parent.width
            label: qsTr("Shadows")
            from: 0
            to: 50
            defaultValue: 5
            // int 0-50 -> black_pct 0.0-5.0; default 5 -> 0.5
            value: Math.round(root.controller.blackPct * 10)
            displayText: (value / 10.0).toFixed(1) + "%"
            onMoved: root.controller.blackPct = value / 10.0
        }

        ParamSlider {
            width: parent.width
            label: qsTr("Highlights")
            from: 0
            to: 50
            defaultValue: 45
            // int 0-50 -> white_pct 95.0-100.0; default 45 -> 99.5
            value: Math.round((root.controller.whitePct - 95.0) * 10)
            displayText: (95.0 + value / 10.0).toFixed(1) + "%"
            onMoved: root.controller.whitePct = 95.0 + value / 10.0
        }
    }

    StageCard {
        title: qsTr("Clarity")
        width: root.width
        stageEnabled: !root.controller.claheBypass
        onStageEnabledChanged: root.controller.claheBypass = !stageEnabled

        ParamSlider {
            width: parent.width
            label: qsTr("Clarity")
            from: 10
            to: 100
            defaultValue: 20
            // int 10-100 -> clahe_clip 1.0-10.0; default 20 -> 2.0
            value: Math.round(root.controller.claheClip * 10)
            displayText: (value / 10.0).toFixed(1)
            onMoved: root.controller.claheClip = value / 10.0
        }

        ParamSlider {
            width: parent.width
            label: qsTr("Tile Size")
            from: 4
            to: 16
            defaultValue: 8
            // 1:1, no mapping needed - clahe_tile is already the UI's
            // integer tile count.
            value: root.controller.claheTile
            displayText: String(value)
            onMoved: root.controller.claheTile = value
        }
    }
}
