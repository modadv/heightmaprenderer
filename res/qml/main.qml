import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import BGFXModule 1.0

ApplicationWindow {
    id: window
    visible: true
    width: 1024
    height: 768
    title: qsTr("Qt + BGFX Integration")
    color: "#303030"
    
    BGFXItem {
        id: bgfxItem
        anchors.fill: parent
        anchors.margins: 10
        
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.color: "#505050"
            border.width: 2
            radius: 4
        }
    }
    
    // UI覆盖层
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        
        Label {
            text: "Qt 5.15 + BGFX Rendering"
            color: "white"
            font.pixelSize: 24
            font.bold: true
        }
        
        Item { Layout.fillHeight: true }
        
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 20
            
            Button {
                text: bgfxItem.running ? "Pause" : "Resume"
                onClicked: bgfxItem.running = !bgfxItem.running
            }
            
            Button {
                text: "Fullscreen"
                onClicked: {
                    if (window.visibility === Window.FullScreen) {
                        window.showNormal()
                    } else {
                        window.showFullScreen()
                    }
                }
            }
        }
    }
}