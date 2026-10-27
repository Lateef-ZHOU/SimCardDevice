#pragma once
#include "Device/DAQ/Sim/SimCardProcessing.h"
#include <QDateTime>
#include <QtQml/qqmlregistration.h>
#include "UI/Chart/DAQChartSpecification.h"
#include "Acquisition/AcqSaver.h"
#include "Algorithm/Wind/VerticalWindProfile.h"
#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QTimer>
#include <QUrl>
#include <QVector>

#include "Algorithm/Sim/LidarForward.h"     // Detector / Environment / Degradation
#include "Algorithm/Wind/RangeGate.h"       // 门 → 斜距：真值列的高度
#include "Device/DAQ/DAQDevice.h"           // 采集卡族两层
#include "Device/DAQ/Sim/SimCardRunSide.h"  // 宪法 + 收集器
#include "Device/DAQ/Sim/WindFieldSource.h" // 虚拟风场源
#include "Device/GMB/GMBDevice.h"           // GMBDeviceBase：所架云台的连接、身份与退场通知
#include "Device/Handle/LidarSimCardHandle.h"    // 连接机制：钟 + 采样器

// 虚拟采集卡：主线程维护场景历史和页面设置，Handle 主线程按固定累积窗取数。
// 风廓线与真值图共享 const 帧；角度由 QML 创建的 AngleChartItem 独持历史。
// 交给 Handle 工作线程的计算输入仅含值与只读帧份额，不捕获页面或设备。
class SimCardDevice final
#ifdef Q_MOC_RUN
    : public DAQDeviceBase // 模板层继承的族级元对象。
#else
    : public DAQDevice<LidarSimCardHandle, SimCardCollector>
#endif
{
    Q_OBJECT
    QML_ELEMENT
    QML_ATTACHED(SimCardDevice)
    QML_UNCREATABLE("由对应 QML 官邸创建和拥有")
    // 采集设置的型号范围与初值；可编辑值由 QML 官邸持有。
    Q_PROPERTY(QVariantMap acquisitionParameters READ acquisitionParameters CONSTANT)
    // ── 架在哪台云台上：名单（主名称、副名称）+ 选中的下标；QML 组合显示 ──
    Q_PROPERTY(QVariantList mountOptions READ mountOptions NOTIFY mountOptionsChanged)
    Q_PROPERTY(int mount READ mount WRITE setMount NOTIFY mountChanged)
    // ── 场景：风场源是对象（页面直接编辑它），其余是数 ──
    Q_PROPERTY(WindFieldSource *field READ fieldSource CONSTANT)
    Q_PROPERTY(double cnrAt1km READ cnrAt1km WRITE setCnrAt1km NOTIFY sceneChanged)
    Q_PROPERTY(double extinction READ extinction WRITE setExtinction NOTIFY sceneChanged)
    Q_PROPERTY(double overlapRange READ overlapRange WRITE setOverlapRange NOTIFY sceneChanged)
    Q_PROPERTY(double floorCounts READ floorCounts WRITE setFloorCounts NOTIFY sceneChanged)
    Q_PROPERTY(double peakWidth READ peakWidth WRITE setPeakWidth NOTIFY sceneChanged)
    Q_PROPERTY(double temperature READ temperature WRITE setTemperature NOTIFY sceneChanged)
    Q_PROPERTY(double lowC READ lowC WRITE setLowC NOTIFY sceneChanged)
    Q_PROPERTY(double highC READ highC WRITE setHighC NOTIFY sceneChanged)
    Q_PROPERTY(double dbPerDegBelow READ dbPerDegBelow WRITE setDbPerDegBelow NOTIFY sceneChanged)
    Q_PROPERTY(double dbPerDegAbove READ dbPerDegAbove WRITE setDbPerDegAbove NOTIFY sceneChanged)
    Q_PROPERTY(double penaltyDb READ penaltyDb NOTIFY sceneChanged) // 只读：当前环境折减，页面回显
    Q_PROPERTY(int seed READ seed WRITE setSeed NOTIFY sceneChanged)
    Q_PROPERTY(int rows READ rows CONSTANT)
    Q_PROPERTY(int cols READ cols CONSTANT)
    Q_PROPERTY(double gateSpacing READ gateSpacing CONSTANT)

public:
    static SimCardDevice *qmlAttachedProperties(QObject *owner) { return new SimCardDevice({}, owner); }
    // 页面初建时复用最新帧；时间、层数组和周期通知完全相同。
    Q_INVOKABLE void publishTruth()
    {
        if (const auto profile = slot_.latest()) Q_EMIT truthReady(profile);
    }
    // JSON 配置一次写完后，立即把完整场景纳入采样历史。
    Q_INVOKABLE void captureScene() { onTick(); }
    using Mounts = std::vector<std::reference_wrapper<GMBDeviceBase>>;

    // mounts = 可以架上去的云台（装配根交，次序即页面次序）；出生选中虚拟云台（没有就选第一台），连接时才架上去起采样器。
    explicit SimCardDevice(Mounts mounts, QObject *parent = nullptr)
        : DAQDevice(QStringLiteral("虚拟采集卡"), QStringLiteral("仿真 · 风场 → 功率谱"), parent)
    {
        setObjectName(QStringLiteral("SimCard")); // ASCII 标识：落盘会话目录名用
        for (GMBDeviceBase &gimbal : mounts) addGimbal(&gimbal);
        tick_.setInterval(SimCardConfig::fieldTick);
        connect(&tick_, &QTimer::timeout, this, &SimCardDevice::onTick);
        onTick(); // 出生先出业务快照；页面初建经 publishTruth 取得同一拍的真值列
        tick_.start();
        connect(&field_, &WindFieldSource::changed, this, &SimCardDevice::onFieldChanged);
        startProcessing();
    }

    ~SimCardDevice() override { PreDestruction(); }

    [[nodiscard]] QVariantMap acquisitionParameters() const { return {{"accumMin", SimCardConfig::AccumSecMin}, {"accumMax", SimCardConfig::AccumSecMax},
                 {"accumDefault", SimCardConfig::AccumSecDefault}, {"saveDirDefault", QUrl::fromLocalFile(QDir::currentPath())}}; }

    Q_INVOKABLE [[nodiscard]] QVariantMap chartParameters() const
    {
        const QVariantMap truthConfiguration{
            {"heightBins", rows()}, {"maxHeight", rows() * gateSpacing()},
            {"windowSeconds", SimCardConfig::truthWindowSec},
            {"columnSeconds", std::chrono::duration<double>(SimCardConfig::fieldTick).count()}};
        const auto plotConfiguration = DAQChartSpecification::make(
            SimCardConfig::geometry.row, SimCardConfig::geometry.col, SimCardConfig::gateSpacingM,
            -static_cast<int>(SimCardConfig::geometry.wsAbsMax()),
            static_cast<int>(SimCardConfig::geometry.wsAbsMax()), SimCardConfig::maxCounts);
        return {{"truthConfiguration", truthConfiguration},
                 {"plotConfiguration", plotConfiguration}};
    }

    // 云台 QML 官邸逐台登记；设备退场先断开当前采样链，再撤销借用。
    Q_INVOKABLE void addGimbal(QObject *object)
    {
        auto *gimbal = dynamic_cast<GMBDeviceBase *>(object);
        if (!gimbal || std::ranges::any_of(mounts_, [&](const auto &m) { return &m.get() == gimbal; })) return;
        mounts_.emplace_back(*gimbal);
        if (object->objectName() == QStringLiteral("SimGimbal")) mount_ = static_cast<int>(mounts_.size()) - 1;
        connect(&gimbal->device(), &DeviceBase::retiring, this, [this, gimbal] {
            const auto *selected = mounts_.empty() ? nullptr : &mounts_[mount_].get();
            if (selected == gimbal) close();
            std::erase_if(mounts_, [&](const auto &m) { return &m.get() == gimbal; });
            mount_ = 0;
            for (int i = 0; i < static_cast<int>(mounts_.size()); ++i)
                if (&mounts_[i].get() == selected) mount_ = i;
            Q_EMIT mountOptionsChanged();
            Q_EMIT mountChanged();
        });
        Q_EMIT mountOptionsChanged();
        Q_EMIT mountChanged();
    }

    // ── QML 的读口 ──
    [[nodiscard]] QVariantList mountOptions() const
    {
        QVariantList options;
        for (const auto &m : mounts_)
        {
            const auto &device = m.get().device();
            options << QVariantMap{{"name", device.displayName()}, {"subtitle", device.subtitle()}};
        }
        return options;
    }
    [[nodiscard]] int mount() const noexcept { return mount_; }
    // 换云台：连着就即刻重架，没连就记下、下次连接生效；设备被占用（任务或单卡测试）时（锁着）拒绝，界面按 value 弹回；没变 / 越界也不动。
    void setMount(const int i)
    {
        if (i == mount_ || i < 0 || i >= static_cast<int>(mounts_.size()))
            return;
        if (!this->device().lockMatches({}))
        {
            Q_EMIT this->device().logSource().logMessage(QStringLiteral("设备正被「%1」占用，不能换云台").arg(this->device().holder()));
            return;
        }
        mount_ = i;
        if (this->device().isOpen())
            open();
        else
            Q_EMIT this->device().logSource().logMessage(QStringLiteral("架在：%1 · %2（连接时生效）")
                                             .arg(mounts_[i].get().device().displayName(), mounts_[i].get().device().subtitle()));
        Q_EMIT mountChanged();
    }
    [[nodiscard]] WindFieldSource *fieldSource() { return &field_; }
    [[nodiscard]] double cnrAt1km() const noexcept { return det_.cnrAt1kmDb; }
    [[nodiscard]] double extinction() const noexcept { return det_.extinctionDbPerKm; }
    [[nodiscard]] double overlapRange() const noexcept { return det_.overlapRangeM; }
    [[nodiscard]] double floorCounts() const noexcept { return det_.floorCounts; }
    [[nodiscard]] double peakWidth() const noexcept { return det_.peakWidthBins; }
    [[nodiscard]] double temperature() const noexcept { return env_.temperatureC; }
    [[nodiscard]] double lowC() const noexcept { return deg_.lowC; }
    [[nodiscard]] double highC() const noexcept { return deg_.highC; }
    [[nodiscard]] double dbPerDegBelow() const noexcept { return deg_.dbPerDegBelow; }
    [[nodiscard]] double dbPerDegAbove() const noexcept { return deg_.dbPerDegAbove; }
    [[nodiscard]] double penaltyDb() const { return deg_.penaltyDb(env_); }
    [[nodiscard]] int seed() const noexcept { return static_cast<int>(seed_); }
    [[nodiscard]] static int rows() { return SimCardConfig::geometry.row; }
    [[nodiscard]] static int cols() { return SimCardConfig::geometry.col; }
    [[nodiscard]] static double gateSpacing() { return SimCardConfig::gateSpacingM; }
    void setCnrAt1km(const double v) { assignScene(det_.cnrAt1kmDb, v, QStringLiteral("1 km 处 CNR（dB）")); }
    void setExtinction(const double v) { assignScene(det_.extinctionDbPerKm, v, QStringLiteral("消光（dB/km）")); }
    void setOverlapRange(const double v) { assignScene(det_.overlapRangeM, v, QStringLiteral("重叠距离（m）")); }
    void setFloorCounts(const double v) { assignScene(det_.floorCounts, v, QStringLiteral("单帧噪声底（计数）")); }
    void setPeakWidth(const double v) { assignScene(det_.peakWidthBins, v, QStringLiteral("谱峰宽（频点）")); }
    void setTemperature(const double v) { assignScene(env_.temperatureC, v, QStringLiteral("温度（℃）")); }
    void setLowC(const double v) { assignScene(deg_.lowC, v, QStringLiteral("舒适区下限（℃）")); }
    void setHighC(const double v) { assignScene(deg_.highC, v, QStringLiteral("舒适区上限（℃）")); }
    void setDbPerDegBelow(const double v) { assignScene(deg_.dbPerDegBelow, v, QStringLiteral("低温折减（dB/℃）")); }
    void setDbPerDegAbove(const double v) { assignScene(deg_.dbPerDegAbove, v, QStringLiteral("高温折减（dB/℃）")); }
    void setSeed(const int v)
    {
        const auto s = static_cast<unsigned>(v);
        if (s == seed_)
            return;
        seed_ = s;
        Q_EMIT this->device().logSource().logMessage(QStringLiteral("随机种子：%1").arg(v));
        Q_EMIT sceneChanged();
    }

Q_SIGNALS:
    void truthReady(VerticalWindProfilePtr profile);
    void specReady(const QVector<double> &values);
    void wsReady(const QVector<double> &values);
    void cnrReady(const QVector<double> &values);
    void profileReady(VerticalWindProfilePtr profile);
    void mountChanged();
    void mountOptionsChanged();
    void sceneChanged();

public Q_SLOTS:
    // 连接固定所选云台；查询在主线程同步执行，QML 图表消失即缺数据。
    bool open()
    {
        if (mounts_.empty())
        {
            Q_EMIT this->device().logSource().logMessage(QStringLiteral("没有可架的云台，虚拟采集卡起不来"));
            return false;
        }
        GMBDeviceBase &g = mounts_[mount_].get();
        LidarSimCardHandle::ParamType p;
        p.sourceKey = g.objectName();
        p.capture = [this, &g](const RxTime begin, const RxTime end) -> std::optional<SimSamplingInput> {
            SimSamplingInput input{readAngles(g, begin, end), slot_.recent(begin, end)};
            if (input.poses.empty() || input.scenes.empty()) return std::nullopt;
            return input;
        };
        const bool accepted = this->device().open(p);
        if (accepted)
            Q_EMIT this->device().logSource().logMessage(QStringLiteral("架在：%1 · %2").arg(g.device().displayName(), g.device().subtitle()));
        else
            Q_EMIT this->device().logSource().logMessage(QStringLiteral("虚拟采集卡已在运行")); // 已连接时不改当前请求。
        return accepted;
    }
    void close() { this->device().close(); }
    void startTest(const int accumSec, const bool save, const QUrl &folder)
    {
        DAQDevice::startTest(accumSec, save, folder, AcqSaver::sessionDir(
            QDateTime::currentDateTime(), this->objectName() + QStringLiteral("Test")));
    }
    void cancelTest() noexcept { DAQDevice::cancelTest(); }

private:

    void run() override { consumeFrames(makeSimCardProcessor(this)); }

    void PreDestruction() noexcept final
    {
        static_assert(FinalDevice<SimCardDevice, decltype(&SimCardDevice::PreDestruction)>);
        tick_.stop(); // 主线程定时器；预析构不泵事件，不会在等待期间调用历史查询。
        DAQDevice::PreDestruction();
    }

    // [主] 风廓线按只读整帧入环；参数按值存槽。后台只拿本次窗口的份额，不再共享可变历史。
    struct SceneSlot
    {
        void append(VerticalWindProfilePtr profile, SimSceneSettings settings)
        {
            const auto slot = profiles_.append(std::move(profile));
            settings_[slot] = std::move(settings);
        }
        [[nodiscard]] VerticalWindProfilePtr latest() const { return profiles_.latest(); }
        [[nodiscard]] std::vector<SimSceneStamp> recent(const RxTime begin, const RxTime end) const
        {
            if (end <= begin || profiles_.size() == 0 || profiles_.at(0)->at > begin) return {};
            for (std::size_t i = 1; i < profiles_.size(); ++i)
                if (profiles_.at(i)->at < profiles_.at(i - 1)->at) return {};
            std::size_t first = 0;
            while (first + 1 < profiles_.size() && profiles_.at(first + 1)->at <= begin) ++first;
            std::vector<SimSceneStamp> result;
            result.reserve(profiles_.size() - first);
            for (std::size_t i = first; i < profiles_.size() && profiles_.at(i)->at < end; ++i)
                result.push_back({profiles_.at(i), settings_[profiles_.slotAt(i)]});
            return result;
        }
    private:
        VerticalWindProfileHistory profiles_{static_cast<std::size_t>(
            std::chrono::seconds(SimCardConfig::AccumSecMax) / SimCardConfig::fieldTick) + 2};
        std::vector<SimSceneSettings> settings_ = std::vector<SimSceneSettings>(profiles_.capacity());
    };

    // 探测器与折减的出生值：页面初值从这两口取，一份账。
    [[nodiscard]] static LidarForward::Detector defaultDetector()
    {
        const auto &g = SimCardConfig::geometry;
        return {.rows = g.row,
                .cols = g.col,
                .gateSpacingM = SimCardConfig::gateSpacingM,
                .freq2WsSlope = SimCardConfig::freq2WsSlope,
                .freq2WsIntercept = g.freq2WsIntercept(),
                .cnrAt1kmDb = SimCardConfig::cnrAt1kmDefault,
                .extinctionDbPerKm = SimCardConfig::extinctionDefault,
                .overlapRangeM = SimCardConfig::overlapRangeDefault,
                .floorCounts = SimCardConfig::floorCountsDefault,
                .peakWidthBins = SimCardConfig::peakWidthDefault,
                .accumFrames = 0, // 句柄按每次的累积秒数填
                .maxCounts = SimCardConfig::maxCounts};
    }
    [[nodiscard]] static LidarForward::Degradation defaultDegradation()
    {
        return {.lowC = SimCardConfig::lowCDefault,
                .highC = SimCardConfig::highCDefault,
                .dbPerDegBelow = SimCardConfig::dbPerDegBelowDefault,
                .dbPerDegAbove = SimCardConfig::dbPerDegAboveDefault};
    }
    // 场景参数的写口只改值、记日志：下一拍进快照。
    void assignScene(double &slot, const double v, const QString &what)
    {
        if (slot == v)
            return;
        slot = v;
        Q_EMIT this->device().logSource().logMessage(what + QStringLiteral("：%1").arg(v));
        Q_EMIT sceneChanged();
    }
    void onFieldChanged() { Q_EMIT this->device().logSource().logMessage(QStringLiteral("风场：") + field_.describe()); }

    // [主] 生成固定高度网格的一帧；采样历史和风羽图共用同一层数组。
    void onTick()
    {
        const RxTime time = std::chrono::system_clock::now();
        const auto at = std::chrono::steady_clock::now();
        VerticalWindProfileUnit grid{time, RangeGate::rangeOf(0, SimCardConfig::gateSpacingM),
            SimCardConfig::gateSpacingM, std::vector<LinAlg::Vec3>(SimCardConfig::geometry.row)};
        const VerticalWindProfilePtr profile = std::make_shared<const VerticalWindProfileUnit>(
            field_.field(std::move(grid), at, seed_));
        slot_.append(profile, {det_, penaltyDb(), SimCardConfig::framesPerSec, seed_});
        Q_EMIT truthReady(profile);
    }

    // [主] 后端由各自 QML 官邸直接拥有；只向自己的官邸请求数据，不接触云台图窗。
    [[nodiscard]] WorldPoseHistory readAngles(GMBDeviceBase &g, const RxTime begin, const RxTime end) const
    {
        Q_ASSERT(QThread::currentThread() == thread());
        if (!parent() || !g.parent() || !g.device().isOpen()) return {};
        QVariant result;
        if (!QMetaObject::invokeMethod(parent(), "worldHistory", Qt::DirectConnection,
                                      Q_RETURN_ARG(QVariant, result),
                                      Q_ARG(QVariant, QVariant::fromValue(g.parent())),
                                      Q_ARG(QVariant, QVariant::fromValue(begin)),
                                      Q_ARG(QVariant, QVariant::fromValue(end)))) return {};
        return result.value<WorldPoseHistory>();
    }

    // ── 可架的云台与选中项 ──
    Mounts mounts_;
    int mount_ = 0;

    // ── 场景设置与主线程历史 ──
    WindFieldSource field_{nullptr};
    LidarForward::Detector det_ = defaultDetector();
    LidarForward::Environment env_{SimCardConfig::temperatureDefault};
    LidarForward::Degradation deg_ = defaultDegradation();
    unsigned seed_ = SimCardConfig::seedDefault;
    SceneSlot slot_;
    QTimer tick_; // [主] 风场节拍（SimCardConfig::fieldTick）：出生即跑；声明在最末，最先亡
};
