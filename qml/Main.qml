import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: root
    title: "Usage Float"
    visible: true
    width: compactMode ? capsuleWidth : 328
    height: targetHeight
    minimumWidth: 280
    maximumWidth: 560
    minimumHeight: 44
    color: "transparent"
    flags: Qt.Window | Qt.WindowCloseButtonHint | Qt.WindowMinimizeButtonHint
        | (controller.borderless ? Qt.FramelessWindowHint : 0)
        | (!controller.wayland && controller.pinned ? Qt.WindowStaysOnTopHint : 0)
    property bool settingsOpen: false
    property bool trayAvailable: false
    readonly property bool compactMode: controller.compact
    readonly property int capsuleWidth: controller.wide ? 536 : 328
    readonly property int availableHeight: Math.max(44, controller.availableScreenHeight - 80)
    readonly property int targetHeight: compactMode ? 44 : Math.min(availableHeight, Math.ceil(mainContent.implicitHeight + 32))
    function fitContent() {
        height = targetHeight
        if (compactMode) width = capsuleWidth
    }
    onCompactModeChanged: {
        width = compactMode ? capsuleWidth : 328
        Qt.callLater(fitContent)
    }
    onCapsuleWidthChanged: if (compactMode) Qt.callLater(fitContent)
    onTargetHeightChanged: Qt.callLater(fitContent)
    // Wayland configure events can arrive after a view change and restore the
    // old size. Reapply the content size after that event has settled.
    onHeightChanged: if (height !== targetHeight) Qt.callLater(fitContent)
    onWidthChanged: if (compactMode && width !== capsuleWidth) Qt.callLater(fitContent)
    readonly property color ink: "#eef2f8"
    readonly property color muted: "#8292aa"
    onClosing: function(event) {
        if (trayAvailable) { event.accepted = false; root.hide() }
    }
    Shortcut { sequence: "Ctrl+R"; onActivated: controller.refresh() }
    Shortcut { sequence: "Ctrl+,"; onActivated: { controller.compact = false; root.settingsOpen = !root.settingsOpen } }
    Shortcut { sequence: "Ctrl+M"; onActivated: controller.compact = !controller.compact }
    Shortcut { sequence: "Ctrl+W"; onActivated: { controller.wide = !controller.wide; controller.compact = true } }
    Shortcut { sequence: "Ctrl+Q"; onActivated: controller.quit() }

    Rectangle {
        anchors.fill: parent
        radius: controller.borderless ? (controller.compact ? height / 2 : 18) : 0
        color: "#121926"
        border.color: "#303c50"
        border.width: controller.borderless ? 1 : 0
        opacity: controller.panelOpacity
    }

    Item {
        id: compactContent
        visible: controller.compact
        anchors.fill: parent
        DragHandler {
            target: null
            onActiveChanged: if (active) controller.moveWindow()
        }
        TapHandler {
            acceptedButtons: Qt.RightButton
            onTapped: capsuleMenu.popup()
        }
        TapHandler { onDoubleTapped: controller.compact = false }
        RowLayout {
            anchors { fill: parent; leftMargin: 17; rightMargin: 9; topMargin: 6; bottomMargin: 6 }
            spacing: 12
            Repeater {
                model: [{name: "Claude", state: controller.claude, accent: "#d6ac89"},
                        {name: "Codex", state: controller.codex, accent: "#a7dcca"}]
                RowLayout {
                    id: capsuleProvider
                    required property var modelData
                    readonly property var rows: modelData.state.rows || []
                    Layout.fillWidth: true
                    spacing: 7
                    Rectangle {
                        implicitWidth: 4; implicitHeight: 19; radius: 2
                        color: capsuleProvider.modelData.state.stale || capsuleProvider.modelData.state.status === "error"
                            ? "#e6bd8f" : capsuleProvider.modelData.accent
                    }
                    ColumnLayout {
                        visible: !controller.wide
                        Layout.fillWidth: true
                        spacing: 1
                        RowLayout {
                            Layout.fillWidth: true; spacing: 8
                            ColumnLayout {
                                spacing: 0
                                Text { text: capsuleProvider.modelData.name; color: capsuleProvider.modelData.accent; font.pixelSize: 10; font.weight: Font.Medium }
                                Text { text: capsuleProvider.modelData.state.model; color: root.muted; font.pixelSize: 8; Layout.maximumWidth: 82; elide: Text.ElideRight }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 0
                            Repeater {
                                model: capsuleProvider.rows.slice(0, 2)
                                Row {
                                    required property var modelData
                                    spacing: 3
                                    Text { text: modelData.label === "Session" ? "S" : modelData.label === "Weekly" ? "W" : modelData.label.charAt(0); color: root.muted; font.pixelSize: 10 }
                                    Text { text: Math.round(modelData.displayPercent) + "%"; color: modelData.percent >= 90 ? "#ef8f93" : root.ink; font.pixelSize: 11; font.weight: Font.DemiBold }
                                }
                            }
                            Text { visible: capsuleProvider.rows.length === 0; text: capsuleProvider.modelData.state.busy ? "Syncing…" : "Unavailable"; color: root.muted; font.pixelSize: 10 }
                            Text { visible: capsuleProvider.modelData.state.stale; text: "!"; color: "#e6bd8f"; font.pixelSize: 10 }
                            }
                        }
                    }
                    RowLayout {
                        visible: controller.wide
                        Layout.fillWidth: true
                        spacing: 10
                        ColumnLayout {
                            spacing: 1
                            Text { text: capsuleProvider.modelData.name; color: capsuleProvider.modelData.accent; font.pixelSize: 11; font.weight: Font.Medium }
                            Text { text: capsuleProvider.modelData.state.model; color: root.muted; font.pixelSize: 8; Layout.maximumWidth: 78; elide: Text.ElideRight }
                        }
                        Repeater {
                            model: capsuleProvider.rows.slice(0, 2)
                            ColumnLayout {
                                required property var modelData
                                Layout.fillWidth: true
                                spacing: 4
                                RowLayout {
                                    Layout.fillWidth: true; spacing: 4
                                    Text { text: modelData.label; color: root.muted; font.pixelSize: 9; Layout.fillWidth: true; elide: Text.ElideRight }
                                    Text { text: Math.round(modelData.displayPercent) + "%"; color: root.ink; font.pixelSize: 10; font.weight: Font.DemiBold }
                                }
                                Rectangle {
                                    Layout.fillWidth: true
                                    implicitHeight: 4; radius: 2; color: "#293345"
                                    Rectangle {
                                        height: parent.height; width: parent.width * modelData.displayPercent / 100; radius: 2
                                        color: modelData.percent >= 90 ? "#ef8f93" : modelData.percent >= 75 ? "#e6c17e" : capsuleProvider.modelData.accent
                                        Behavior on width { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
                                    }
                                }
                            }
                        }
                        Text { visible: capsuleProvider.rows.length === 0; text: capsuleProvider.modelData.state.busy ? "Syncing…" : "Unavailable"; color: root.muted; font.pixelSize: 10; Layout.fillWidth: true }
                        Text { visible: capsuleProvider.modelData.state.stale; text: "!"; color: "#e6bd8f"; font.pixelSize: 10 }
                    }
                    UsageToolTip {
                        objectName: "providerToolTip"
                        visible: providerHover.hovered
                        text: {
                            let lines = [capsuleProvider.modelData.name]
                            lines.push("Default model: " + capsuleProvider.modelData.state.model)
                            for (let row of capsuleProvider.rows.slice(0, 2))
                                lines.push(row.label + ": " + Math.round(row.displayPercent) + (controller.showRemaining ? "% remaining" : "% used") + " · resets in " + row.countdown)
                            if (capsuleProvider.modelData.state.stale) lines.push("Stale reading — open details")
                            else if (capsuleProvider.modelData.state.status === "error") lines.push(capsuleProvider.modelData.state.message)
                            return lines.join("\n")
                        }
                    }
                    HoverHandler { id: providerHover }
                }
            }
            IconButton {
                text: "⋯"; hint: "Options · double-click capsule to expand"
                implicitWidth: 23; implicitHeight: 26
                onClicked: capsuleMenu.popup()
            }
        }
        Menu {
            id: capsuleMenu
            palette.window: "#1b2331"
            palette.text: root.ink
            palette.highlight: "#354258"
            palette.highlightedText: root.ink
            MenuItem { text: "Expand details"; onTriggered: controller.compact = false }
            MenuItem {
                text: "Wide usage bars"
                checkable: true; checked: controller.wide
                onTriggered: controller.wide = checked
            }
            MenuItem {
                text: "Show remaining instead of used"
                checkable: true; checked: controller.showRemaining
                onTriggered: controller.showRemaining = checked
            }
            MenuItem { text: "Refresh usage"; onTriggered: controller.refresh() }
            MenuItem { text: "Settings"; onTriggered: { controller.compact = false; root.settingsOpen = true } }
            MenuSeparator {}
            MenuItem {
                text: "System title bar"
                checkable: true; checked: !controller.borderless
                onTriggered: controller.borderless = !checked
            }
            MenuItem { text: "Quit"; onTriggered: controller.quit() }
        }
    }

    Flickable {
        id: scroll
        objectName: "detailsScroll"
        visible: !controller.compact
        anchors.fill: parent
        contentWidth: width
        contentHeight: mainContent.implicitHeight + 32
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        ColumnLayout {
            id: mainContent
            x: 16; y: 16; width: scroll.width - 32
            spacing: 12
            RowLayout {
                Layout.fillWidth: true; spacing: 4
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 4
                    Text { text: "USAGE FLOAT"; color: "#9baec9"; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 2.2 }
                    Text { text: "Room to create."; color: root.ink; font.pixelSize: 21; font.weight: Font.DemiBold; font.letterSpacing: -0.5 }
                }
                IconButton { text: "↻"; hint: "Refresh · Ctrl+R"; onClicked: controller.refresh() }
                IconButton { text: "−"; hint: "Compact view · Ctrl+M"; onClicked: controller.compact = true }
                IconButton { text: "⚙"; hint: "Settings · Ctrl+,"; selected: root.settingsOpen; onClicked: root.settingsOpen = !root.settingsOpen }
            }
            RowLayout {
                Layout.fillWidth: true
                Rectangle { implicitWidth: 5; implicitHeight: 5; radius: 3; color: controller.refreshing ? "#90a6de" : controller.claude.status === "ready" && controller.codex.status === "ready" ? "#a7dcca" : "#e6bd8f" }
                Text { text: controller.statusText; color: root.muted; font.pixelSize: 10; Layout.fillWidth: true; elide: Text.ElideRight }
                MouseArea { Layout.preferredWidth: 25; Layout.preferredHeight: 18; cursorShape: Qt.SizeAllCursor; onPressed: controller.moveWindow(); Text { anchors.centerIn: parent; text: "⠿"; color: root.muted } }
            }

            Rectangle {
                visible: root.settingsOpen
                Layout.fillWidth: true
                implicitHeight: settingsContent.implicitHeight + 26
                radius: 12; color: "#1b2331"; border.color: "#354258"
                ColumnLayout {
                    id: settingsContent
                    anchors { left: parent.left; right: parent.right; top: parent.top; margins: 13 }
                    spacing: 10
                    Text { text: "Make it yours"; color: root.ink; font.pixelSize: 13; font.weight: Font.DemiBold }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Show remaining allowance"; color: "#bcc6d7"; font.pixelSize: 11; Layout.fillWidth: true }
                        Switch { checked: controller.showRemaining; onToggled: controller.showRemaining = checked; implicitHeight: 28 }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Start at login"; color: "#bcc6d7"; font.pixelSize: 11; Layout.fillWidth: true }
                        Switch { checked: controller.autoStart; onToggled: controller.autoStart = checked; implicitHeight: 28 }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "System title bar"; color: "#bcc6d7"; font.pixelSize: 11; Layout.fillWidth: true }
                        Switch { checked: !controller.borderless; onToggled: controller.borderless = !checked; implicitHeight: 28 }
                    }
                    RowLayout {
                        visible: !controller.wayland; Layout.fillWidth: true
                        Text { text: "Always on top"; color: "#bcc6d7"; font.pixelSize: 11; Layout.fillWidth: true }
                        Switch { checked: controller.pinned; onToggled: controller.pinned = checked; implicitHeight: 28 }
                    }
                    Text {
                        visible: controller.wayland
                        text: "On Wayland, use the system title bar’s menu to select Always on Top. Your desktop controls placement."
                        color: root.muted; font.pixelSize: 10; wrapMode: Text.WordWrap; Layout.fillWidth: true
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Refresh interval"; color: "#bcc6d7"; font.pixelSize: 11; Layout.fillWidth: true }
                        SpinBox { from: 1; to: 30; value: controller.intervalMinutes; onValueModified: controller.intervalMinutes = value; implicitWidth: 104; implicitHeight: 30; palette.text: root.ink; palette.base: "#263144"; palette.button: "#354258"; palette.buttonText: root.ink }
                        Text { text: "min"; color: root.muted; font.pixelSize: 10 }
                    }
                    Text { text: "Background opacity"; color: "#bcc6d7"; font.pixelSize: 11 }
                    Slider { Layout.fillWidth: true; from: 0.45; to: 1; value: controller.panelOpacity; onMoved: controller.panelOpacity = value; implicitHeight: 24 }
                    Text { text: "Codex executable (optional)"; color: "#bcc6d7"; font.pixelSize: 11 }
                    TextField {
                        Layout.fillWidth: true; text: controller.codexPath
                        placeholderText: "Auto-detect from PATH"; color: root.ink; placeholderTextColor: "#74849d"
                        font.pixelSize: 11; selectByMouse: true
                        onEditingFinished: controller.codexPath = text
                        background: Rectangle { radius: 6; color: "#121926"; border.color: parent.activeFocus ? "#90a6de" : "#354258" }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "v" + Qt.application.version + " · C++ / Qt Quick"; color: root.muted; font.pixelSize: 9; Layout.fillWidth: true }
                        Button { text: "Quit"; onClicked: controller.quit(); implicitHeight: 28; palette.button: "#354258"; palette.buttonText: root.ink }
                    }
                }
            }

            ProviderCard { visible: !root.settingsOpen; provider: "claude"; providerName: "Claude Code"; monogram: "✳"; accent: "#d6ac89"; state: controller.claude }
            ProviderCard { visible: !root.settingsOpen; provider: "codex"; providerName: "Codex"; monogram: ">_"; accent: "#a7dcca"; state: controller.codex }
            Text {
                visible: controller.notice.length > 0
                text: controller.notice; color: "#c5d3e8"; font.pixelSize: 10
                wrapMode: Text.WordWrap; Layout.fillWidth: true
            }
            RowLayout {
                Layout.fillWidth: true
                Text { text: "Your logins. Your limits. All local."; color: "#64758e"; font.pixelSize: 9; Layout.fillWidth: true }
                IconButton { visible: root.trayAvailable; text: "⌄"; hint: "Hide to tray"; implicitHeight: 22; implicitWidth: 22; onClicked: controller.hideWindow() }
            }
        }
    }
    MouseArea {
        enabled: !controller.compact
        anchors { right: parent.right; top: parent.top; bottom: parent.bottom }
        width: 5; cursorShape: Qt.SizeHorCursor
        onPressed: controller.resizeWindow()
    }
}
