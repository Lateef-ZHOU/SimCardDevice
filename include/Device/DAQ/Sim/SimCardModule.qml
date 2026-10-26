pragma ComponentBehavior: Bound
import QtQuick
import Lidar.UI
import Lidar.TaskUI
import Lidar.Charts
import QtQuick.Controls

// 末端页面独持具体型号后端，在这里连接业务、图窗与控制分区。
ModulePage {
    id: root
    readonly property string kind: "device"
    readonly property DAQDeviceBase family: internals.device
    property var acquisitionParameters
    property int accum: acquisitionParameters.accumDefault
    property url saveDir: acquisitionParameters.saveDirDefault
    property bool save: false
    sections: [connectionSection, acquisitionSection].filter(item => item !== null)
    mainPages: [
        Page {
            title: qsTr("功率谱 3D")
            contentItem: SurfaceView {
                id: surface
                specification: internals.chartParameters.plotConfiguration
                Connections {
                    target: internals.device
                    function onSpecReady(values) { surface.updateSpectrum(values) }
                }
            }
        },
        Page {
            title: qsTr("风速 | CNR")
            contentItem: Wind2DView {
                id: wind
                specification: internals.chartParameters.plotConfiguration
                Connections {
                    target: internals.device
                    function onWsReady(values) { wind.updateWS(values) }
                    function onCnrReady(values) { wind.updateCNR(values) }
                }
            }
        },
        Page {
            title: qsTr("风羽图")
            contentItem: WindBarbItem {
                id: barb
                readonly property var spec: internals.chartParameters.plotConfiguration
                configuration: ({ heightBins: spec.rows, maxHeight: spec.rows * spec.gateSpacing,
                    windowSeconds: spec.barbWindow, columnSeconds: spec.barbColumn })
                Connections {
                    target: internals.device
                    function onProfileReady(profile) { barb.appendColumn(profile) }
                }
            }
        },
        Page {
            title: qsTr("风场真值")
            contentItem: WindBarbItem {
                id: truth
                objectName: "truthChart"
                configuration: internals.chartParameters.truthConfiguration
                Connections {
                    target: internals.device
                    function onTruthReady(profile) { truth.appendColumn(profile) }
                }
                Component.onCompleted: internals.device.publishTruth()
            }
        }
    ]
    readonly property Section connectionSection: SimCardField {
        device: internals.device
    }
    acquisitionParameters: internals.device.acquisitionParameters
    readonly property Section acquisitionSection: SimCardControl {
        device: internals.device
        acquisition: root
    }
    objectName: "SimCardModule"
    readonly property string name: internals.device.device.displayName
    readonly property string subtitle: internals.device.device.subtitle
    readonly property bool isOpen: internals.device.device.isOpen
    readonly property bool isLocked: internals.device.device.isLocked
    readonly property string holder: internals.device.device.holder


    required property list<GMBDeviceBase> gimbals
    function worldHistory(gimbal, begin, end) {
        return gimbal && root.gimbals.includes(gimbal.family) ? gimbal.worldHistory(begin, end) : undefined
    }

    Component.onCompleted: {
        for (const gimbal of gimbals) internals.device.addGimbal(gimbal)
    }

    QtObject {
        id: internals
        readonly property var chartParameters: device.chartParameters()
        readonly property SimCardDevice device: root.SimCardDevice
    }
    Connections {
        target: internals.device.device.logSource
        function onLogMessage(text): void { root.logMessage(text) }
    }
}
