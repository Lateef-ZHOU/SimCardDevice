pragma ComponentBehavior: Bound
import QtQuick
import Lidar.UI
import Lidar.TaskUI
import Lidar.Charts
import QtQuick.Layouts
import QtQuick.Controls

// 末端页面独持具体型号后端，在这里连接业务、图窗与控制分区。
ModulePage {
    id: root
    readonly property string kind: "device"
    readonly property DAQDeviceBase family: internals.device
    sections: [fieldSection, detectorSection]
    mainPages: plots.pages
    SimCardPlots { id: plots; device: internals.device }
    readonly property Section fieldSection: Section {
        title: qsTr("风场设置")
        SimWindFields { device: internals.device; Layout.fillWidth: true }
    }
    readonly property Section detectorSection: Section {
        title: qsTr("探测器与环境")
        SimDetectorFields { device: internals.device; Layout.fillWidth: true }
    }
    objectName: "SimCardSimOnlyModule"
    readonly property string name: internals.device.device.displayName
    readonly property string subtitle: internals.device.device.subtitle
    readonly property bool isOpen: internals.device.device.isOpen
    readonly property bool isLocked: internals.device.device.isLocked
    readonly property string holder: internals.device.device.holder


    required property GMBDeviceBase gimbal
    function worldHistory(page, begin, end) {
        return page && page.family === root.gimbal ? page.worldHistory(begin, end) : undefined
    }

    Component.onCompleted: {
        internals.device.addGimbal(root.gimbal)
    }

    function open(): bool { return internals.device.open() }
    signal configurationChanged()
    function applyConfiguration(field, detector, environment): void {
        const f = internals.device.field
        f.kind = ["uniform", "linearShear"].indexOf(field.kind)
        f.temporal = ["steady", "swing", "ramp", "gust"].indexOf(field.temporal)
        f.speed = field.speedMS
        f.direction = field.directionDeg
        f.vertical = field.verticalMS
        f.speedGain = field.speedGainMSPerKm
        f.directionGain = field.directionGainDegPerKm
        f.top = field.topM
        f.swingSpeed = field.swingSpeedMS
        f.swingDirection = field.swingDirectionDeg
        f.period = field.periodS
        f.rampSpeed = field.rampSpeedMS
        f.rampDirection = field.rampDirectionDeg
        f.rampDuration = field.rampDurationS
        f.gustSpeed = field.gustSpeedMS
        f.gustDirection = field.gustDirectionDeg
        f.gustScale = field.gustScaleS
        const d = internals.device
        d.cnrAt1km = detector.cnrAt1kmDb
        d.extinction = detector.extinctionDbPerKm
        d.overlapRange = detector.overlapRangeM
        d.floorCounts = detector.floorCounts
        d.peakWidth = detector.peakWidthBins
        d.seed = detector.seed
        d.lowC = detector.lowC
        d.highC = detector.highC
        d.dbPerDegBelow = detector.dbPerDegBelow
        d.dbPerDegAbove = detector.dbPerDegAbove
        d.temperature = environment.temperatureC
        // 同步记录完整配置；首次采样窗之前一定有配置后的场景快照。
        d.captureScene()
    }
    function configuration() {
        const d = internals.device, f = d.field
        return {
            field: { kind: ["uniform", "linearShear"][f.kind],
                temporal: ["steady", "swing", "ramp", "gust"][f.temporal],
                speedMS: f.speed, directionDeg: f.direction, verticalMS: f.vertical,
                speedGainMSPerKm: f.speedGain, directionGainDegPerKm: f.directionGain, topM: f.top,
                swingSpeedMS: f.swingSpeed, swingDirectionDeg: f.swingDirection, periodS: f.period,
                rampSpeedMS: f.rampSpeed, rampDirectionDeg: f.rampDirection, rampDurationS: f.rampDuration,
                gustSpeedMS: f.gustSpeed, gustDirectionDeg: f.gustDirection, gustScaleS: f.gustScale },
            detector: { cnrAt1kmDb: d.cnrAt1km, extinctionDbPerKm: d.extinction,
                overlapRangeM: d.overlapRange, floorCounts: d.floorCounts, peakWidthBins: d.peakWidth,
                seed: d.seed, lowC: d.lowC, highC: d.highC,
                dbPerDegBelow: d.dbPerDegBelow, dbPerDegAbove: d.dbPerDegAbove },
            environment: { temperatureC: d.temperature }
        }
    }
    Connections {
        target: internals.device
        function onSceneChanged(): void { root.configurationChanged() }
    }
    Connections {
        target: internals.device.field
        function onChanged(): void { root.configurationChanged() }
    }

    QtObject {
        id: internals
        readonly property SimCardDevice device: root.SimCardDevice
    }
    Connections {
        target: internals.device.device.logSource
        function onLogMessage(text): void { root.logMessage(text) }
    }
}
