# Iteration1：QClip 核心接入与快捷面板

> **文档信息**
>
> | 字段 | 值 |
> |---|---|
> | 文档类型 | SPEC |
> | 文档状态 | 待实施 |
> | 创建日期 | 2026-09-28 |
> | 最后更新 | 2026-09-28 |
> | 作者 | zerovoxxx |
> | 关联文档 | [迭代索引](../INDEX.md)、[项目入口](../../../CLAUDE.md)、[Iteration2](Iteration2_CopyQCompatibility_SPEC.md)、[Iteration3](Iteration3_AlfredDesktopRelease_SPEC.md) |
> | 一句话目标 | 建立 Qt Quick 与 CopyQ 同一核心的接入契约，完成可验证的搜索、预览、回车粘贴链路，并先证明全界面迁移依赖的插件和平台能力可行。 |

## 目标与非目标

### 首版共同目标与 SPEC 分工

用户已确认：品牌 **QClip**，基于 CopyQ 深度开发并开源；Windows/macOS 优先，Linux 持续支持；首版重做全部界面，剪贴板及相关 Snippet 交互完整对标 Alfred，保留 CopyQ 全部能力。技术方向为 **C++17 + Qt 6 Quick/QML + Qt Quick Controls + CMake + 现有 Win32/AppKit/Linux 平台实现**。

本文件保留原路径，作为公共架构约束和第一阶段的唯一权威。只设三个实施 SPEC：本阶段负责核心接入与快捷面板；[Iteration2](Iteration2_CopyQCompatibility_SPEC.md)负责原有全部界面、CopyQ 能力和历史管理；[Iteration3](Iteration3_AlfredDesktopRelease_SPEC.md)负责 Snippet、自动展开、连续复制合并、品牌迁移、性能及三端发布验收。需求归属见 INDEX，后两份引用本文件公共契约，避免维护三份相同规则。

三个阶段共同构成首版；Iteration1/2 的内部产物不能单独宣称完整首版已交付。当前工作仅拆解文档，三个阶段均未开始产品实现。执行时按本文里程碑推进，只有确实需要进一步拆解复杂实现时才增加 PLAN，不再默认拆更多 SPEC 或旁路报告。

### 本阶段目标

- 接入独立 Qt Quick 窗口，读取真实 CopyQ 历史，完成搜索、选择、标准格式预览、仅复制和回车粘贴。
- 明确模型生命周期、筛选与选择映射、动作、焦点及插件边界，让后续管理界面和 Snippet 复用。
- 优先验证富文本/FakeVim 编辑、插件设置以及 Windows/macOS 后台输入的技术可行性；避免到第三阶段才发现平台能力缺失。
- 建立原生平台隔离运行、聚焦测试及 QML 部署入口，留下可复现基线。

### 非目标

- 全产品不扩展为 Alfred 启动器、全盘搜索、Workflow 编辑器、云账号、云端历史同步或 OCR；CopyQ 已有脚本和文件同步能力保留。
- 本阶段不全面迁移管理/设置窗口，不实现完整 Snippet 业务，不批量更名内部 CopyQ 标识。这些分别由 Iteration2/3 实施。
- 保留必要的 QWidget 内部实现可以作为技术手段；首版所有可见界面仍必须完成重做，不能把旧窗口作为最终兼容入口。
- 不建立第二套剪贴板历史、Rust 服务或 WebView 界面，不以跨端为由删除平台功能。

## 已核对的代码事实与假设

| 来源 | 事实及实现含义 |
|---|---|
| [ClipboardModel](../../../src/item/clipboardmodel.h) | 继承 QAbstractListModel 且为 final；使用组合/代理适配展示，不直接派生。现有 QPersistentModelIndex 可跟踪行移动，内容 hash 不是唯一身份。 |
| [ClipboardBrowser](../../../src/gui/clipboardbrowser.cpp)、[ItemFilter](../../../src/item/itemfilter.h) | 过滤通过 ItemFactory::matches 和分批隐藏 QListView 行实现，不能直接把现有视图当 QML 过滤模型。只提取必要匹配/调度职责，保留插件匹配语义。 |
| [MainWindow](../../../src/gui/mainwindow.cpp) | updateFocusWindows 依赖 matchesWidget；activateCurrentItemHelper 还处理默认动作、剪贴板辅助进程及 paste() 脚本覆盖，不能用一次原生按键发送替代整条链路。 |
| [插件接口](../../../src/item/itemwidget.h)、[PersistentDisplayItem](../../../src/item/persistentdisplayitem.h) | 呈现、设置、编辑与显示命令都存在 QWidget 依赖；数据保存、匹配和脚本职责也在插件体系内，不能仅迁移缩略图。 |
| [构建](../../../src/CMakeLists.txt)、[脚本](../../../src/scriptable/scriptable.cpp) | Qt Qml 已用于 QJSEngine；新增的是 Quick/QuickControls2、QML UI 资源和部署，不能替换原脚本引擎来充当界面引擎。 |
| [Windows CI](../../../.github/workflows/build-windows.yml)、[macOS CI](../../../.github/workflows/build-macos.yml) | 当前分别使用 Qt 6.10.3/6.11.1；I1-1 固定公共 QML 模块的三端版本下限，平台样式按可用平台选择/部署，不能把 Windows 专用样式变成 Linux 的硬依赖。 |
| [测试启动器](../../../src/tests/tests.cpp) | COPYQ_PLUGINS 为空时，测试子进程会补入 itemtests 插件路径；该环境不能单独证明所有正式插件已加载。 |
| 工程判断 | 同 Qt 进程接入比额外跨语言/跨进程桥接更适合保留现有能力；尚无 QClip 原型、三端实机或性能结果。 |

## 公共架构与接入契约

~~~text
QML 快捷面板 / 后续管理与 Snippet 界面
             ↓ 展示角色、选择、显式动作
最小 C++ 展示适配与窗口控制
             ↓
现有模型 / ItemFactory / 配置 / 命令与脚本 / 序列化
             ↓
现有剪贴板服务与辅助进程 / Win32、AppKit、X11/Wayland
~~~

1. **数据和生命周期。** 历史仍由既有 C++ 模型与持久化拥有；QML 只持有展示数据和动作入口。C++ 保持 QObject/模型生命周期，模型在所属线程访问。选中项通过代理映射到源模型并由 QPersistentModelIndex 跟踪；插入/排序不改选中内容，删除、reset、卸载、切换历史源后失效即取消动作。不能把行号或内容 hash 当持久 ID。
2. **筛选和展示。** 复用 ItemFilter/ItemFactory 的匹配、排序及显示命令语义，按需为 QML 暴露文本、类型、预览引用和状态。分批/异步结果必须丢弃旧查询结果；插件调用不能未经证明就移到工作线程。显示命令只能修改展示副本，不能把其结果写回原始 MIME。图片按可见范围加载，不向每个 delegate 复制完整二进制历史。
3. **动作。** 确认时在 C++ 校验选择和目标，显式传入历史源、选中项及所需 MIME 快照，进入既有剪贴板/脚本/粘贴链路；不能借修改隐藏旧窗口的当前行来传递选择。必须处理异步写入完成、重复 Enter、取消和失败；新面板不能绕过用户现有 paste() 覆盖及自定义命令。默认 Enter 粘贴；显式用户绑定/默认动作的冲突在 I1-1 列表中确定并可见，不能静默丢弃。
4. **窗口。** 使用同一 QApplication 下独立 QQuickWindow/QQuickView；Qt Quick UI 引擎与既有脚本引擎分别管理。按必要范围扩展 PlatformWindow 与焦点判断，覆盖面板、管理、预览、设置、菜单及保留的编辑窗口。应用自己的窗口不得覆盖本次面板调用保存的外部粘贴目标，同时保留原管理/编辑窗口自身的粘贴用途。
5. **插件。** 先验证 itemtext、itemfakevim 及一个带设置的插件。优先保留 QTextEdit/FakeVim 行为，必要时使用独立、重新设计的 Widgets 编辑窗口；不能假定 QWidget 可直接放入 QML。只在原型证明需要时调整既有接口，不预先创建通用插件框架。所有内置插件功能由 Iteration2 逐项验收；旧二进制 ABI 未验证时不得宣称兼容。
6. **后续数据。** 历史写入/删除/清空仍走既有命令和插件钩子。Iteration2 所需时间元数据、Iteration3 的独立 Snippet 逻辑实体和迁移文件必须遵守原有序列化、备份和清理规则；新增 Snippet 数据不等于复制一份剪贴板历史。改动契约时同步拥有该能力的 SPEC。
7. **三端和外观。** 共用布局/操作，适配平台字体、Cmd/Ctrl、主题、系统材质和窗口行为。Windows 提权目标、macOS 授权/安全输入、Linux 显示协议是实际能力边界。无法获得可靠能力时取消依赖动作并反馈，不能把写入剪贴板称为目标应用已粘贴成功。
8. **变更控制。** 用户数据测试始终隔离；延迟常量不因界面重做而直接清零；上游同步保留祖先关系。品牌数据目录/应用标识在 Iteration3 统一变更，前两阶段使用隔离会话避免触及日常历史。

## 快捷面板行为

### 一次调用的完整流程

1. 全局热键触发时先记录外部应用的具体窗口，再显示面板并使搜索框可输入；同一应用的两个窗口必须区分。
2. 输入筛选，上下键选择；历史为空和无匹配分别显示。浏览、切换选择、预览、Esc 和失焦关闭均不改剪贴板。
3. 输入法组词期间 Enter 用于候选确认，不激活记录。普通 Enter 只提交一次；仅复制作为独立显式动作。
4. 确认后保持条目/目标身份，等待选定 MIME 写入、关闭面板、恢复目标并执行平台粘贴。期间目标关闭、权限失效或被用户切换后，按可确认的焦点状态取消，不能向新前台窗口盲发按键。
5. 标准文本/HTML/图片/文件链接可预览；打开文件或 URL 必须是显式动作。macOS 系统预览和其他平台等价预览分别留证，原始数据不因预览而降级。
6. 受控测试通过目标输入内容确认粘贴结果；普通运行只能根据实际可观测的提交/错误反馈，不假称能读取所有第三方输入框。

### 开始实现时收敛的选择

下列是工程默认建议，用户尚未指定，允许在隔离原型中先使用；最终设计与冲突处理写回本表。

| 项目 | 暂定处理与收敛时点 |
|---|---|
| 外观 | Alfred 的紧凑搜索/结果/预览结构，跟随系统明暗；I1-1 先固定尺寸、行高、预览区及键盘提示样板，Iteration2 扩展为全界面视觉规范。 |
| 粘贴格式 | 保留原始 MIME，另提供纯文本动作；I1-1 对照 Alfred 和 CopyQ 已有激活配置确定默认及覆盖关系。 |
| 默认热键及搜索源 | 沿用可配置热键与当前历史源；I1-1 做冲突检查并确定默认组合、跨标签搜索入口，不能任意修改用户已有绑定。 |
| Alfred 基线 | 沿用此前核对的 5.8.1/build 2349；官方文档用于能力说明，实际动作、顺序、默认值及修饰键需在对应里程碑核对。 |
| 平台范围 | 原生 Windows/macOS 优先；Linux 首个完整基线建议 X11，Wayland 单独标明桌面/协议。I1-1 记录可用设备、OS、架构、Qt 与编译器版本。 |

## 变更范围与文件清单

以下为未来产品实现清单；本次只修改治理/规格文档。因涉及构建、视图、模型、插件、平台与测试，实施必然超过十个源码文件，按里程碑分批修改，每批先列实际路径及原因。新文件名已按现有小写 C++/PascalCase QML 风格检索，实施前再次检查冲突。

| 操作 | 文件 / 位置 | 必要改动 |
|---|---|---|
| MODIFY | [src/CMakeLists.txt](../../../src/CMakeLists.txt)、[CMakeLists.txt](../../../CMakeLists.txt)、[CMakePresets.json](../../../CMakePresets.json) | Quick/Controls、QML 资源、按需测试目标；延续现有 preset，不重建构建体系。 |
| NEW | src/gui/clipboardpalette.h、src/gui/clipboardpalette.cpp、src/gui/qml/ClipboardPalette.qml | 独立面板、生命周期和动作接入。 |
| NEW | src/gui/clipboardpalettemodel.h、src/gui/clipboardpalettemodel.cpp | 最小展示/筛选适配，源数据仍来自现有 ClipboardModel。 |
| MODIFY | [mainwindow.cpp](../../../src/gui/mainwindow.cpp)、[mainwindow.h](../../../src/gui/mainwindow.h)、[clipboardbrowser.cpp](../../../src/gui/clipboardbrowser.cpp)、[filterlineedit.cpp](../../../src/gui/filterlineedit.cpp) | 提取实际需要的筛选/激活动作，接入面板，保留命令覆盖与控制生命周期。 |
| MODIFY | [clipboardmodel.h](../../../src/item/clipboardmodel.h)、[itemfactory.h](../../../src/item/itemfactory.h)、[itemwidget.h](../../../src/item/itemwidget.h) 及对应实现 | 仅在现有公共接口不足时修改；插件呈现/编辑原型先证明必要性。 |
| MODIFY | [platformwindow.h](../../../src/platform/platformwindow.h)、[Windows](../../../src/platform/win/winplatformwindow.cpp)、[macOS](../../../src/platform/mac/macplatformwindow.mm)、[X11](../../../src/platform/x11) | Quick 窗口识别、原窗口恢复、权限和最小平台可行性验证。 |
| MODIFY | [globalshortcutcommands.cpp](../../../src/common/globalshortcutcommands.cpp)、[appconfig.h](../../../src/common/appconfig.h)、[qxt](../../../qxt) | 按需接入现有热键/配置，不建立第二套全局快捷键系统。 |
| NEW / MODIFY | src/tests/tests_palette.cpp；[tests.h](../../../src/tests/tests.h)、[tests.cpp](../../../src/tests/tests.cpp)、[tests.cmake](../../../src/tests/tests.cmake) | 新增相关测试并注册，按需接入 Qt Quick Test；命令必须与实际目标/函数一致。 |
| MODIFY / NEW | [Windows 部署](../../../utils/github/deploy-windows.sh)、[macOS 部署](../../../src/platform/mac/deploy.cmake.in)、[CI](../../../.github/workflows)、utils/ 内必要隔离入口 | Windows 移除部署中的 --no-quick 限制并实际扫描 QML 依赖，三端部署资源；新增原生隔离运行入口。 |

## 实施里程碑

| 顺序 | 本段交付 | 完成条件 |
|---|---|---|
| I1-1 核心与构建接入 | 固定平台/产品默认值；独立 Quick 窗口、真实源模型和最小动作接口；建立隔离入口 | 编译、最小 QML 加载及模型变更验证通过，接口/实际文件名和运行命令写入本文。 |
| I1-2 高风险原型 | 源模型选择/过滤、Quick 焦点和粘贴；富文本/FakeVim/插件设置；Windows/macOS 原生输入监听和受控替换可行性 | 原型记录实际路径、失败条件和结果；不把只有静态页面或空插件运行当作通过。 |
| I1-3 快捷面板闭环 | 搜索、预览/打开、仅复制、回车粘贴、IME、Esc、失焦、多屏；处理模型变化和命令覆盖 | P1–P4/P8 通过本地自动化与可用原生平台操作验证；平台缺项保持未通过。 |
| I1-4 三端接入与交接 | 三端构建/部署冒烟、真实插件原型结果、初始性能数据和后续接口 | 交接内容完整，独立安装包可加载 QML；形成 Iteration2 可以直接复用的基线。 |

进入 Iteration2 前，共享模型/动作/插件方案必须已有原型证据；三端窗口与输入结论分别记录。缺少 macOS 设备时可以继续不依赖该设备的共享工作，但相应平台原型及 Iteration1 总验收不能标为通过，也不能到最终发布才首次验证。

## 验收标准

原 P 编号继续使用，以便验证拆分没有丢需求；以下项目由本 SPEC 拥有。

| 编号 | 可观察结果 | 必须覆盖的失败/变化场景 |
|---|---|---|
| P1 | 呼出即输入，中文/英文搜索，空历史与无结果不同 | 连续改查询、清空查询、超长文本、旧查询结果晚到 |
| P2 | Enter 返回原应用的同一窗口和输入位置 | 同应用两窗口、目标关闭、用户切换前台、自有面板/预览不能成为目标 |
| P3 | IME 候选确认、Esc、失焦均不误粘贴 | 中文组合输入、重复 Enter、权限拒绝、异步写入失败 |
| P4 | 浏览无写入，显式复制/粘贴保留正确条目和 MIME | 新增/移动/删除/reset/卸载/切换源、重复内容、用户自定义激活与 paste() 覆盖 |
| P8 | 文本/HTML/图片/文件预览和显式打开有效 | 多 MIME 往返、失效文件、URL、图片加载失败；平台预览能力分别验证 |
| I1-G | 真模型、必要插件原型、隔离入口和 QML 部署可复用 | 打包后无开发机 Qt 路径依赖，资源缺失明确失败，未验证平台不计通过 |

- 每个自动化断言均对应实际行为；旧测试通过不能替代新增面板测试。
- 交接时写明：模型/动作入口、对象所有者、筛选与选择规则、平台窗口识别方法、插件编辑/设置方案、实际测试命令、未通过项。
- 原有 H1–H4/D1–D4 文档与讨论要求由公共文档质量门、INDEX 范围映射及下方历史证据承接；它们不是产品通过记录。

## 验证计划

本次文档工作从仓库根目录执行：

~~~powershell
pwsh -NoProfile -File utils/check-harness.ps1
git diff --check
wsl.exe -d Ubuntu --cd /mnt/d/CodeSpace/Github/CopyQ -- bash /home/alexk/data/codebase/github/astack/astack-marketplace/plugins/astack-workflow/skills/spec/scripts/spec-lint.sh docs/astack/version
~~~

预期全部退出 0，三份 SPEC 的 errors=0、warnings=0，索引/状态/本地链接一致，Windows 治理入口继续共享硬链接。

产品实现时：

- Windows：配置 MSVC/Qt/依赖/sccache 后运行 `cmake --preset Windows`、`cmake --build --preset Windows`。
- macOS：配置原生依赖后按架构运行 `cmake --preset macOS-13` / `cmake --build --preset macOS-13`，以及 macOS-13-m1 对应命令；Linux 按 CLAUDE 配置后运行 `cmake --build build`。
- Linux 聚焦基线：在 CLAUDE 要求的全部环境变量、Xvfb/openbox/DISPLAY 就绪后运行 `build/copyq-tests "testCore:configPath" "testCore:searchItemsAndCopy" "testCore:keysAndFocusing" "testCore:clipboardUriList"`。后续各 SPEC 按影响面裁剪；禁止无筛选运行全套。
- Windows/macOS 原生入口必须显式隔离 session、settings/item-data/state 路径、密码、日志和插件；Qt 平台分别采用 windows/cocoa，不能复制 Linux xcb 配置。I1-1 补充实际可执行文件路径、初始化/结束命令，测试结束仅清理本测试会话。
- I1-1 接入实际 QML lint/Quick Test 目标后，把命令写入本文；尚未存在的函数或工具链不能提前记录通过。I1-2/3 新增断言需覆盖上表场景，记录正式插件实际加载列表。
- 原生验收包含文本编辑器、浏览器输入框、终端、同应用双窗口、中文输入与多屏缩放。复制/粘贴测试使用受控内容，不能用日常剪贴板历史验证删除或迁移。

## 公共技术依据与已核对边界

| 参照 | 借鉴点 | 本项目的取舍 |
|---|---|---|
| [Alfred Clipboard History](https://www.alfredapp.com/help/features/clipboard/) | 搜索、文本/图片/文件链接、回车粘贴、历史控制、忽略应用、Snippet、合并 | 全部纳入正式对齐范围，按下文矩阵逐项验收 |
| [Alfred Snippets](https://www.alfredapp.com/help/features/snippets/)、[动态占位符](https://www.alfredapp.com/help/workflows/advanced/placeholders/)、[自动展开设置](https://www.alfredapp.com/help/features/snippets/advanced/) | 分类、富文本、动态内容、光标位置、自动展开及应用例外 | 与剪贴板相关的 Snippet 能力纳入对齐；保留 CopyQ 自动化，完整 Alfred Workflow 引擎不在范围 |
| [Alfred Changelog](https://www.alfredapp.com/changelog/) | 本轮重新核对 5.8.1/build 2349；能力基线固定，后续变化需复核 | 历史预览及直接打开 URL/文件均是正式需求；具体原版键位/细节在实机交互核对中留证，不误称为本轮新发布功能 |
| [Alfred Appearance](https://www.alfredapp.com/help/appearance/) | 可变主题、结果数量、多屏位置以及面板焦点模式 | “最新版”不能唯一确定像素外观；Windows 应验证自身焦点链路，不能直接照搬 macOS 非激活面板实现 |
| [本仓库 CMake](../../../CMakeLists.txt)、[Qt Quick](https://doc.qt.io/qt-6/qtquick-index.html)、[C++ 模型接入 QML](https://doc.qt.io/qt-6/qtquick-modelviewsdata-cppmodels.html) | Qt 6/C++17；现有模型可通过 Qt 原生模型接口供 QML 使用 | 同一 Qt 服务进程内重做界面，复用历史与平台能力，展示适配不持久化第二份历史 |

| UI 方案 | 本项目的能力复用与全界面改造 | 取舍 |
|---|---|---|
| Qt Quick + C++/QML（本轮推荐） | 与现有 Qt/C++ 模型、信号槽、插件和三端实现处于同一生态；适合统一定制界面和动效；仍需迁移 QWidget 呈现接口 | 采用为目标技术栈；最小整合原型和插件兼容验证是进入全面迁移的前提 |
| Qt Widgets 全面重做 | 能保留较多控件与插件界面，但统一视觉、复杂布局和动画需要更多定制绘制 | 原先“小范围独立面板”的方案；用户本轮范围升级后不再作为主界面推荐 |
| Tauri + React + Rust | Web 样式开发便利，但现有核心包含 Qt 对象、事件循环和 QWidget 插件；需要额外 FFI/进程接口及系统集成工作 | 不作为本项目首选；截图中的“开发成本中等”不能直接套用于深度保留 CopyQ 的迁移项目 |
| WinUI 3 / AppKit 分别开发 | 原生平台表现可控，但要维护两套主界面并另补 Linux | 不符合三端共享界面的目标 |

[Tauri 官方说明](https://v2.tauri.app/reference/webview-versions/)确认 Windows 使用 WebView2，macOS 使用 WKWebView，Linux 使用 WebKitGTK。由此推断其界面需要处理不同系统 WebView 的差异；“Rust 核心快/安装包小”不能证明 CopyQ 集成后的常驻内存或粘贴延迟更低。本轮没有做性能实测。

Qt Quick 的界面定制能力和 Qt/C++ 的复用，是针对本项目的选择理由。快捷面板建议使用同一 QApplication 下独立的 QQuickWindow/QQuickView，避免把整块面板嵌进 QQuickWidget；[Qt 文档](https://doc.qt.io/qt-6/qquickwidget.html)指出 QQuickWidget 有额外渲染通道且会禁用线程渲染循环。常驻内存、隐藏时资源保留和打开延迟仍需测量，不承诺未测得的数字。

### Windows/macOS 能力与原生适配

推荐技术栈的完整含义是 C++17 + Qt 6 Quick/QML + Qt Quick Controls + CMake，继续使用现有 Windows C++/Win32 与 macOS Objective-C++/AppKit 平台实现。Qt 的[平台集成接口](https://doc.qt.io/qt-6/platform-integration.html)允许取得原生窗口句柄并调用系统 API；共享界面不要求所有系统行为都用 QML 实现。工程判断是该组合具备覆盖已确认产品目标的技术条件，全部能力保留仍须通过清单、迁移原型及三端实机验收证明。

| 能力 | 共享部分与平台部分 | 结论与边界 |
|---|---|---|
| 历史、多格式 MIME、搜索、标签、脚本、CLI 与插件数据 | 复用 Qt/C++ 核心，QML 负责新界面 | 三端复用基础已存在；插件的 QWidget 呈现/编辑迁移仍待验证 |
| 全局热键、托盘/菜单栏、拖放、文件对话框 | 复用 Qt 与既有平台实现；必要时补充原生代码 | 能覆盖产品目标；热键冲突、菜单栏行为、系统对话框与 MIME 互操作分别验证 |
| 呼出前窗口记录、焦点恢复、自动粘贴 | Windows 使用 Win32，macOS 使用 AppKit/系统输入接口 | 现有基础可复用；新 Quick 窗口身份、同应用多窗口、权限及目标应用响应须验证 |
| Snippet 后台自动展开、光标位置、中文输入 | 共享 Snippet 规则，平台负责全局输入检测与替换 | 需要新增/完善平台实现；QML 键盘事件不能替代后台输入检测 |
| 面板激活方式、置顶、多屏和系统材质 | Qt 窗口/屏幕接口结合 Win32/DWM、AppKit | 可以定制；Alfred 非激活面板语义和系统材质不能仅靠控件样式保证 |

“保留全部能力”是产品范围，不构成绕过操作系统限制的承诺。[Windows SendInput 文档](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput)规定输入注入只能面向相同或更低完整性级别，普通权限进程向管理员应用自动粘贴不能保证成功。macOS 的自动输入/全局监听须按实际实现处理辅助功能及[输入监控授权](https://support.apple.com/guide/mac-help/control-access-to-input-monitoring-on-mac-mchl4cedafb6/mac)，并尊重安全输入；[Alfred 自动展开文档](https://www.alfredapp.com/help/features/snippets/auto-expansion/)同样要求辅助功能授权且不在安全密码框展开。权限拒绝、目标应用不接受注入等边界在界面中提供明确结果，并按本 SPEC 的 P2/P3 及 Iteration3 的 P10 验证。

### 接近原生外观的方案

- Windows：Qt 6.8 起提供 [FluentWinUI3 样式](https://doc.qt.io/qt-6/qtquickcontrols-fluentwinui3.html)，针对 Windows 11 外观设计，跟随主题/配色；它是 Qt 绘制的控件样式，部分控件回退到 Fusion，不等于使用真正的 WinUI 3 控件。Mica/Acrylic 系统背景需要结合原生窗口与 [DWM API](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/ne-dwmapi-dwm_systembackdrop_type)，按 Windows 版本及窗口类型验证。
- macOS：[Qt Quick macOS 样式](https://doc.qt.io/qt-6/qtquickcontrols-macos.html)调用原生框架绘制并跟随系统主题，但部分控件仍回退到 Fusion，而且不适合大幅定制。半透明/材质可评估 AppKit 的 [NSVisualEffectView](https://developer.apple.com/documentation/appkit/nsvisualeffectview)；实际 Quick 窗口合成效果、面板行为和系统版本仍待原型验证。
- 设计建议：共享 QClip 的布局、信息密度和操作语义，分别适配系统字体、快捷键、主题和材质。高度定制的快捷面板采用自定义 QML 控件；管理/设置界面按所需定制程度选择平台样式或自定义控件，支持时使用[原生文件对话框](https://doc.qt.io/qt-6/qml-qtquick-dialogs-filedialog.html)。具体主题和样板按 I1-1 暂定、Iteration2 收敛；不承诺两端逐像素一致或自动跟随所有未来系统视觉变化。

### 性能判断与待测量项目

[Qt Quick 场景图](https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph.html)通过原生图形 API 渲染，Windows/macOS 可分别使用 Direct3D/Metal；历史与系统操作继续在 C++ 核心中完成。其机制适合本项目的紧凑列表、预览和动画，但 QML 引擎、绑定、图像缓存和 GPU 资源都有成本，不能据此承诺固定内存、安装包大小或相对其他框架的领先幅度。[Qt 性能文档](https://doc.qt.io/qt-6/qtquick-performance.html)建议分析实际热点、避免阻塞 GUI 线程、控制列表 delegate 和图片开销。实现时按测量结果处理耗时搜索、预览加载与隐藏窗口动画，保留现有搜索/插件语义。

本轮静态代码核对发现两个需区分的延迟来源：

- Windows 的[剪贴板采集](../../../src/platform/win/winplatformclipboard.cpp)通过 Qt 的变化事件监听；[粘贴配置](../../../src/common/appconfig.h)中，Windows 激活目标窗口后的等待默认值为 150ms。这是保证焦点稳定的现有配置，不是本轮测得的总粘贴耗时，不能为追求数字直接清零。
- macOS 的[剪贴板采集](../../../src/platform/mac/macclipboard.mm)默认每 500ms 检查 `NSPasteboard.changeCount`，还设置 5000ms 的定时器调度容差。[MacTimer](../../../src/platform/mac/mactimer.mm)通过 `CFRunLoopTimerSetTolerance` 设置容差；这是系统调度/节能空间，不表示每次固定延迟 5 秒。采集及时性与常驻功耗需在 macOS 实机取舍，重做界面不会自动消除这部分等待。

原型测量建议采用相同机器、相同隔离历史，对比未改动 CopyQ 与 QClip，计划覆盖 1000/10000 条文本及固定图片样本。分别记录冷启动、常驻呼出、输入到结果更新、复制到历史出现、确认到目标输入完成的 p50/p95，以及隐藏/显示时 CPU、进程/进程组内存、动画帧耗时和安装包大小。测量记录注明系统/架构、显示缩放、权限、历史容量与原有等待配置；性能验收阈值在原型测量后、Iteration3 性能优化前冻结。本轮只核对机制和代码，没有性能实测。

### 开源与上游同步基线

- 2026-09-28 同步时核对的官方稳定发布为 [CopyQ 16.0.0](https://github.com/hluk/CopyQ/releases/tag/v16.0.0)；当时官方 `master` 提交为 `1cddb851e5841be32e5692fb8d20520245f1492f`，描述为 `v16.0.0-64-g1cddb851`。开发主线与稳定发布包分别记录。
- 本轮已添加 `upstream=https://github.com/hluk/copyq.git` 并拉取；`origin`/`origin-https` 继续指向用户仓库。本地 `master` 已包含官方最新提交，多出的提交 `831853ff` 是此前 harness/SPEC 交付，合并检查为已是最新。
- 后续通过 `git fetch --prune upstream` 后审核差异再合并官方更新，保留 Git 祖先关系、上游版权和修复来源；不通过目录覆盖或大规模改名伪装成同步。
- CopyQ 源码标注 `GPL-3.0-or-later`，见 [ClipboardModel](../../../src/item/clipboardmodel.h) 与 [LICENSE](../../../LICENSE)。QClip 作为派生项目延续该许可与原作者说明，发布时提供对应源码；Qt 模块及插件的各自许可继续保留，见 [Qt 许可说明](https://doc.qt.io/qt-6/licensing.html)。


## 验证记录

以下保留拆分前的历史记录；其中“本轮”和旧的整体验收范围均指记录当次讨论，不代表三个实施阶段已经完成。

| 日期 | 命令 / 检查 | 结果与证据边界 |
|---|---|---|
| 2026-09-28 | astack 原始 `init-harness.sh --dry-run` 与实际迁移 | 退出 0，生成命名空间和入口备份；原脚本把自身生成的 INDEX 也提示为 legacy，这是工具既有提示，不另建旁路文档。 |
| 2026-09-28 | Windows / WSL 链接兼容探测 | Windows 原生软链创建报告需要管理员权限；WSL 软链不能从 Windows 读取。采用 NTFS 硬链接适配，最终以项目校验器验证。 |
| 2026-09-28 | `pwsh -NoProfile -File utils/check-harness.ps1` | 退出 0：入口链接身份、3 份权威文档的本地引用、SPEC/INDEX 状态及隔离环境一致；仅为文档初始化验证。 |
| 2026-09-28 | astack `spec/scripts/spec-lint.sh` | 退出 0，1 份 SPEC，errors=0、warnings=0。 |
| 2026-09-28 | `git diff --check` 与有效规则迁移核对 | 空白检查退出 0；原规则完整保留，仅将两条 CMake build/install 命令改为有效语法；模板核心原则逐字匹配。 |
| 2026-09-28 | WSL `stat` 与 Windows 硬链接身份检查 | 两入口 inode 相同、链接数均为 2，Windows 可正常读取；本仓库 `core.symlinks=true` 使 Git 正确识别 `CLAUDE.md` 从 120000 到 100644 的类型转换，全局配置未变。 |
| 2026-09-28 | astack `ship/scripts/archive_specs.py` 的 dry-run 与实际执行 | 均退出 0；根目录仅 1 份 SPEC，未超过 10 份阈值，归档为 no-op。 |
| 2026-09-28 | 原始 Windows UI 预览 | 使用上游 [CI run 36302430329](https://github.com/hluk/CopyQ/actions/runs/36302430329) 的 Windows 构建，源码为初始化基线 `1cddb851e5841be32e5692fb8d20520245f1492f`；下载包 SHA-256 与上游 artifact 摘要一致。已启动隔离会话 `original-preview`，窗口标题为 `CopyQ-original-preview`；用户按 Esc 停止界面检查，未完成视觉或粘贴验收。本地程序及会话数据位于已忽略的 `_install/original-preview/`，不纳入提交。 |
| 2026-09-28 | 产品验收 P1–P6 | 尚未实施，未本机构建或启动定制面板，未验证真实 Windows 粘贴；原始程序预览不能作为产品验收证据。 |
| 2026-09-28 | 本轮 `git fetch --prune upstream`、`git merge --ff-only upstream/master` 与远端 SHA 核对 | 拉取退出 0；合并返回 `Already up to date`。官方 `master` SHA 为 `1cddb851e5841be32e5692fb8d20520245f1492f`，HEAD 为 `831853ffc7a4a4d3267ea4dfd6815b3b82ee225a`，`HEAD...upstream/master` 为 `1 0`；差异仅此前 7 个 harness 文件，没有漏合的上游提交。 |
| 2026-09-28 | 本轮修改前 harness 检查与硬链接修复 | 检查报告入口没有软链/硬链；两文件 SHA-256 相同。按已记录的 Windows 适配规则恢复 `AGENTS.md` 到 `CLAUDE.md` 的硬链接，复检退出 0；不归因于未经核查的编辑器或 Git 操作。 |
| 2026-09-28 | 本轮官方资料与代码核对 | 核对 Alfred 5.8.1、Qt Quick/C++ 模型、Tauri WebView 和 CopyQ 平台边界；发现 QWidget 插件/焦点接口及 Windows `--no-quick` 部署限制。仅源码/文档核对，没有框架性能或三端 UI 测试。 |
| 2026-09-28 | 本轮文档更新后 `pwsh -NoProfile -File utils/check-harness.ps1`、SPEC lint、`git diff --check`、上游祖先及远端 SHA 检查 | 均退出 0，SPEC errors=0、warnings=0；入口硬链接与内容一致，INDEX/SPEC 状态一致。`git diff --quiet -- src plugins qxt shared utils .github CMakeLists.txt CMakePresets.json` 退出 0，只有 4 个治理/规格文件变化；本轮文档改动未提交/推送。 |
| 2026-09-28 | 本轮产品验收 P1–P14 | 全部待实施；本轮不构建、不安装、不启动剪贴板监控，不把需求扩充记为功能完成。 |
| 2026-09-28 | 技术栈续议：Qt/微软/Apple/Alfred 官方资料与平台源码核对 | 明确原生 API 适配、Fluent/macOS 样式边界与权限条件；核对 Windows 粘贴等待、macOS 采集间隔和定时器容差。仅文档/静态源码证据，未运行应用或性能基准。 |
| 2026-09-28 | 技术栈续议更新后的 harness、SPEC lint、`git diff --check` 与产品源码差异检查 | 均退出 0；SPEC errors=0、warnings=0，入口硬链接与文档状态一致，产品源码无变更。外观/性能结论为官方资料及静态代码分析，实机验收与基准仍待执行。 |
| 2026-09-28 | 三份 SPEC 拆分：源码接入点及官方 Alfred 文档核对 | 确认 final 模型、视图过滤、脚本覆盖、显示命令、八个内置插件、测试插件环境及 expire_tab 语义；产品源码未修改，未执行应用或产品测试。 |
| 2026-09-28 | 三份 SPEC 的 harness、目录级 spec-lint、`git diff --check`；PowerShell 需求/验收归属及测试声明核对 | 均退出 0：3 份 SPEC、0 errors/0 warnings；A1–A7/C1–C4 完整映射，P1–P14 各有唯一归属，里程碑为 4/3/4，18 个既有测试选择对应实际声明；入口硬链接有效，产品源码无差异。 |

## 变更记录

| 日期 | 作者 | 内容 |
|---|---|---|
| 2026-09-28 | zerovoxxx | 从既有需求讨论创建首个 SPEC；区分已确认需求、技术决定和待明确项，记录源码影响面、验收映射、astack 流程与 Windows 链接适配。 |
| 2026-09-28 | zerovoxxx | 补充原始 UI 预览的构建来源与验证边界；按用户要求交付当前 harness 和需求文件，产品状态保持待实施。 |
| 2026-09-28 | zerovoxxx | 用户确定品牌 QClip、公开开源、三端优先级、首版全界面重做与 Alfred 剪贴板完整对齐；保留 CopyQ 全部能力。方案由独立 Widgets 面板升级为 Qt Quick/QML，补齐能力矩阵、插件/焦点/部署影响面及上游同步证据。 |
| 2026-09-28 | zerovoxxx | 继续讨论 Windows/macOS 能力、接近原生的外观与性能；补充原生适配职责、系统限制、样式覆盖、采集/粘贴默认等待及原型测量建议，外观主题和性能阈值保持待明确。 |
| 2026-09-28 | zerovoxxx | 按用户要求收敛为三个实施 SPEC；本文件保留公共约束和核心/面板阶段，原功能与验收编号分配到 Iteration2/3，补充依赖、接入契约、里程碑和面向 GPT-6 Sol xhigh 的执行入口。 |
