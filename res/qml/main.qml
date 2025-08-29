import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import BgfxModule 1.0

ApplicationWindow {
    id: mainWindow
    
    // 窗口属性
    title: "BGFX Heightmap Terrain"
    width: 1280
    height: 720
    visible: true
    
    // 窗口图标（如果有的话）
    // icon: "qrc:/icons/app_icon.png"
    
    GridLayout {
        anchors.fill: parent
        columns: 4
        rows: 2
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.row: 0
            Layout.column: 0
            Layout.columnSpan: 1
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.row: 0
            Layout.column: 1
            Layout.columnSpan: 1
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.row: 1
            Layout.column: 0
            Layout.columnSpan: 1
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.row: 1
            Layout.column: 1
            Layout.columnSpan: 1

            id: contentArea
            color: "#2b2b2b"
            
            // BGFX 渲染区域
            BgfxItem {
                id: bgfxRenderer
                objectName: "bgfxRenderer"  // 这个objectName很重要，C++代码会通过它查找
                
                anchors {
                    left: parent.left
                    top: parent.top
                    bottom: parent.bottom
                    rightMargin: 5
                }
                
                renderingEnabled: true
                focus: true  // 确保能接收键盘事件
                
                // 鼠标区域，确保能正确处理鼠标事件
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.AllButtons
                    hoverEnabled: true
                    
                    // 将鼠标事件传递给父项（BgfxItem）
                    onPressed: mouse.accepted = false
                    onReleased: mouse.accepted = false
                    onPositionChanged: mouse.accepted = false
                    onWheel: wheel.accepted = false
                }
                
                // 显示帧率等调试信息的文本（可选）
                Text {
                    anchors {
                        top: parent.top
                        left: parent.left
                        margins: 10
                    }
                    color: "white"
                    text: "BGFX Terrain Renderer"
                    font.pixelSize: 14
                    visible: showDebugInfo.checked
                }
            }
            
        }
    }
}