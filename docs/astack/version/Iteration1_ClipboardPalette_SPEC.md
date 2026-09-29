# Iteration1：QClip 核心接入与快捷面板

> **文档信息**
>
> | 字段 | 值 |
> |---|---|
> | 文档类型 | SPEC |
> | 文档状态 | 开发中 |
> | 创建日期 | 2026-09-28 |
> | 最后更新 | 2026-09-29 |
> | 作者 | zerovoxxx |
> | 关联文档 | [迭代索引](../INDEX.md)、[项目入口](../../../CLAUDE.md)、[Iteration2](Iteration2_CopyQCompatibility_SPEC.md)、[Iteration3](Iteration3_AlfredDesktopRelease_SPEC.md) |
> | 一句话目标 | 建立 Qt Quick 与 CopyQ 同一核心的接入契约，完成可验证的搜索、预览、回车粘贴链路，并先证明全界面迁移依赖的插件和平台能力可行。 |

## 目标与非目标

### 首版共同目标与 SPEC 分工

用户已确认：品牌 **QClip**，基于 CopyQ 深度开发并开源；Windows/macOS 优先，Linux 持续支持；首版重做全部界面，剪贴板及相关 Snippet 交互完整对标 Alfred，保留 CopyQ 全部能力。技术方向为 **C++17 + Qt 6 Quick/QML + Qt Quick Controls + CMake + 现有 Win32/AppKit/Linux 平台实现**。

本文件保留原路径，作为公共架构约束和第一阶段的唯一权威。只设三个实施 SPEC：本阶段负责核心接入与快捷面板；[Iteration2](Iteration2_CopyQCompatibility_SPEC.md)负责原有全部界面、CopyQ 能力和历史管理；[Iteration3](Iteration3_AlfredDesktopRelease_SPEC.md)负责 Snippet、自动展开、连续复制合并、品牌迁移、性能及三端发布验收。需求归属见 INDEX，后两份引用本文件公共契约，避免维护三份相同规则。

三个阶段共同构成首版；Iteration1/2 的内部产物不能单独宣称完整首版已交付。2026-09-29 开始实施本阶段；后两阶段仍待实施。执行时按本文里程碑推进，只有确实需要进一步拆解复杂实现时才增加 PLAN，不再默认拆更多 SPEC 或旁路报告。

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
| 工程判断 | 同 Qt 进程接入比额外跨语言/跨进程桥接更适合保留现有能力；实施前没有原型。本轮 macOS/X11 原型与单次性能结果见下文，Windows 仍缺原生证据。 |

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

本阶段采用下列工程默认值；用户未指定全局键位，不自动改动已有绑定。Iteration2 可基于已验证接口扩展视觉规范和搜索入口。

| 项目 | 本阶段实现 |
|---|---|
| 外观 | Basic Controls + 系统调色板；800×520 逻辑像素、52 像素结果行，结果/预览两栏；当前鼠标屏幕居中并限制在可用区域。Iteration2 扩展全界面视觉规范。 |
| 粘贴格式 | Enter 原 MIME 粘贴；Cmd/Ctrl+Enter 仅复制；Shift+Enter 纯文本，Cmd/Ctrl+Shift+Enter 仅复制纯文本。已有 Enter 命令优先且显示名称；Alt+Enter 绕过默认命令。平台粘贴键仍遵守原配置。 |
| 默认热键及搜索源 | 新增可配置的 Show/hide clipboard palette 命令模板，默认键位为空，复用冲突检测；托盘和 palette() 可呼出。搜索当前历史源，选择器可切换已有标签；本阶段不合并多个标签的结果。 |
| Alfred 基线 | 沿用此前核对的 5.8.1/build 2349；官方文档用于能力说明，实际动作、顺序、默认值及修饰键需在对应里程碑核对。 |
| 平台范围 | 原生 macOS 26.6.2 arm64、Qt 6.11.2、AppleClang 21；Linux Debian 12 arm64、Qt 6.4.2、GCC 12.2、Xvfb/openbox。Windows 无可用原生主机；Wayland 未启动 compositor，不能计通过。 |

## 变更范围与文件清单

本轮实际改动如下。构建、视图、模型、既有动作、三端窗口及测试共同构成调用链，因此超过十个文件；各批影响面先记录于本轮实现决策。CMakePresets、ClipboardModel、ItemFactory、ItemWidget 和 qxt 公共接口无需修改。

| 操作 | 文件 / 位置 | 必要改动 |
|---|---|---|
| MODIFY | [src/CMakeLists.txt](../../../src/CMakeLists.txt)、[CMakeLists.txt](../../../CMakeLists.txt) | Qt 下限 6.4；Quick/Controls、静态 QML 模块、资源与测试链接；沿用既有 preset。 |
| NEW | src/gui/clipboardpalette.h、src/gui/clipboardpalette.cpp、src/gui/qml/ClipboardPalette.qml | 独立面板、生命周期和动作接入。 |
| NEW | src/gui/clipboardpalettemodel.h、src/gui/clipboardpalettemodel.cpp | 最小展示/筛选适配，源数据仍来自现有 ClipboardModel。 |
| MODIFY | [mainwindow.cpp](../../../src/gui/mainwindow.cpp)、[mainwindow.h](../../../src/gui/mainwindow.h)、[clipboardbrowser.cpp](../../../src/gui/clipboardbrowser.cpp) / h、[filterlineedit.cpp](../../../src/gui/filterlineedit.cpp) / h | 显式选择数据、插件 copy 钩子、原过滤语义、命令与 provider 确认、取消、编辑/设置窗口。 |
| MODIFY | [persistentdisplayitem.cpp](../../../src/item/persistentdisplayitem.cpp) / h、[itemeditorwidget.cpp](../../../src/item/itemeditorwidget.cpp) | 展示副本和版本有效性；修复整段加粗被判为纯文本的真实回归。 |
| MODIFY | [platformwindow.h](../../../src/platform/platformwindow.h)、[Windows](../../../src/platform/win/winplatformwindow.cpp) / h、[macOS](../../../src/platform/mac/macplatformwindow.mm) / h 与 [macplatform.mm](../../../src/platform/mac/macplatform.mm)、[X11](../../../src/platform/x11/x11platformwindow.cpp) / h | Quick 窗口识别、具体窗口有效性/焦点、权限及发送前的动作有效性回调。 |
| MODIFY | [globalshortcutcommands.cpp](../../../src/common/globalshortcutcommands.cpp)、[scriptable.cpp](../../../src/scriptable/scriptable.cpp) / h、[scriptableproxy.cpp](../../../src/scriptable/scriptableproxy.cpp) / h、[scripting-api.rst](../../../docs/scripting-api.rst)、[commandcompleterdocumentation.h](../../../src/gui/commandcompleterdocumentation.h) | 可配置呼出模板和 palette()，paste() 传入 actionId 以保护原目标；同步脚本补全文档。 |
| NEW | [tests_palette.cpp](../../../src/tests/tests_palette.cpp)、[tests_paletteserver.cpp](../../../src/tests/tests_paletteserver.cpp)、[clipboardguard.cpp](../../../src/tests/clipboardguard.cpp) / h | 独立模型/QML/原生输入原型及真实服务测试；保存并恢复系统剪贴板。 |
| MODIFY | [tests.h](../../../src/tests/tests.h)、[tests.cpp](../../../src/tests/tests.cpp)、[tests.cmake](../../../src/tests/tests.cmake)、[itemtests.cpp](../../../src/tests/itemtests/itemtests.cpp)、[itemfakevimtests.cpp](../../../src/tests/itemfakevimtests.cpp) / h | 注册聚焦测试，保留调用方隔离路径，加入 QML 测试回调及 FakeVim 新编辑窗口保存。 |
| NEW | [run-isolated.sh](../../../utils/run-isolated.sh)、[run-isolated.ps1](../../../utils/run-isolated.ps1) | 临时 session/config/data/state、平台显示前置校验、短 socket 路径、清理及环境恢复。 |
| MODIFY | [Windows 部署](../../../utils/github/deploy-windows.sh)、[macOS 部署](../../../src/platform/mac/deploy.cmake.in)、[fixup_bundle.cmake.in](../../../src/platform/mac/fixup_bundle.cmake.in)、[test-linux.sh](../../../utils/github/test-linux.sh)、[test-macos.sh](../../../utils/github/test-macos.sh)、三端 [CI](../../../.github/workflows) | QML 扫描/部署、包依赖与签名校验、指定测试；Windows 移除 --no-quick，Linux AppImage 传入实际 QML 源和导入路径。 |
| MODIFY | [CLAUDE.md](../../../CLAUDE.md)、[INDEX](../INDEX.md)、本文；AGENTS.md 类型恢复为符号链接 | 本机入口、实施状态、验证和交接保持一致，不维护旁路报告。 |

## 本轮实现决策（2026-09-29）

- 原生环境为 macOS arm64 / Darwin 25.6.0 / AppleClang 21，Qt 6.11.2；共享 QML 下限固定为 Qt 6.4（使用 Qt 官方 6.4 引入的 ComponentBehavior: Bound；[依据](https://doc.qt.io/qt-6/qtqml-documents-structure.html)），使用 Basic Controls 和系统调色板，不依赖 Windows 专有样式。
- 面板以 800×520 逻辑像素、52 像素结果行、结果/预览两栏起步；屏幕可用区域不足时缩小，在当前鼠标所在屏幕居中。默认搜索当前历史源，源选择器可切换已有标签，不新增历史。
- 新增 `palette()` CLI/脚本与“Clipboard Palette”全局命令模板及托盘入口。默认全局键位为空，沿用现有快捷键配置与冲突检测；避免抢占用户绑定。Enter 默认保留原 MIME 粘贴，Cmd/Ctrl+Enter 仅复制，Shift+Enter 纯文本粘贴；已有 Enter 命令优先且在面板可见，Alt+Enter 显式绕过默认命令。
- 每批改动先检查同类命名：新增 clipboardpalette / clipboardpalettemodel / ClipboardPalette.qml；复用 FilterLineEdit 的过滤器、ItemFactory 的匹配、PersistentDisplayItem 的显示命令、selectionData 的显式选择数据与 MainWindow 的剪贴板 provider。
- I1-1 实际影响面：根/src CMake（Quick/Controls 和资源）、上述新模型/窗口/QML、filterlineedit（复用过滤构造）、mainwindow（所有权与动作）、scriptable/proxy 和 scripting-api（显式调用）、globalshortcutcommands（配置模板）、测试和隔离运行脚本。I1-2/3 扩展 PersistentDisplayItem 以处理展示副本；PlatformWindow 及 Win/mac/X11 实现增加 Quick 窗口识别、有效性和保守粘贴校验；I1-4 修改部署与相关 CI 的 QML 扫描和指定测试入口。超过十文件是这些既有接口与三端实现的必要接入，不迁移 I2/3 业务。
- 原生运行使用 cocoa/windows，Linux 使用预先启动的 Xvfb/openbox。隔离脚本显式设置全部 session/config/data/state/plugins/theme/password/log 环境；原生剪贴板测试保存并恢复测试前 MIME。
- 新增独立 `copyq-palette-tests` 使用 QApplication，只运行指定函数，覆盖模型身份、查询失效、QML 键盘/IME、预览、副本、取消和平台识别；另运行相关旧 Core/FakeVim 测试。构建/部署失败先记录并修复，再推进。
- 初始 macOS configure 因缺 ECM/KF6 通知依赖失败；本机验证关闭 WITH_NATIVE_NOTIFICATIONS/WITH_AUDIO，保留 QCA/keychain 与正式插件。此限制不改变产品/CI 默认能力，打包验证必须记录依赖范围。
- Windows `createInput()` 当前为非零 wVk 附加 KEYEVENTF_UNICODE，与微软 [KEYBDINPUT](https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-keybdinput) 契约冲突；本阶段的虚拟快捷键发送改为普通虚拟键标志，并按 [SendInput](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput) 返回的完整事件数量判断提交。该修改涉及原有快捷键发送，必须由新增原生受控粘贴测试在 Windows 验证；本机无 Windows 主机，静态核对不能记为实机通过。
- 安全粘贴接口在实际发送前同时检查原窗口焦点与调用方传入的动作有效性回调；Windows/X11 等待修饰键释放会处理事件，因此不能只在进入平台方法前检查条目。主进程保持 provider 身份直到提交，写入后剪贴板被替换也取消等待中的动作。
- macOS 以 [macdeployqt 的 QML 扫描](https://doc.qt.io/qt-6/macos-deployment.html)部署实际导入。迁移包启动发现 Homebrew 的 QtQuick.Layouts 元数据软链被部署工具跳过；部署脚本只为已部署模块补齐真实 qmldir，再执行传递依赖修正与签名，包冒烟必须实际调用 palette()，不能只看 otool 结果。
- 完整包检查覆盖主程序、CopyQ 的 .so、QML dylib、framework 与软链；既有 fixup 补齐非系统绝对依赖并改写库 ID，复制真实文件而非 Homebrew 外部链接。依赖目录不在 CMAKE_PREFIX_PATH 时按被引用的真实库目录查找（本机 Hunspell）；三端发布默认依赖仍须分别验证。

## 实施里程碑

| 顺序 | 本段交付 | 完成条件 |
|---|---|---|
| I1-1 核心与构建接入 | 固定平台/产品默认值；独立 Quick 窗口、真实源模型和最小动作接口；建立隔离入口 | 编译、最小 QML 加载及模型变更验证通过，接口/实际文件名和运行命令写入本文。 |
| I1-2 高风险原型 | 源模型选择/过滤、Quick 焦点和粘贴；富文本/FakeVim/插件设置；Windows/macOS 原生输入监听和受控替换可行性 | 原型记录实际路径、失败条件和结果；不把只有静态页面或空插件运行当作通过。 |
| I1-3 快捷面板闭环 | 搜索、预览/打开、仅复制、回车粘贴、IME、Esc、失焦、多屏；处理模型变化和命令覆盖 | P1–P4/P8 通过本地自动化与可用原生平台操作验证；平台缺项保持未通过。 |
| I1-4 三端接入与交接 | 三端构建/部署冒烟、真实插件原型结果、初始性能数据和后续接口 | 交接内容完整，独立安装包可加载 QML；形成 Iteration2 可以直接复用的基线。 |

进入 Iteration2 前，共享模型/动作/插件方案必须已有原型证据；三端窗口与输入结论分别记录。缺少 macOS 设备时可以继续不依赖该设备的共享工作，但相应平台原型及 Iteration1 总验收不能标为通过，也不能到最终发布才首次验证。

### 本轮里程碑结果与后续接口

四个里程碑的开发内容已落地；Windows 和下方手工场景未验证，SPEC 总状态保持 **开发中**，不能把代码实现等同于三端验收完成。

| 里程碑 | 当前结果 | 剩余验证 |
|---|---|---|
| I1-1 | Qt 6.4/6.11 构建、QML lint、真实模型接入、呼出模板和隔离入口已实现；macOS/Linux 编译通过 | Windows 原生编译与 QML 加载 |
| I1-2 | 持久选择/展示副本、原生窗口识别、富文本/FakeVim/插件设置原型通过；macOS 后台监听与受控替换通过 | Windows WH_KEYBOARD_LL/SendInput 原型与权限场景；插件全能力仍属 Iteration2 |
| I1-3 | 搜索/标准预览/复制/粘贴/命令覆盖、IME 事件、取消及异步失败已实现；macOS/X11 受控双窗口实际输入通过 | 真中文输入法、浏览器/终端、不同 DPI 多屏、Quick Look/文件关联实际预览与权限拒绝 |
| I1-4 | 三端 QML 扫描、隔离聚焦 CI 和后续接口已落地；本机安装验证与单次 Debug 性能基线见验证记录 | Windows 安装包、Linux 自包含 AppImage、macOS 13/x86_64 与完整默认依赖配置 |

| 接入点 | 所有权、行为与复用方式 |
|---|---|
| MainWindow::togglePalette()、Scriptable/Proxy::palette() | MainWindow 以 unique_ptr 惰性拥有 ClipboardPalette；沿用 QApplication 服务进程，QQuickView 自有 QQmlEngine，原 QJSEngine 不变。析构先释放面板，再释放旧 UI/核心。QML URI 为 QClip，资源路径为 qrc:/copyq/QClip/gui/qml/ClipboardPalette.qml。 |
| ClipboardPaletteModel::setSourceModel()/selectedIndex()/selectedData() | ClipboardBrowser 继续拥有源 ClipboardModel；展示适配只保存 QPointer、QPersistentModelIndex 和当前展示副本。筛选沿用 FilterLineEdit::createFilter 和 ItemFactory::matches，GUI 线程每批约 5ms；新查询停止旧批次。插入/移动保留身份，删除/reset/销毁/切换源取消，不能退化为邻行或相同内容。 |
| PersistentDisplayItem、displayData()/setDisplayData() | 选中项发给原显示命令链；持久索引和展示版本同时有效才接受结果。预览只用展示副本，确认使用原 MIME 经 ClipboardBrowser::copyItem()/ItemSaver 钩子；原源数据不被显示命令覆盖。文本预览上限 100000 字符，行摘要 512；图片只为选中项解码并限制约 720 像素。 |
| activatePaletteItem()/registerClipboardProviderAction()/finishPaletteClipboard() | 显式索引、历史源、原 MIME 快照和 activationId；等待独立 provider 真实写入确认，5 秒超时/进程退出报错。确认期间 busy 防重复 Enter；取消/失效使旧结果无效。等待原配置焦点延迟后校验具体目标与条目，再发送平台粘贴。 |
| updatePaletteCommands()/runPaletteCommand()/pasteToCurrentWindow(actionId) | 复用 selectionData、原命令筛选与快捷键冲突规则，不修改隐藏管理视图当前行。默认 Enter 命令名称可见，Alt+Enter 可绕过。paste() 覆盖/用户命令用 actionId 绑定本次目标；覆盖中的 copy() 同样等待 provider，取消后的 actionId 保持保护直到结束。 |
| PlatformWindow::matchesWindow()/isValid()/isActive()/pasteFromClipboardSafely(canPaste) | Windows 保存 HWND+PID，macOS 保存具体 AX 窗口及应用身份，X11 保存窗口 ID。发送前再次检查目标焦点及 canPaste；macOS 授权/安全输入、Windows 注入失败、窗口关闭都取消。Wayland 无可靠原窗口接口时允许复制，依赖目标的粘贴反馈不可用。 |
| openPaletteEditor()/openPalettePluginSettings() | 独立新 Widgets 编辑/插件设置窗口，复用 ItemEditorWidget、ItemFactory::setData 及现有插件 settings widget/applySettings。QTextEdit/FakeVim 行为保留；不尝试 QWidget 嵌进 QML。插件设置与旧 Preferences 互斥以防 loader 保留悬空 widget；这些原型不是首版全部界面验收。 |
| copyq-palette-tests --input-target、nativeInputListeningAndReplacement | 仅测试的受控外部双窗口进程；原型监听在另一进程后台，验证原生按键删除/替换后真实文本为 expanded 并收到事件。Windows 使用低级钩子，macOS 使用 listen-only event tap；测试结束移除监听/结束接收进程，不实现 Snippet 业务。 |

正式插件加载记录为 itemimage、itemencrypted、itemfakevim、itemnotes、itempinned、itemsync、itemtags、itemtext 全部 enabled。本阶段实际验证 itemtext 编辑、itemfakevim 新窗口保存、itemimage 设置写入及标准预览；加载成功不代表八个插件全功能或旧二进制 ABI 已验收。

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

本轮文档质量门从仓库根目录执行：

~~~sh
pwsh -NoProfile -File utils/check-harness.ps1
git diff --check
bash /Users/alexjhwen/data/codebase/GitHub/astack/astack-marketplace/plugins/astack-workflow/skills/spec/scripts/spec-lint.sh docs/astack/version
~~~

预期全部退出 0，三份 SPEC 的 errors=0、warnings=0，索引/状态/本地链接一致；本机治理入口为标准符号链接，Windows 硬链接说明保留为该平台适配。

本机 macOS 配置和针对性测试（GUI 测试连续保持前台，禁止与其他 GUI 测试同时执行）：

~~~sh
CMAKE_PREFIX_PATH='/opt/homebrew/opt/qt;/opt/homebrew/opt/qca;/opt/homebrew/opt/qtkeychain' \
  cmake --preset macOS-13-m1 -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DWITH_NATIVE_NOTIFICATIONS=OFF -DWITH_AUDIO=OFF
cmake --build build/copyq/macOS-13-m1 -j 6
cmake --build build/copyq/macOS-13-m1 --target copyq-palette-ui_qmllint
utils/run-isolated.sh build/copyq/macOS-13-m1/copyq-palette-tests \
  modelIdentity queryChanges displayCopiesAndPreview sourceDestructionAndReset \
  qmlKeyboardAndIme actionsAndCancellation explicitCommands standardPreviews \
  nativeWindowIdentification pluginEditorAndSettings performanceBaseline
utils/run-isolated.sh build/copyq/macOS-13-m1/copyq-palette-tests nativeInputListeningAndReplacement
utils/run-isolated.sh build/copyq/macOS-13-m1/copyq-tests \
  'testCore:palettePaste' 'testItemFakeVim:paletteEditor'
cmake --install build/copyq/macOS-13-m1 --prefix "$PWD/build/spec1-installed"
codesign --verify --deep --strict build/spec1-installed/CopyQ.app
~~~

应用为 build/copyq/macOS-13-m1/CopyQ.app/Contents/MacOS/CopyQ；runner 给 copyq-tests 自动设置该路径，可通过 COPYQ_TESTS_EXECUTABLE 指向迁移后的安装包。测试框架自行结束测试服务，runner 仅移除本次临时 profile；测试 main 保存/恢复系统剪贴板。手工前台切换导致面板取消时按失败记录，不能重试到成功后抹去原因。

只改 post-deploy 脚本时可在 build 生成最新脚本后运行 `cmake -DCMAKE_INSTALL_PREFIX="$PWD/build/spec1-installed" -P build/copyq/macOS-13-m1/src/fixup_copyq_bundle.cmake`；本轮最终库修正以该命令验证。`utils/github/test-macos.sh` 检查包括 .so 的外部/未解析引用、所有包内软链和签名；实际 QML 加载由指定服务测试覆盖。迁移包使用 `ditto build/spec1-installed/CopyQ.app build/spec1-relocated/CopyQ.app`，等待复制完成后再做依赖/启动检查。

本机 Linux 为隔离 Debian 12 arm64 容器。Qt 6.4.2 开发库及 qml6-module-qtquick、qtquick-controls、qtquick-layouts、qtquick-templates、qtquick-window、qtqml、qtqml-models、qtqml-workerscript 已安装；Xvfb :99 与 openbox 先启动。配置关闭本容器缺失的 QCA/原生通知/音频，不改变 CI 默认值：

容器保留用于复现，本轮结束仅停止 qclip-spec1-linux；复用时先启动 Colima/容器，再按 CLAUDE 启动 Xvfb/openbox。下面使用 DOCKER_CONTEXT=colima，不要求修改全局 Docker context。

~~~sh
export DOCKER_CONTEXT=colima
docker exec qclip-spec1-linux cmake -S /workspace -B /workspace/build/linux -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DWITH_TESTS=ON \
  -DWITH_QCA_ENCRYPTION=OFF -DWITH_NATIVE_NOTIFICATIONS=OFF -DWITH_AUDIO=OFF -DPEDANTIC=ON
docker exec qclip-spec1-linux cmake --build /workspace/build/linux -j 4
docker exec qclip-spec1-linux cmake --build /workspace/build/linux --target copyq-palette-ui_qmllint
docker exec -e DISPLAY=:99 -e QT_QUICK_BACKEND=software qclip-spec1-linux \
  /workspace/utils/run-isolated.sh /workspace/build/linux/copyq-tests \
  'testCore:configPath' 'testCore:searchItemsAndCopy' 'testCore:keysAndFocusing' \
  'testCore:clipboardUriList' 'testCore:paletteSearchAndCopy' 'testCore:paletteCommands' \
  'testCore:paletteClipboardFailure' 'testCore:paletteEditor' 'testCore:palettePaste' \
  'testCore:paletteMimeAndDisplayCommands' 'testItemFakeVim:createItem' 'testItemFakeVim:paletteEditor'
docker exec -e DISPLAY=:99 -e QT_QUICK_BACKEND=software qclip-spec1-linux \
  /workspace/utils/run-isolated.sh /workspace/build/linux/copyq-palette-tests \
  modelIdentity queryChanges displayCopiesAndPreview sourceDestructionAndReset \
  qmlKeyboardAndIme actionsAndCancellation explicitCommands standardPreviews \
  nativeWindowIdentification pluginEditorAndSettings performanceBaseline
~~~

其他平台及 CI 验证入口：

- Windows：配置 MSVC/Qt/依赖/sccache 后运行 `cmake --preset Windows`、`cmake --build --preset Windows`。
- macOS：配置原生依赖后按架构运行 `cmake --preset macOS-13` / `cmake --build --preset macOS-13`，以及 macOS-13-m1 对应命令；Linux 按 CLAUDE 配置后运行 `cmake --build build`。
- Linux 聚焦基线：在 CLAUDE 要求的全部环境变量、Xvfb/openbox/DISPLAY 就绪后运行 `build/copyq-tests "testCore:configPath" "testCore:searchItemsAndCopy" "testCore:keysAndFocusing" "testCore:clipboardUriList"`。后续各 SPEC 按影响面裁剪；禁止无筛选运行全套。
- Windows/macOS 原生入口必须显式隔离 session、settings/item-data/state 路径、密码、日志和插件；Qt 平台分别采用 windows/cocoa，不能复制 Linux xcb 配置。I1-1 补充实际可执行文件路径、初始化/结束命令，测试结束仅清理本测试会话。
- 三端 CI 运行 copyq-palette-ui_qmllint 和上述指定核心/面板测试；Windows/macOS 增加 nativeInputListeningAndReplacement，权限缺失的 skip 仍表示平台门未验证。独立目标使用 QtTest + QApplication + 真 QML 窗口，没有空 Quick Test 占位目标。
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

[Tauri 官方说明](https://v2.tauri.app/reference/webview-versions/)确认 Windows 使用 WebView2，macOS 使用 WKWebView，Linux 使用 WebKitGTK。由此推断其界面需要处理不同系统 WebView 的差异；“Rust 核心快/安装包小”不能证明 CopyQ 集成后的常驻内存或粘贴延迟更低。本轮没有做框架对照性能实测。

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

完整性能测量仍需采用相同机器、相同隔离历史，对比未改动 CopyQ 与 QClip，覆盖 1000/10000 条文本及固定图片样本。分别记录冷启动、常驻呼出、输入到结果更新、复制到历史出现、确认到目标输入完成的 p50/p95，以及隐藏/显示时 CPU、进程/进程组内存、动画帧耗时和安装包大小。测量记录注明系统/架构、显示缩放、权限、历史容量与原有等待配置；性能验收阈值在 Iteration3 优化前冻结。本次只增加下面的可复现单次 Debug 基线，未做完整对照和分位数测量。

| 平台 / 样本 | QML 加载 | 打开与模型完成（含加载） | 查询完成 | 测试进程 RSS |
|---|---|---|---|---|
| macOS arm64 / Qt 6.11.2 / Debug / 10000 文本 | 5ms | 76ms | 63ms | 173391872 bytes |
| Debian 12 arm64 / Qt 6.4.2 / Debug / 10000 文本 / Xvfb 软件渲染 | 4ms | 10ms | 58ms | 70524928 bytes |

数据来自 performanceBaseline；QTRY 等待粒度影响完成时间，RSS 包含测试进程、已加载插件和 QML，不是产品空闲常驻内存。两个环境不能用于框架或操作系统性能排名。原生粘贴未清零原等待配置，当前系统采集间隔也未调整。

### 开源与上游同步基线

- 2026-09-28 同步时核对的官方稳定发布为 [CopyQ 16.0.0](https://github.com/hluk/CopyQ/releases/tag/v16.0.0)；当时官方 `master` 提交为 `1cddb851e5841be32e5692fb8d20520245f1492f`，描述为 `v16.0.0-64-g1cddb851`。开发主线与稳定发布包分别记录。
- 本轮已添加 `upstream=https://github.com/hluk/copyq.git` 并拉取；`origin`/`origin-https` 继续指向用户仓库。本地 `master` 已包含官方最新提交，多出的提交 `831853ff` 是此前 harness/SPEC 交付，合并检查为已是最新。
- 后续通过 `git fetch --prune upstream` 后审核差异再合并官方更新，保留 Git 祖先关系、上游版权和修复来源；不通过目录覆盖或大规模改名伪装成同步。
- CopyQ 源码标注 `GPL-3.0-or-later`，见 [ClipboardModel](../../../src/item/clipboardmodel.h) 与 [LICENSE](../../../LICENSE)。QClip 作为派生项目延续该许可与原作者说明，发布时提供对应源码；Qt 模块及插件的各自许可继续保留，见 [Qt 许可说明](https://doc.qt.io/qt-6/licensing.html)。


## 验证记录

| 日期 | 命令 / 场景 | 结果 |
|---|---|---|
| 2026-09-29 | 全部本地修改提交前：macOS/Linux `cmake --build ... -j 6`、`copyq-palette-ui_qmllint`，Linux 隔离 runner 的本文 12 个服务函数加 SPEC2 的 4 个 management 函数，本文 11 个面板函数 | 两平台构建/lint 均退出 0；Linux 服务 18 passed / 0 failed、面板 13 passed / 0 failed（含 init/cleanup），8 个正式插件 enabled。新鲜日志 `build/ship-mac-build.log`、`build/ship-linux-build.log`、`build/ship-*-qmllint.log`、`build/ship-linux-server.log`、`build/ship-linux-palette.log`。macOS 管理窗口暴露失败另记 SPEC2，未重跑已知受前台限制的粘贴/编辑用例；不替代 Windows、macOS 13 或完整产品验收。本次用户授权将全部改动提交并推送，迭代保持开发中。 |
| 2026-09-29 | 本文 macOS configure、cmake --build build/copyq/macOS-13-m1 -j 6 与 copyq-palette-ui_qmllint | 均退出 0；原生 arm64、Qt 6.11.2、AppleClang 21。通知/音频为本机关闭，QCA/keychain 开启；不能据此证明完整发布配置或 macOS 13。 |
| 2026-09-29 | 本文 Linux configure、cmake --build /workspace/build/linux -j 4 与 copyq-palette-ui_qmllint | 均退出 0；Qt 6.4.2 下共享模块实际编译/lint 通过。Debian 容器关闭缺失依赖，Xvfb/openbox + QT_QUICK_BACKEND=software；QSG_RHI_BACKEND=software 不适用于该 Qt，已改用正确变量。 |
| 2026-09-29 | Linux run-isolated.sh copyq-tests：本文指定的 12 个 Core/FakeVim 函数 | 14 passed / 0 failed / 0 skipped（含 init/cleanup）。覆盖原聚焦基线、真实历史查询/复制、显示命令不改原 MIME、纯文本、Enter 命令、普通/自定义 paste()、同应用第二窗口/关闭、异步 provider 失败以及新普通/FakeVim 编辑窗口保存。 |
| 2026-09-29 | Linux run-isolated.sh copyq-palette-tests：本文指定的 11 个模型/UI/性能函数 | 13 passed / 0 failed / 0 skipped。覆盖索引变化、reset/销毁、中文/超长/连续查询、IME preedit Enter、不重复提交、失效目标、展示副本、有效/失效文件与 HTML URL 显式打开、坏图片错误、原生窗口/发送回调保护、富文本和插件设置。URL handler 受控验证不等于系统浏览器/文件关联手工验收。 |
| 2026-09-29 | macOS run-isolated.sh copyq-palette-tests：11 个模型/UI/性能函数；末轮 modelIdentity/queryChanges/displayCopiesAndPreview/sourceDestructionAndReset/pluginEditorAndSettings | 首组 13 passed / 0 failed；末轮 7 passed / 0 failed。8 个正式插件均 enabled；整段加粗富文本检测修复后 plain 与 HTML 均正确。最新有效 URL/坏图片追加断言已在 Linux 验证，未据此补称 macOS 手工预览通过。 |
| 2026-09-29 | macOS run-isolated.sh copyq-palette-tests nativeInputListeningAndReplacement | 3 passed / 0 failed / 0 skipped，935ms。当前辅助功能/输入监控可用，listen-only 后台监听到原生输入，独立前台输入框由 qclip 实际替换为 expanded；结束后移除 event tap 并终止接收进程。拒绝授权/安全输入实机场景仍待验证。 |
| 2026-09-29 | macOS run-isolated.sh copyq-tests testCore:palettePaste testItemFakeVim:paletteEditor | 用户提供连续前台测试时间后 4 passed / 0 failed / 0 skipped，6226ms。实际输入为 qclipPALETTE、覆盖后 qclipPALETTEOVERRIDE，第二窗口不变；切到第二窗口取消，关闭目标显示错误；新编辑窗口 FakeVim :wq 保存成功。先前桌面切换导致取消的失败保留为环境边界。 |
| 2026-09-29 | Linux cmake --install ... --prefix /workspace/build/spec1-linux-installed；COPYQ_TESTS_EXECUTABLE=该安装路径的 copyq，指定 paletteSearchAndCopy/paletteMimeAndDisplayCommands | 4 passed / 0 failed / 0 skipped；安装后的 QML 窗口与原 MIME 链路可用。使用容器系统 Qt，仅为安装布局验证，不是自包含 AppImage 验收。 |
| 2026-09-29 | macOS 首轮安装、迁移目录下实际 palette() 启动 | 早期库检查只覆盖复制中的子集，不能作为完整包证据；迁移包 QML 加载失败：QtQuick.Layouts 缺 qmldir。已修正部署元数据，最终复验另行记录。 |
| 2026-09-29 | macOS 全新目录 cmake --install 与完整 Mach-O/软链检查 | 新包签名验证失败：上游 fixup 将 Homebrew framework 的外部软链原样复制。改为复制真实 framework/库并处理旧断链；完整检查另发现原脚本只扫描 dylib/@rpath，漏掉 CopyQ .so 和绝对 Homebrew 链接。已扩展同一 fixup，包含主程序、模块、库 ID 与非系统依赖；最终复验另行记录。 |
| 2026-09-29 | macOS 最终 cmake --install ... --prefix build/spec1-installed；生成最新脚本后运行上方 cmake -P fixup；ditto 迁移并校验 | install/fixup/签名均退出 0。迁移目录完整扫描 203 个 Mach-O，外部/未解析引用及断链/外部软链均为 0；约 221MiB Debug app，不能视为最终发布包大小。Hunspell 依赖已补齐；包未写入 /Applications。 |
| 2026-09-29 | run-isolated.sh python3 - build/spec1-relocated/CopyQ.app/Contents/MacOS/CopyQ：隔离启动服务、disable/add/palette()、palette/exit；DYLD_PRINT_LIBRARIES=1 | QML 打开返回 true，迁移包实际加载成功；开发机 /opt/homebrew 和 build/copyq 库加载为 0，服务正常退出 0。与完整静态审计共同证明当前 macOS arm64 测试包可独立加载；Qt 支持的 macOS 13、x86_64、完整发布配置和第三方应用行为仍未验收。 |
| 2026-09-29 | pwsh run-isolated.ps1 调用原生 CopyQ --version，检查调用前 COPYQ_SETTINGS_PATH 哨兵；脚本解析 / bash -n / 三端 workflow YAML 解析 | 全部退出 0；PowerShell runner 在本机 cocoa 下可用并恢复环境。此项不替代 Windows 原生运行验证。 |
| 2026-09-29 | Windows 环境确认 | 用户确认本轮没有可用 Windows 环境，保持待验证；MSVC 构建、部署包、双窗口粘贴、低级输入监听及权限/DPI 门均未计通过。 |
| 2026-09-29 | 最终 pwsh utils/check-harness.ps1、三份 SPEC 的 spec-lint.sh、git diff --check | 均退出 0；harness 全项通过，SPEC errors=0 / warnings=0，入口/链接/状态一致。实现未提交或推送，未发布或安装到系统应用目录。 |
| 2026-09-29 | 首轮 paletteClipboardFailure 故障注入；修正后 Linux 指定该函数 | provider 错误已按预期反馈，测试最初被框架的 warning 检查拒绝；改用明确 throw new Error 并只忽略该 provider 的预期退出 4。复验 3 passed / 0 failed；macOS 同项被桌面切换取消，未计通过。 |
| 2026-09-29 | I1-2/3 首轮 UI 测试及 Linux 依赖安装 | 首轮 UI 7 passed/2 failed：窗口识别测试缺少 requestActivate，富文本测试发现全段加粗被旧编辑器当成纯文本；修正测试调用并补强 ItemEditorWidget 富文本检测。Debian 12 无 Qt6 QCA 开发包；该容器关闭 QCA 标签加密/原生通知/音频，不改变 CI 产品默认配置。itemencrypted 插件仍加载，不把这一记录称为完整加密验证。 |
| 2026-09-29 | I1-1 原生 macOS configure/build、copyq-palette-ui_qmllint；独立指定 modelIdentity/queryChanges/displayCopiesAndPreview/sourceDestructionAndReset | build 与 QML lint 退出 0；4 个行为测试与 init/cleanup 共 6 passed/0 failed。验证重复内容身份、插入/移动/删除、查询替换、展示副本和源销毁。初次尝试 OBJECT QML target 不受 Qt 支持，改为 static module；测试补齐 QElapsedTimer 头后编译通过。Homebrew 库 minOS 高于 preset 13，只证明当前 macOS 26.6.2 能运行，不能证明 macOS 13 兼容。 |

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
| 2026-09-29 | zerovoxxx | 实施 I1-1–I1-4 的核心/快捷面板、原动作/provider 与具体窗口保护、富文本/FakeVim/插件设置及原生输入原型；建立三端 QML 部署/隔离聚焦测试，记录 macOS/X11 证据、性能样本、部署修复及后续接口。Windows 和剩余手工验收未通过，总状态保持开发中。 |
