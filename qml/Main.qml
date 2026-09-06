import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1000
    height: 660
    minimumWidth: 720
    minimumHeight: 480
    visible: true
    title: qsTr("雷电将军播放器")
    color: "#16161e"

    // 播放列表（输入模块：直接来自 SQLite）
    PlaylistView {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: controls.top
    }

    // 底部控制栏（播放引擎：播放/暂停/倍速等）
    PlayerControls {
        id: controls
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
}
