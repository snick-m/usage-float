import QtQuick
import QtQuick.Controls

ToolTip {
    id: tip
    // An item popup is clipped by the 44px capsule. A separate popup surface
    // lets the compositor place the complete tooltip outside the parent.
    popupType: Popup.Window
    delay: 450
    padding: 11
    margins: 8
    width: Math.min(300, implicitWidth)
    font.pixelSize: 11
    contentItem: Text {
        text: tip.text
        font: tip.font
        color: "#dbe5f3"
        textFormat: Text.PlainText
        wrapMode: Text.WordWrap
        lineHeight: 1.3
    }
    background: Rectangle {
        radius: 10
        color: "#202a3b"
        border.color: "#3a4a61"
    }
}
