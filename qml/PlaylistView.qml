import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: root

    // 导入时 title 存的是文件名（artist 为空），文件名约定是「歌手 - 歌名」。
    // 这里在显示时把歌手从歌名里拆出来；若数据库之后解析出了 artist，则直接使用。
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

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 搜索栏
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: "#ffffff"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 8

                Image {
                    source: "qrc:/icons/search.png"
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 30
                    Layout.alignment: Qt.AlignVCenter
                    fillMode: Image.PreserveAspectFit
                    opacity: 0.6
                }

                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: qsTr("搜索")
                    color: "#23232f"
                    placeholderTextColor: "#a0a0ad"
                    font.pixelSize: 14
                    background: Rectangle {
                        implicitHeight: 30
                        radius: 6
                        color: "#f4f4f7"
                        border.width: 1
                        border.color: searchField.activeFocus ? "#3d6fd4" : "#e6eefb"
                    }
                    // 输入停顿一小会儿再过滤，避免每敲一个字都查一次数据库
                    onTextChanged: searchTimer.restart()
                    Keys.onEscapePressed: clear()
                }

                // 自绘清除按钮：不用原生 Button，避免出现焦点虚线框、也与整体风格一致
                Item {
                    Layout.preferredWidth: 22
                    Layout.preferredHeight: 22
                    Layout.alignment: Qt.AlignVCenter
                    visible: searchField.text.length > 0

                    Rectangle {
                        anchors.fill: parent
                        radius: width / 2
                        color: clearMouse.containsMouse ? "#efeff5" : "transparent"

                        Behavior on color { ColorAnimation { duration: 100 } }
                    }

                    Text {
                        anchors.centerIn: parent
                        text: "\u2715"
                        color: "#8a8a97"
                        font.pixelSize: 13
                    }

                    MouseArea {
                        id: clearMouse
                        anchors.fill: parent
                        enabled: parent.visible
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: searchField.clear()
                    }
                }
            }
        }

        // 顶部工具栏
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: "#ffffff"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                anchors.topMargin: 8
                anchors.bottomMargin: 8
                spacing: 10

                Text {
                    text: qsTr("播放列表")
                    color: "#23232f"
                    font.pixelSize: 18
                    font.bold: true
                    Layout.fillWidth: true
                }

                Text {
                    text: qsTr("共 %1 首").arg(trackModel.count)
                    color: "#6e6e7b"
                    font.pixelSize: 13
                }

                StandardButton {
                    text: qsTr("导入文件")
                    onClicked: fileDialog.open()
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 30
                    textPixelSize: 13
                }
                StandardButton {
                    text: qsTr("导入文件夹")
                    onClicked: folderDialog.open()
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 30
                    textPixelSize: 13
                }
                StandardButton {
                    text: qsTr("清空")
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 30
                    textPixelSize: 13
                    onClicked: {
                        player.stop()
                        library.clear()
                    }
                }
            }
        }

        // 曲目列表
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: trackModel
            currentIndex: player.currentIndex
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {}

            delegate: Rectangle {
                id: itemRect
                width: listView.width
                height: 44
                color: index === listView.currentIndex ? "#e6eefb"
                       : (itemMouse.containsMouse ? "#efeff5" : "transparent")

                // 歌名 / 歌手：数据库没存歌手时从「歌手 - 歌名」的文件名里拆出来
                readonly property var trackInfo: root.parseTrackName(model.title, model.artist)

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: 10

                    Text {
                        text: index === listView.currentIndex && player.playing ? "▶" : (index + 1)
                        color: index === listView.currentIndex ? "#3d6fd4" : "#8a8a97"
                        font.pixelSize: 13
                        Layout.preferredWidth: 30
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Text {
                        text: itemRect.trackInfo.title !== "" ? itemRect.trackInfo.title : qsTr("(无标题)")
                        color: "#23232f"
                        font.pixelSize: 14
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Text {
                        text: itemRect.trackInfo.artist !== "" ? itemRect.trackInfo.artist : "—"
                        color: "#6e6e7b"
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        Layout.preferredWidth: 160
                    }

                    Text {
                        text: player.formatDuration(model.durationMs)
                        color: "#6e6e7b"
                        font.pixelSize: 13
                        Layout.preferredWidth: 56
                        horizontalAlignment: Text.AlignRight
                    }
                }

                MouseArea {
                    id: itemMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: player.playTrack(index)
                }
            }

            // 空状态提示
            Text {
                anchors.centerIn: parent
                text: searchField.text.length > 0
                      ? qsTr("未找到匹配的歌曲")
                      : qsTr("曲库为空")
                color: "#a0a0ad"
                font.pixelSize: 15
                horizontalAlignment: Text.AlignHCenter
                visible: trackModel.count === 0
            }
        }
    }

    // 搜索防抖：停止输入 150ms 后才通知模型过滤
    Timer {
        id: searchTimer
        interval: 150
        repeat: false
        onTriggered: trackModel.setFilter(searchField.text.trim())
    }

    // 切歌时把当前曲目滚动到可见区域
    Connections {
        target: player
        function onCurrentTrackChanged() {
            if (player.currentIndex >= 0)
                listView.positionViewAtIndex(player.currentIndex, ListView.Contain)
        }
    }

    FileDialog {
        id: fileDialog
        title: qsTr("导入音乐文件")
        fileMode: FileDialog.OpenFiles
        nameFilters: [
            qsTr("音频与加密音乐 (*.mp3 *.wav *.flac *.ogg *.m4a *.mflac *.mgg)"),
            qsTr("普通音频 (*.mp3 *.wav *.flac *.ogg *.m4a)"),
            qsTr("加密音乐 (*.mflac *.mgg)"),
            qsTr("所有文件 (*.*)")
        ]
        onAccepted: library.importFiles(selectedFiles)
    }

    FolderDialog {
        id: folderDialog
        title: qsTr("导入文件夹（递归扫描普通音频与加密音乐）")
        onAccepted: library.importFolder(selectedFolder)
    }

    DecryptDialog {
        id: decryptDialog
    }

    Connections {
        target: library
        function onDecryptingChanged(decrypting) {
            if (decrypting)
                decryptDialog.openRunning()
        }
        function onImportFinished(added, errors) {
            // 只要弹窗已开，或存在错误（含同步失败），都展示汇总，避免错误被吞掉
            if (decryptDialog.opened || (errors && errors.length > 0))
                decryptDialog.showSummary(added, errors)
        }
    }
}
