import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import BGFX 1.0

ApplicationWindow {
    id: window
    visible: true
    width: 1280
    height: 720
    title: qsTr("BGFX Heightmap Renderer - Qt QML")
    color: "#202020"
    
    // Main render area
    BgfxRenderItem {
        id: bgfxRenderer
        anchors.fill: parent
        focus: true
        
        Component.onCompleted: {
            // Initialize BGFX
            Qt.callLater(initializeBgfx)
        }
        
        onInitializedChanged: {
            if (initialized) {
                statusText.text = "BGFX Initialized Successfully"
                statusText.color = "#00ff00"
            }
        }
        
        // Keyboard handling
        Keys.onPressed: {
            switch(event.key) {
                case Qt.Key_W:
                    console.log("W pressed - Move forward")
                    break
                case Qt.Key_S:
                    console.log("S pressed - Move backward")
                    break
                case Qt.Key_A:
                    console.log("A pressed - Move left")
                    break
                case Qt.Key_D:
                    console.log("D pressed - Move right")
                    break
                case Qt.Key_Escape:
                    Qt.quit()
                    break
            }
        }
        
        // Mouse handling
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
            hoverEnabled: true
            
            property point lastPos
            
            onPressed: {
                lastPos = Qt.point(mouse.x, mouse.y)
                parent.forceActiveFocus()
                console.log("Mouse pressed at:", mouse.x, mouse.y)
            }
            
            onPositionChanged: {
                if (pressed) {
                    var dx = mouse.x - lastPos.x
                    var dy = mouse.y - lastPos.y
                    console.log("Mouse dragged:", dx, dy)
                    lastPos = Qt.point(mouse.x, mouse.y)
                }
            }
            
            onWheel: {
                console.log("Mouse wheel delta:", wheel.angleDelta.y)
            }
        }
    }
    
    // Status overlay
    Item {
        anchors.fill: parent
        
        // Status info
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 10
            width: 300
            height: 80
            color: "#80000000"
            radius: 5
            
            Column {
                anchors.centerIn: parent
                spacing: 5
                
                Text {
                    id: statusText
                    text: "Initializing BGFX..."
                    color: "yellow"
                    font.pixelSize: 14
                    font.family: "Consolas, Monaco, monospace"
                }
                
                Text {
                    text: "Resolution: " + window.width + "x" + window.height
                    color: "white"
                    font.pixelSize: 12
                    font.family: "Consolas, Monaco, monospace"
                }
                
                Text {
                    id: fpsText
                    text: "FPS: Calculating..."
                    color: "white"
                    font.pixelSize: 12
                    font.family: "Consolas, Monaco, monospace"
                }
            }
        }
        
        // Control panel
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 80
            color: "#CC202020"
            
            RowLayout {
                anchors.centerIn: parent
                spacing: 15
                
                Button {
                    text: "Reset Camera"
                    onClicked: {
                        console.log("Reset camera clicked")
                    }
                }
                
                Button {
                    text: "Wireframe"
                    checkable: true
                    onCheckedChanged: {
                        console.log("Wireframe toggled:", checked)
                    }
                }
                
                Button {
                    text: "Freeze"
                    checkable: true
                    onCheckedChanged: {
                        console.log("Freeze toggled:", checked)
                    }
                }
                
                Slider {
                    id: lodSlider
                    from: 0
                    to: 3
                    value: 1
                    stepSize: 1
                    snapMode: Slider.SnapAlways
                    
                    onValueChanged: {
                        console.log("LOD Level changed:", value)
                    }
                }
                
                Text {
                    text: "LOD: " + lodSlider.value
                    color: "white"
                }
            }
        }
    }
    
    // FPS calculator
    Timer {
        id: fpsTimer
        interval: 1000
        running: bgfxRenderer.initialized
        repeat: true
        property int frameCount: 0
        
        onTriggered: {
            fpsText.text = "FPS: " + frameCount
            frameCount = 0
        }
    }
    
    Connections {
        target: bgfxRenderer
        function onFrameRendered() {
            fpsTimer.frameCount++
        }
    }
}