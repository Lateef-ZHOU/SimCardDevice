// SimCardField —— 虚拟采集卡的「架设与风场」分区：架在哪台云台上 + 风场参数 + 连接 / 断开。连接 = 架到选中的云台上起采样器
// （2026-10-15 用户定：主线上不出生即起——不一定用，默认起白占资源；风场不是可选的外部源，是卡自己的场景参数，没有"选风场源"）。
//
// 连着换云台即刻重架（设备被占用时被拒，下拉框弹回），没连时下次连接生效；风场住 device.field（WindFieldSource，
// C++ 持值），这里只是它的壳，改了下一拍进场景——之后的帧按新风场合成、「风场真值」图跟着变，任务跑着也能改
// （场景不是设备设置）。风场分两根轴：竖直结构（匀强 / 线性剪切）与时域变化（定常 / 周期摆动 / 线性过渡 / 随机起伏），
// 时域作用在地面值上、整列同步；改任一项都从此刻重新起算。
import QtQuick
import Lidar.UI
import QtQuick.Controls
import QtQuick.Layouts

Section {
    id: root
    required property var device // 分区模板的创建参数传入本台设备
    title: qsTr("架设与风场")

    Label { text: qsTr("架在哪台云台上"); font.bold: true }
    FieldRow {
        label: qsTr("云台")
        ChoiceField { Layout.fillWidth: true
                      model: root.device.mountOptions.map(function (device) {
                          return device.name + " · " + device.subtitle
                      })
                      value: root.device.mount
                      onCommitted: function (i) { root.device.mount = i } }
    }
    Label {
        text: qsTr("采样时实时取这台云台此刻的指向合成，云台没有指向就采不了；连着换云台即刻生效（任务或单卡测试占用时不能换），没连时下次连接生效。单卡测试的角度软件不记录，与真卡一样。")
        color: palette.placeholderText
        font.pointSize: 9
        wrapMode: Text.Wrap
        Layout.preferredWidth: 1
        Layout.fillWidth: true
    }

    SimWindFields { device: root.device; Layout.fillWidth: true }
    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 4
        spacing: 8
        Button { Layout.fillWidth: true; text: qsTr("连接"); onClicked: root.device.open() }
        Button { Layout.fillWidth: true; text: qsTr("断开"); onClicked: root.device.close() }
    }
}
