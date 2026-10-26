#pragma once
#include <SimCardHandle.h>
#include "Device/DAQ/Sim/SimSpectrumGenerator.h"

// 模板执行层与本项目合成器在末端绑定；只补 Qt 通知和同步析构入口。
class LidarSimCardHandle final : public SimCardHandle<SimSpectrumGenerator>
{
    Q_OBJECT
public:
    using SimCardHandle::SimCardHandle;
    ~LidarSimCardHandle() override { stop(); }
Q_SIGNALS:
    void connectionChanged(bool connected) override;
    void logMessage(const QString &text) const final;
};
