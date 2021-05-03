import QtQuick 2.0

Rectangle {
    property string colorOn: "#00FF00"
    property string colorOff: "#AAAAAA"
    property bool state: false

    function setOn() {
        state = true
    }

    function setOff() {
        state = false
    }

    function set(value) {
        if (value === true)
        {
            setOn()
        }
        else
        {
            setOff()
        }
    }

    radius: 20
    color: colorOff
    onStateChanged: {
        if (state === true)
        {
            color = colorOn
        }
        else
        {
            color = colorOff
        }
    }
}
