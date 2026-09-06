import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root
    title: qsTr("解密音乐")
    modal: true
    width: 480
    height: 360
    anchors.centerIn: Overlay.overlay

    property bool running: false
    property int addedCount: 0
    property var errors: []

    function openRunning() {
        running = true
        addedCount = 0
        errors = []
        open()
    }

    function showSummary(added, errs) {
        running = false
        addedCount = added
        errors = (errs && errs.length) ? errs : []
        open()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 12

        Text {
            text: running ? qsTr("正在解密…") : qsTr("导入完成")
            color: "#ffffff"
            font.pixelSize: 16
            font.bold: true
        }

        // 解密进行中
        ColumnLayout {
            visible: running
            Layout.fillWidth: true
            spacing: 8

            ProgressBar {
                Layout.fillWidth: true
                from: 0
                to: Math.max(1, library.decryptTotal)
                value: library.decryptCurrent
            }

            Text {
                text: qsTr("%1 / %2").arg(library.decryptCurrent).arg(library.decryptTotal)
                color: "#9a9ab0"
                font.pixelSize: 13
            }

            Text {
                text: library.decryptFileName
                color: "#e8e8f0"
                font.pixelSize: 13
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }
        }

        // 完成态
        ColumnLayout {
            visible: !running
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            Text {
                text: qsTr("成功加入 %1 首").arg(addedCount)
                color: "#8be28b"
                font.pixelSize: 14
            }

            Text {
                text: qsTr("失败 %1 个").arg(errors.length)
                color: "#e28b8b"
                font.pixelSize: 13
                visible: errors.length > 0
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: errors.length > 0
                clip: true

                TextArea {
                    text: errors.join("\n")
                    readOnly: true
                    wrapMode: TextEdit.Wrap
                    color: "#d0d0d8"
                    background: Rectangle { color: "#26263a"; radius: 4 }
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            Button {
                text: running ? qsTr("取消") : qsTr("关闭")
                onClicked: {
                    if (running)
                        library.cancelImport()
                    root.close()
                }
            }
        }
    }
}
