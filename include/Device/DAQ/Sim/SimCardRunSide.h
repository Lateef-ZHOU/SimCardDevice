#pragma once
#include <algorithm>
#include <chrono>
#include <atomic>
#include <QByteArray>
#include <QList>
#include <QVector>
#include <QtEndian>

#include "Device/DAQ/DAQCollector.h"         // DAQCollector / RxTime
#include <SimCardProtocol.h>     // 命令协议 SimCardWire（收集器编码，句柄解码）

// ============================================================
// 虚拟采集卡的宪法常量店 + 收集器
//
// 虚拟卡自行定义尺寸、标定与计数量程，不引用任何实物采集卡的协议类型。探测器灵敏度与环境折减的
// 默认值也住这里，页面从这儿取初值。指向不住这里：连接时选择云台，主线程在固定累积窗口内取它的角度历史。
// 句柄交请求号与一整帧行主序 quint16；收集器验型、验请求号后转为 double，worker 只读共享数组。
// ============================================================
namespace SimCardConfig
{
inline constexpr double freq2WsSlope = 1.0;
struct Geometry
{
    int row;
    int col;
    [[nodiscard]] constexpr double freq2WsIntercept() const noexcept { return -col * freq2WsSlope / 2.0; }
    [[nodiscard]] constexpr double wsAbsMax() const noexcept { return col * freq2WsSlope / 2.0; }
};
inline constexpr Geometry geometry{450, 200};
inline constexpr int AccumSecDefault = 2;
inline constexpr int AccumSecMin = 1;
inline constexpr int AccumSecMax = 3600;
inline constexpr double gateSpacingM = 60.0; // 门距（m）
inline constexpr int framesPerSec = 10;      // 累积帧率：累积帧数 N = 秒数 × 它；计数 = N × 单帧谱（逐帧求和），相对噪声 1/√N
inline constexpr std::chrono::milliseconds fieldTick{1000}; // 风场节拍：官邸每拍出一张风场——场景换一张快照、真值图追一列
inline constexpr double truthWindowSec = 3600.0; // 真值图设计窗 1 h；环保存只读帧份额，与风场源共享层数组。
inline constexpr double cnrAt1kmDefault = 10.0;      // dB
inline constexpr double extinctionDefault = 2.0;     // dB/km，双程
inline constexpr double overlapRangeDefault = 500.0; // m
inline constexpr double floorCountsDefault = 2.0; // 单帧噪声底（计数）：累积 2 s（20 帧）后 40、近场谱峰约 4 万不顶满；≥ 4 s 近场开始顶满（16 位饱和，真卡同理）
inline constexpr double peakWidthDefault = 3.0; // 频点
inline constexpr double maxCounts = 65535.0;
inline constexpr double temperatureDefault = 20.0; // ℃
inline constexpr double lowCDefault = -20.0;
inline constexpr double highCDefault = 40.0;
inline constexpr double dbPerDegBelowDefault = 0.3;
inline constexpr double dbPerDegAboveDefault = 0.5;
inline constexpr unsigned seedDefault = 7; // 随机种子：同一种子同一场景同一指向，合成结果可复现
inline constexpr qsizetype frameBytes =
    static_cast<qsizetype>(geometry.row) * geometry.col * sizeof(quint16); // 一帧的字节数
} // namespace SimCardConfig

class SimCardCollector final : public DAQCollector
{
public:
    // 原型只持不可变请求信息；不为尚未运行的请求分配谱数组。
    [[nodiscard]] static std::shared_ptr<const SimCardCollector> createPrototype(const int accumSec, DAQContext context = {})
    {
        return std::shared_ptr<const SimCardCollector>(new const SimCardCollector(accumSec, std::move(context)));
    }
    [[nodiscard]] const QVector<double> &constFrame() const
    {
        static const QVector<double> empty;
        return settled() ? frame_ : empty;
    }
    [[nodiscard]] bool settled() const override { return received_; }
    [[nodiscard]] QList<QByteArray> buildStart() const override
    {
        return {SimCardWire::encode(SimCardWire::Command{SimCardWire::Command::Accumulate, accumSec_, requestId_})};
    }
    [[nodiscard]] QList<QByteArray> buildStop() const override
    {
        return {SimCardWire::encode(SimCardWire::Command{SimCardWire::Command::Stop, 0, requestId_})};
    }
    // 看门狗：累积时长之外再给两秒余量
    [[nodiscard]] int timeoutMs() const override { return accumSec_ * 1000 + 2000; }
    [[nodiscard]] std::shared_ptr<SimCardCollector> makeSelf() const
    {
        auto next = std::shared_ptr<SimCardCollector>(new SimCardCollector(accumSec_, context()));
        next->frame_.resize(SimCardConfig::geometry.row * SimCardConfig::geometry.col);
        return next;
    }

private:
    explicit SimCardCollector(const int accumSec, DAQContext context)
        : DAQCollector(std::move(context)), accumSec_(std::clamp(accumSec, SimCardConfig::AccumSecMin, SimCardConfig::AccumSecMax)) {}

    void onFeed(const QByteArray &raw, const RxTime at) override
    {
        if (raw.size() != SimCardWire::headerBytes + SimCardConfig::frameBytes
            || qFromUnaligned<quint64>(raw.constData()) != requestId_)
            return; // 不完整报文或已取消/重试的旧请求：不认
        double *const out = frame_.data();
        const char *const bytes = raw.constData() + SimCardWire::headerBytes;
        constexpr int elements = SimCardConfig::geometry.row * SimCardConfig::geometry.col;
        for (int i = 0; i < elements; ++i)
            out[i] = qFromUnaligned<quint16>(bytes + i * sizeof(quint16));
        frameTime_ = at;
        received_ = true;
    }
    inline static std::atomic<quint64> nextRequestId_{0};
    const quint64 requestId_ = ++nextRequestId_; // 每次 makeSelf（含重试）获得不同的请求身份。
    const int accumSec_;
    QVector<double> frame_;
    bool received_ = false;
};

Q_DECLARE_METATYPE(std::shared_ptr<const SimCardCollector>)
