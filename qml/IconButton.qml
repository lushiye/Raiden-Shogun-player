import QtQuick

// 纯图标按钮：用 Image 显示图标，悬停/按下有圆形底色反馈，点击触发 clicked()
Item {
    id: root
    property string source: ""        // 图标路径，如 "qrc:/icons/prev.png"
    property real iconSize: 22        // 图标边长
    property real iconPadding: 8      // 图标四周留白（悬停底色的范围）
    property color hoverColor: "#efeff5"
    property color pressColor: "#e2e2ea"
    signal clicked()

    implicitWidth: iconSize + iconPadding * 2
    implicitHeight: iconSize + iconPadding * 2

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: !root.enabled ? "transparent"
                              : (mouseArea.pressed ? root.pressColor
                                                   : (mouseArea.containsMouse ? root.hoverColor : "transparent"))

        Behavior on color { ColorAnimation { duration: 100 } }
    }

    Image {
        anchors.centerIn: parent
        width: root.iconSize
        height: root.iconSize
        source: root.source
        fillMode: Image.PreserveAspectFit
        opacity: root.enabled ? 1.0 : 0.35   // 禁用时变淡
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: root.enabled                // 禁用时不响应点击、也不显示手型
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
