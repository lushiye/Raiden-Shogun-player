import QtQuick
import QtQuick.Controls
// 必须基于非原生样式：Windows/macOS 原生样式不支持定制（自绘 background/contentItem
// 会报警告，内边距也被样式写成 5），Imagine 是纯图片样式，定制完全生效。
import QtQuick.Controls.Imagine

// 应用统一按钮：背景与文字全部自绘，不使用任何控件样式的图形，
// 因此点击后不会出现系统样式残留的虚线焦点框。
Button {
    id: root

    property real radius: 6
    property int textPixelSize: 15

    implicitWidth: Math.max(72, contentItem.implicitWidth + 28)
    implicitHeight: 32

    // 背景整块自绘，不需要文字内边距（否则 80px 宽的按钮文字会被挤掉）
    padding: 0

    background: Rectangle {
        radius: root.radius
        color: "#e9e9f0"

        Rectangle {
            anchors.fill: parent
            radius: root.radius
            border.width: 1
            border.color: "#c9c9d4"
            color: root.pressed ? "#1f000000" : (root.hovered ? "#14000000" : "transparent")

            Behavior on color { ColorAnimation { duration: 120 } }
        }
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    contentItem: Text {
        text: root.text
        color: "#2b2b37"
        font.pixelSize: root.textPixelSize
        font.letterSpacing: 1
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
