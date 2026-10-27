pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import Lidar.Charts

QtObject {
    id: root
    required property SimCardDevice device
    readonly property var chartParameters: device.chartParameters()
    readonly property list<Page> pages: [
        Page {
            title: qsTr("功率谱 3D")
            contentItem: SurfaceView {
                id: surface
                specification: root.chartParameters.plotConfiguration
                Connections {
                    target: root.device
                    function onSpecReady(values) { surface.updateSpectrum(values) }
                }
            }
        },
        Page {
            title: qsTr("风速 | CNR")
            contentItem: Wind2DView {
                id: wind
                specification: root.chartParameters.plotConfiguration
                Connections {
                    target: root.device
                    function onWsReady(values) { wind.updateWS(values) }
                    function onCnrReady(values) { wind.updateCNR(values) }
                }
            }
        },
        Page {
            title: qsTr("风羽图")
            contentItem: WindBarbItem {
                id: barb
                readonly property var spec: root.chartParameters.plotConfiguration
                configuration: ({ heightBins: spec.rows, maxHeight: spec.rows * spec.gateSpacing,
                    windowSeconds: spec.barbWindow, columnSeconds: spec.barbColumn })
                Connections {
                    target: root.device
                    function onProfileReady(profile) { barb.appendColumn(profile) }
                }
            }
        },
        Page {
            title: qsTr("风场真值")
            contentItem: WindBarbItem {
                id: truth
                objectName: "truthChart"
                configuration: root.chartParameters.truthConfiguration
                Connections {
                    target: root.device
                    function onTruthReady(profile) { truth.appendColumn(profile) }
                }
                Component.onCompleted: root.device.publishTruth()
            }
        }
    ]
}
