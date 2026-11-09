# SimCardDevice

2026-11-09 用户定：云台族基不再是 QObject。虚拟卡登记云台仍收 QObject* 并交叉转换验族，退场监听设备对象本身的 destroyed；取名称/QML 父页时经同一对象交叉转换到 QObject。页面 gimbals/gimbal 类型改为 QtObject。本卡（DAQ 族）不变。

2026-11-03 上传接口：官邸仍拥有唯一具体 Core；Core 继承 `Device<HandleT, CollectorT>`，Device 直接继承 Handle 并重写 onData。Core 只提供 Qt 属性、锁信号与末端预析构；不增设上传回调、状态缓存或逐包信号转发。协议、Collector 与 UI 逻辑保持。

全功能与 simonly 页面共用的采集图窗统一命名为 `功率谱`、`径向风`、`风廓线`，径向风页继续包含风速和 CNR；独立的 `风场真值` 图保持原名称。仅调整名称，不改历史采样、处理与保存接口。

LiDAR 的独立官邸：具体 C++ 后端、Collector/协议/处理逻辑及其 QML 页面在同一仓库维护。来自 LiDAR_DataACQ master `5cc6d65`，分仓后的全功能入口保持原行为。

## 边界与接入

- QML 模块：`Lidar.SimCard`；页面：`SimCardModule`。
- CMake 目标：`Lidar::SimCardDevice`；静态插件：`LidarSimCardDeviceplugin`，最终可执行文件显式 `Q_IMPORT_QML_PLUGIN(LidarSimCardDevicePlugin)`，并导入 TaskUI/UI/Charts 的静态插件。
- 页面以 attached 对象拥有唯一具体后端，后端不能在 QML 直接创建。设备通过只读 `family` 借出族引用；任务页面只接收族列表，不依赖设备官邸。
- 依赖由 `cmake/Dependencies.cmake` 固定到提交；可用标准 FetchContent 源码覆盖做联调。多个官邸并用时，先开启所有需要的 HandleCore/TaskCore 可选项，再配置共享库，避免已创建目标后补开模块。
- 保留 QML 角度/风场历史源；不显示窗口时仍需创建历史对象，但可以关闭绘制。

## 构建与验证

C++20、CMake >= 3.28、Qt 6.11.2（Quick/Qml/GuiPrivate/ShaderTools）。Windows 使用 MSVC x64；Linux 使用 GCC，Qt 与原生依赖由环境提供。

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="<Qt前缀;需要的原生依赖前缀>"
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
cmake --build build --config Release --target LidarSimCardDevice_qmllint
```

独立测试逐头编译、创建真实页面并销毁后端；不连接实物设备。硬件收发仍需对应设备实测。主程序负责跨官邸仿真、任务及取消链回归。

## 许可

MIT，Copyright (c) 2026 周涛（ZHOU TAO）。Qt、libserialport、libpcap/Npcap 等依赖各自授权，MIT 不替代这些许可；详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。本仓库不包含第三方 SDK、运行库或驱动。

## 两套 QML 官邸（2026-10-27）

同一 QML 模块同时提供 `SimCardModule`（全功能）和 `SimCardSimOnlyModule`（数字样机）。每个页面以 Qt attached 机制拥有唯一的同类型 C++ 后端，没有第二套设备/任务实现。全功能主程序继续使用原入口。

simonly 页面接收唯一云台设备 gimbal（QtObject），保留完整风场、探测器、环境及四个图窗，不提供单卡试采、挂载选择或保存设置。两套页面共用 SimWindFields、SimDetectorFields、SimCardPlots。配置应用后 captureScene 同步记录完整场景，再由应用连接；历史查询仍经云台 QML 官邸。field 暴露 WindFieldSource 的实际只读类型，设置和采样机制不变。

[LiDAR_DataACQ-simonly](https://github.com/Lateef-ZHOU/LiDAR_DataACQ-simonly) 通过固定版本装配这些入口，负责 JSON、命令行、设备就绪后启动和系统信号取消。窗口隐藏仍保留历史，不触发实际绘制。测试创建两套真实页面并检查后端唯一性、销毁和精简分区；跨设备的三任务采样与保存由应用集成测试覆盖。

## 资源阶段通知（2026-11-02）

连接采用 HandleCore 的 `connectionChanged(ResourcePhase)`；主动关闭前在所属线程同步发 `AboutToRelease`，完成事件用于回读现状。页面借出已有 `deviceState` 机制对象，不再保存 isOpen/isLocked/holder 的 QML 转交属性；标签通过连接/锁完成阶段回读，不恢复通用 statusChanged。固定依赖已同步；本轮提交尚未推送，验证使用显式本地源码覆盖。


2026-11-03 验证：Windows Qt 6.11.2 主程序 Release 编译、15 组 QML 检查、跨官邸生命周期和三任务回归 7/7 通过。接口联调使用相邻源码覆盖，新依赖固定为本地提交，尚未推送；未连接实物设备或更新板卡。

## 官邸自有的仿真 Handle（2026-11-03）

Handle、内部协议及其测试由 HandleCore 原样迁回；头文件名与目标名保持，运行、重连、取消及关闭逻辑不变。`handle/` 可独立构建，仅需 Qt Core >= 6.8.3、C++20 与 HandleCore，不依赖 QML。完整官邸仍需上面的 Qt Quick 依赖。

```sh
cmake -S handle -B build-handle -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="<Qt前缀>"
cmake --build build-handle --config Release --parallel 4
ctest --test-dir build-handle -C Release --output-on-failure
```

本仓库提供 `Lidar::SimCard`；完整官邸构建自动包含该目标及 `tests/handle`。HandleCore 不再提供仿真模块开关。仿真协议仍只用于进程内部，不作为网络协议。

- `Generator` 提供 `using Input = ...` 和 `QByteArray synthesize(quint64 requestId, RxTime begin, RxTime end, const Input &)`。concept 要求默认构造及准确返回类型；Input 可移动即可，合成器无需可复制或移动。合成器按值放在连接中，在工作线程构造、连续使用、销毁，不能每次请求重建；随机数等算法状态随连接保持。重连会建立新的合成器。
- `SimCardParams<Input>`：`sourceKey` 表示数据源身份，`capture(begin,end)` 在所属线程同步返回 `optional<Input>`。相等比较只看 sourceKey；已连接且同 key 的 start 不替换查询函数，换源用新 key 或先 stop。缺少 capture 在打开时失败；回调所借用的对象必须存活到 stop 返回，输入须自持快照，后台不能回访 UI。
- 收到 Accumulate 即固定 system_clock 起止窗口，用所属线程单发 PreciseTimer 等待，到点取数；界面忙导致迟到时仍查询原窗口。累积秒数须为 1..INT_MAX/1000。快照移动进绑定原连接 QObject 的排队闭包，worker 调用合成器，非空结果按原请求窗口结束时刻交付。Handle 不认识风场、角度、采样算法或输出谱布局；请求号与具体输出字节的关联由合成器落实。
- `nullopt` 不启动合成，空 QByteArray 不交付，两者均静默。Stop 仅撤销请求号匹配的待计时请求，旧 Stop 不影响新请求。已经进入后台的工作保持原请求号，可完成交付，由应用 Collector 拒绝过期结果；不新增取消令牌、阻塞取数、超时或排空。
- 同步关闭先撤 timer/pending，再向原连接队列投递退出并等待工作线程；排队闭包不强持连接、不借新连接。HandleBase 的首连失败、重连、RX/TX 与关闭机制未改。`SimCardWire::Command` 保留原生 POD 布局和 Accumulate/Stop，编解码仅接受 Command 和结果头的 quint64 请求号；它是进程内部协议，不作为跨 ABI 网络协议。

本轮验证：Windows/MSVC/Qt 6.11.2 完整官邸 Release 与独立头编译通过，页面及 Handle CTest 2/2；Qt Core 独立入口 1/1。Linux/GCC 16.2.1/Qt Core 6.8.3 独立入口 1/1，未测 Linux QML 页面或 ARM。主程序生命周期与三任务回归 7/7、Collector 验证 2/2、八秒冒烟通过，两官邸 qmllint 完成并保留既有静态分析提示。证据 build-migration、build-handle 及 Linux ~/.cache/lidar-sim-handle-move-20261103。固定本地 HandleCore 与 TaskCore 提交，联调使用源码覆盖；未推送。

## 源码目录（2026-11-03）

公开文件直接放在 `include/`，引用时使用文件名，不再携带原单仓库的业务目录前缀。CMake 目标名称、C++ 类型和 QML 模块名保持不变；使用方须同步更新头文件路径及固定依赖版本。

目录整理验证（2026-11-03）：Windows：公开头独立编译、页面和 Handle 测试 2/2 通过；Linux Qt Core 6.8.3：独立 Handle 构建、CTest 1/1 通过。本轮通过本地源码覆盖验证，固定依赖同步更新到本地提交；尚未推送，不代表已验证远端拉取。

## 2026-11-04：设备日志直接接线（用户定）

删除具体设备的 logSource 自引用属性，QML 直接监听现有设备对象的 logMessage/connectionChanged；设备失败日志直接调用继承自 Handle 的信号。协议、收发、重连和关闭顺序不变。 本轮仅本地整理，未推送；Windows 集成验证记录见主程序维护操作。

## 2026-11-04：设备身份与占用查询（用户定）

设备名称/副标题由构造传入 DeviceBase 的公开 const 成员，Qt 通过 MEMBER CONSTANT 直接暴露；占用查询只保留 holder，Qt 用无效 QVariant 表示空闲、字符串表示占用，删除 isLocked 属性。既有连接、重连、收发和取消逻辑保持。

本轮仅本地整理；集成验证记录见主程序 docs/maintenance/validation.md，不推送。


## 2026-11-04：Handle 连接入口统一（用户定）

仿真 Handle 验证同步使用 open/close 连接入口；定时器、合成、采集任务及重连关闭行为不变。

Windows 独立 Handle 构建与 CTest 1/1 通过，主程序增量构建和八秒离屏冒烟通过。 依赖固定到相邻本地提交，尚未推送，联调使用源码覆盖。

## 2026-11-04：进料在消费端取值（用户定）

Collector 的 onFeed 接收 optional<QByteArray> 的 const 引用，由本型号解析端取出字节。空输入由 DeviceCore 公共 feed 拦截；请求号、解析及收齐规则不变。

本轮仅本地整理，不推送；集成验证见主程序维护记录。

2026-11-05 表单统一（用户定）：舒适区双输入列采用 FieldStrip 权重，范围和输入值仍由官邸管理。 公共视觉规格由 UICore 提供，不增加业务状态。Windows 主程序增量构建、八秒离屏冒烟通过；固定依赖在本地更新，暂不推送。


### 2026-11-05 公共视觉与连接布局

说明与小标题使用 UICore HintText/Subheading，当前折减使用 ReadoutValue；风场配置、布局和采样行为不变。 本轮仅本地整理，未推送；Windows 集成验证记录见主程序维护操作。

### 2026-11-05 表单行距补齐

SimWindFields 与 SimDetectorFields 的普通行距明确使用 UICore FormStyle.rowSpacing，已有组间距、字段布局和业务参数保持。主程序 Windows 增量构建及八秒冒烟通过；仅本地提交，未推送。


### 2026-11-05 连接间距、累积默认值与径向风页面整理

虚拟卡默认累积时间统一为1秒；显式输入及仿真采样行为保持。

主程序 Windows Release 增量构建、八秒桌面冒烟及四窗口装配检查通过；运行 stderr 为空，保留既有 CDL5/Qt 生成代码 C4702 编译警告。仅本地验证，未连接硬件、未推送。
