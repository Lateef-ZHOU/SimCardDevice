#pragma once
#include "Device/Base/RxTime.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <random>
#include <vector>
#include <QByteArray>
#include <QtEndian>
#include <SimCardProtocol.h>
#include "Algorithm/Geometry/GimbalModel.h"
#include "Algorithm/Sim/LidarForward.h"

// 主项目的采样快照；历史拥有者在主线程取数，后台只读这一份自持输入。
struct SimSceneSettings
{
    LidarForward::Detector detector;
    double penaltyDb;
    int framesPerSec;
    unsigned seed;
};
struct SimSceneStamp
{
    VerticalWindProfilePtr profile; // 时间使用帧本身的 RxTime，不混用风场演化的单调钟。
    SimSceneSettings settings;
};
struct SimSamplingInput
{
    WorldPoseHistory poses;
    std::vector<SimSceneStamp> scenes;
};
// 每个连接按值拥有一个合成器；随机序列跨请求延续，换种子或重连时重设。
class SimSpectrumGenerator
{
public:
    using Input = SimSamplingInput;
    // [工作线程] 风场与指向都按前值保持，按二者变化点分段累积。
    // 帧数累计后再取整，避免高频角度回报把每段都舍成零。
    [[nodiscard]] QByteArray synthesize(const quint64 requestId, const RxTime begin, const RxTime end,
                                       const SimSamplingInput &input)
    {
        const auto &poses = input.poses;
        const auto &scenes = input.scenes;
        if (end <= begin || poses.empty() || scenes.empty() || poses.front().first > begin
            || !scenes.front().profile || scenes.front().profile->at > begin) return {};
        for (std::size_t i = 0; i < poses.size(); ++i)
            if ((i && poses[i].first < poses[i - 1].first)
                || !std::isfinite(poses[i].second.azimuth()) || !std::isfinite(poses[i].second.elevation())) return {};
        for (std::size_t i = 0; i < scenes.size(); ++i)
            if (!scenes[i].profile || !scenes[i].profile->hasValidGrid() || scenes[i].settings.framesPerSec <= 0
                || (i && scenes[i].profile->at < scenes[i - 1].profile->at)) return {};

        const auto seed = scenes.back().settings.seed;
        if (!seeded_ || *seeded_ != seed) { rng_.seed(seed); seeded_ = seed; }
        std::vector<std::uint64_t> sum;
        std::size_t poseIndex = 0, sceneIndex = 0;
        double frameCount = 0.0;
        auto from = begin;
        while (from < end) {
            while (poseIndex + 1 < poses.size() && poses[poseIndex + 1].first <= from) ++poseIndex;
            while (sceneIndex + 1 < scenes.size() && scenes[sceneIndex + 1].profile->at <= from) ++sceneIndex;
            auto to = end;
            if (poseIndex + 1 < poses.size()) to = std::min(to, poses[poseIndex + 1].first);
            if (sceneIndex + 1 < scenes.size()) to = std::min(to, scenes[sceneIndex + 1].profile->at);
            const auto &scene = scenes[sceneIndex];
            const auto previousFrames = std::llround(frameCount);
            frameCount += std::chrono::duration<double>(to - from).count() * scene.settings.framesPerSec;
            const auto frames = std::llround(frameCount) - previousFrames;
            if (frames > std::numeric_limits<int>::max()) return {};
            if (frames > 0) {
                auto detector = scene.settings.detector;
                detector.accumFrames = static_cast<int>(frames);
                detector.maxCounts = 1e12;
                const auto part = LidarForward::synthesize(detector, scene.settings.penaltyDb,
                                                          *scene.profile, poses[poseIndex].second, rng_);
                if (sum.empty()) sum = part;
                else {
                    if (part.size() != sum.size()) return {};
                    for (std::size_t k = 0; k < sum.size(); ++k) sum[k] += part[k];
                }
            }
            from = to;
        }
        if (sum.empty()) return {};
        const auto cap = static_cast<std::uint64_t>(scenes.back().settings.detector.maxCounts);
        QByteArray frame(SimCardWire::headerBytes + static_cast<qsizetype>(sum.size() * sizeof(quint16)), Qt::Uninitialized);
        qToUnaligned(requestId, frame.data());
        for (std::size_t i = 0; i < sum.size(); ++i)
            qToUnaligned(static_cast<quint16>(std::min<std::uint64_t>(std::min(sum[i], cap), 65535)),
                         frame.data() + SimCardWire::headerBytes + i * sizeof(quint16));
        return frame;
    }
private:
    std::mt19937 rng_;
    std::optional<unsigned> seeded_;
};
