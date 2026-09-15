import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root
    title: qsTr("解密音乐")
    modal: true
    width: 480
    height: 360
    // 解密过程中不允许点外部/Esc 关闭，避免误操作打断
    closePolicy: running ? Popup.NoAutoClose : Popup.CloseOnEscape
    // 位置交给 Popup 的默认居中逻辑，不再用 anchors.centerIn: Overlay.overlay

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
            color: "#23232f"
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
                color: "#6e6e7b"
                font.pixelSize: 13
            }

            Text {
                text: library.decryptFileName
                color: "#23232f"
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
                color: "#2f8f5f"
                font.pixelSize: 14
            }

            Text {
                text: qsTr("失败 %1 个").arg(errors.length)
                color: "#c8504a"
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
                    color: "#6e6e7b"
                    background: Rectangle { color: "#fbecec"; radius: 4 }
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight

            // 用应用自绘按钮，避免原生按钮在点击后残留焦点虚线框
            StandardButton {
                text: running ? qsTr("取消") : qsTr("关闭")
                Layout.preferredWidth: 88
                Layout.preferredHeight: 30
                textPixelSize: 13
                onClicked: {
                    if (running)
                        library.cancelImport()
                    root.close()
                }
            }
        }
    }
}
