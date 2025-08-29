import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import BgfxModule 1.0

ApplicationWindow {
    id: window
    visible: true
    width: 1280
    height: 720
    title: "BGFX with Qt"
    color: "#2b2b2b"
    
    // 背景填充 - 覆盖整个窗口以隐藏 BGFX 的全屏渲染
    Rectangle {
        anchors.fill: parent
        color: "#2b2b2b"
        z: 0
    }
    
    GridLayout {
        anchors.fill: parent
        columns: 2
        rows: 2
        columnSpacing: AisUi.Style.theme.spacingXSmall
        rowSpacing: AisUi.Style.theme.spacingXSmall

        Rectangle {
            id: topLeftView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.row: 0
            Layout.column: 0
            Layout.columnSpan: 1
            
        }

        Rectangle {
            id: topRightView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.row: 0
            Layout.column: 1
            Layout.columnSpan: 1
            
        }

        Rectangle {
            id: bottomLeftView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.row: 1
            Layout.column: 0
            Layout.columnSpan: 1
            
            
            // 添加加载指示器（可选）
            BusyIndicator {
                anchors.centerIn: parent.centerIn
                running: bgfxItem.status !== "Ready"
                visible: running
            }
        }

        // 用 BgfxItem 替换原来的 bottomRightView
        Rectangle {
            id: bgfxContainer
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.row: 1
            Layout.column: 1
            Layout.columnSpan: 1
            
            // BgfxItem 渲染区域
            BgfxItem {
                id: bgfxItem
                anchors.top: titleBar.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
            }
        }
    }
}