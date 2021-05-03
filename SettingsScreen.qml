import QtQuick 2.10
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0
import QtQuick.VirtualKeyboard 2.3

Item {
    id: root
    signal requestUpdate
    signal sendValue

    property var comboBoxSelections: []
    property var newValues: []

    function updateValues(messages)
    {
        let i = 0;
        for(i = 0; i < comboBoxes.count; i++)
        {
            currentValues.itemAt(i).text = messages[i] + " " + comboBoxes.itemAt(i).delegateModel.items.get(comboBoxes.itemAt(i).currentIndex).model.unit
        }
    }

    IntValidator {
        id: myValidator
    }

    GridLayout {
        anchors.fill: parent
        columns: 3
        Label {
            Layout.alignment: "AlignHCenter"

            text: "Informação"
            horizontalAlignment: "AlignHCenter"
            Layout.fillWidth: true
        }

        Label {
            Layout.alignment: "AlignHCenter"
            text: "Valor atual"
            horizontalAlignment: "AlignHCenter"
            Layout.fillWidth: true
           Layout.preferredWidth: 150
        }

        Label {
            Layout.alignment: "AlignHCenter"
            text: "Novo valor"
            horizontalAlignment: "AlignHCenter"
            Layout.fillWidth: true
        }

        Repeater {
            model: [0, 1, 2, 3]
            id: comboBoxes
            ComboBox {

                Layout.row: index+1
                Layout.column: 0
                model: template
                textRole: 'name'
                Layout.alignment: "AlignHCenter"
                Layout.fillWidth: true
                currentIndex: modelData
            }
        }
        Repeater {
            model: 4
            id: currentValues
            Label {
                Layout.row: index+1
                Layout.column: 1
                //id: currentValue0
                Layout.alignment: "AlignHCenter"
                text: "-"
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
                //Layout.preferredWidth: 150
            }
        }
        Repeater {
            model: 4
            id: txtNewValues
            TextField {
                horizontalAlignment: Text.AlignHCenter
                Layout.row: index+1
                Layout.column: 2
                //id: newValue0
                verticalAlignment: Text.AlignVCenter
                Layout.alignment: "AlignHCenter"
                inputMethodHints: Qt.ImhDigitsOnly
                Layout.preferredWidth: 100
                Layout.fillWidth: true
            }
        }

        Button {
            Layout.row: 6
            Layout.column: 1
            text:'Atualizar'
            Layout.alignment: "AlignHCenter"
            onClicked: {
                comboBoxSelections = []
                let i = 0
                for(i = 0; i < comboBoxes.count; i++)
                {
                    comboBoxSelections.push(comboBoxes.itemAt(i).delegateModel.items.get(comboBoxes.itemAt(i).currentIndex).model)
                }

                for(i = 0; i < comboBoxes.count; i++)
                {
                    console.log('oia o nome', comboBoxSelections[i].name)
                }
                root.requestUpdate()
            }
        }

        Button {
            text:'Enviar'
            Layout.alignment: "AlignHCenter"
            onClicked: {
                let i = 0
                newValues = []
                comboBoxSelections = []
                for(i = 0; i < comboBoxes.count; i++)
                {
                    if(txtNewValues.itemAt(i).text !== "" && !isNaN(txtNewValues.itemAt(i).text))
                    {
                        newValues.push(parseInt(txtNewValues.itemAt(i).text))
                    }
                    else
                    {
                        newValues.push(null)
                    }

                    comboBoxSelections.push(comboBoxes.itemAt(i).delegateModel.items.get(comboBoxes.itemAt(i).currentIndex).model)
                }
                root.sendValue()
            }
        }
    }

    ListModel {
        id:template
        ListElement {
            name: "MaxFollowingError"
            indexSubIndex: 0x606500
            dataLength: 32
            unit: "qc"
        }
        ListElement {
            name: "MinPositionLimit"
            indexSubIndex: 0x607D01
            dataLength: 32
            unit: "qc"
        }
        ListElement {
            name: "MaxPositionLimit"
            indexSubIndex: 0x607D02
            dataLength: 32
            unit: "qc"
        }
        ListElement {
            name: "MaxProfileVelocity"
            indexSubIndex: 0x607F00
            dataLength: 32
            unit: "rpm"
        }
        ListElement {
            name: "ProfileVelocity"
            indexSubIndex: 0x608100
            dataLength: 32
            unit: "rpm"
        }
        ListElement {
            name: "ProfileAcceleration"
            indexSubIndex: 0x608300
            dataLength: 32
            unit: "rpm/s"
        }
        ListElement {
            name: "ProfileDeceleration"
            indexSubIndex: 0x608400
            dataLength: 32
            unit: "rpm/s"
        }
        ListElement {
            name: "OperationMode"
            indexSubIndex: 0x606000
            dataLength: 8
            unit: ""
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
##^##*/
