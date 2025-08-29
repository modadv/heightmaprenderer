import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import BgfxModule 1.0

ApplicationWindow {
    id: mainWindow
    
    title: "BGFX Heightmap Terrain"
    width: 1280
    height: 720
    visible: true
    
    // 主内容区域
    Item {
        anchors.fill: parent
        anchors.margins: 5
        
        GridLayout {
            anchors.fill: parent
            columns: 2
            rows: 2
            columnSpacing: 5
            rowSpacing: 5

            // 左上角区域
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#404040"
                border.color: "#606060"
                border.width: 1
                
                Text {
                    anchors.centerIn: parent
                    text: "Left Top\n(Business Area 1)"
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            // 右上角区域
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#404040"
                border.color: "#606060"
                border.width: 1
                
                Text {
                    anchors.centerIn: parent
                    text: "Right Top\n(Business Area 2)"
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            // 左下角区域
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#404040"
                border.color: "#606060"
                border.width: 1
                
                Text {
                    anchors.centerIn: parent
                    text: "Left Bottom\n(Business Area 3)"
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            // 右下角 - BGFX渲染区域
            Item {
                id: bgfxContainer
                Layout.fillWidth: true
                Layout.fillHeight: true
                
                // 添加边框以可视化容器
                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    border.color: "red"
                    border.width: 2
                    z: 10  // 确保边框在最上层
                }
                
                // 添加标签
                Text {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.margins: 5
                    text: "BGFX Area: " + Math.round(parent.width) + "x" + Math.round(parent.height)
                    color: "yellow"
                    z: 11
                    font.pixelSize: 12
                    font.bold: true
                }

                BgfxItem {
                    id: bgfxItem
                    objectName: "bgfxRenderer" // <--- 添加这一行
                    anchors.fill: parent
                    
                    onWidthChanged: {
                        console.log("BgfxItem width changed to:", width)
                    }
                    onHeightChanged: {
                        console.log("BgfxItem height changed to:", height)
                    }
                }
            }
        }
    }
    
    // 窗口尺寸信息显示
    Text {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 10
        color: "white"
        text: "Window: " + Math.round(mainWindow.width) + "x" + Math.round(mainWindow.height)
        font.pixelSize: 14
        z: 100
    }
    
    Component.onCompleted: {
        console.log("=== QML Debug Info ===")
        console.log("Window size:", width + "x" + height)
        console.log("BGFX renderer should be in bottom-right quadrant")
    }
}