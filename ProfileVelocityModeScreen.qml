import QtQuick 2.11
import QtQuick.Controls 2.4
import io.qt.examples.backend 1.0
import QtQuick.Layouts 1.3

GroupBox {

    id: root

    property int velocity: parseInt(sldVelocity.value)
    property bool coninuous: false
    signal send()

    onVelocityChanged: sldVelocity.value = velocity

    function updateVelocity()
    {
        root.velocity = parseInt(sldVelocity.value)
        lblSliderVelocity.text = root.velocity
    }

    ColumnLayout {
        anchors.fill: parent

        RowLayout{
            width: parent.width
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Label {
                text: "Demanda"
            }
            Switch {
                id: contDemand
                onPositionChanged: {
                    root.coninuous = (position == 1)
                    console.log("continuous", root.coninuous)
                    btnSend.visible = !root.continuous
                }
            }
            Label {
                text: "Contínuo"
            }
        }

        RowLayout {
            width: parent.width
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            spacing: 30
            ColumnLayout {
                spacing: 2
                Button {
                    text:"+100"
                    font.pointSize: 10
                    onClicked: {
                        sldVelocity.value+=100
                    }
                }

                Button {
                    text:"+10"
                    font.pointSize: 10
                    onClicked: {
                        sldVelocity.value+=10
                    }
                }

                Button {
                    text:"+1"
                    font.pointSize: 10
                    onClicked: {
                        sldVelocity.value+=1
                    }
                }

                Button {
                    text:"-1"
                    font.pointSize: 10
                    onClicked: {
                        sldVelocity.value-=1
                    }
                }

                Button {
                    text:"-10"
                    font.pointSize: 10
                    onClicked: {
                        sldVelocity.value-=10
                    }
                }

                Button {
                    text:"-100"
                    font.pointSize: 10
                    onClicked: {
                        sldVelocity.value-=100
                    }
                }
            }

            Slider {
                id: sldVelocity
                spacing: 0
                orientation: Qt.Vertical
                stepSize: 1
                to: 2000
                from: -2000
                onValueChanged: {
                    root.updateVelocity()
                    if(root.coninuous == true)
                    {
                        console.log("velocidade desejada: ", velocity)
                        root.send()
                    }
                }
            }

            Button {
                id: btnSend
                text: "Enviar"
                visible: true
                onClicked: {
                    root.updateVelocity()
                    root.send()
                    console.log(root.velocity)
                }
            }
        }

        Label {
            id: lblSliderVelocity
            text: "0"
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            font.pointSize: 20
        }
    }
}




/*##^##
Designer {
    D{i:0;formeditorZoom:1.3300000429153442}
}
##^##*/
