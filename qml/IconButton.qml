import QtQuick
import QtQuick.Controls

AbstractButton {
    id: button
    property string hint: ""
    property bool selected: false
    implicitWidth: 28
    implicitHeight: 28
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    Accessible.name: hint
    ToolTip.visible: hovered && hint.length > 0
    ToolTip.text: hint
    ToolTip.delay: 550
    background: Rectangle {
        radius: 8
        color: button.down ? "#303b4c" : button.hovered || button.selected ? "#253043" : "transparent"
        border.width: button.activeFocus ? 1 : 0
        border.color: "#9cbbdf"
        Behavior on color { ColorAnimation { duration: 120 } }
    }
    contentItem: Text {
        text: button.text
        color: button.hovered || button.selected ? "#eef2f8" : "#9aa8bd"
        font.pixelSize: 16
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
