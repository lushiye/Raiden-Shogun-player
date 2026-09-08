import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Imagine

Button {
    id: root
    clip: true

    property double radius: 12
    property int textPixelSize: 15

    background: Rectangle {
        border.width: 0
        implicitWidth: parent.width
        implicitHeight: parent.height
        radius: 5
        color: "#4F4F64"

        Rectangle {
            border.width: 1
            border.color: "#030303"
            anchors.fill: parent
            radius: 5
            color: {
                if (root.hovered) return "#80808080"
                else return "#00000000"
            }

            Behavior on color {
                ColorAnimation {
                    duration: 120
                }
            }
        }

        Behavior on color {
            ColorAnimation {
                duration: 120
            }
        }
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    contentItem: Text {
        text: root.text
        color: "#B2DFDB"
        font.pixelSize: root.textPixelSize
        font.letterSpacing: 1
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
