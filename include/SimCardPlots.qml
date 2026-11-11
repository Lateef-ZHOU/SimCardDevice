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
            title: qsTr("功率谱")
            contentItem: SurfaceView {
                id: surface
                specification: root.chartParameters.plotConfiguration
                Connections {
                    target: root.device
                    function onSpectrumReady(channel, values) { surface.updateSpectrum(values) }
                }
            }
        },
        Page {
            title: qsTr("径向风")
            contentItem: Wind2DView {
                id: wind
                specification: root.chartParameters.plotConfiguration
                Connections {
                    target: root.device
                    function onRadialReady(channel, speeds, cnr) {
                        wind.updateWS(speeds)
                        wind.updateCNR(cnr)
                    }
                }
            }
        },
        Page {
            title: qsTr("风廓线")
            contentItem: WindBarbItem {
                id: barb
                readonly property var spec: root.chartParameters.plotConfiguration
                configuration: ({ heightBins: spec.rows, maxHeight: spec.rows * spec.gateSpacing,
                    windowSeconds: spec.barbWindow, columnSeconds: spec.barbColumn })
                Connections {
                    target: root.device
                    function onProfileReady(channel, profile) { barb.appendColumn(profile) }
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
