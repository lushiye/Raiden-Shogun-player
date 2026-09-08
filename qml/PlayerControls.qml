import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    height: 168
    color: "#1b1b27"

    // 顶部分隔线
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: "#2e2e3e"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 8

        // 当前曲目信息
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: player.currentTitle !== "" ? player.currentTitle : qsTr("未选择曲目")
                color: "#ffffff"
                font.pixelSize: 17
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Text {
                text: player.currentArtist !== "" ? player.currentArtist : qsTr("未知艺术家")
                color: "#9a9ab0"
                font.pixelSize: 13
                elide: Text.ElideRight
                Layout.preferredWidth: 160
            }
        }

        // 进度条
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                text: player.formatDuration(player.position)
                color: "#9a9ab0"
                font.pixelSize: 11
                Layout.preferredWidth: 44
            }

            Slider {
                id: positionSlider
                Layout.fillWidth: true
                from: 0
                to: Math.max(1, player.duration)
                value: player.position
                enabled: player.duration > 0
                onMoved: player.seek(positionSlider.value)
            }

            Text {
                text: player.formatDuration(player.duration)
                color: "#9a9ab0"
                font.pixelSize: 11
                Layout.preferredWidth: 44
                horizontalAlignment: Text.AlignRight
            }
        }

        // 控制按钮行
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            StandardButton {
                text: qsTr("⏮ 上一首")
                height: 40
                width: 120
                enabled: trackModel.count > 0
                onClicked: player.previous()
            }

            StandardButton {
                text: player.playing ? qsTr("⏸ 暂停") : qsTr("▶ 播放")
                height: 40
                width: 120
                enabled: trackModel.count > 0
                font.bold: true
                onClicked: player.toggle()
            }

            StandardButton {
                text: qsTr("下一首 ⏭")
                height: 40
                width: 120
                enabled: trackModel.count > 0
                onClicked: player.next()
            }

            Item { Layout.fillWidth: true }

            Text {
                text: qsTr("倍速")
                color: "#9a9ab0"
                font.pixelSize: 13
            }

            ComboBox {
                id: rateBox
                textRole: "text"
                valueRole: "value"
                model: [
                    { text: "0.5x", value: 0.5 },
                    { text: "0.75x", value: 0.75 },
                    { text: "1.0x", value: 1.0 },
                    { text: "1.25x", value: 1.25 },
                    { text: "1.5x", value: 1.5 },
                    { text: "2.0x", value: 2.0 }
                ]
                currentIndex: 2
                onActivated: player.setPlaybackRate(model[currentIndex].value)
            }

            Text {
                text: qsTr("音量")
                color: "#9a9ab0"
                font.pixelSize: 13
            }

            Slider {
                Layout.preferredWidth: 110
                from: 0
                to: 1.0
                value: player.volume
                onMoved: player.setVolume(value)
            }
        }
    }
}
