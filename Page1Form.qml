import QtQuick 2.11
import QtQuick.Controls 2.4
import io.qt.examples.backend 1.0

Page {
    width: 600
    height: 400

    header: Label {
        text: qsTr("Page 1")
        font.pixelSize: Qt.application.font.pixelSize * 2
        padding: 10
    }

    TextField {
        text: backend.userName
        placeholderText: qsTr("User Name")
        anchors.centerIn: parent

        onTextChanged: backend.userName = text
    }

    BackEnd {
        id: backend
    }
}
