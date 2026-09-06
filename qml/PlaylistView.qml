import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 顶部工具栏
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: "#1f1f2b"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                anchors.topMargin: 8
                anchors.bottomMargin: 8
                spacing: 10

                Text {
                    text: qsTr("播放列表")
                    color: "#e8e8f0"
                    font.pixelSize: 18
                    font.bold: true
                    Layout.fillWidth: true
                }

                Text {
                    text: qsTr("共 %1 首").arg(trackModel.count)
                    color: "#9a9ab0"
                    font.pixelSize: 13
                }

                Button {
                    text: qsTr("导入文件")
                    onClicked: fileDialog.open()
                }
                Button {
                    text: qsTr("导入文件夹")
                    onClicked: folderDialog.open()
                }
                Button {
                    text: qsTr("清空")
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
                color: index === listView.currentIndex ? "#33334e"
                       : (itemMouse.containsMouse ? "#242438" : "transparent")

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: 10

                    Text {
                        text: index === listView.currentIndex && player.playing ? "▶" : (index + 1)
                        color: index === listView.currentIndex ? "#7aa2ff" : "#7a7a92"
                        font.pixelSize: 13
                        Layout.preferredWidth: 30
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Text {
                        text: model.title !== "" ? model.title : qsTr("(无标题)")
                        color: "#e8e8f0"
                        font.pixelSize: 14
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Text {
                        text: model.artist !== "" ? model.artist : "—"
                        color: "#9a9ab0"
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        Layout.preferredWidth: 140
                    }

                    Text {
                        text: player.formatDuration(model.durationMs)
                        color: "#9a9ab0"
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
                text: qsTr("曲库为空\n点击“导入文件”或“导入文件夹”添加音乐")
                color: "#6a6a80"
                font.pixelSize: 15
                horizontalAlignment: Text.AlignHCenter
                visible: trackModel.count === 0
            }
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
