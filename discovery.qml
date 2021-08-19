import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

import zkun 1.0

Item {
    Bridge {
        id: bridge
    }

    ColumnLayout {
        anchors.fill: parent

        GroupBox {
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent

                Label {
                    text: qsTr("网络适配器:")
                }

                ComboBox {
                    textRole: "name"
                    valueRole: "value"

                    model: bridge.allInterfaces()
                    onCurrentValueChanged: bridge.iface = currentValue
                }

                Label {
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr("搜索")
                    onClicked: bridge.discovery()
                }
            }
        }
        ScrollView {
            clip: true
            Layout.fillWidth: true
            Layout.fillHeight: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ListView {
                id: view
                model: bridge
                delegate: ItemDelegate {
                    width: view.width
                    text: model.display
                    onClicked: console.log(text)
                }
            }
        }
    }
}
