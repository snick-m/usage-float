import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: meter
    required property var reading
    property color accent: "#a7dcca"
    readonly property real used: reading.percent === undefined ? 0 : reading.percent
    readonly property color meterColor: used >= 90 ? "#ef8f93" : used >= 75 ? "#e6c17e" : accent
    spacing: 7
    Layout.fillWidth: true
    RowLayout {
        Layout.fillWidth: true
        Text { text: meter.reading.label; color: "#bcc6d7"; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
        Text { text: Math.round(meter.used) + "%"; color: "#edf2f8"; font.pixelSize: 12; font.weight: Font.DemiBold }
    }
    Rectangle {
        Layout.fillWidth: true
        implicitHeight: 5
        radius: 3
        color: "#293345"
        Rectangle {
            height: parent.height; radius: 3
            width: parent.width * meter.used / 100
            color: meter.meterColor
            Behavior on width { NumberAnimation { duration: 450; easing.type: Easing.OutCubic } }
        }
        UsageToolTip {
            visible: hover.hovered
            text: meter.reading.resetDate || "Reset time unavailable"
        }
        HoverHandler { id: hover }
    }
    Text {
        text: "Resets in " + meter.reading.countdown
        color: "#7f8fa8"; font.pixelSize: 10
        Layout.fillWidth: true; elide: Text.ElideRight
    }
}
