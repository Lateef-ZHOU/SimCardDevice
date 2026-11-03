#pragma once
#include <SimCardHandle.h>
#include "SimSpectrumGenerator.h"

// 模板执行层与本项目合成器在末端绑定；只补 Qt 通知和同步析构入口。
class LidarSimCardHandle : public SimCardHandle<SimSpectrumGenerator>
{
    Q_OBJECT
public:
    using SimCardHandle::SimCardHandle;
    ~LidarSimCardHandle() override = default;
Q_SIGNALS:
    void connectionChanged(HandleCore::ResourcePhase connected) override;
    void logMessage(const QString &text) const final;
};
