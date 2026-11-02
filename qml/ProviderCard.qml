import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: card
    required property var state
    required property string provider
    required property string providerName
    required property string monogram
    required property color accent
    property bool expanded: false
    readonly property var rows: state.rows || []
    Layout.fillWidth: true
    implicitHeight: content.implicitHeight + 28
    radius: 13
    color: "#1b2331"
    border.color: "#2a3445"
    ColumnLayout {
        id: content
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 14 }
        spacing: 15
        RowLayout {
            spacing: 9
            Rectangle {
                implicitWidth: 27; implicitHeight: 27; radius: 8
                color: Qt.rgba(card.accent.r, card.accent.g, card.accent.b, 0.12)
                Text { anchors.centerIn: parent; text: card.monogram; color: card.accent; font.pixelSize: 14; font.weight: Font.DemiBold }
            }
            ColumnLayout {
                spacing: 2; Layout.fillWidth: true
                Text { text: card.providerName; color: "#edf1f7"; font.pixelSize: 12; font.weight: Font.DemiBold }
                Text { text: card.state.model; color: card.accent; font.pixelSize: 10; Layout.fillWidth: true; elide: Text.ElideRight }
                Text {
                    text: card.state.plan ? card.state.plan.charAt(0).toUpperCase() + card.state.plan.slice(1) + " plan" : "Subscription allowance"
                    color: "#8090a8"; font.pixelSize: 10
                }
            }
            Rectangle {
                implicitWidth: statusLabel.implicitWidth + 12; implicitHeight: 20; radius: 6
                color: card.state.stale || card.state.status === "error" ? "#3b2e29" : "#233a35"
                Text {
                    id: statusLabel; anchors.centerIn: parent
                    text: card.state.busy ? "Syncing" : card.state.stale ? "Stale" : card.state.status === "error" ? "Offline" : "Live"
                    color: card.state.stale || card.state.status === "error" ? "#e6bd8f" : "#a7dcca"
                    font.pixelSize: 9; font.weight: Font.Medium
                }
            }
        }
        Repeater {
            model: card.expanded ? card.rows.length : Math.min(2, card.rows.length)
            Meter {
                required property int index
                objectName: card.provider + "UsageMeter" + index
                reading: card.rows[index] || {}
                accent: card.accent
            }
        }
        Text {
            visible: card.state.status === "error" || !card.state.hasData
            text: card.state.message || "Waiting for your allowance…"
            color: card.state.status === "error" ? "#d9b792" : "#8b9ab0"
            font.pixelSize: 11; wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        RowLayout {
            visible: card.rows.length > 2
            Layout.fillWidth: true
            Text {
                text: card.expanded ? "Fewer limits ↑" : "+ " + (card.rows.length - 2) + " model limits ↓"
                color: card.accent; font.pixelSize: 10
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: card.expanded = !card.expanded }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 5
            Text { text: card.state.source + " · " + card.state.age; color: "#74849d"; font.pixelSize: 9; Layout.fillWidth: true; elide: Text.ElideRight }
            IconButton { text: "↗"; hint: "Open " + card.providerName + " usage"; implicitWidth: 22; implicitHeight: 20; onClicked: controller.openUsage(card.provider) }
            IconButton { visible: card.state.status === "error"; text: "⌘"; hint: "Copy login command"; implicitWidth: 22; implicitHeight: 20; onClicked: controller.copyLoginCommand(card.provider) }
        }
    }
}
