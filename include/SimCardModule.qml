pragma ComponentBehavior: Bound
import QtQuick
import Lidar.UI
import Lidar.Charts
import QtQuick.Controls

// 末端页面独持具体型号后端，在这里连接业务、图窗与控制分区。
ModulePage {
    id: root
    readonly property string kind: "device"
    readonly property QtObject family: internals.device // 采集卡族基不是 QObject，交出设备对象本身
    property var acquisitionParameters
    property int accum: acquisitionParameters.accumDefault
    property url saveDir: acquisitionParameters.saveDirDefault
    property bool save: false
    sections: [connectionSection, acquisitionSection].filter(item => item !== null)
    mainPages: plots.pages
    SimCardPlots { id: plots; device: internals.device }
    readonly property Section connectionSection: SimCardField {
        device: internals.device
    }
    acquisitionParameters: internals.device.acquisitionParameters
    readonly property Section acquisitionSection: SimCardControl {
        device: internals.device
        acquisition: root
    }
    objectName: "SimCardModule"
    readonly property string name: internals.device.displayName
    readonly property string subtitle: internals.device.subtitle
    // 借用公共只读资源接口（即设备对象本身），不另存状态。
    readonly property var deviceState: internals.device


    required property list<QtObject> gimbals // 云台设备本身；族基不是 QObject，登记时由 C++ 验族
    function worldHistory(gimbal, begin, end) {
        return gimbal && root.gimbals.includes(gimbal.family) ? gimbal.worldHistory(begin, end) : undefined
    }

    Component.onCompleted: {
        for (const gimbal of gimbals) internals.device.addGimbal(gimbal)
    }

    QtObject {
        id: internals
        readonly property SimCardDevice device: root.SimCardDevice
    }
    Connections {
        target: internals.device
        function onLogMessage(text): void { root.logMessage(text) }
    }
}
