import QtQuick 2.11
import QtQuick.Controls 2.4
import io.qt.examples.backend 1.0
import QtQuick.Layouts 1.3

GroupBox {

    id: root

    property int position: parseInt(diaPosition.value)
    property bool coninuous: false
    signal send()

    onPositionChanged: diaPosition.value = position

    function updatePosition()
    {
        root.position = parseInt(diaPosition.value)
        lblDialPosition.text = root.position
    }

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            width: parent.width
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Dial {
                id: diaPosition
                stepSize: 1
                to: 2000
                onValueChanged: {
                    root.updatePosition()
                    if(root.coninuous == true)
                    {
                        root.send()
                    }
                }
            }

            Button {
                id: btnSend
                text: "Enviar"
                visible: true
                onClicked: {
                    root.updatePosition()
                    root.send()
                    console.log(root.position)
                }
            }
        }

        Label {
            id: lblDialPosition
            text: "0"
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            font.pointSize: 20
        }
/*
        RowLayout{
            width: parent.width
            spacing: 10
            Label {
                Layout.fillWidth: true
                text: "Objetivo atingido"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            Led {
                height: 30
                width: 120
            }
        }*/
    }
}




/*##^##
Designer {
    D{i:0;formeditorZoom:1.3300000429153442}
}
##^##*/
