import QtQuick 2.15
import QtQuick.Controls 2.15

ApplicationWindow {
    width: 640
    height: 480
    visible: true

    function closing(close) {
        if (stack.depth <= 1)
            return;

        stack.pop();
        close.accepted = false;
    }

    StackView {
        id: stack
        anchors.fill: parent
        anchors.margins: 13

        initialItem: "qrc:/discovery.qml"
    }
}
