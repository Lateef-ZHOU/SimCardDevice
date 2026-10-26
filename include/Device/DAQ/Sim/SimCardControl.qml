// SimCardControl —— 虚拟采集卡的「设备控制」分区：探测器（含随机种子）/ 环境，再加族级的累积、落盘与起停。
//
// 场景那几组（探测器、环境、种子）改了即生效、不验锁（见 SimCardDevice 类注）；累积时间、落盘与起停与真卡
// 相同。上下限是界面的校验器，C++ 写口不再判。指向不在这里：卡架在哪台云台上是「架设与风场」的事。
import QtQuick
import Lidar.TaskUI
import Lidar.UI
import QtQuick.Controls
import QtQuick.Layouts

DAQControl {
    id: root

    Label { text: qsTr("探测器（%1 门 × %2 频点，门距 %3 m）").arg(root.device.rows).arg(root.device.cols).arg(root.device.gateSpacing)
            font.bold: true }
    FieldStrip {
        FieldStrip.Col {
            label: qsTr("1 km 处 CNR（dB）")
            RealField { Layout.fillWidth: true; decimals: 1; minimum: -30; maximum: 60
                        value: root.device.cnrAt1km
                        onCommitted: function (v) { root.device.cnrAt1km = v } }
        }
        FieldStrip.Col {
            label: qsTr("消光（dB/km）")
            RealField { Layout.fillWidth: true; decimals: 1; minimum: 0; maximum: 50
                        value: root.device.extinction
                        onCommitted: function (v) { root.device.extinction = v } }
        }
        FieldStrip.Col {
            label: qsTr("重叠距离（m）")
            RealField { Layout.fillWidth: true; decimals: 0; minimum: 1; maximum: 5000
                        value: root.device.overlapRange
                        onCommitted: function (v) { root.device.overlapRange = v } }
        }
    }
    FieldStrip {
        FieldStrip.Col {
            label: qsTr("单帧噪声底（计数）")
            RealField { Layout.fillWidth: true; decimals: 1; minimum: 0.1; maximum: 1000
                        value: root.device.floorCounts
                        onCommitted: function (v) { root.device.floorCounts = v } }
        }
        FieldStrip.Col {
            label: qsTr("谱峰宽（频点）")
            RealField { Layout.fillWidth: true; decimals: 1; minimum: 0.5; maximum: 50
                        value: root.device.peakWidth
                        onCommitted: function (v) { root.device.peakWidth = v } }
        }
        FieldStrip.Col {
            label: qsTr("随机种子")
            IntField { Layout.fillWidth: true; minimum: 0; maximum: 2147483647
                       value: root.device.seed
                       onCommitted: function (v) { root.device.seed = v } }
        }
    }
    Label {
        text: qsTr("种子只改噪声实现：同一种子、同一场景、同一指向，结果可复现。")
        color: palette.placeholderText
        font.pointSize: 9
        wrapMode: Text.Wrap
        Layout.preferredWidth: 1
        Layout.fillWidth: true
    }

    Label { text: qsTr("环境"); font.bold: true; Layout.topMargin: 6 }
    // 舒适区两端互相咬住对方的当前值（下限不得高于上限），与铭辰俯仰扇扫的两个框同一手法。
    FieldStrip {
        FieldStrip.Col {
            label: qsTr("温度（℃）")
            RealField { Layout.fillWidth: true; decimals: 1; minimum: -60; maximum: 80
                        value: root.device.temperature
                        onCommitted: function (v) { root.device.temperature = v } }
        }
        FieldStrip.Col {
            label: qsTr("舒适区（℃，下限 ~ 上限）")
            Layout.preferredWidth: 160
            RealField { Layout.fillWidth: true; decimals: 0; minimum: -80; maximum: root.device.highC
                        value: root.device.lowC
                        onCommitted: function (v) { root.device.lowC = v } }
            RealField { Layout.fillWidth: true; decimals: 0; minimum: root.device.lowC; maximum: 100
                        value: root.device.highC
                        onCommitted: function (v) { root.device.highC = v } }
        }
    }
    FieldStrip {
        FieldStrip.Col {
            label: qsTr("低温折减（dB/℃）")
            RealField { Layout.fillWidth: true; decimals: 2; minimum: 0; maximum: 10
                        value: root.device.dbPerDegBelow
                        onCommitted: function (v) { root.device.dbPerDegBelow = v } }
        }
        FieldStrip.Col {
            label: qsTr("高温折减（dB/℃）")
            RealField { Layout.fillWidth: true; decimals: 2; minimum: 0; maximum: 10
                        value: root.device.dbPerDegAbove
                        onCommitted: function (v) { root.device.dbPerDegAbove = v } }
        }
        FieldStrip.Col {
            label: qsTr("当前折减")
            Label { text: root.device.penaltyDb.toFixed(1) + " dB"; Layout.fillWidth: true }
        }
    }

    Label { text: qsTr("采集"); font.bold: true; Layout.topMargin: 6 }
    accumulationHint: qsTr("累积 = 到点回看：取累积窗内每秒一张的风场快照，各按覆盖时长折成帧数合成后求和，相对起伏 1/√N，超过计数上限顶满——累积越久近场越先顶满。")
}
