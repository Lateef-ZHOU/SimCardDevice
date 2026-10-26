#pragma once
#include "Device/Base/RxTime.h"
#include <utility>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <numbers>
#include <random>
#include <QObject>
#include <QString>
#include <QStringList>

#include "Algorithm/Sim/WindField.h" // 虚拟风场工厂

// ============================================================
// WindFieldSource —— 虚拟风场源：页面上可编辑的风场描述，按时刻交 field() 出一张值类
//
// 它是虚拟采集卡的"连接对象"（真卡连网卡，虚拟卡连风场）。只有描述与出值，没有行为；
// 改了就发 changed（官邸记一条日志）；场景快照由官邸按节拍从 field() 现出，不在这里推。
//
// 描述分两根正交的轴（10-13 用户定"加几个典型类型"）：
//   · **竖直结构**（kind）：匀强、线性剪切——WindField 的两个具名工厂，交出共同的等间隔 ENU 帧。
//   · **时域变化**（temporal）：作用在**地面值**上、整列同步平移，剪切的梯度不变。四型：
//       定常     —— 不随时间变；
//       周期摆动 —— 风速、风向围绕基值按正弦摆（幅值、周期）：阵风与日变化都是它，只是周期不同；
//       线性过渡 —— 风速、风向在一段时长内线性变到新值然后保持（锋面过境）；
//       随机起伏 —— 若干谐波叠加、相位由种子定：谱法合成湍流的最简版（幅值平坦、周期自时间尺度起按 √2 递增、
//                   不公度故不整周期重复），是 (种子, 时刻) 的纯函数——可复现、不留状态。**声明了假设的模型，不是实测**。
//   时域的零点 = **上一次改动的那一刻**（任一参数改了都重新起算，规则只有一条），由本类用单调钟盖章（量时长用单调钟，
//   用户定）；之后的时刻由调用方（官邸的节拍）交进来，本类不再读钟。随机起伏的种子也由调用方交——与探测器噪声共用
//   页面上那一个种子，改种子两边一起重播。
// ============================================================
class WindFieldSource final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList kinds READ kinds CONSTANT)
    Q_PROPERTY(int kind READ kind WRITE setKind NOTIFY changed)
    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY changed)             // 地面风速（m/s）
    Q_PROPERTY(double direction READ direction WRITE setDirection NOTIFY changed) // 地面风向（°，来向）
    Q_PROPERTY(double vertical READ vertical WRITE setVertical NOTIFY changed)    // 垂直速度（m/s）
    Q_PROPERTY(double speedGain READ speedGain WRITE setSpeedGain NOTIFY changed) // 剪切：每千米风速增量
    Q_PROPERTY(double directionGain READ directionGain WRITE setDirectionGain NOTIFY changed) // 每千米风向增量
    Q_PROPERTY(double top READ top WRITE setTop NOTIFY changed)                   // 剪切顶高（m）
    Q_PROPERTY(QStringList temporalKinds READ temporalKinds CONSTANT)
    Q_PROPERTY(int temporal READ temporal WRITE setTemporal NOTIFY changed)
    Q_PROPERTY(double swingSpeed READ swingSpeed WRITE setSwingSpeed NOTIFY changed)             // 周期摆动：风速幅值（m/s）
    Q_PROPERTY(double swingDirection READ swingDirection WRITE setSwingDirection NOTIFY changed) // 风向幅值（°）
    Q_PROPERTY(double period READ period WRITE setPeriod NOTIFY changed)                         // 周期（s）
    Q_PROPERTY(double rampSpeed READ rampSpeed WRITE setRampSpeed NOTIFY changed)                // 线性过渡：风速变化量（m/s）
    Q_PROPERTY(double rampDirection READ rampDirection WRITE setRampDirection NOTIFY changed)    // 风向变化量（°）
    Q_PROPERTY(double rampDuration READ rampDuration WRITE setRampDuration NOTIFY changed)       // 过渡时长（s）
    Q_PROPERTY(double gustSpeed READ gustSpeed WRITE setGustSpeed NOTIFY changed)                // 随机起伏：风速标准差（m/s）
    Q_PROPERTY(double gustDirection READ gustDirection WRITE setGustDirection NOTIFY changed)    // 风向标准差（°）
    Q_PROPERTY(double gustScale READ gustScale WRITE setGustScale NOTIFY changed)                // 时间尺度（最短周期，s）
public:
    using Clock = std::chrono::steady_clock;

    explicit WindFieldSource(QObject *parent = nullptr) : QObject(parent), since_(Clock::now()) {}

    [[nodiscard]] static QStringList kinds() { return {QStringLiteral("匀强"), QStringLiteral("线性剪切")}; }
    [[nodiscard]] static QStringList temporalKinds()
    {
        return {QStringLiteral("定常"), QStringLiteral("周期摆动"), QStringLiteral("线性过渡"), QStringLiteral("随机起伏")};
    }
    [[nodiscard]] int kind() const noexcept { return kind_; }
    [[nodiscard]] double speed() const noexcept { return speed_; }
    [[nodiscard]] double direction() const noexcept { return direction_; }
    [[nodiscard]] double vertical() const noexcept { return vertical_; }
    [[nodiscard]] double speedGain() const noexcept { return speedGain_; }
    [[nodiscard]] double directionGain() const noexcept { return directionGain_; }
    [[nodiscard]] double top() const noexcept { return top_; }
    [[nodiscard]] int temporal() const noexcept { return temporal_; }
    [[nodiscard]] double swingSpeed() const noexcept { return swingSpeed_; }
    [[nodiscard]] double swingDirection() const noexcept { return swingDirection_; }
    [[nodiscard]] double period() const noexcept { return period_; }
    [[nodiscard]] double rampSpeed() const noexcept { return rampSpeed_; }
    [[nodiscard]] double rampDirection() const noexcept { return rampDirection_; }
    [[nodiscard]] double rampDuration() const noexcept { return rampDuration_; }
    [[nodiscard]] double gustSpeed() const noexcept { return gustSpeed_; }
    [[nodiscard]] double gustDirection() const noexcept { return gustDirection_; }
    [[nodiscard]] double gustScale() const noexcept { return gustScale_; }

    void setKind(const int v) { assign(kind_, v); }
    void setSpeed(const double v) { assign(speed_, v); }
    void setDirection(const double v) { assign(direction_, v); }
    void setVertical(const double v) { assign(vertical_, v); }
    void setSpeedGain(const double v) { assign(speedGain_, v); }
    void setDirectionGain(const double v) { assign(directionGain_, v); }
    void setTop(const double v) { assign(top_, v); }
    void setTemporal(const int v) { assign(temporal_, v); }
    void setSwingSpeed(const double v) { assign(swingSpeed_, v); }
    void setSwingDirection(const double v) { assign(swingDirection_, v); }
    void setPeriod(const double v) { assign(period_, v); }
    void setRampSpeed(const double v) { assign(rampSpeed_, v); }
    void setRampDirection(const double v) { assign(rampDirection_, v); }
    void setRampDuration(const double v) { assign(rampDuration_, v); }
    void setGustSpeed(const double v) { assign(gustSpeed_, v); }
    void setGustDirection(const double v) { assign(gustDirection_, v); }
    void setGustScale(const double v) { assign(gustScale_, v); }

    // 一句话描述（日志用）。
    [[nodiscard]] QString describe() const
    {
        const QString base = QStringLiteral("%1 m/s 自 %2°，垂直 %3 m/s")
                                 .arg(speed_, 0, 'f', 1).arg(direction_, 0, 'f', 0).arg(vertical_, 0, 'f', 1);
        QString s = kind_ == 1 ? QStringLiteral("线性剪切，地面 ") + base +
                                     QStringLiteral("，每千米风速 %1 m/s、风向 %2°，顶高 %3 m")
                                         .arg(speedGain_, 0, 'f', 1).arg(directionGain_, 0, 'f', 0).arg(top_, 0, 'f', 0)
                               : QStringLiteral("匀强，") + base;
        switch (temporal_)
        {
        case Swing:
            s += QStringLiteral("；周期摆动：风速 ±%1 m/s、风向 ±%2°，周期 %3 s")
                     .arg(swingSpeed_, 0, 'f', 1).arg(swingDirection_, 0, 'f', 0).arg(period_, 0, 'f', 0);
            break;
        case Ramp:
            s += QStringLiteral("；线性过渡：风速 %1 m/s、风向 %2°，用 %3 s")
                     .arg(rampSpeed_, 0, 'f', 1).arg(rampDirection_, 0, 'f', 0).arg(rampDuration_, 0, 'f', 0);
            break;
        case Gust:
            s += QStringLiteral("；随机起伏：风速 σ %1 m/s、风向 σ %2°，时间尺度 %3 s")
                     .arg(gustSpeed_, 0, 'f', 1).arg(gustDirection_, 0, 'f', 0).arg(gustScale_, 0, 'f', 0);
            break;
        default:
            break;
        }
        return s;
    }

    // 将时刻 now 的风场写入官邸给定的网格；时域变化按 now − 上次改动算，seed 只供随机起伏。
    [[nodiscard]] VerticalWindProfileUnit field(VerticalWindProfileUnit profile, const Clock::time_point now, const unsigned seed) const
    {
        const double t = std::chrono::duration<double>(now - since_).count();
        double s = speed_;
        double d = direction_;
        switch (temporal_)
        {
        case Swing:
        {
            const double w = std::sin(2.0 * std::numbers::pi * t / period_);
            s += swingSpeed_ * w;
            d += swingDirection_ * w;
            break;
        }
        case Ramp:
        {
            const double r = std::clamp(t / rampDuration_, 0.0, 1.0);
            s += rampSpeed_ * r;
            d += rampDirection_ * r;
            break;
        }
        case Gust:
            s += gustSpeed_ * harmonics(t, gustScale_, seed);
            d += gustDirection_ * harmonics(t, gustScale_, seed ^ kDirectionSalt);
            break;
        default:
            break;
        }
        s = std::max(s, 0.0); // 风速是模：摆动 / 起伏压过基值时贴地为零，不出负风速（风向自有其字段，WindField 按圆周收）
        if (kind_ == 1)
            return WindField::linearShear(std::move(profile), s, speedGain_, d, directionGain_, top_, vertical_);
        return WindField::uniform(std::move(profile), s, d, vertical_);
    }

Q_SIGNALS:
    void changed();

private:
    enum Temporal : int
    {
        Steady,
        Swing,
        Ramp,
        Gust
    };
    static constexpr unsigned kDirectionSalt = 0x5bd1e995u; // 风向的相位用另一串，与风速不同步

    // 谐波叠加：N 个正弦，周期自 scale 起按 √2 递增，幅值平坦、相位由种子定；合成的标准差为 1（每个正弦方差 1/2）。
    [[nodiscard]] static double harmonics(const double t, const double scale, const unsigned seed)
    {
        constexpr int N = 6;
        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> phase(0.0, 2.0 * std::numbers::pi);
        double sum = 0.0;
        double period = scale;
        for (int i = 0; i < N; ++i)
        {
            sum += std::sin(2.0 * std::numbers::pi * t / period + phase(rng));
            period *= std::numbers::sqrt2;
        }
        return sum * std::sqrt(2.0 / N);
    }

    // 写口：变了才记、才发；每一次改动都把时域零点挪到此刻（单调钟）。
    template <typename T>
    void assign(T &slot, const T v)
    {
        if (slot == v)
            return;
        slot = v;
        since_ = Clock::now();
        Q_EMIT changed();
    }
    int kind_ = 0;
    double speed_ = 10.0;
    double direction_ = 270.0;
    double vertical_ = 0.0;
    double speedGain_ = 5.0;
    double directionGain_ = 30.0;
    double top_ = 3000.0;
    int temporal_ = Steady;
    double swingSpeed_ = 3.0;
    double swingDirection_ = 20.0;
    double period_ = 60.0;
    double rampSpeed_ = 5.0;
    double rampDirection_ = 90.0;
    double rampDuration_ = 600.0;
    double gustSpeed_ = 1.0;
    double gustDirection_ = 10.0;
    double gustScale_ = 30.0;
    Clock::time_point since_; // 时域零点：上一次改动的那一刻
};
