import QtQuick
import QtQuick.Controls

// One titled card per pipeline stage (PLAN.md §8.1): a bypass toggle
// (FR-8) plus whatever's put inside. Qt Quick Controls' GroupBox already
// draws the titled frame - no need to hand-roll one.
GroupBox {
    id: root

    // Params.h's bypass flag means "stage OFF" - inverted here so the
    // checkbox reads naturally as "<stage> enabled".
    property alias stageEnabled: enabledCheck.checked

    label: CheckBox {
        id: enabledCheck
        text: root.title
        Accessible.name: root.title + " enabled"
    }

    default property alias content: contentColumn.children

    Column {
        id: contentColumn
        width: root.availableWidth
        spacing: 8
    }
}
