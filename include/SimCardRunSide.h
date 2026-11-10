#pragma once
#include <optional>
#include <algorithm>
#include <chrono>
#include <atomic>
#include <QByteArray>
#include <QList>
#include <QVector>
#include <QtEndian>

#include "DAQCollector.h"         // DAQCollector / RxTime
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
inline constexpr double wavelengthM = 1.55e-6; // 激光波长（m）；频点 i 记作 i MHz
inline constexpr double binMHz = 1.0; // 频点间隔（MHz）
inline constexpr double freq2WsSlope = RadialWind::metersPerSecondPerMHz(wavelengthM) * binMHz; // bin → m/s，合成器与收集器共用
struct Geometry
{
    int row;
    int col;
    [[nodiscard]] constexpr double freq2WsIntercept() const noexcept { return -col * freq2WsSlope / 2.0; }
    [[nodiscard]] constexpr double wsAbsMax() const noexcept { return col * freq2WsSlope / 2.0; }
};
inline constexpr Geometry geometry{450, 200};
inline constexpr int AccumSecDefault = 1;
inline constexpr int AccumSecMin = 1;
inline constexpr int AccumSecMax = 3600;
inline constexpr double rangeStartM = 0.0;   // 第 0 门斜距（m）；第 k 门 = rangeStartM + k·gateSpacingM
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
    // 族模板层按请求参数造完整实例；每次创建（含重试）获得新的请求身份。
    [[nodiscard]] static std::shared_ptr<SimCardCollector> create(const DAQParameters &parameters)
    {
        return std::shared_ptr<SimCardCollector>(new SimCardCollector(parameters));
    }
    [[nodiscard]] QList<QByteArray> buildStart() const override
    {
        return {SimCardWire::encode(SimCardWire::Command{SimCardWire::Command::Accumulate, parameters().accumSec, requestId_})};
    }
    [[nodiscard]] QList<QByteArray> buildStop() const override
    {
        return {SimCardWire::encode(SimCardWire::Command{SimCardWire::Command::Stop, 0, requestId_})};
    }
    // 看门狗：累积时长之外再给两秒余量
    [[nodiscard]] int timeoutMs() const override { return parameters().accumSec * 1000 + 2000; }
    [[nodiscard]] std::shared_ptr<SimCardCollector> makeSelf() const { return create(parameters()); }

private:
    explicit SimCardCollector(DAQParameters parameters)
        : DAQCollector(std::move(parameters), SimCardConfig::frameBytes, SimCardConfig::wavelengthM) {}

    void onFeed(const std::optional<QByteArray> &item, const RxTime at) override
    {
        const auto &raw = *item;
        if (raw.size() != SimCardWire::headerBytes + SimCardConfig::frameBytes
            || qFromUnaligned<quint64>(raw.constData()) != requestId_)
            return; // 不完整报文或已取消/重试的旧请求：不认
        frameTime_ = at;
        (void)store(0, QByteArrayView(raw).sliced(SimCardWire::headerBytes)); // 去掉报头，一包即一整帧
    }

    // 单路 SIM：与合成器同一组标定（频点 i 记作 i MHz、系数 freq2WsSlope，零多普勒在中心频点）。
    std::vector<DAQChannel> decode(const QByteArray &frame) const override
    {
        const auto &geo = SimCardConfig::geometry;
        LinearArray2D<double> spectrum(geo.row, geo.col, SimCardConfig::rangeStartM, SimCardConfig::gateSpacingM, 0.0,
                                       SimCardConfig::binMHz);
        spectrum.reserve(std::size_t(geo.row) * geo.col);
        for (qsizetype offset = 0; offset < frame.size(); offset += qsizetype(sizeof(quint16)))
            spectrum.push_back(qFromUnaligned<quint16>(frame.constData() + offset));
        std::vector<DAQChannel> channels;
        channels.emplace_back(QStringLiteral("SIM"), std::move(spectrum), geo.col / 2.0, wavelengthM());
        return channels;
    }
    inline static std::atomic<quint64> nextRequestId_{0};
    const quint64 requestId_ = ++nextRequestId_; // 每次创建（含重试）获得不同的请求身份。
};

Q_DECLARE_METATYPE(std::shared_ptr<const SimCardCollector>)
