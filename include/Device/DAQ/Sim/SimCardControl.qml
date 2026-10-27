// 全功能页面：探测器共用字段与单卡测试控制。
pragma ComponentBehavior: Bound
import QtQuick
import Lidar.UI
import QtQuick.Controls
import QtQuick.Layouts

import Lidar.TaskUI

DAQControl {
    id: root
    SimDetectorFields { device: root.device; Layout.fillWidth: true }
    Label { text: qsTr("采集"); font.bold: true; Layout.topMargin: 6 }
    accumulationHint: qsTr("累积 = 到点回看：取累积窗内每秒一张的风场快照，各按覆盖时长折成帧数合成后求和，相对起伏 1/√N，超过计数上限顶满——累积越久近场越先顶满。")
}
