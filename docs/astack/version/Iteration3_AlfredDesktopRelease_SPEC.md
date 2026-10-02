# Iteration3：QClip Alfred 增强能力与三端发布

> **文档信息**
>
> | 字段 | 值 |
> |---|---|
> | 文档类型 | SPEC |
> | 文档状态 | 开发中 |
> | 创建日期 | 2026-09-28 |
> | 最后更新 | 2026-10-02 |
> | 作者 | zerovoxxx |
> | 关联文档 | [迭代索引](../INDEX.md)、[公共架构与面板](Iteration1_ClipboardPalette_SPEC.md)、[全界面与兼容](Iteration2_CopyQCompatibility_SPEC.md)、[项目入口](../../../CLAUDE.md) |
> | 一句话目标 | 补齐 Snippet、自动展开和连续复制合并，完成品牌/数据迁移、性能及三端安装包验收，使三个阶段共同满足首版完整范围。 |

## 目标与非目标

本阶段依赖 Iteration1 的窗口/粘贴/平台输入原型，以及 Iteration2 的编辑、设置、插件与历史策略。复用已建立的 QML 组件和 C++ 接入契约，新增业务仍接入同一 CopyQ 服务进程及现有辅助进程。

本 SPEC 拥有 A4/A5/A6、P9/P10/P11/P13/P14；A7 的平台差异和 C4 的品牌路径迁移在此完成最终验收。P1–P8/P12 仍由对应 SPEC 维护实现证据，本阶段在发布候选上做相关集成回归，不复制三套验收定义。

- 目标：Snippet 集合、关键词、富文本、动态内容、导入导出、后台展开、应用例外、合并与反馈；三端产品可用，Windows/macOS 优先。
- 非目标：不实现 Alfred Workflow 编辑器、启动器或云服务，不更换 CopyQ 核心/脚本引擎；Snippet 触发自动化复用既有命令。
- 不把缺少权限、不可用平台或未迁移功能记为通过。公开发布、签名和商店上传依据实际授权/凭据执行，产物构建、安装验证和已发布状态分别记录。

## 能力基线与设计假设

能力参照 [Alfred Snippets](https://www.alfredapp.com/help/features/snippets/)、[自动展开](https://www.alfredapp.com/help/features/snippets/auto-expansion/)、[高级设置](https://www.alfredapp.com/help/features/snippets/advanced/)、[动态占位符](https://www.alfredapp.com/help/workflows/advanced/placeholders/)及[剪贴板合并](https://www.alfredapp.com/help/features/clipboard/#using-clipboard-merging)。沿用既有 Alfred 5.8.1 基线；来源用于行为对齐，未观察的默认值/键位不凭印象补全。

| 事项 | 工程默认建议及确定时点 |
|---|---|
| 自动展开 | 初始关闭，由用户显式启用；每条 Snippet 可单独允许展开，应用例外始终有效。I3-1 固定启用/授权流程。 |
| 关键词 | 区分大小写、集合前后缀与条目关键词合成有效关键词；冲突必须可见，不能随机选取。I3-1 固定冲突/词边界及删除原关键词规则。 |
| 格式 | 保留纯文本和富文本正文，复用 Iteration2 编辑能力；动态替换只作用于这次输出，原模板保持不变。 |
| 集合交换 | 首先保证版本化 QClip 数据无损往返；Alfred 文件格式互通需真实样例及单独兼容证据，不因提供导入按钮就宣称兼容。 |
| 品牌标识 | 显示名 QClip 已确认；应用 ID、可执行文件、配置/session/IPC、自动启动和 CLI 兼容入口在 I3-1 形成完整映射后一次实施。 |
| 性能阈值 | 根据 Iteration1 基线，在优化前冻结测试硬件、数据集及各指标阈值；不能测完结果后反向设成“通过”。 |
| Linux 会话 | 完整基线优先 X11；Wayland 的已支持能力继续保留，新增输入能力按桌面/协议逐项声明和验证。 |

## 本轮固定的实施约束（2026-09-30）

- 数据：版本 1；集合/条目使用 UUID，文件位于 `configurationFilePath("_snippets.dat")`。通过现有 `serializeData` 写入完整内联 MIME 字节和版本化记录，`QSaveFile` 原子提交；不引用历史外置数据文件，因此历史清理不会删 Snippet。复用共享加密密钥，锁定或文件损坏时禁止覆盖；未知记录字段原样保留，未知数据版本拒绝。集合交换和整体备份均带 Snippet 数据。模板编码与存储文件均限制 100 MB，超限保存失败并保留旧文件；不提交自身无法读取的文件。
- 关键词：大小写敏感，前缀 + 关键词 + 后缀；空关键词只能显式使用。重复有效关键词显示冲突并停止自动展开/引用；最长后缀匹配，默认要求前一字符不是字母/数字/下划线，允许用户开启任意位置匹配。删除单位为已确认关键词的 Unicode 字符；未确认 IME/安全输入暂停，编辑窗口和本进程窗口全部排除。
- 模板：采用 Alfred 官方页面的 date/time/datetime、ISO、日期运算、clipboard/偏移、random/范围/候选/UUID、连续修饰、cursor 和 snippet 引用语法。未知占位符保留字面量；已知格式错误返回错误；引用只展开一层。动态值不再次解释为模板，富文本使用 QTextDocument/QTextCursor 替换正文并保留格式；Workflow 变量不自动构造。日期采用 macOS CoreFoundation / Windows 系统 ICU / Linux ICU，默认 medium，short/medium/long/full 独立处理；不支持的字段明确报错。Linux 构建依赖 libicu-dev，Windows 安装最低 10.0.18362（系统 ICU）。
- 展开：初始关闭；按用户显式启用启动平台监听，状态展示授权/协议不可用。生成先于删除，使用现有 clipboard provider 完成通知后才注入；目标/输入代际/权限改变则取消。剪贴板恢复初始开启、延迟 500ms，仅 provider 和实际 MIME 仍属于本次写入时恢复，且再次检查用户复制事件；失败不盲目重放关键词。命令触发采用既有 action/Command 入口。
- 合并：初始关闭；macOS Cmd+C+C，Windows/X11 Ctrl+C+C，同一 modifier 持有期间两次 C 的工程阈值 400ms（官方未公开数值，不声称其默认值）。候选剪贴板与手势窗口绑定，重复通知不重复合并；分隔支持无分隔、空格、换行，默认换行；反馈在 provider 写入成功后，声音可关闭。非文本、敏感/忽略应用和自身写入清除候选。
- 品牌：显示 QClip，二进制 Windows/Linux `qclip`、macOS `QClip.app/Contents/MacOS/QClip`，bundle/desktop ID `io.github.zerovoxxx.QClip`；配置/状态/session/IPC 使用 `qclip[-session]`。GNOME 扩展 UUID 为 `qclip-clipboard@zerovoxxx.github.com`，D-Bus 服务/接口采用 `io.github.zerovoxxx.QClip` 前缀，独立于 CopyQ。保留 COPYQ_* 环境变量、脚本 API、MIME、插件接口和历史序列化格式；不创建全局 copyq 别名、不改 CopyQ 自动启动。旧数据必须用户显式选取隔离源目录后导入，源保持可用，源目录内文件按 UUID 放入 QClip 独立导入目录，设置/命令路径改写，备份纳入这些资产并使用可移植路径。源加密材料只用于解锁与保存原始副本，接收端以自己的共享密钥重编码；导入加密源前须先启用并解锁 QClip 加密。目录外引用拒绝预检，避免继续共享 CopyQ 的可写文件。
- 性能：本机 Apple Silicon/macOS 与 Debian arm64 X11 软件渲染容器分别记录，冻结 1000/10000 条、固定图片、30 次样本。工程门：常驻呼出 p95 ≤ 250ms、搜索更新 p95 ≤ 200ms、复制到历史 p95 ≤ 1000ms、确认到粘贴 p95 ≤ 1000ms、冷启动 ≤ 3000ms；同机 CopyQ 对照缺失时不宣称比较通过。隐藏 UI 不运行动画；CPU/进程组内存/帧耗时/包体积独立输出，不以模型耗时替代端到端耗时。

导入资产在既有 CopyQ v4/v5 备份 metadata 中增加 files 字段：预检所有相对路径和源文件，总资产上限 1 GiB，原子写入各资产文件；配置路径用 qclip-import:// 标识在导入时重定位。CopyQ 历史外置文件的绝对路径若不在所选副本内，按现有四段 SHA-256 文件路径查找副本内唯一文件；不读取副本外原路径，缺失或歧义拒绝导入。`copyqimport.cpp` 建立文件索引，`serialize.cpp` 物化 MIME；`copyQExternalMigration` 覆盖原位置删除后的明文/加密副本与缺失文件。部分磁盘写入失败返回错误，保留原数据与源；不会清空原配置。

新增必要位置除下表外还有 `src/gui/clipboardsnippets.{h,cpp}`（QML 承载与既有编辑器）、`src/platform/platforminput.h` 及三端实现（监听和安全键注入）、`src/common/clipboardmerge.{h,cpp}`（可控时序）、`src/tests/tests_snippets.cpp`（独立聚焦目标）及 `utils/check-release.py`（安装产物审计）。实际验证及未实现/未验证边界写在本文，不另建报告。

实际实施还包含 `src/common/snippetdate.{h,cpp}`（原生区域日期格式）、`src/common/copyqimport.{h,cpp}`（旧目录预检/资产重定位）、`src/gui/mainwindow_snippets.cpp`（provider/焦点/输入代际及命令复用）、`src/tests/tests_snippetserver.cpp` / `tests_desktopperformance.cpp`（真实服务/原生接收窗口与性能采集）、`utils/package-source.py`（对应源码与逐文件哈希）。`src/platform/x11/x11platformclipboard.cpp`、GNOME 扩展与测试脚本共同修改协议名称，防止 QClip 安装覆盖 CopyQ 扩展；`utils/bump_version.sh` 指向 QClip AppData。`shared/licenses/` 和三端部署审计保留许可文本；`.gitignore` 排除构建/发布生成物，避免源码归档夹带运行库。

## Snippet 数据与行为契约

### 持久化与界面

Snippet 是持久保存的逻辑实体，不能只是会被普通历史期限/容量清理的标签。历史转 Snippet 是一次显式复制内容；后续编辑 Snippet 不修改原历史，删除历史也不删除 Snippet。

| 数据对象 | 必须表达的信息与不变量 |
|---|---|
| 集合 | 稳定身份、名称、关键词前后缀及启用状态；重命名不丢失关联。 |
| Snippet | 稳定身份、集合身份、标题、关键词、展开启用、正文 MIME；身份不依赖列表行号或内容 hash。 |
| 本次展开上下文 | 目标窗口、已确认输入、时间/区域设置及必要剪贴板/历史快照；一次展开保持同一上下文。 |
| 生成结果 | 输出 MIME 与逻辑光标位置，或明确错误；计算失败不先删除用户已输入内容。 |

- 使用 C++ 逻辑模型，持久层复用现有 QVariantMap、serializeData 和项目文件写入/加密能力；在 I3-1 固定数据版本、文件归属和配置位置，不引入第二套剪贴板历史数据库。
- [ItemStore](../../../src/item/itemstore.h)已有清理与文件迁移机制，新 Snippet 引用的数据文件必须纳入清理、备份、加密及导入导出规则，不能被当作孤立文件删除。旧版本/未知字段和导入失败按既有数据安全方式处理。
- 新 Snippet 界面提供集合/条目创建、编辑、移动、删除、查找、历史转入、预览和显式粘贴；面板可进入 Snippet 浏览，按标题/关键词筛选并保持集合关系。
- 支持集合级导入/导出，重复身份/关键词、缺失集合、部分损坏必须有可复现结果；导入失败不覆盖有效原数据。按用户配置调用现有 CopyQ 命令，保留原脚本入口。

### 动态内容

下面是对齐清单，具体语法用固定基线样例验证；适用于 Snippet 的能力不得只做几个示例后宣称全部完成。

| 类别 | 实施与验证重点 |
|---|---|
| 日期/时间 | date/time/datetime、ISO 形式、格式化和日期运算；固定时钟、时区、区域设置、月末及夏令时样例。 |
| 剪贴板内容 | 当前剪贴板与指定历史偏移的纯文本；历史为空、偏移越界、多 MIME 和本次写入改变剪贴板后的上下文一致性。 |
| 随机内容 | 数值范围、候选项和 UUID；使用可控随机源测试边界，不能让不稳定结果使测试失去断言。 |
| 文本变换 | 大小写、首字母、trim、反转、去音标/非字母数字及连续修饰；中文、emoji 和组合字符不能按字节破坏。 |
| 光标与引用 | cursor 位置、按有效关键词引用其他 Snippet、富文本保留；对齐非递归引用规则，防止循环展开。 |

Workflow 专属上下文不凭空构造；需要命令参数时通过现有 CopyQ 命令明确传入。本功能不执行模板中任意文本作为代码。可识别的模板错误在编辑预览/确认阶段暴露；未知占位符的处理与富文本转义规则在 I3-1 固定并测试。

## 自动展开与连续复制合并

### 后台展开流程

平台输入可行性已在 I1-2 验证，本阶段实现完整业务；共享层处理关键词/模板，Windows/macOS/Linux 平台层提供实际可用输入与注入能力。

1. 仅在功能启用、条目允许、应用不在例外名单、权限和输入状态满足条件时匹配。QClip 自己的搜索/设置/编辑框默认排除。
2. 维护必要的短期输入状态，响应退格、移动光标、切换窗口、粘贴、IME 组词和安全输入；不能把原始按键码直接等同于已提交字符。无法确认组合输入状态时暂停匹配，不能猜测并发送删除。
3. 命中后先生成输出并验证原目标，再删除触发关键词、经统一 MIME/粘贴链路插入、调整光标。目标变化、渲染错误、权限失效或超时即终止，避免删掉无关文本。
4. 标记本程序注入/剪贴板写入来源，避免自己再次触发；保持平台 modifier 状态正确。错误恢复不能靠盲目重放输入。
5. 暂存和恢复剪贴板按基线设置实现；只在剪贴板仍属于本次展开写入时恢复，用户其间新复制的数据不能被旧快照覆盖。富文本/图片也必须保持原 MIME。
6. 纯规则使用可控时钟、输入和剪贴板替身验证；实际焦点、权限、IME、目标应用粘贴和光标位置必须原生验收。日志不持久化用户完整键入内容。

### 连续复制合并

- 提供启用/禁用、合并分隔方式和确认反馈；macOS 对齐 Cmd+C+C，Windows/Linux 采用适用平台的复制组合。触发阈值和候选分隔方式在 I3-1 用基线行为固定。
- 将复制手势和对应剪贴板数据变化关联，保留前一段文本与本次选中文本，合并一次后通过既有写入链路更新；不能把任意两个剪贴板变化事件当作双次复制。
- 普通复制、超时、相同内容重复复制、重复系统通知、图片/文件等非文本、忽略应用和自身写入分别处理。失败时不破坏已有有效历史或不断重复合并。
- 写入成功后再反馈；声音可以关闭，平台采用自身可用的反馈资源。合并不修改 Snippet 模板，且与自动展开共享来源识别/防重入规则。

## 三端发布、兼容迁移与性能

### 品牌与数据

显示品牌使用 QClip；此前普通检索发现[同名应用记录](https://appagg.com/windows/productivity/qclip-43950651.html?hl=zh)，不能宣称名称独占。本阶段工程工作围绕已选品牌执行。

I3-1 固定显示名、安装路径、可执行文件、应用/bundle ID、配置/历史/state、session/IPC、自动启动、卸载、文件关联和 CLI 的完整映射。复用 [app.cpp](../../../src/app/app.cpp)、[config.cpp](../../../src/common/config.cpp) 的现有路径机制；配置/历史中内嵌旧应用名的文件引用也要迁移，不能只改窗口标题。

- QClip 与日常 CopyQ 可并存；导入先读取隔离副本，复制数据并验证后才完成导入，源文件保持可用。部分失败可重试，禁止清空原配置以修复迁移。
- 旧配置、历史、标签、命令、主题、插件设置和加密材料均列入映射；新增 Snippet 进入后续 QClip 备份/恢复流程。不自动重绑或覆盖现有 CopyQ 的全局 CLI/自动启动入口。
- 构建与许可沿用 [RELEASE.md](../../../RELEASE.md)、[LICENSE](../../../LICENSE)、[Windows 安装脚本](../../../shared/copyq.iss)及现有三端 CI，保留上游 GPL-3.0-or-later 和第三方归属信息。

### 平台与性能门

| 平台 | 发布候选必须检查 |
|---|---|
| Windows | 独立安装包 QML/插件依赖、普通权限粘贴、提权目标限制、热键冲突、中文 IME、缩放/多屏、更新/卸载及启动路径。 |
| macOS | Intel/Apple Silicon 对应包、QML/插件、辅助功能/输入授权与撤销、同应用多窗口、自动展开、安全输入、系统主题/材质；签名后的权限行为重新验证。 |
| Linux | 至少声明并验证一个完整桌面会话；X11/Wayland 分别记录热键、监控、焦点、粘贴与展开，不能以一个桌面通过代表全部 Linux。 |

性能对比使用未改动 CopyQ 与 QClip、相同机器及隔离历史（1000/10000 条文本和固定图片样本）。记录冷启动、常驻呼出、搜索更新、复制到历史、确认到粘贴的 p50/p95，隐藏/显示 CPU、进程组内存、动画帧耗时和安装包大小；注明样本次数、Qt/OS/架构/缩放/插件与等待配置。

已有 Windows 激活后等待默认 150ms、macOS 采集默认 500ms 与 5000ms 调度容差的含义见 Iteration1；分别测量这些链路，优化不得破坏焦点/采集可靠性。先分析数据，再处理图片缓存、不可见动画、筛选批次和实际热点；性能结果和失败门槛不能用“Qt 比其他框架快”替代。

## 变更范围与文件清单

本轮开始实施四个里程碑。数据、输入、UI、备份、部署与迁移必然涉及十个以上文件；仅修改这些接入位置，保留现有历史/插件/脚本数据流。

| 操作 | 文件 / 位置 | 必要改动 |
|---|---|---|
| NEW | src/item/snippetstore.h、src/item/snippetstore.cpp | Snippet/集合模型与持久化，复用既有序列化，不复制历史数据库。 |
| NEW | src/common/snippets.h、src/common/snippets.cpp | 关键词、模板及一次展开的纯规则；实际类型/字段实现前再检索同类命名。 |
| NEW | src/gui/qml/Snippets.qml 及按实际需要的组件 | Snippet 浏览、编辑、设置和面板入口，复用 Iteration2 表单/编辑能力。 |
| MODIFY | [src/platform](../../../src/platform)、[qxt](../../../qxt)、[globalshortcutcommands](../../../src/common/globalshortcutcommands.cpp) | 按原型接入原生后台输入、权限/IME 与合并手势，扩展既有职责而非新造通用事件框架。 |
| MODIFY | [clipboardserver](../../../src/app/clipboardserver.cpp)、[appconfig](../../../src/common/appconfig.h)、[itemstore](../../../src/item/itemstore.cpp)、[serialize](../../../src/item/serialize.cpp)、[src/scriptable](../../../src/scriptable) | 生命周期/来源抑制、设置、文件清理/加密、命令复用与导入导出。 |
| MODIFY | [app](../../../src/app/app.cpp)、[config](../../../src/common/config.cpp)、[version.cmake](../../../src/version.cmake)、[shared](../../../shared)、[RELEASE.md](../../../RELEASE.md) | 品牌标识、隔离迁移、CLI/session/启动、安装卸载和对应源码材料。 |
| MODIFY | [Windows 部署](../../../utils/github/deploy-windows.sh)、[macOS 部署](../../../src/platform/mac/deploy.cmake.in)、[CI](../../../.github/workflows)、[CMakePresets](../../../CMakePresets.json) | 最终依赖打包与三端发布产物，记录真实签名/安装状态。 |
| NEW / MODIFY | src/tests/tests_snippets.cpp；[src/tests](../../../src/tests)及既有测试注册/构建 | 模板/触发/合并/迁移聚焦测试与性能采集；实际命令随新增入口写回。 |

## 实施里程碑

| 顺序 | 本段交付 | 完成条件 |
|---|---|---|
| I3-1 数据与 Snippet 产品闭环 | 固定数据/格式/触发/品牌/性能约束；集合、历史转入、编辑、浏览、动态内容、导入导出 | P9 的主要行为和规则测试通过，数据清理/加密/备份兼容有证据。 |
| I3-2 平台自动展开与合并 | 关键词监听/替换/光标/恢复、应用例外、权限、连续复制合并与反馈 | P10/P11 的纯规则与可用原生平台操作通过；其余平台缺项保持未完成。 |
| I3-3 品牌、迁移与性能 | 按完整映射更名、旧数据导入/CLI/session、实际性能定位与优化 | P14 通过，原数据保持可用；已冻结的性能门达标或明确记录失败，不边测边放宽。 |
| I3-4 发布候选总验收 | 三端构建/安装包、实际窗口/输入、相关集成回归、许可与源码材料 | P13 和三份 SPEC 所有产品验收通过，才可标首版完成；外部发布状态按事实记录。 |

## 验收标准

| 编号 | 可观察结果 | 必须覆盖 |
|---|---|---|
| P9 | 从历史或空白创建 Snippet，集合/关键词/富文本/动态内容/浏览/导入导出有效 | 集合重命名、冲突、损坏输入、数据版本、重启/备份恢复、清理历史不丢 Snippet、中文/emoji/光标与非递归引用 |
| P10 | 后台关键词准确替换并保持正确目标、格式和光标 | 退格/光标移动、IME、窗口切换、自身输入、权限拒绝/撤销、安全输入、注入失败、防重入、用户中途新复制 |
| P11 | 连续复制按设置合并一次，普通复制不受影响 | 受控时钟、超时/重复通知/同内容、非文本、忽略应用、自身剪贴板写入、反馈关闭 |
| P13 | 声明的三端安装包可独立运行，完整 UI 和能力可用 | QML/八插件部署、OS/架构/显示会话、原生输入及冻结性能门；缺项不能记为通过 |
| P14 | QClip 品牌与旧历史/配置/CLI/session/插件兼容，开源来源完整 | 并存、导入前后数据对照、失败重试、加密、启动/升级/卸载、包内许可及对应源码 |

- 首版总验收需要 P1–P14 全部有证据，具体实现归属见 INDEX；已有范围缺失时不得用“后续优化”关闭迭代。
- 共享逻辑测试、真实目标输入、安装包和发布行为分别记录；未获得 macOS 或某 Linux 会话证据时，可继续其他独立工作，但不能标全平台完成。

## 验证计划

文档阶段执行 Iteration1 的公共文档命令，lint 整个 docs/astack/version，预期三份 SPEC 均 0 errors/0 warnings。

产品阶段复用 Iteration1 的构建、隔离入口和 QML 检查。以下既有回归按改动选择，执行前完成 CLAUDE 环境/显示会话约束：

~~~bash
build/copyq-tests "testCore:configPath" "testCore:commandSession" "testCore:commandsExportImport" "testCore:importExportTab"
build/copyq-tests "testCore:keysAndFocusing" "testCore:clipboardUriList" "testCore:clipboardMimeSizeLimit"
~~~

新增模板/触发/合并测试使用可控时钟、输入事件、随机源和剪贴板变化；与实际测试注册保持一致，在实现后填入准确筛选命令。单元测试不能替代原生自动展开/粘贴验证；不得用日常历史做时间清理/迁移测试。

本轮准确入口（从根目录执行；Linux 命令在已启动 Xvfb/openbox 的容器中执行）：

~~~bash
cmake --build build/copyq/macOS-13-m1 -j 4
cmake --build build/copyq/macOS-13-m1 --target copyq-palette-ui_qmllint
utils/run-isolated.sh build/copyq/macOS-13-m1/copyq-snippet-tests storeRoundTrip damagedStorage encryptedStorage storageLimit dynamicDates dynamicClipboardAndRandom richTextAndReferences keywords copyMerging copyQMigration copyQExternalMigration qmlSnippets
utils/run-isolated.sh build/copyq/macOS-13-m1/copyq-tests testCore:snippetLifecycle testCore:snippetExpansion testCore:configPath testCore:commandSession testCore:commandsExportImport testCore:importExportTab
cmake --build build/linux -j 4
cmake --build build/linux --target copyq-palette-ui_qmllint
DISPLAY=:99 utils/run-isolated.sh build/linux/copyq-snippet-tests storeRoundTrip damagedStorage encryptedStorage storageLimit dynamicDates dynamicClipboardAndRandom richTextAndReferences keywords copyMerging copyQMigration copyQExternalMigration qmlSnippets
DISPLAY=:99 utils/run-isolated.sh build/linux/copyq-tests testCore:snippetLifecycle testCore:snippetExpansion testCore:configPath testCore:commandSession testCore:commandsExportImport testCore:importExportTab testCore:paletteSearchAndCopy testCore:palettePaste
DISPLAY=:99 QTEST_FUNCTION_TIMEOUT=1800000 COPYQ_PERF_OUTPUT=/workspace/build/spec3-qclip-performance.json utils/run-isolated.sh build/linux/copyq-tests testCore:desktopPerformance
DISPLAY=:99 COPYQ_TESTS_EXECUTABLE=/workspace/build/spec3-upstream-linux/copyq COPYQ_PERF_UPSTREAM=1 QTEST_FUNCTION_TIMEOUT=1800000 COPYQ_PERF_OUTPUT=/workspace/build/spec3-copyq-performance.json utils/run-isolated.sh build/linux/copyq-tests testCore:desktopPerformance
cpack --config build/copyq/macOS-13-m1/CPackConfig.cmake -G DragNDrop -B build/spec3-packages-final
DESTDIR="$PWD/build/spec3-linux-staged" cmake --install build/linux
python3 utils/package-source.py build/QClip-source.tar.gz
python3 utils/check-release.py --platform macos --root build/spec3-macos-mounted/QClip.app --min-macos 26.0 --source build/QClip-source.tar.gz --output build/spec3-macos-dmg-audit.json
python3 utils/check-release.py --platform macos --root build/spec3-macos-mounted/QClip.app --min-macos 13.0 --output build/spec3-macos-minimum-audit.json
python3 utils/check-release.py --platform linux --root build/spec3-linux-staged/usr/local --source build/QClip-source.tar.gz --output build/spec3-linux-package-audit.json
pwsh -NoProfile -File utils/check-harness.ps1
bash /Users/alexjhwen/data/codebase/GitHub/astack/astack-marketplace/plugins/astack-workflow/skills/spec/scripts/spec-lint.sh docs/astack/version
python3 utils/check-compatibility.py
git diff --check
~~~

上游性能对照源：`git archive 1cddb851 | tar -x -C build/spec3-upstream-src`，使用容器同一 Qt 6.4.2、GCC 12、Debug/Ninja，关闭 QCA、QtKeychain、原生通知和音频；`cmake -S build/spec3-upstream-src -B build/spec3-upstream-linux -G Ninja -DCMAKE_BUILD_TYPE=Debug -DWITH_TESTS=OFF -DWITH_QCA_ENCRYPTION=OFF -DWITH_KEYCHAIN=OFF -DWITH_NATIVE_NOTIFICATIONS=OFF -DWITH_AUDIO=OFF`，再 `cmake --build build/spec3-upstream-linux -j 4`。QCA 关闭时 CMake 同时关闭 keychain；对照引擎源码未改，仅由同一测试插件观察窗口和发送测试键。

macOS 原生例外沿用 SPEC1：`run-isolated.sh` 使用 cocoa，无 Xvfb；所有配置/状态/历史/GnuPG 路径为临时目录，测试父进程备份/恢复剪贴板。Linux 容器为 `qclip-spec1-linux`，通过 `docker --context colima exec` 执行，工作目录 `/workspace`，`DISPLAY=:99`，Xvfb/openbox 已就绪。未运行任何全量套件。

I3-4 在完整安装包与隔离数据上执行表中场景，保留构建/依赖、包哈希、安装/进程路径、平台/输入场景结果和性能数据摘要。若平台或签名条件缺失，明确未验证项，不使用 Linux/Xvfb 结果替代 Windows/macOS 前台行为。

## 验证记录

| 日期 | 命令 / 检查 | 结果与证据边界 |
|---|---|---|
| 2026-09-28 | Alfred 官方能力文档及 CopyQ 序列化/路径/平台/测试入口核对 | 已核对行为来源与数据复用位置；本阶段尚无新产品代码、构建、实机或性能结果。 |
| 2026-09-28 | 三份 SPEC 文档质量门 | harness、目录级 SPEC lint、git diff --check 均退出 0；三份 SPEC 0 errors/0 warnings，P1–P14 唯一归属及 18 个既有测试声明核对通过。仅文档/静态检查，产品验收仍待实施。 |
| 2026-09-30 | macOS arm64 构建与 UI qmllint（准确命令见上） | 退出 0；macOS 26.6.2、Apple M4 Pro/64 GiB、Homebrew Qt 6.11.2/QCA 2.3.12。部分依赖最低版本高于 preset 的 macOS 13，见安装包门。 |
| 2026-09-30 | macOS 隔离 `copyq-snippet-tests` 的 11 个指定方法 | 13 passed / 0 failed / 0 skipped（含 init/cleanup）：内联 MIME/集合交换、损坏/超限保存保护、加密/换钥/解密、日期月末/DST/固定偏移、随机/变换、富文本/引用/光标边界、关键词冲突、合并、源文件/资产保持与 QML。 |
| 2026-09-30 | macOS 服务生命周期、CLI/session/命令/历史导入导出 | `snippetLifecycle configPath commandSession commandsExportImport importExportTab`：7 passed / 0 failed；历史转 Snippet 保留 HTML/自定义 MIME，删历史后仍可读，整体备份导入及服务重启可恢复。 |
| 2026-09-30 | macOS 服务集成 `snippetLifecycle snippetExpansion paletteSearchAndCopy paletteCommands paletteClipboardFailure palettePaste` | 6 passed / 0 failed / 2 skipped；AX 权限不可用，`snippetExpansion` 与 `palettePaste` 明确跳过，不能计作原生输入通过。 |
| 2026-09-30 | Linux arm64 构建与 UI qmllint | 均退出 0；Debian 12/Qt 6.4.2/GCC 12，Colima 4 vCPU、约 7.7 GiB、Xvfb 1280×1024×24/openbox、xcb/basic、1×、软件渲染。安装 libicu-dev/xdotool；本机构建关闭 QCA/QtKeychain/原生通知/音频。 |
| 2026-09-30 | Linux 隔离 `copyq-snippet-tests` 同上 11 个方法 | 12 passed / 0 failed / 1 skipped；QCA 关闭，加密方法跳过。ICU 与 macOS CF 两套日期实现均通过指定样例。 |
| 2026-09-30 | Linux 隔离服务 8 个指定方法（准确命令见上） | 10 passed / 0 failed / 0 skipped；含 X11 RECORD/TEST 实际键入、中文粘贴、逻辑光标、两次展开、后续用户复制不被覆盖、关闭历史时 Ctrl+C+C 仅合并一次、命令输入与公共 `paste()` 覆盖，以及面板实际粘贴/焦点取消。修复修饰键快照、X11 modifier 跟踪和覆盖命令主动更换 provider 时的错误取消。 |
| 2026-09-30 | 修复后两端 `copyQMigration copyQExternalMigration` 与 `snippetLifecycle importExportTab` | 外置文件副本迁移新增测试先在 macOS 明文/加密两行复现失败，修复后 5 passed / 0 failed；Linux 4 passed / 0 failed / 1 skipped（QCA 关闭）。覆盖删除原目录、完整 MIME、保持源副本、缺失/歧义拒绝；两端服务导入回归均 4 passed / 0 failed。 |
| 2026-09-30 | 两套 `testCore:desktopPerformance`，两组数据/每链路 30 次 | QClip 和未修改 `1cddb851` CopyQ 完整采集；QClip 五项 p95 冻结门均通过。冷启动每次验证指定历史实际恢复，不用空窗口代替；详见性能表。 |
| 2026-09-30 | `cpack --config build/copyq/macOS-13-m1/CPackConfig.cmake -G DragNDrop -B build/spec3-packages-final` | 已生成本机 arm64 开发 DMG；Homebrew 可选模块触发 macdeployqt rpath 警告，后置 fixup 统一复制/改写并签名，最终包以逐 Mach-O 审计为准。 |
| 2026-09-30 | 安装产物与对应源码审计 | macOS 挂载 DMG 的当前主机 profile（`--min-macos 26.0`）依赖/八插件/QML/许可/ad-hoc 签名通过，隔离 `configPath snippetLifecycle` 4 passed / 0 failed；严格 `--min-macos 13.0` 退出 1，194 项依赖最低版本不满足。Linux staged install 的 host-resolved ELF 审计通过，不声称是独立 AppImage 验收。Windows 审计已接入 CI，本机无原生执行证据。 |
| 2026-09-30 | harness、目录级 spec-lint、兼容清单、脚本/工作流语法、`git diff --check` | 通过；兼容清单为 49 动作/90 配置/24 主表单/7 插件表单/150 API/8 插件；源码归档带逐文件哈希，工作树明确标 dirty。三端工作流准备构建/测试/审计产物，未执行推送或公开发布。 |
| 2026-10-02 | 提交前 macOS 构建、`copyq-palette-ui_qmllint`；上述 12 个指定 Snippet 方法及 6 个服务方法 | 构建/QML 均退出 0；Snippet 15 passed / 0 failed / 0 skipped，服务 7 passed / 0 failed / 1 skipped（`snippetExpansion` 缺少 AX 权限）。最低系统限制仍保留，未重新打包或执行发布候选总验收。 |
| 2026-10-02 | 提交前 Linux 构建、`copyq-palette-ui_qmllint`；上述 12 个指定 Snippet 方法及 8 个服务方法 | Xvfb/openbox 与隔离入口；构建/QML 均退出 0，Snippet 13 passed / 0 failed / 2 skipped（QCA 关闭），服务 10 passed / 0 failed / 0 skipped。未运行全量套件。 |
| 2026-10-02 | `pwsh -NoProfile -File utils/check-harness.ps1`、目录级 spec-lint、`python3 utils/check-compatibility.py`、脚本/工作流语法、`git diff --check` | 全部退出 0；三份 SPEC 0 errors / 0 warnings，兼容清单保持 49 动作/90 配置/24 主表单/7 插件表单/150 API/8 插件。 |

本轮产物：`build/spec3-packages-final/QClip-16.0.0-arm64.dmg` 为 66,771,454 bytes（63.7 MiB），包内文件 224,726,880 bytes；DMG SHA-256 为 `4b19ae56faf0b96eccd52c3ea5f67f1540370ecaaa11b5af0a7e4bbb5cb842ed`。这是 macOS 26 当前主机的 arm64/ad-hoc 开发包，版本号沿用仓库 16.0.0，未声明正式首版。Linux debug staged install 文件共 121,578,705 bytes；原始审计 JSON 与匹配源码 `build/QClip-source.tar.gz` 均保留在隔离 build 路径。

### 性能结果（2026-09-30）

同一 Colima 主机/Qt/GCC/Debug，启用全部可用插件；每组 1000/10000 条固定文本 + 五张 256×256 RGBA 图片，随后 30 次独立采集。每值为 p50 / p95，单位 ms；呼出/搜索含 CLI IPC 和轮询，粘贴以外部 QLineEdit 收到文本为终点，冷启动含进程就绪、指定历史加载与真实窗口呼出。保留平台等待配置。

| 应用/文本条数 | 常驻呼出 | 搜索 | 复制到历史 | 确认到实际粘贴 | 冷启动与历史窗口 |
|---|---|---|---|---|---|
| QClip / 1000 | 71.0 / 84.0 | 69.6 / 80.5 | 133.0 / 144.3 | 263.4 / 274.4 | 266.4 / 294.3 |
| CopyQ / 1000 | 50.3 / 61.5 | 78.5 / 95.3 | 147.8 / 172.0 | 152.2 / 163.9 | 253.3 / 302.5 |
| QClip / 10000 | 148.0 / 176.3 | 147.3 / 178.4 | 190.5 / 219.9 | 295.5 / 305.1 | 421.4 / 493.2 |
| CopyQ / 10000 | 47.9 / 124.8 | 96.9 / 212.2 | 150.6 / 241.0 | 151.4 / 192.6 | 224.9 / 261.7 |

QClip 隐藏/显示进程组 RSS：1000 条约 153.0/152.5 MiB，10000 条约 157.1/156.4 MiB；CopyQ 隐藏约 54.9/58.2 MiB。3 秒 `ps` CPU 窗口均未检出计时增量，不解释为绝对 0 CPU。QClip `beforeRendering`→`afterRendering` CPU 绘制 p50/p95 为 1000 条 0.067/0.113 ms（627 帧）、10000 条 0.051/0.100 ms（1135 帧）；不是 GPU 呈现延迟/FPS，旧 Widgets 对照未采集此 QSG 指标。原始 JSON 为 `build/spec3-{qclip,copyq}-performance.json`。

分析：搜索 p95 和历史采集达到冻结门；相对上游的呼出、粘贴、冷启动及 RSS 仍有成本，不能宣称全面更快/更省内存。初次 CPU 采样发现测试初始化遗留管理窗口，已修正入口使全部 UI 隐藏，并按相同条件重新采集两套全部样本。当前无需为已达标链路增加猜测性的缓存/批处理；三端真实显示和发布构建仍须测量。

### 里程碑实施状态与剩余边界

| 里程碑 | 已实施 | 尚未通过的验收 |
|---|---|---|
| I3-1 | 独立存储/加密、集合/详情/富文本编辑、历史转入、搜索/预览/复制/粘贴、模板、`.qcs` 集合交换、整体备份 | 三端产品场景总验收；Alfred `.alfredsnippets` 文件互通未承诺/未验证。`.qcs` 为显式导出的明文模板，整体密码备份使用既有 v5 路径。 |
| I3-2 | 三端输入实现、关键词/应用例外、确认 provider 后删除/粘贴/光标/恢复、合并反馈、CopyQ 命令和公共 paste；X11 实际流程通过 | Windows 原生输入/权限/UIPI/多窗口/IME/DPI；macOS 授权/撤销/安全输入/签名后路径、实际输入法及多应用。 |
| I3-3 | 独立配置/session/IPC/安装 ID/启动与 GNOME 标识；旧目录预检/资产复制重定位/原始文件保留及备份；同机 CopyQ 对照 | 真实加密 CopyQ 目录及插件私有配置/外部脚本/同步路径的完整迁移；GNOME 实际协议；Windows/macOS 原生性能及现有 I1/I2 未通过项。 |
| I3-4 | 三端构建/测试/依赖/许可/源码审计入口、macOS 当前系统 DMG、Linux staged install | Windows 构建/portable/安装/卸载，macOS Intel/13 兼容、Developer ID/公证及授权，Linux x86_64 AppImage/真实桌面/Wayland；Qt/KDE 内嵌第三方清单仍需匹配正式 SDK。首版总验收未通过。 |

平台限制：macOS 对非键盘布局输入源暂停；Windows 对 IMM IME/非默认桌面/标准 Edit 密码字段暂停，其他应用的密码/安全字段尚无完整实机证据；X11 仅接受确认 ASCII 键入，配置 ibus/fcitx 等时暂停，Wayland 全局监听明确不可用。X11 注入用限定键序列/到期队列排除自身输入，协议无法绝对区分同时同键的外部物理事件，需补真实桌面竞争输入场景。输出可含中文/emoji，规则测试不等于中文 IME 承诺。

交接：本轮代码、聚焦测试和打包入口已实施；按表继续补原生设备/权限/正式 SDK 和发布候选场景，不放宽冻结门槛。SPEC3 与首版保持**开发中**，未将跳过、未执行或系统版本失败标为通过。2026-09-30 开发记录未包含提交、推送、安装到日常应用目录或公开发布；2026-10-02 按用户要求提交并推送当前代码，代码交付不改变产品验收状态，未执行安装或公开发布。

## 变更记录

| 日期 | 作者 | 内容 |
|---|---|---|
| 2026-09-28 | zerovoxxx | 从原总 SPEC 分出第三阶段，定义 Snippet/展开/合并的数据和时序契约，纳入品牌/性能/三端发布总验收，保持首版完整范围。 |
| 2026-09-30 | zerovoxxx | 实施 I3-1–I3-4 业务代码、三端输入/品牌/迁移、性能与发布审计；记录两端新鲜聚焦证据、同机上游对照及 Windows/macOS 权限、最低系统和总验收缺项。 |
| 2026-10-02 | zerovoxxx | 按用户要求交付全部工作区变更，补充提交前两端构建/QML/聚焦验证，保留开发中状态及未通过的产品验收。 |
