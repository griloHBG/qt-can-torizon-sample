import QtQuick 2.11
import QtQuick.Controls 2.4
import io.qt.examples.backend 1.0
import QtQuick.Layouts 1.3
import QtQuick.VirtualKeyboard 2.3
import QtQuick.VirtualKeyboard.Settings 2.2

ApplicationWindow {
    id: window
    width: 800
    height: 480
    visible: true
    title: qsTr("CANopen")
    //visibility: "FullScreen"
    //Component.onCompleted: VirtualKeyboardSettings.styleName = "retro"

    property int initTime: 0
    property int statusChangedCount: 0
    property int operationMode: 1

    /*Timer {
        id: timer
        function setTimeout(cb, delayTime) {
            timer.interval = delayTime;
            timer.repeat = true;
            timer.triggered.connect(cb);
            //timer.triggered.connect(function release () {
            //    timer.triggered.disconnect(cb); // This is important
            //    timer.triggered.disconnect(release); // This is important as well
            //});
            timer.start();
        }
    }

    function calculateStatusChangedFPS()
    {
        statusChangeFPS.text =  window.statusChangedCount.toString() + "FPS"
        window.statusChangedCount = 0
    }*/

    BackEnd {
        readonly property int movementModeProfilePosition:  0x01
        readonly property int movementModeHoming:           0x06
        readonly property int movementModeProfileVelocity:  0x03
        readonly property int movementModePosition:         0xFF
        readonly property int movementModeVelocity:         0xFE
        readonly property int movementModeCurrent:          0xFD

        id: backend
        onConnectedChanged:
        {
            console.log("onConnectedChanged", connected)
            if(connected == true)
            {
                led.color = "#00AA55"
                console.log(led.color, "connected (",connected,") is true")
                btnTryConnection.enabled = false
                btnStop.enabled = true
                lblConnectionStatus.text = "CONECTADO"
                lblConnectionStatus.color = "#00ff55"
                btnControllerOn.enabled = true
            }
            else
            {
                led.color = "#AAAAAA"
                console.log(led.color)
                console.log(led.color, "connected (",connected,") is false")
                btnTryConnection.enabled = true
                btnStop.enabled = false
                lblConnectionStatus.text = "DESCONECTADO"
                lblConnectionStatus.color = "#000000"
                btnControllerOn.enabled = false
            }
        }
        onMotorPositionChanged:
        {
            lblPosition.text = motorPosition
        }
        onMotorVelocityChanged:
        {
            lblVelocity.text = motorVelocity
        }
        onStatusChanged:
        {
            let i = 0
            for(i = 0; i < leds.count; i++)
            {
                leds.itemAt(i).children[0].state = status[i]
            }
        }

        onMessagesChanged:
        {
            settingsScreen.updateValues(messages)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            id: tabBar
            Layout.fillWidth: true

            /*TabButton {
                text: "Posição"
                Layout.fillWidth: true
                onClicked: {
                    backend.setPreOperational()
                    backend.stop()
                    backend.setMovementMode(backend.movementModePosition)
                }
            }*/
            TabButton {
                text: "Perfil de Posição"
                Layout.fillWidth: true
                onClicked: {
                    backend.setPreOperational()
                    backend.stop()
                    window.operationMode = backend.movementModeProfilePosition
                }
            }
            TabButton {
                text: "Velocidade"
                Layout.fillWidth: true
                onClicked: {
                    backend.setPreOperational()
                    backend.stop()
                    window.operationMode = backend.movementModeProfileVelocity
                }
            }
            TabButton {
                text: "Configurações"
                Layout.fillWidth: true
                onClicked: {
                    backend.stop()
                    backend.setPreOperational()
                }
            }
            /*TabButton {
                text: "Perfil de Velocidade"
                Layout.fillWidth: true
            }
            TabButton {
                text: "Homing"
                Layout.fillWidth: true
            }*/
        }

        RowLayout {
            id: item1
            Layout.leftMargin: 5
            Layout.bottomMargin: 5
            Layout.rightMargin: 5
            Layout.topMargin: 0
            rotation: 0
            spacing: 3

            StackLayout {
                id: screenStack
                currentIndex: tabBar.currentIndex;
                width:700

                /*PositionModeScreen {
                    id: positionModeScreen
                    onSend: {
                        if (backend.connected)
                        {
                            backend.sendPosition(position)
                        }
                    }
                }*/

                ProfilePositionModeScreen {
                    id: profilePositionModeScreen
                    onSend: {
                        if (backend.connected)
                        {
                            backend.sendProfilePosition(position)
                        }
                    }
                }

                ProfileVelocityModeScreen {
                    id: profileVelocityModeScreen
                    onSend: {
                        console.log("ProfileVelocityModeScreen onSend")
                        if (backend.connected)
                        {
                            console.log("ProfileVelocityModeScreen onSend backend is connected")
                            backend.sendProfileVelocity(velocity)
                        }
                    }
                }

                SettingsScreen {
                    id:settingsScreen
                    function selectToJson(select)
                    {
                        return {'name':select.name,
                                'indexSubIndex':select.indexSubIndex,
                                'dataLength':select.dataLength,
                                'unit': select.unit}
                    }

                    onRequestUpdate: {
                        console.log("enviando requisicao pro backend")
                        backend.registerValuesForUpdate(comboBoxSelections[0],
                                                        comboBoxSelections[1],
                                                        comboBoxSelections[2],
                                                        comboBoxSelections[3])
                    }
                    onSendValue: {
                        backend.sendMessages(newValues, comboBoxSelections)
                    }
                }
            }

            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 200
                //color:'red'
                ColumnLayout {
                    Layout.preferredWidth: 100
                    //anchors.centerIn: parent
                    ColumnLayout {
                        //anchors.fill: parent

                        GroupBox {
                            Layout.fillWidth: true
                            Layout.columnSpan: 2
                            ColumnLayout {
                                anchors.fill: parent

                                Item {
                                    height: 20
                                    Layout.fillWidth: true
                                    Rectangle {
                                        id: led
                                        color: "#aaaaaa"
                                        anchors.fill: parent
                                        radius: 10
                                        Label {
                                            id: lblConnectionStatus
                                            text:"Desconectado"
                                            anchors.fill: parent
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                    }
                                }

                                Button {
                                    id: btnTryConnection
                                    text: "Tentar conectar"
                                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                                    onClicked: {
                                        backend.initCANConnection()
                                    }
                                }
                            }
                        }
                        GridLayout {

                            columns: 2

                            Label {
                                Layout.fillWidth: true
                                text: "Posição"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            Label {
                                Layout.preferredWidth: 70
                                id: lblPosition
                                text: "0000"
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Label {
                                Layout.fillWidth: true
                                text: "Velocidade"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            Label {
                                Layout.preferredWidth: 70
                                id: lblVelocity
                                text: "0000"
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                            }

                        }
                        ColumnLayout {
                            Button {
                                id: btnControllerOn
                                Layout.columnSpan: 2
                                text: "Ligar\nControlador"
                                Layout.fillWidth: true
                                enabled:false
                                font.pointSize: 10
                                onClicked:
                                {
                                    backend.initEPOSController(window.operationMode)
                            }
                        }

                        Button {
                            text:"Resetar\nAplicação"
                            Layout.fillWidth: true
                            font.pointSize: 10
                            onClicked:
                            {
                                backend.resetApp()
                                backend.setPreOperational()
                            }
                        }
                        Button {
                            text:"Resetar\nComunicação"
                            Layout.fillWidth: true
                            font.pointSize: 10
                            onClicked:
                            {
                                backend.resetComm()
                                backend.setPreOperational()
                            }
                        }
                        Button {
                            id:btnStop
                            text: "Parar"
                            Layout.columnSpan: 2
                            Layout.fillWidth: true
                            font.pointSize: 10
                            onClicked:
                            {
                                backend.setPreOperational()
                                backend.stop()
                            }
                        }
                    }
                    }
                }
            }
        }
        RowLayout {
            id: ledStrip
            Layout.fillWidth: true
            width:parent.width
            visible: false
            Repeater {
                id: leds
                model: [[15, false],
                        [14, false],
                        [13,  true],
                        [12, false],
                        [11, false],
                        [10, false],
                        [ 9, false],
                        [ 8, false],
                        [ 7, false],
                        [ 6, false],
                        [ 5, false],
                        [ 4, false],
                        [ 3, false],
                        [ 2, false],
                        [ 1, false],
                        [ 0, false]]
                Column {
                    Layout.fillWidth: true
                    Led {
                        width: 15
                        height: 15
                        anchors.horizontalCenter: parent.horizontalCenter
                        Component.onCompleted: modelData[1] ? setOn() : setOff
                    }
                    Label {
                        text: modelData[0]
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
        }

    }
    Binding {
        target: VirtualKeyboardSettings
        property: "fullScreenMode"
        value: true//(window.width / window.height) > (16.0 / 9.0)
    }

    InputPanel {
        id: inputPanel
        y: Qt.inputMethod.visible ? parent.height - inputPanel.height : window.height
        anchors.left: parent.left
        anchors.right: parent.right
    }
/*    InputPanel {
        id: inputPanel
        z: 99
        x: 0//window.width/2
        y: window.height
        width: window.width//2

        states: State {
            name: "visible"
            when: inputPanel.active
            PropertyChanges {
                target: inputPanel
                y: window.height - inputPanel.height
            }
        }
        transitions: Transition {
            from: ""
            to: "visible"
            reversible: true
            ParallelAnimation {
                NumberAnimation {
                    properties: "y"
                    duration: 250
                    easing.type: Easing.InOutQuad
                }
            }
        }
    }*/
}
