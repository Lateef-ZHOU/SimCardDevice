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

    Label { text: qsTr("虚拟风场"); font.bold: true; Layout.topMargin: 6 }
    FieldRow {
        label: qsTr("风场类型")
        ChoiceField { Layout.fillWidth: true
                      model: root.device.field.kinds
                      value: root.device.field.kind
                      onCommitted: function (i) { root.device.field.kind = i } }
    }
    FieldStrip {
        FieldStrip.Col {
            label: qsTr("风速（m/s）")
            RealField { Layout.fillWidth: true; decimals: 1; minimum: 0; maximum: 100
                        value: root.device.field.speed
                        onCommitted: function (v) { root.device.field.speed = v } }
        }
        FieldStrip.Col {
            label: qsTr("风向（°，来向）")
            RealField { Layout.fillWidth: true; decimals: 0; minimum: 0; maximum: 360
                        value: root.device.field.direction
                        onCommitted: function (v) { root.device.field.direction = v } }
        }
        FieldStrip.Col {
            label: qsTr("垂直速度（m/s）")
            RealField { Layout.fillWidth: true; decimals: 1; minimum: -20; maximum: 20
                        value: root.device.field.vertical
                        onCommitted: function (v) { root.device.field.vertical = v } }
        }
    }
    // 线性剪切才有的三项：地面值在上面那排，这里是每千米的增量与顶高（之上取顶值）。
    FieldStrip {
        visible: root.device.field.kind === 1
        FieldStrip.Col {
            label: qsTr("风速增量（m/s 每 km）")
            RealField { Layout.fillWidth: true; decimals: 1; minimum: -50; maximum: 50
                        value: root.device.field.speedGain
                        onCommitted: function (v) { root.device.field.speedGain = v } }
        }
        FieldStrip.Col {
            label: qsTr("风向增量（° 每 km）")
            RealField { Layout.fillWidth: true; decimals: 0; minimum: -180; maximum: 180
                        value: root.device.field.directionGain
                        onCommitted: function (v) { root.device.field.directionGain = v } }
        }
        FieldStrip.Col {
            label: qsTr("顶高（m）")
            RealField { Layout.fillWidth: true; decimals: 0; minimum: 10; maximum: 20000
                        value: root.device.field.top
                        onCommitted: function (v) { root.device.field.top = v } }
        }
    }

    FieldRow {
        label: qsTr("时域变化")
        ChoiceField { Layout.fillWidth: true
                      model: root.device.field.temporalKinds
                      value: root.device.field.temporal
                      onCommitted: function (i) { root.device.field.temporal = i } }
    }
    // 周期摆动：风速、风向围绕上面的地面值按正弦摆；阵风与日变化只差周期。
    FieldStrip {
        visible: root.device.field.temporal === 1
        FieldStrip.Col {
            label: qsTr("风速幅值（m/s）")
            RealField { Layout.fillWidth: true; decimals: 1; minimum: 0; maximum: 50
                        value: root.device.field.swingSpeed
                        onCommitted: function (v) { root.device.field.swingSpeed = v } }
        }
        FieldStrip.Col {
            label: qsTr("风向幅值（°）")
            RealField { Layout.fillWidth: true; decimals: 0; minimum: 0; maximum: 180
                        value: root.device.field.swingDirection
                        onCommitted: function (v) { root.device.field.swingDirection = v } }
        }
        FieldStrip.Col {
            label: qsTr("周期（s）")
            RealField { Layout.fillWidth: true; decimals: 0; minimum: 1; maximum: 86400
                        value: root.device.field.period
                        onCommitted: function (v) { root.device.field.period = v } }
        }
    }
    // 线性过渡：从改动那一刻起，在过渡时长内线性变到「地面值 + 变化量」然后保持（锋面过境）。
    FieldStrip {
        visible: root.device.field.temporal === 2
        FieldStrip.Col {
            label: qsTr("风速变化量（m/s）")
            RealField { Layout.fillWidth: true; decimals: 1; minimum: -50; maximum: 50
                        value: root.device.field.rampSpeed
                        onCommitted: function (v) { root.device.field.rampSpeed = v } }
        }
        FieldStrip.Col {
            label: qsTr("风向变化量（°）")
            RealField { Layout.fillWidth: true; decimals: 0; minimum: -180; maximum: 180
                        value: root.device.field.rampDirection
                        onCommitted: function (v) { root.device.field.rampDirection = v } }
        }
        FieldStrip.Col {
            label: qsTr("过渡时长（s）")
            RealField { Layout.fillWidth: true; decimals: 0; minimum: 1; maximum: 86400
                        value: root.device.field.rampDuration
                        onCommitted: function (v) { root.device.field.rampDuration = v } }
        }
    }
    // 随机起伏：谐波叠加，相位由「设备控制」分区的随机种子定，同一种子同一时刻同一个风。
    FieldStrip {
        visible: root.device.field.temporal === 3
        FieldStrip.Col {
            label: qsTr("风速起伏（σ，m/s）")
            RealField { Layout.fillWidth: true; decimals: 1; minimum: 0; maximum: 20
                        value: root.device.field.gustSpeed
                        onCommitted: function (v) { root.device.field.gustSpeed = v } }
        }
        FieldStrip.Col {
            label: qsTr("风向起伏（σ，°）")
            RealField { Layout.fillWidth: true; decimals: 0; minimum: 0; maximum: 90
                        value: root.device.field.gustDirection
                        onCommitted: function (v) { root.device.field.gustDirection = v } }
        }
        FieldStrip.Col {
            label: qsTr("时间尺度（s）")
            RealField { Layout.fillWidth: true; decimals: 0; minimum: 1; maximum: 3600
                        value: root.device.field.gustScale
                        onCommitted: function (v) { root.device.field.gustScale = v } }
        }
    }
    Label {
        text: qsTr("水平均匀、只随高度变，时域变化整列同步。定常时各束看到同一个风、反演误差只来自噪声；时域变化时各束采于不同时刻，差异进入反演。")
        color: palette.placeholderText
        font.pointSize: 9
        wrapMode: Text.Wrap
        Layout.preferredWidth: 1
        Layout.fillWidth: true
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 4
        spacing: 8
        Button { Layout.fillWidth: true; text: qsTr("连接"); onClicked: root.device.open() }
        Button { Layout.fillWidth: true; text: qsTr("断开"); onClicked: root.device.close() }
    }
}
