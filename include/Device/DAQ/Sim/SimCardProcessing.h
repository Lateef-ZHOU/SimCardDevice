#pragma once
#include <array>
#include <memory>
#include <QString>
#include <QVector>
#include "Algorithm/Wind/RadialWind.h"
#include "Algorithm/Wind/WindProfile.h"
#include "Acquisition/AcqSaver.h"
#include "Device/DAQ/Sim/SimCardRunSide.h"

// 型号帧处理工厂；Emitter 只提供结果信号，不向其读取或写入业务成员。
// 每条处理线程只创建一份；四束环及写位置仅由该闭包维护。
// 任务保证首次 sync 前写满四束，后续同步点仍使用最近四个互异方向。
template<class Emitter>
auto makeSimCardProcessor(Emitter *self)
{
    return [self, ring_ = std::array<WindProfile::Input, WindProfile::kBeams>{}, ringW_ = 0]
        (const std::shared_ptr<const SimCardCollector> &collector) mutable {
        if (!collector || !collector->settled())
            return; // 只处理完整帧
        const auto &[pose, saveRoot, sessionDir, sync] = collector->context();
        const auto &specArray = collector->constFrame();
        const QString fileStem = AcqSaver::fileStem(collector->frameTime());
        static const QString tag = QStringLiteral("SIM");
        const auto &geo = SimCardConfig::geometry;

        // 1. double 谱 → 推 GUI → 落盘。发出后本数组只读（GUI 的 3D 模块直接接住这份份额）。
        Q_EMIT self->specReady(specArray);
        if (saveRoot)
            AcqSaver::saveSpec(*saveRoot, sessionDir, fileStem, specArray, tag);

        // 2. 径向风 + CNR：与真卡同一条算法、同一组标定（合成时用的就是这两个数）。
        const auto [ws, cnr] =
            RadialWind::Calc(specArray, geo.row, geo.col, SimCardConfig::freq2WsSlope, geo.freq2WsIntercept());
        Q_EMIT self->wsReady(ws);
        Q_EMIT self->cnrReady(cnr);
        if (saveRoot)
            AcqSaver::saveRadialWind(*saveRoot, sessionDir, fileStem, AcqSaver::timeStem(collector->frameTime()),
                                     {{tag, {ws, cnr}}});

        // 3. 风廓线：进环记任务链交来的朝向（测试时为空 → 整组拒绝）。
        ring_[ringW_] = {pose, ws};
        ringW_ = (ringW_ + 1) % WindProfile::kBeams;
        if (!sync)
            return;
        const auto profile = WindProfile::Calc(ring_, SimCardConfig::gateSpacingM, collector->frameTime());
        if (!profile)
            return;
        Q_EMIT self->profileReady(profile);
        if (saveRoot)
            AcqSaver::saveWindProfile(profile, *saveRoot, sessionDir, tag);
    };
}
