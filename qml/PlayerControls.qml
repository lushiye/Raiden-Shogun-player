import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    height: 168
    color: "#ffffff"

    // 倍速选项：只在这里维护一份数据，按钮与下拉菜单共用
    readonly property var rateOptions: [
        { label: "0.5x",  value: 0.5 },
        { label: "0.75x", value: 0.75 },
        { label: "1.0x",  value: 1.0 },
        { label: "1.25x", value: 1.25 },
        { label: "1.5x",  value: 1.5 },
        { label: "2.0x",  value: 2.0 }
    ]

    // 当前倍速文本：优先匹配预设档位，播放器倍速被外部修改时也能正确显示
    readonly property string rateLabel: {
        for (var i = 0; i < rateOptions.length; ++i) {
            if (Math.abs(rateOptions[i].value - player.playbackRate) < 0.001)
                return rateOptions[i].label
        }
        return Number(player.playbackRate.toFixed(2)) + "x"
    }

    // 当前曲目：数据库里 title 存的是文件名、artist 为空，
    // 文件名约定为「歌手 - 歌名」，这里拆开——左边显示歌名，右边显示歌手。
    readonly property var currentTrack: parseTrackName(player.currentTitle, player.currentArtist)

    function parseTrackName(rawTitle, rawArtist) {
        var title = (rawTitle === undefined || rawTitle === null) ? "" : String(rawTitle).trim()
        var artist = (rawArtist === undefined || rawArtist === null) ? "" : String(rawArtist).trim()
        if (artist !== "" || title === "")
            return { title: title, artist: artist }

        // 优先按「空格 - 空格」拆分，取第一个横杠（歌名里可能还有横杠）
        var pos = title.indexOf(" - ")
        if (pos > 0)
            return { title: title.slice(pos + 3).trim(), artist: title.slice(0, pos).trim() }

        // 兜底：整串只有一个横杠（-、– 或 —）时同样按「歌手 - 歌名」拆
        var m = title.match(/^([^\-–—]+)[\-–—]([^\-–—]+)$/)
        if (m)
            return { title: m[2].trim(), artist: m[1].trim() }

        return { title: title, artist: "" }
    }

    // 顶部分隔线
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: "#e4e4ea"
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
                text: root.currentTrack.title !== "" ? root.currentTrack.title : qsTr("未选择曲目")
                color: "#23232f"
                font.pixelSize: 17
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Text {
                text: root.currentTrack.artist !== "" ? root.currentTrack.artist : qsTr("未知艺术家")
                color: "#6e6e7b"
                font.pixelSize: 13
                elide: Text.ElideRight
                Layout.preferredWidth: 160
                horizontalAlignment: Text.AlignRight
            }
        }

        // 进度条
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                text: player.formatDuration(player.position)
                color: "#6e6e7b"
                font.pixelSize: 11
                Layout.preferredWidth: 44
            }

            Slider {
                id: positionSlider
                Layout.fillWidth: true
                from: 0
                to: Math.max(1, player.duration)
                enabled: player.duration > 0
                focusPolicy: Qt.NoFocus          // 抢占焦点没有意义，顺手避免任何焦点装饰
                onMoved: player.seek(value)

                // 拖动时暂停跟随播放进度，否则 position 的刷新会把滑块拉回去
                Binding on value {
                    when: !positionSlider.pressed
                    value: player.position
                }
            }

            Text {
                text: player.formatDuration(player.duration)
                color: "#6e6e7b"
                font.pixelSize: 11
                Layout.preferredWidth: 44
                horizontalAlignment: Text.AlignRight
            }
        }

        // 控制按钮行
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 56

            // 居中的播放控制
            RowLayout {
                anchors.centerIn: parent
                spacing: 18

                IconButton {
                    source: "qrc:/icons/prev.png"
                    iconSize: 28
                    enabled: trackModel.count > 0
                    onClicked: player.previous()
                }

                IconButton {
                    source: player.playing ? "qrc:/icons/pause.png" : "qrc:/icons/play.png"
                    iconSize: 38
                    enabled: trackModel.count > 0
                    onClicked: player.toggle()
                }

                IconButton {
                    source: "qrc:/icons/next.png"
                    iconSize: 28
                    enabled: trackModel.count > 0
                    onClicked: player.next()
                }
            }

            // 右侧：倍速 + 音量
            RowLayout {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10

                Text {
                    text: qsTr("倍速")
                    color: "#6e6e7b"
                    font.pixelSize: 13
                }

                // 倍速选择器：完全自绘，不继承任何控件样式。
                // 之前用 ComboBox 时，Windows 11 样式会为原生样式项画一个虚线焦点框
                // （NativeStyle 的 WindowsFocusFrame，setLineDash([1, 1])），
                // 而且点完后不会自己消失；改为自绘后不存在任何原生焦点装饰。
                Rectangle {
                    id: rateButton
                    Layout.preferredWidth: 76
                    Layout.preferredHeight: 30
                    radius: 6
                    color: rateMouse.containsMouse || ratePopup.opened ? "#efeff5" : "#f6f6f9"
                    border.width: 1
                    border.color: ratePopup.opened ? "#3d6fd4" : "#e0e0e8"

                    Behavior on color { ColorAnimation { duration: 120 } }
                    Behavior on border.color { ColorAnimation { duration: 120 } }

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.rateLabel
                        color: "#23232f"
                        font.pixelSize: 13
                    }

                    // 下拉箭头（用字符画，避免额外图标资源）
                    Text {
                        anchors.right: parent.right
                        anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        text: "\u25BE"
                        color: "#8a8a97"
                        font.pixelSize: 12
                    }

                    MouseArea {
                        id: rateMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: ratePopup.opened ? ratePopup.close() : ratePopup.open()
                    }

                    Popup {
                        id: ratePopup
                        // 控制栏贴在窗口底部：默认向上弹出，上方空间不足时才向下
                        x: 0
                        y: (rateButton.mapToItem(null, 0, 0).y > height + 10)
                           ? -height - 6
                           : rateButton.height + 6
                        width: rateButton.width
                        padding: 4

                        // 不抢焦点：菜单只做鼠标选择，弹出/关闭都不会留下焦点残留
                        focus: false
                        closePolicy: Popup.CloseOnPressOutside

                        background: Rectangle {
                            radius: 6
                            color: "#ffffff"
                            border.width: 1
                            border.color: "#e0e0e8"
                        }

                        contentItem: Column {
                            spacing: 2

                            Repeater {
                                model: root.rateOptions

                                delegate: Rectangle {
                                    id: rateOption
                                    property real optionValue: modelData.value
                                    property string optionLabel: modelData.label

                                    width: ratePopup.availableWidth
                                    height: 28
                                    radius: 4
                                    color: optionMouse.containsMouse ? "#e6eefb" : "transparent"

                                    Behavior on color { ColorAnimation { duration: 90 } }

                                    Text {
                                        anchors.left: parent.left
                                        anchors.leftMargin: 10
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: rateOption.optionLabel
                                        color: Math.abs(rateOption.optionValue - player.playbackRate) < 0.001 ? "#3d6fd4" : "#23232f"
                                        font.pixelSize: 13
                                        font.bold: Math.abs(rateOption.optionValue - player.playbackRate) < 0.001
                                    }

                                    MouseArea {
                                        id: optionMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            player.setPlaybackRate(rateOption.optionValue)
                                            ratePopup.close()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Text {
                    text: qsTr("音量")
                    color: "#6e6e7b"
                    font.pixelSize: 13
                }

                Slider {
                    id: volumeSlider
                    Layout.preferredWidth: 96
                    from: 0
                    to: 1.0
                    value: player.volume
                    focusPolicy: Qt.NoFocus
                    onMoved: player.setVolume(value)

                    ToolTip.visible: volumeSlider.hovered
                    ToolTip.text: Math.round(volumeSlider.value * 100) + "%"
                }
            }
        }
    }
}
