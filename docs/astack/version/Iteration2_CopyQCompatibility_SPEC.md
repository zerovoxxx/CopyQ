# Iteration2：QClip 全界面重做与 CopyQ 能力保留

> **文档信息**
>
> | 字段 | 值 |
> |---|---|
> | 文档类型 | SPEC |
> | 文档状态 | 开发中 |
> | 创建日期 | 2026-09-28 |
> | 最后更新 | 2026-09-29 |
> | 作者 | zerovoxxx |
> | 关联文档 | [迭代索引](../INDEX.md)、[公共架构与面板](Iteration1_ClipboardPalette_SPEC.md)、[Alfred 增强与发布](Iteration3_AlfredDesktopRelease_SPEC.md)、[项目入口](../../../CLAUDE.md) |
> | 一句话目标 | 重做 CopyQ 所有既有可见界面，保持原有功能、插件、脚本和数据行为，并补齐 Alfred 式历史管理。 |

## 目标与非目标

本阶段依赖 Iteration1 的源模型/动作/窗口契约、插件原型和基础 QML 部署。目标是让日常快捷面板与完整管理能力共同使用同一 CopyQ 核心，完成既有界面的全面迁移。Snippet 新界面在 Iteration3 随业务实现，复用本阶段组件和编辑能力。

公共技术、隔离环境和平台限制以 [Iteration1](Iteration1_ClipboardPalette_SPEC.md)为准。本阶段负责 A3、C1–C3 及 C4 的主题/旧格式兼容；负责 P5/P6/P7/P12。新品牌标识与实际数据迁移属于 Iteration3 的 P14。

- 首版必须保留多标签、批量管理、编辑、备注、标签、固定、拖放、插件、脚本/CLI、外部编辑器、通知/托盘、导入导出、加密和文件同步等全部既有能力。
- 主界面采用 QML；经过原型验证的 QTextEdit/FakeVim 等实现可保留在重新设计的编辑窗口中，窗口结构、布局、主题和交互仍属于改造范围。
- 非目标：本阶段不增加 Snippet 自动展开/合并引擎，不更换存储格式/脚本引擎，不批量改品牌工程标识，不用旧管理窗口兜底代替完成验收。
- 不新增 OCR、云同步、通用插件市场或与此次迁移无关的配置/框架。

## 输入条件与关键风险

进入本阶段前读取 Iteration1 交接记录，核对实际类/方法/测试命令。先通过共享模型、动作、焦点排除及插件编辑/设置原型；不得重新实现一套历史状态。缺少平台设备时可推进不依赖它的工作，但平台验收保持未完成。

| 已核对位置 | 对实施的约束 |
|---|---|
| [ConfigurationManager](../../../src/gui/configurationmanager.h) | 本身是 QDialog，但同时提供 options/optionValue/setOptionValue 等配置职责；新 UI 必须继续接入同一配置及 CLI 语义。 |
| [ItemLoaderInterface](../../../src/item/itemwidget.h)、[ItemFactory](../../../src/item/itemfactory.h) | 插件同时提供显示、设置、匹配、数据、保存和编辑能力；迁移不能仅检查是否能加载动态库。 |
| [PersistentDisplayItem](../../../src/item/persistentdisplayitem.h)、[显示命令](../../../src/tests/tests.h) | 显示命令的数据副本和生命周期依赖旧 UI；新 delegate 需要承接结果，不能只保留脚本入口而丢失可见效果。 |
| [插件构建清单](../../../plugins/CMakeLists.txt) | 当前有 itemtext、itemimage、itemnotes、itemtags、itempinned、itemencrypted、itemsync、itemfakevim 八个内置插件。 |
| [历史元数据](../../../src/common/mimetypes.h)、[标签卸载](../../../src/gui/clipboardbrowserplaceholder.cpp) | 当前 MIME 常量没有通用历史时间字段；expire_tab 触发不活跃标签卸载，不等于按记录年龄删除。按时间保留/清理需单独设计。 |

## 目标界面与能力清单

下面是迁移清单的起点。I2-1 必须从旧菜单、src/ui、AppConfig、脚本 API 和全部插件逐项补齐“旧入口 → 新入口 → 验证”。在本文记录覆盖数量和所有未迁移项；分类表本身不是“完整保留”的证据。

| 能力组 | 现有入口 / 语义锚点 | 新界面职责与验收重点 |
|---|---|---|
| 管理与标签页 | [mainwindow](../../../src/gui/mainwindow.cpp)、[clipboardbrowser](../../../src/gui/clipboardbrowser.cpp)、[tabdialog](../../../src/ui/tabdialog.ui) | 新管理窗口：标签创建/重命名/排序/属性、选择/排序/移动/复制、批量操作、上下文动作、拖放；保留脚本对这些状态的控制。 |
| 设置 | [configurationmanager](../../../src/gui/configurationmanager.cpp)、[src/ui](../../../src/ui)、[AppConfig](../../../src/common/appconfig.h) | 通用、历史、外观、布局、通知、托盘、标签、快捷键及插件设置；Apply/Cancel/恢复默认、即时生效及重启后生效的语义一致。 |
| 命令与脚本 | [commanddialog](../../../src/gui/commanddialog.cpp)、[commandedit](../../../src/gui/commandedit.cpp)、[actiondialog](../../../src/gui/actiondialog.cpp)、[脚本 API](../../../docs/scripting-api.rst) | 编辑、触发条件、快捷键、脚本补全/错误位置、执行结果、显示命令和自动命令；可视入口与 CLI 均可用。 |
| 内容查看与编辑 | [clipboarddialog](../../../src/gui/clipboarddialog.cpp)、[itemeditorwidget](../../../src/item/itemeditorwidget.cpp)、[itemeditor](../../../src/item/itemeditor.cpp) | 原始格式查看、文本/富文本编辑、撤销重做、外部编辑器、保存/取消；不会因 QML 显示转换而覆盖其他 MIME。 |
| 导入导出及数据管理 | [importexportdialog](../../../src/gui/importexportdialog.cpp)、[serialize](../../../src/item/serialize.cpp)、[itemstore](../../../src/item/itemstore.cpp) | 既有文件/密码/标签/命令导入导出与旧数据可读；迁移本身的品牌路径变更留给 Iteration3。 |
| 辅助界面 | [src/ui](../../../src/ui)、[src/gui](../../../src/gui) | 托盘菜单、通知、日志、关于、密码提示、快捷键录入、动作进度/错误、文件对话框等均纳入入口枚举和新视觉。 |
| 主题与可用性 | [theme](../../../src/gui/theme.cpp)、[shared/themes](../../../shared/themes)、快捷面板 | 统一字号/间距/色彩/焦点态/明暗/缩放，跨端快捷键和键盘导航；设置页/编辑器/菜单不能各用一套临时样式。 |

### 八个插件的功能保留

| 插件 | 必须保留和实测的内容 |
|---|---|
| itemtext | 文本/HTML 呈现、匹配、换行/显示设置、格式数据；纯文本与富文本往返。 |
| itemimage | 图片展示/预览、缩放及格式保存；大图/损坏图片处理与原始 MIME 保留。 |
| itemnotes | 备注查看/编辑/显示和脚本可访问性；不覆盖主体内容。 |
| itemtags | 标签设置/筛选/批量操作及脚本命令；与文本搜索结合时行为正确。 |
| itempinned | 固定状态、位置及容量变化保护；普通删除/清理不得绕过现有保护语义。 |
| itemencrypted | 加解密、密码/取消/错误提示、锁定及相关设置；凭据失效和重启后行为一致。 |
| itemsync | 文件到条目/条目到文件、编辑同步、路径/自有文件判断和相关设置；测试只用隔离目录。 |
| itemfakevim | 模态编辑、选择、搜索、撤销分组及中文输入；QTextEdit/FakeVim 原型必须有真实编辑证据。 |

第三方插件加载能力和接口变动须列出源码兼容/重编译要求。未验证旧二进制 ABI 不宣称兼容；某个插件在某系统无法构建时记录原有支持范围与实际原因，不直接删除它。

## 设计契约

### 模型、配置和命令

- 面板与管理窗口访问相同源模型，各自维护视图选择/查询状态；动作显式携带来源和选中项，不能依赖隐藏旧主窗口的“当前行”。
- 按功能迁移控制代码；保留已有配置名称、值和变更通知，QML 临时表单状态在 Apply 时写入同一配置。迁移完成前旧界面可以用于对照，但最终用户入口不得依赖它。
- 脚本/CLI 参数、输出/错误、session、快捷键、自动/显示命令都需要兼容验证。API 文档改动时运行已有生成器更新补全文档；仅换 UI 不应随意修改 API。
- 显示命令的异步结果绑定正确的条目和视图生命周期；关闭页面、删除条目或刷新后旧结果失效，不能串到其他记录。
- 富文本/FakeVim 编辑优先保留现有成熟引擎并重做承载窗口；选择纯 QML 编辑器必须先证明行为等价，再替换原实现。

### Alfred 式历史管理

能力参照 [Alfred Clipboard History](https://www.alfredapp.com/help/features/clipboard/)：按类型/期限记录、容量控制、暂停、按应用和敏感标记忽略、单项和最近时间范围清理。应用于现有历史采集/写入链路，不能依靠显示时隐藏条目来冒充未记录。

1. 在原采集链路取得可用来源标识，复用已取得的数据；按应用匹配优先使用可稳定识别的进程/应用身份，不能只靠可能变化的窗口标题。平台缺少元数据时明确边界。
2. 为新采集历史设计必要时间元数据，按既有内部 MIME 命名规则存入原记录并验证序列化/加密/导入导出。记录最近一次外部复制采集时间，浏览、预览、显示命令和普通编辑不刷新它；重复复制的去重更新时间行为在 I2-1 固定。
3. 旧记录缺少时间时保留“未知”，不伪造复制时间；自动到期及“最近若干分钟”删除不能凭猜测删除旧记录。用户显式全量清理按界面告知的范围执行。
4. 普通历史的期限/容量清理遵守固定项保护，与手工保存的其他标签页及 Iteration3 的 Snippet 生命周期分开；现有卸载缓存和密码到期行为保持原义。
5. 最近时间范围边界使用可测试的时钟语义，固定时区/夏令时/系统时间变化规则；先得到待清理条目集合，再走既有删除和插件钩子。暂停/忽略/失败时不得出现“先持久化再隐藏”的泄漏。
6. 自身粘贴/自动展开/合并产生的剪贴板事件如何记录由既有 owner/事件来源规则衔接 Iteration3；不能形成循环或重复历史。

### 全界面外观与旧主题

采用 Iteration1 的布局基线，I2-1 固定管理、设置、命令、编辑和辅助界面的样板及共用组件。QML 平台样式按实际定制需求选用，系统材质仅在支持的平台应用；无材质时仍保证对比度与完整布局。

旧 QSS、主题配置和导入导出必须做专项映射：保留原内容、支持的属性与生效区域，提供新界面的等价主题能力。QSS 无法自动控制 QML；只保存旧文本但使原有可定制能力失效，不能算 C4 完成。遇到无法等价的属性/交互，先给出具体兼容实现或可审阅替代方案，在解决前保留未完成项，不能静默缩小用户范围。

## 变更范围与文件清单

全界面迁移涉及十个以上文件，原因是多个独立窗口、插件与脚本可见行为均属于用户确认范围；按下表和里程碑分批，禁止同时无边界改写全部 GUI。2026-09-29 从 I2-1 开始实施，保留工作区中未提交的 SPEC1 实现。

| 操作 | 文件 / 位置 | 改动原因 |
|---|---|---|
| NEW | src/gui/qml/ManagementWindow.qml、Settings.qml、Commands.qml 及实际复用组件 | 新管理/设置/命令界面；基于 Iteration1 QML 模块，不另建 UI 工程。 |
| MODIFY | [mainwindow](../../../src/gui/mainwindow.cpp)、[clipboardbrowser](../../../src/gui/clipboardbrowser.cpp)、[clipboardbrowserplaceholder](../../../src/gui/clipboardbrowserplaceholder.cpp) | 迁移窗口控制与标签生命周期，承接原行为。 |
| MODIFY | [configurationmanager](../../../src/gui/configurationmanager.cpp)、[appconfig](../../../src/common/appconfig.h)、[config](../../../src/common/config.cpp) | 同一配置源、历史策略、加载/保存/取消语义。 |
| MODIFY | [commanddialog](../../../src/gui/commanddialog.cpp)、[commandedit](../../../src/gui/commandedit.cpp)、[actiondialog](../../../src/gui/actiondialog.cpp)、[src/scriptable](../../../src/scriptable) | 新命令 UI、补全/执行/显示动作与脚本状态接入。 |
| MODIFY | [itemfactory](../../../src/item/itemfactory.cpp)、[itemwidget](../../../src/item/itemwidget.h)、[persistentdisplayitem](../../../src/item/persistentdisplayitem.h)、[itemeditorwidget](../../../src/item/itemeditorwidget.cpp)、[plugins](../../../plugins) | 全插件呈现/设置/编辑兼容及显示命令生命周期。 |
| MODIFY | [clipboardserver](../../../src/app/clipboardserver.cpp)、[mimetypes](../../../src/common/mimetypes.h)、[serialize](../../../src/item/serialize.cpp) | 在原采集/持久化链路接入必要历史时间/来源信息；已有能力足够时不改序列化协议。 |
| MODIFY | [src/ui](../../../src/ui)、[theme](../../../src/gui/theme.cpp)、[shared/themes](../../../shared/themes)、[importexportdialog](../../../src/gui/importexportdialog.cpp) | 剩余辅助界面、主题和旧数据 UI；删除旧 UI 前有入口覆盖证据。 |
| MODIFY / NEW | [src/tests](../../../src/tests)、[docs/scripting-api.rst](../../../docs/scripting-api.rst)、[生成器](../../../utils/script_docs_to_cpp.py)、[构建](../../../src/CMakeLists.txt) | 聚焦回归与新界面测试；只有 API 确实变化时更新文档/补全，实际新增文件先检索命名。 |

## 实施里程碑

接续的插件呈现采用 `ClipboardItemPreview`：在 GUI 线程用原 ItemFactory 创建预览控件并渲染为带 DPR 的图像，Quick 的绘制线程只读取图像；鼠标、滚轮和只读键盘操作转发给原控件。选中项、显示命令副本或插件配置变化时销毁旧控件，不改 ItemLoaderInterface。管理和面板共用此桥接，未知格式仍保留原 MIME。原 QSS 在插件控件上继续由 Qt 样式引擎执行。

旧主题接续方案：颜色/字体仍映射到 QML；按钮、搜索背景和列表行通过 `ClipboardStyle` 使用原 Qt 样式引擎绘制，并映射 `MainWindow`、工具栏、`searchBar`、`ClipboardBrowser` 的原对象名与模板，保留渐变、图片、边框和状态规则。原 CSS 文本及模板导入/导出不改协议。新增 Theme 的 Quick 间距/行高/圆角字段作为新布局的等价调整入口，保留原字段。任意旧结构选择器在新结构中的语义不能凭保存文本宣称等价：在外观页显示实际覆盖范围，并把无法对应的新布局结构调整列为需审阅的兼容替代；第三方复杂控件绘制与性能也必须实测。

剩余辅助界面保留成熟控制器，但重排实际布局：关于改导航/正文，日志改级别侧栏/正文，进程改筛选/动作侧栏/表格，格式查看改标题/格式侧栏/内容，动作改执行参数/代码两栏，导入导出改范围/集合两栏，快捷键录入改说明/录入/确认。设置中的快捷键改侧栏导航，外观改分组表单和预览，插件设置改说明卡和可滚动表单。涉及对应 `src/gui/*dialog.cpp`、`shortcutswidget.cpp`、`configtabappearance.cpp`、`pluginwidget.cpp`、`actionhandlerdialog.cpp` 与测试；原字段、按钮信号、密码/同步/编辑引擎保持职责。此处的承载改造必须有控件和行为验证，不能只用全局 QSS 代替。

| 顺序 | 本段交付 | 完成条件 |
|---|---|---|
| I2-1 功能清单与管理窗口 | 补齐旧入口/配置/命令/插件映射、主题方案，迁移管理和标签/批量操作 | 有可审阅的界面样板和未迁移项列表；管理使用真实历史并通过相关动作测试。 |
| I2-2 设置、命令、编辑与插件 | 迁移所有设置、脚本编辑/补全、内容编辑及八个插件，完成辅助窗口 | 原入口均有新入口，插件数据/显示/设置/编辑均验证；撤销/取消和显示命令有效。 |
| I2-3 历史策略与兼容收尾 | 类型/期限/容量/忽略/清理、旧主题/配置/数据往返、三端布局与交互 | P5/P6/P7/P12 全部有对应证据，未覆盖项清零；新 Snippet 功能明确交给 Iteration3。 |

### I2-1 初始实施决策（2026-09-29，后续实现以当前实现表为准）

假设来自 SPEC1 代码和交接：ClipboardBrowser 仍拥有 ClipboardModel、ItemSaver 与标签持久化；ClipboardPaletteModel 已支持批次筛选、持久索引及显示命令副本，可直接作为管理视图的适配器。管理与面板各自实例化该适配器，只共享源历史。平台未完成项不阻止此共享实现。

- 新增 ClipboardManagement 独立 QQuickView 和 ManagementWindow.qml；管理窗口从面板及托盘进入，成功加载后隐藏旧主窗口。过渡期间原 Show/Hide 入口用于行为对照，不能计为全界面迁移完成；I2-2 完成前不得删除旧控制代码。强制卸载立即清空新视图和选择，重新点击集合复用 createBrowserAgain 的原加载/密码流程；Quick 正在显示的源不能因隐藏 Widgets 被自动卸载。
- 管理扩展现有适配器的多选能力：普通点击单选、Cmd/Ctrl 点击切换、Shift 连续选择、全选只覆盖当前筛选结果；索引跟随插入/移动，删除、reset、卸载和切换标签不转移到邻项。批量动作显式传入来源、选中索引及当前索引，不设置隐藏 QListView 的当前行来执行动作。
- 复用 ClipboardBrowser::copyIndexes/removeIndexes/sortItems/reverseItems/move/add、Tabs 与 MainWindow::renameTab/removeTab/createTab。ClipboardBrowser::transferIndexes 在写入目标前验证全部删除保护并取得脚本删除授权，取消时目标不插入；授权后重新校验持久索引和保护，再写入目标并执行既有插件删除钩子。固定项和脚本 onItemsRemoved 继续生效。
- 管理的自定义命令复用 commandsForMenu、selectionData、CommandAction 与异步 matchCmd；QML 暴露命令名称/键位/可用状态。显示命令继续通过 PersistentDisplayItem 绑定管理适配器，不写入源 MIME。
- QML 使用 Qt Quick Controls Basic；管理基线为左侧标签、顶部搜索/工具、中间多选列表、右侧预览/元数据、底部状态。共用主题组件映射 Theme 的 bg/fg/alt_bg/sel_bg/sel_fg、字体和编辑/搜索配色。设置为导航+表单，命令为列表+条件/脚本，编辑窗口保留 ItemEditorWidget/FakeVim 引擎并使用新承载布局。旧 QSS 原文/导入导出保留，任意选择器、控件几何及自定义 css_template 的等价映射仍属于 I2-3 未完成项；配色映射通过不等于 C4 通过。
- 历史时间规则固定为外部复制采集的 UTC epoch 毫秒；完全重复内容重新复制时更新同一条目的采集时间并沿用原去重移动；编辑/显示/预览不更新时间。旧记录保持未知；最近范围为闭区间 [now-duration, now]，未来时间和未知时间不自动删除。期限按 elapsed UTC 判断，系统时钟回拨时负年龄暂不清理；固定项和手工标签保护保持原义。实现和测试在 I2-3。

| 操作 | 本批实际路径 | 必须涉及的原因 |
|---|---|---|
| NEW | src/gui/clipboardmanagement.h、src/gui/clipboardmanagement.cpp、src/gui/qml/ManagementWindow.qml | 独立管理窗口与显式动作请求；不保存新历史。 |
| MODIFY | src/gui/clipboardpalettemodel.h、src/gui/clipboardpalettemodel.cpp、src/gui/clipboardbrowser.h、src/gui/clipboardbrowser.cpp | 复用筛选/展示，加入管理多选和元数据角色；保持面板单选契约；跨标签移动在目标写入前取得源删除授权。 |
| NEW / MODIFY | src/gui/mainwindow_management.cpp、src/gui/mainwindow.h、src/gui/mainwindow.cpp、src/gui/clipboardpalette.h、src/gui/clipboardpalette.cpp、src/gui/qml/ClipboardPalette.qml | 复用标签/插件/命令/动作并提供面板与托盘的新入口；图片 provider 移到已有适配器供两个窗口共用。 |
| NEW / MODIFY | src/gui/qml/Theme.qml、src/gui/theme.h、src/gui/theme.cpp | 同一旧主题数据映射，避免管理另存一套配色。 |
| MODIFY | src/scriptable/scriptableproxy.cpp | 管理可见时 CLI 选中项读取/写入新视图；命令携带的显式快照优先。 |
| MODIFY | src/gui/clipboardbrowserplaceholder.h、src/gui/clipboardbrowserplaceholder.cpp | 原缓存/密码到期判断纳入真实 Quick 源可见性；不把视图迁移误当成标签不活跃。 |
| NEW / MODIFY | src/tests/tests_management.cpp、src/tests/tests_managementserver.cpp、src/tests/tests.h、src/tests/tests.cpp、src/tests/itemtests/itemtests.cpp、src/tests/tests.cmake、src/CMakeLists.txt | 真实源模型、QML 管理动作、删除保护及窗口生命周期聚焦测试和资源打包；测试插件通过 QObject 元调用检查真实服务的管理窗口。原测试初始化仅清配置路径，新隔离 runner 的 tabDataFileBasePath 位于独立数据路径的父目录；补齐该目录的同名测试文件清理，避免指定函数之间保留上次历史。 |
| MODIFY | utils/github/test-linux.sh、utils/github/test-macos.sh、utils/github/deploy-windows.sh、.github/workflows/build-windows.yml | 把指定管理测试接入原隔离 runner；Windows 部署并分离新测试可执行文件。CI 配置通过不表示本机 Windows 验收通过。 |
| NEW / MODIFY | utils/check-compatibility.py、本文、docs/astack/INDEX.md、CLAUDE.md | 逐项入口清单与机械检查、实施状态和新鲜证据。 |

本批成功条件：真 QML 管理窗口加载；独立查询/多选保持条目身份；标签增删改排序与属性持久化；批量复制/排序/移动/删除遵守插件保护；自定义命令收到显式选择；面板不回归。验证使用 `copyq-management-tests` 指定函数、原生隔离 runner、QML lint、文档质量门；不使用日常历史。

### 当前实现与后续入口（2026-09-29）

本轮已实施 I2-1–I2-3 的管理、设置、命令、编辑/辅助界面、插件桥接及历史策略代码，本机可独立实施的开发已落地，当前进入平台与兼容验收。登记清单为 49 个动作、90 项 AppConfig（原 87 项加 3 项历史策略）、24 个主表单、7 个插件表单、149 个文档 API 和八插件。代码入口覆盖与行为、平台、安装包验收分别记录，P5/P6/P7/P12 尚未通过完整三端验收。

| 里程碑 | 已实施的新入口与数据契约 | 剩余验收 |
|---|---|---|
| I2-1 | Quick 管理为 Show/Hide/Toggle/ShowAt 主入口；真正集合树/平铺选项、折叠持久化、图标、组级改名/删除/移动/排序、碰撞与过期快照保护；完整 MIME 的多选、排序、移动、拖放和受保护删除；原生菜单/命令使用实际管理选择。 | 原生 Windows、系统输入法、多屏/DPI、外部前台粘贴。 |
| I2-2 | Settings 使用原 ConfigurationManager 的 90 项字段、草稿 Apply/Cancel/默认值与插件启用/排序；Commands 保存 23 个可编辑字段及原 InternalId/本地化名称，保留导入导出、内置模板、代码补全和安全语法检查；编辑/CLI 复用 ItemEditorWidget/FakeVim，完整格式栏、搜索、撤销重做、保存/取消及失效源保留输入。 | macOS 当前前台为 loginwindow，服务键盘/辅助对话框焦点仍需可激活桌面；实际安装包八插件加载与旧二进制 ABI。 |
| I2-2 插件/辅助界面 | 原 ItemFactory 呈现桥接到面板/管理预览，显示命令副本绑定持久索引；七插件表单、外观、快捷键、集合属性保留控制器并重新布局；关于、日志、进程、格式查看、动作、导入导出、密码、图标/快捷键、通知/托盘、脚本输入/截图入口均有新承载。原 ItemSaver 的 Quick 可见集合更新已接续。 | 复杂第三方 QWidget/OpenGL 插件、插件桥接长时性能；富文本和原生辅助窗口的完整三端人工验收。 |
| I2-3 | 原采集链路的类型/应用忽略/敏感/暂停规则、UTC 时间/来源元数据、重复采集更新、普通历史期限/最近范围清理，遵守固定项/脚本/插件保护；旧配置/主题/数据格式保留，原 QSS 引擎绘制按钮/搜索/行/集合/菜单，提供 Quick 布局参数。 | 任意旧结构选择器与自定义 css_template 无法自动等价；已提供具体映射和布局替代，但 C4 完整结论仍需审阅与实机确认，不能按保存文本宣称通过。 |

截图由隔离的实际 Quick 窗口产生，路径为本机 `build/spec2-artifacts/management.png` 与 `management-compact.png`。800×520 和正常尺寸布局检查不能替代真实中文输入法、多屏/DPI 或 Windows 前台验收。Linux 当前为 Debian 12 arm64/Xvfb/openbox，关闭 QCA/原生通知/音频，Wayland 未验证；macOS 当前 Qt 依赖的最低系统版本高于 preset 的 macOS 13，不能证明 macOS 13 运行。

后续从本文最新验证记录接续平台/兼容验收；三个里程碑的完整完成条件仍未满足，文档保持开发中。共享接口为 ClipboardManagement/ClipboardPaletteModel、ClipboardSettings/ClipboardCommands、MainWindow::openItemEditor、ClipboardItemPreview/ClipboardStyle 与 HistoryPolicy；成熟 Widgets 仅作为原引擎/插件/原生辅助承载，隐藏旧管理窗口不作为用户入口。SPEC3 复用这些接口，不另建配置或历史。

### 剩余开发接续（2026-09-29）

用户在 `4a6e1843` 提交并推送后要求继续完成本 SPEC 的所有剩余开发。执行顺序和检查点见 [实施计划](../plan/Iteration2_CopyQCompatibility_PLAN.md)。以下假设以已提交代码为依据：源码树和构建目录可复用；本机没有 Windows 实机，macOS 前台可用性不能预设；纯代码和隔离 Linux 验证可以继续，平台缺项不能虚报通过。

历史策略实施补充：新增 `clipboard_history_types`（text/image/files/other，默认全部）、`clipboard_history_ignore_apps`（每行一个稳定身份，大小写不敏感精确匹配）和 `clipboard_history_max_days`（0 不自动到期），继续复用原 maxitems、clipboard_tab 和暂停开关。字段经原 ConfigurationManager 保存，展示在新 History 页。记录使用 `application/x-copyq-private-history-time` 的 ASCII UTC 毫秒和 `application/x-copyq-private-source-application` 的 UTF-8 身份；它们不参与内容哈希，保留在原完整 MIME 记录中。

来源与窗口标题同一次获取并使用同一延迟队列，macOS 为所持 NSRunningApplication 的 bundle ID，Windows 为所持 HWND 进程完整路径，X11 为 WM_CLASS。Wayland/查询失败/脚本覆盖 owner 且未调用原窗口采集时身份为空，不借标题匹配伪造身份。此元数据沿用原窗口 owner 的采样语义，后台程序修改剪贴板或快速切换应用时不能保证识别真实写入者；该边界属于平台验收，忽略名单按实际可用身份执行。

外部采集先检查类型/忽略/敏感标记，允许后才添加时间并触发自动命令/保存；重复事件仅更新已存在记录的时间和来源，不新建记录或重跑自动动作。到期仅处理配置的普通历史集合，每分钟检查，未知/未来时间和固定项保留；先得到持久索引，再复用原删除授权和钩子。最近清理显式确认 5/15/60 分钟、24 小时或全部普通历史，时间计算与候选选择用指定 now 的聚焦测试覆盖。涉及 `clipboardmonitor/clipboardownermonitor`、三端 PlatformWindow、Scriptable/Proxy、`mimetypes/textdata`、新 `common/historypolicy`、ClipboardBrowser、MainWindow/Management 和测试，原因分别为采集前过滤、复用来源、完整数据去重、受保护删除与界面入口。

1. 管理收尾：基于原集合路径构造有展开状态的树，不增加另一套集合持久化。组级增删改/移动在操作前固定受影响集合，逐项调用原接口；避免路径前缀误伤同名集合。图标、上下文动作、默认激活、窗口位置、主窗口/CLI 的 Show/Hide/Toggle/ShowAt 接入实际 Quick 窗口。
2. 设置：新 QML 设置窗口显示原 ConfigurationManager 的字段与类型、说明、默认值，修改先进入表单草稿，Apply/OK 才走原配置保存/通知；Cancel 不写入。插件设置、主题和快捷键中成熟 Widgets 控件在重新布局的承载窗口中复用，原 ConfigurationManager 对话框不作为新用户入口。插件启用/顺序、语言/自动启动/密码等特殊职责仍走原实现。
3. 命令与编辑：新命令导航和表单完整保留 Command 字段、导入导出和内置命令；代码编辑保留补全/高亮引擎，富文本/FakeVim 保留成熟编辑引擎，但重做布局、格式入口、保存/取消和失效源提示。辅助窗口逐项重排内容/工具/状态，不以统一 QSS 或保留旧管理窗口代替迁移。
4. 插件：正式插件设置/排序、显示副本、备注/标签/固定、加密与文件同步都接入真实数据和原插件钩子。第三方 QWidget 插件界面提供重新设计的原生承载；不修改插件虚接口，不宣称未经验证的旧二进制 ABI。
5. 历史：采集时间和来源信息加入原采集/保存链路；类型/来源/敏感标记在持久化前过滤，年龄/最近范围清理先取得源集合再执行原保护/删除钩子。保留未知时间/未来时间/时钟回拨、固定项与手工集合的边界。
6. 收尾：旧配置/主题/数据往返和 QML 主题等价映射逐项验证；原生 Qt 文件选择/密码输入保留平台能力但重做承载。代码完成、隔离验证、原生 Windows/macOS/Linux 验收分别记录，不能因平台缺设备就停止可实施的开发。

运行兼容收尾还必须接续 ItemSaver::setFocus：Quick 正在呈现的集合启用原同步器的更新，切换集合、关闭或最小化窗口后撤销；不把焦点伪造到隐藏 QListView。Linux X11 的真实剪贴板序号变化即使内容相同也通知上层，定时轮询未变时不重复通知，以实现本阶段已固定的重复复制时间规则。编辑器搜索回车消耗原按键，不向保存按钮或编辑器转发；管理搜索 Tab 进入结果列表。管理快捷键只匹配集合/管理动作，编辑器专用的 Editor_Save–Editor_Search 留给实际编辑器，避免 Editor_Save 的 F2 抢占 Item_Edit 的 F2；可输入状态使用 Qt 的 ItemAcceptsInputMethod 标志，不把带 text 属性的按钮误当成文本输入。

编辑器分别接续 save 与 invalidate：FakeVim 的 :w 保存并继续编辑，:wq 及普通保存快捷键保存后退出；新建首次保存后保持同一持久条目身份，后续保存不重复插入。CLI/窗口重新编辑需等待实际编辑控件取得焦点，旧数据断言保留。

窗口析构先断开自身可见性/状态回调、释放 QML 根对象并隐藏窗口，再销毁成员模型，避免 QWindow 基类析构回调访问已销毁模型。工具栏通过原 QToolButton 绘制保留实际 QSS 文字/按下/悬停状态，并补 Quick 键盘焦点框；像素断言分别检查主窗口主题开启和关闭时的前景色。

显示命令副本按已呈现条目的持久索引缓存：可见 delegate 与选中预览共用同一副本，普通插入/移动不能重复执行同一条目的显示脚本。源数据修改、删除、筛选变化、源切换及窗口关闭使对应旧请求失效；不把未选中的行限制为原数据摘要。涉及 ClipboardPaletteModel、PersistentDisplayItem、两份 QML delegate 与生命周期/服务回归，不改变显示命令协议或源 MIME。

| 操作 | 接续文件 / 组件 | 原因 |
|---|---|---|
| MODIFY | ClipboardManagement、ManagementWindow.qml、mainwindow_management.cpp、ScriptableProxy、MainWindow | 集合树/组操作、主界面入口及显式动作/几何完整接入。 |
| NEW / MODIFY | ClipboardSettings、Settings.qml、ConfigurationManager、Option、各设置/插件表单 | 新设置 UI 与原配置草稿、成熟插件控件的新承载、保存/取消/默认值验证。 |
| NEW / MODIFY | ClipboardCommands、Commands.qml、CommandDialog/CommandEdit/CommandWidget、编辑与辅助窗口 | 完整命令表单、代码编辑器复用、内容/辅助界面重排。 |
| NEW / MODIFY | 历史策略、AppConfig、MIME 常量、ClipboardMonitor/PlatformClipboard/PlatformWindow、Scriptable/saveData、ClipboardBrowser | 原采集链路的来源/时间、采集前过滤及保护清理；不另建存储。 |
| MODIFY | Theme、QML 共用组件、插件呈现、针对性测试、CMake/CI、本文/INDEX | 统一主题及兼容校验、资源打包、逐段新鲜证据和交接。 |

### I2-1 逐项兼容基线

以下清单登记实际新入口；“入口已迁移”不等于该行所有平台/边界均通过。`python3 utils/check-compatibility.py` 核对源码入口与本文登记的集合，不能替代运行验收。配置与 API 按名称逐项登记，旧参数、错误和输出继续以原实现为准。

| 范围 | 登记数 | 当前职责 |
|---|---|---|
| 菜单/编辑动作 | 49 | I2-1 登记，I2-2/I2-3 逐项验证 |
| 主工程表单 | 24 | I2-1 登记，I2-2/I2-3 逐项验证 |
| 插件设置表单 | 7 | I2-1 登记，I2-2/I2-3 逐项验证 |
| AppConfig 配置 | 90 | I2-1 登记，I2-2/I2-3 逐项验证 |
| 文档化脚本函数（重载合并，包含同声明别名） | 149 | I2-1 登记，I2-2/I2-3 逐项验证 |
| 内置插件 | 8 | I2-1 登记，I2-2/I2-3 逐项验证 |

| 旧入口标识 | 新入口 / 验证归属 | 当前状态 |
|---|---|---|
| `action:Edit_CopySelectedItems` | ManagementWindow 复制 / managementActions | macOS/Linux 已通过 |
| `action:Edit_FindItems` | 管理搜索和原 filter() / qmlSelectionAndActions、managementActions | QML/CLI 已通过；实机快捷键待补 |
| `action:Edit_PasteItems` | ManagementWindow 粘贴到集合 / managementActions | macOS/Linux 已通过 |
| `action:Edit_ReverseSelectedItems` | 原 reverseItems 语义 / managementActions | macOS/Linux 已通过 |
| `action:Edit_SortSelectedItems` | 原 sortItems 语义 / managementActions | macOS/Linux 已通过 |
| `action:Editor_Background` | 新编辑窗口完整格式栏、原 ItemEditorWidget 动作 / managementEditor | 入口已迁移；Linux 键盘保存/取消/搜索/撤销重做，格式引擎保留；macOS 前台待验收 |
| `action:Editor_Bold` | 新编辑窗口完整格式栏、原 ItemEditorWidget 动作 / managementEditor | 入口已迁移；Linux 键盘保存/取消/搜索/撤销重做，格式引擎保留；macOS 前台待验收 |
| `action:Editor_Cancel` | 新编辑窗口完整格式栏、原 ItemEditorWidget 动作 / managementEditor | 入口已迁移；Linux 键盘保存/取消/搜索/撤销重做，格式引擎保留；macOS 前台待验收 |
| `action:Editor_EraseStyle` | 新编辑窗口完整格式栏、原 ItemEditorWidget 动作 / managementEditor | 入口已迁移；Linux 键盘保存/取消/搜索/撤销重做，格式引擎保留；macOS 前台待验收 |
| `action:Editor_Font` | 新编辑窗口完整格式栏、原 ItemEditorWidget 动作 / managementEditor | 入口已迁移；Linux 键盘保存/取消/搜索/撤销重做，格式引擎保留；macOS 前台待验收 |
| `action:Editor_Foreground` | 新编辑窗口完整格式栏、原 ItemEditorWidget 动作 / managementEditor | 入口已迁移；Linux 键盘保存/取消/搜索/撤销重做，格式引擎保留；macOS 前台待验收 |
| `action:Editor_Italic` | 新编辑窗口完整格式栏、原 ItemEditorWidget 动作 / managementEditor | 入口已迁移；Linux 键盘保存/取消/搜索/撤销重做，格式引擎保留；macOS 前台待验收 |
| `action:Editor_Redo` | 新编辑窗口完整格式栏、原 ItemEditorWidget 动作 / managementEditor | 入口已迁移；Linux 键盘保存/取消/搜索/撤销重做，格式引擎保留；macOS 前台待验收 |
| `action:Editor_Save` | 新编辑窗口完整格式栏、原 ItemEditorWidget 动作 / managementEditor | 入口已迁移；Linux 键盘保存/取消/搜索/撤销重做，格式引擎保留；macOS 前台待验收 |
| `action:Editor_Search` | 新编辑窗口完整格式栏、原 ItemEditorWidget 动作 / managementEditor | 入口已迁移；Linux 键盘保存/取消/搜索/撤销重做，格式引擎保留；macOS 前台待验收 |
| `action:Editor_Strikethrough` | 新编辑窗口完整格式栏、原 ItemEditorWidget 动作 / managementEditor | 入口已迁移；Linux 键盘保存/取消/搜索/撤销重做，格式引擎保留；macOS 前台待验收 |
| `action:Editor_Underline` | 新编辑窗口完整格式栏、原 ItemEditorWidget 动作 / managementEditor | 入口已迁移；Linux 键盘保存/取消/搜索/撤销重做，格式引擎保留；macOS 前台待验收 |
| `action:Editor_Undo` | 新编辑窗口完整格式栏、原 ItemEditorWidget 动作 / managementEditor | 入口已迁移；Linux 键盘保存/取消/搜索/撤销重做，格式引擎保留；macOS 前台待验收 |
| `action:File_Commands` | Commands 全字段/模板/导入导出 / commandDraftRoundTrip | 入口已迁移；相关行为见最新验证，完整三端验收待补 |
| `action:File_Exit` | 原退出行为 / managementActions | 入口已迁移；相关行为见最新验证，完整三端验收待补 |
| `action:File_Export` | 新导出范围/集合窗口 / importExportTab | 入口已迁移；相关行为见最新验证，完整三端验收待补 |
| `action:File_Import` | 新导入范围/集合窗口 / importExportTab | 入口已迁移；相关行为见最新验证，完整三端验收待补 |
| `action:File_New` | 管理新建编辑窗口 / managementEditor | Linux 保存/取消通过；macOS 前台待验收 |
| `action:File_Preferences` | Settings 草稿表单与重新布局的原生页 / settingsDraftTransaction/allPluginSettings | 入口已迁移；相关行为见最新验证，完整三端验收待补 |
| `action:File_ProcessManager` | 进程筛选侧栏/动作表格 / managementDialogs | 入口已迁移；相关行为见最新验证，完整三端验收待补 |
| `action:File_ShowClipboardContent` | 原始 MIME 格式侧栏/内容窗口 / managementDialogs | 入口已迁移；相关行为见最新验证，完整三端验收待补 |
| `action:File_ShowPreview` | ItemFactory 真实插件预览及原显示副本 / pluginPreviewBridge | 入口已迁移；相关行为见最新验证，完整三端验收待补 |
| `action:File_ToggleClipboardStoring` | 管理暂停/恢复 / managementActions | macOS/Linux 已通过；类型/期限/忽略见历史回归 |
| `action:Help_About` | 关于导航/正文窗口 / managementDialogs | 入口已迁移；相关行为见最新验证，完整三端验收待补 |
| `action:Help_Help` | 原帮助网页入口 / 原实现保留 | 入口已迁移；相关行为见最新验证，完整三端验收待补 |
| `action:Help_ShowLog` | 日志级别侧栏/内容 / managementDialogs | 入口已迁移；相关行为见最新验证，完整三端验收待补 |
| `action:ItemMenu` | 管理 Actions 菜单及原自定义命令 / managementCommands | 显式多选和 matchCmd 通过；原快捷键已接入；菜单/QSS 三端验收待补 |
| `action:Item_Action` | 动作参数/代码承载，显式选中数据 / managementDialogs | 入口已迁移；相关行为见最新验证，完整三端验收待补 |
| `action:Item_Edit` | 共用 ItemEditorWidget、其他 MIME 保留 / managementEditor | Linux 已通过；FakeVim 指定回归见最新记录 |
| `action:Item_EditNotes` | 备注独立合并保存 / managementEditor | Linux 已通过；macOS 前台焦点未通过 |
| `action:Item_EditWithEditor` | 原外部编辑器，显式选中条目；CLI 回退新编辑窗口 / managementEditor | 入口已迁移；相关行为见最新验证，完整三端验收待补 |
| `action:Item_MoveDown` | 原 move / managementActions | macOS/Linux 已通过 |
| `action:Item_MoveToBottom` | 原 move / managementActions | macOS/Linux 已通过 |
| `action:Item_MoveToClipboard` | 原 moveToClipboard 显式选择 / managementActions、qmlSelectionAndActions | 复制/置顶通过；默认动作已接入；外部粘贴实机待补 |
| `action:Item_MoveToTop` | 原 move / managementActions | macOS/Linux 已通过 |
| `action:Item_MoveUp` | 原 move / managementActions | macOS/Linux 已通过 |
| `action:Item_Remove` | 原 removeIndexes 及插件/脚本保护 / managementActions、transferAndDeleteProtection | macOS/Linux 已通过 |
| `action:Item_ShowContent` | 原始格式查看，显式条目数据 / managementDialogs | 入口已迁移；相关行为见最新验证，完整三端验收待补 |
| `action:Tabs_ChangeTabIcon` | 新图标搜索/文件/默认图标 / managementGroups | 入口已迁移；相关行为见最新验证，完整三端验收待补 |
| `action:Tabs_NewTab` | QML 集合表单、原 createTab / managementTabs | 增改删/路径式分组通过；组级管理见 managementGroups |
| `action:Tabs_NextTab` | 原 nextTab / managementTabs | macOS/Linux 已通过 |
| `action:Tabs_PreviousTab` | 原 previousTab / managementTabs | macOS/Linux 已通过 |
| `action:Tabs_RemoveTab` | QML 确认、原 removeTab / managementTabs | 单集合删除通过；组级删除/快照保护见 managementGroups |
| `action:Tabs_RenameTab` | QML 来源快照、原 renameTab / managementTabs、qmlSelectionAndActions | 单集合更名通过；组级更名/碰撞保护见 managementGroups |
| `form:src/ui/aboutdialog.ui` | 关于导航/正文窗口；managementDialogs | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/actiondialog.ui` | 动作参数/代码两栏；managementDialogs | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/actionhandlerdialog.ui` | 进程筛选侧栏/动作表格；managementDialogs | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/addcommanddialog.ui` | 内置命令选择/Commands 模板入口；commandDraftRoundTrip | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/clipboarddialog.ui` | 原始格式侧栏/内容窗口；managementDialogs | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/commanddialog.ui` | Commands 列表/条件/脚本；commandDraftRoundTrip | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/commandedit.ui` | 重排的 CommandEdit 代码承载；commandDraftRoundTrip | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/commandwidget.ui` | Commands 全字段草稿与代码承载；commandDraftRoundTrip | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/configtabappearance.ui` | 外观分组卡片/预览/Quick 布局；allPluginSettings、legacyStyleRules | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/configtabgeneral.ui` | Settings General/原语言与密码入口；settingsDraftTransaction | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/configtabhistory.ui` | Settings History；settingsDraftTransaction、historyTimeAndProtection、managementHistoryCapture | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/configtablayout.ui` | Settings Layout；settingsDraftTransaction、managementGeometry | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/configtabnotifications.ui` | Settings Notifications；settingsDraftTransaction | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/configtabtray.ui` | Settings Tray；settingsDraftTransaction | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/configurationmanager.ui` | Settings 原字段/保存控制器；settingsDraftTransaction | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/importexportdialog.ui` | 导入导出范围/集合两栏；importExportTab | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/itemorderlist.ui` | Plugins 启用/优先级；settingsDraftTransaction、allPluginSettings | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/logdialog.ui` | 日志级别侧栏/内容；managementDialogs | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/mainwindow.ui` | ManagementWindow 为主入口；managementActions/managementTabs/managementGroups/managementCommands | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/pluginwidget.ui` | 插件说明卡/可滚动表单；pluginSettingsDraft、allPluginSettings | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/shortcutdialog.ui` | 快捷键说明/录入/确认新承载；allPluginSettings | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/shortcutswidget.ui` | 快捷键侧栏导航/原表格；allPluginSettings | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/tabdialog.ui` | QML 集合/组改名及新建；managementTabs/managementGroups/qmlSelectionAndActions | 入口已迁移；保留原引擎，完整三端验收待补 |
| `form:src/ui/tabpropertieswidget.ui` | QML 容量/持久化/密码期限与重排原生页；managementTabs/qmlSelectionAndActions | 入口已迁移；保留原引擎，完整三端验收待补 |
| `plugin-form:plugins/itemencrypted/itemencryptedsettings.ui` | Settings Plugins → 重新布局的原生配置承载 / pluginSettingsDraft、allPluginSettings | 原字段/加载/取消/应用均接入；实际安装包与三端验收待补 |
| `plugin-form:plugins/itemfakevim/itemfakevimsettings.ui` | Settings Plugins → 重新布局的原生配置承载 / pluginSettingsDraft、allPluginSettings | 原字段/加载/取消/应用均接入；实际安装包与三端验收待补 |
| `plugin-form:plugins/itemimage/itemimagesettings.ui` | Settings Plugins → 重新布局的原生配置承载 / pluginSettingsDraft、allPluginSettings | 原字段/加载/取消/应用均接入；实际安装包与三端验收待补 |
| `plugin-form:plugins/itemnotes/itemnotessettings.ui` | Settings Plugins → 重新布局的原生配置承载 / pluginSettingsDraft、allPluginSettings | 原字段/加载/取消/应用均接入；实际安装包与三端验收待补 |
| `plugin-form:plugins/itemsync/itemsyncsettings.ui` | Settings Plugins → 重新布局的原生配置承载 / pluginSettingsDraft、allPluginSettings | 原字段/加载/取消/应用均接入；实际安装包与三端验收待补 |
| `plugin-form:plugins/itemtags/itemtagssettings.ui` | Settings Plugins → 重新布局的原生配置承载 / pluginSettingsDraft、allPluginSettings | 原字段/加载/取消/应用均接入；实际安装包与三端验收待补 |
| `plugin-form:plugins/itemtext/itemtextsettings.ui` | Settings Plugins → 重新布局的原生配置承载 / pluginSettingsDraft、allPluginSettings | 原字段/加载/取消/应用均接入；实际安装包与三端验收待补 |

| 原配置（名称保持） | 新入口 / 验证归属 | 当前状态 |
|---|---|---|
| `option:activate_closes` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:activate_focuses` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:activate_item_with_single_click` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:activate_pastes` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:always_on_top` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:autocompletion` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:autostart` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:check_clipboard` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:clipboard_history_types` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig；历史策略见 historyCaptureFilters/historyTimeAndProtection/managementHistoryCapture | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:clipboard_history_ignore_apps` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig；历史策略见 historyCaptureFilters/historyTimeAndProtection/managementHistoryCapture | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:clipboard_history_max_days` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig；历史策略见 historyCaptureFilters/historyTimeAndProtection/managementHistoryCapture | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:check_selection` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:clipboard_mime_size_limit` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:clipboard_notification_lines` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:clipboard_tab` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:close_on_unfocus` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:close_on_unfocus_delay_ms` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:close_on_unfocus_extra_delay_ms` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:command_history_size` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:confirm_exit` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:copy_clipboard` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:copy_selection` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:disable_tray` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:edit_ctrl_return` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:editor` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:encrypt_tabs` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:expire_encrypted_tab_seconds` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:expire_tab` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:filter_case_insensitive` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:filter_regular_expression` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:frameless_window` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:hide_main_window` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:hide_main_window_in_task_bar` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:hide_tabs` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:hide_toolbar` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:hide_toolbar_labels` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:item_data_threshold` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:item_popup_interval` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:max_process_manager_rows` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:maxitems` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:move` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:native_menu_bar` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:native_notifications` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:native_tray_menu` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:navigation_style` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:notification_horizontal_offset` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:notification_maximum_height` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:notification_maximum_width` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:notification_position` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:notification_vertical_offset` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:number_search` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:open_windows_on_current_screen` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:prevent_screen_capture` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:restore_geometry` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:row_index_from_one` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:run_selection` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:save_delay_ms_on_item_added` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:save_delay_ms_on_item_edited` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:save_delay_ms_on_item_modified` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:save_delay_ms_on_item_moved` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:save_delay_ms_on_item_removed` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:save_filter_history` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:save_on_app_deactivated` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:script_paste_delay_ms` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:show_advanced_command_settings` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:show_simple_items` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:show_tab_item_count` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:style` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:tab_tree` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:tabs` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:terminate_action_timeout_ms` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:text_tab_width` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:text_wrap` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:transparency` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:transparency_focused` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:tray_commands` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:tray_images` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:tray_item_paste` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:tray_items` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:tray_menu_open_on_left_click` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:tray_tab` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:tray_tab_is_current` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:update_clipboard_owner_delay_ms` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:use_key_store` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:window_key_press_time_ms` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:window_paste_with_ctrl_v_regex` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:window_wait_after_raised_ms` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:window_wait_before_raise_ms` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:window_wait_for_modifier_released_ms` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |
| `option:window_wait_raised_ms` | Settings 原名称/类型/范围/默认值及 config() / settingsDraftTransaction、commandConfig | 已纳入同一配置草稿；字段/事务验证见最新记录，逐平台生效语义待验收 |

| 原脚本 API（名称及参数保持） | 新入口 / 验证归属 | 当前状态 |
|---|---|---|
| `api:abort`、`api:action`、`api:add`、`api:addCommands`、`api:afterMilliseconds`、`api:change`、`api:clearClipboardData`、`api:clipboard` | 原 Scriptable/Proxy；实际 Quick 窗口/显式选择，新命令编辑/补全；针对性 CLI 验证 | 原接口保留；相关 CLI/窗口/选择回归见最新记录，非 149 API 穷尽验证 |
| `api:clipboardFormatsToSave`、`api:commands`、`api:config`、`api:copy`、`api:copySelection`、`api:count`、`api:currentItem`、`api:currentPath` | 原 Scriptable/Proxy；实际 Quick 窗口/显式选择，新命令编辑/补全；针对性 CLI 验证 | 原接口保留；相关 CLI/窗口/选择回归见最新记录，非 149 API 穷尽验证 |
| `api:data`、`api:dataFormats`、`api:dateString`、`api:dialog`、`api:disable`、`api:edit`、`api:editItem`、`api:env` | 原 Scriptable/Proxy；实际 Quick 窗口/显式选择，新命令编辑/补全；针对性 CLI 验证 | 原接口保留；相关 CLI/窗口/选择回归见最新记录，非 149 API 穷尽验证 |
| `api:escapeHtml`、`api:eval`、`api:execute`、`api:exit`、`api:exportData`、`api:exportTab`、`api:fail`、`api:filter` | 原 Scriptable/Proxy；实际 Quick 窗口/显式选择，新命令编辑/补全；针对性 CLI 验证 | 原接口保留；相关 CLI/窗口/选择回归见最新记录，非 149 API 穷尽验证 |
| `api:focusPrevious`、`api:focused`、`api:forceUnload`、`api:fromBase64`、`api:fromUnicode`、`api:getItem`、`api:hasClipboardFormat`、`api:hasData` | 原 Scriptable/Proxy；实际 Quick 窗口/显式选择，新命令编辑/补全；针对性 CLI 验证 | 原接口保留；相关 CLI/窗口/选择回归见最新记录，非 149 API 穷尽验证 |
| `api:hasSelectionFormat`、`api:help`、`api:hide`、`api:hideDataNotification`、`api:iconColor`、`api:iconTag`、`api:iconTagColor`、`api:ignore` | 原 Scriptable/Proxy；实际 Quick 窗口/显式选择，新命令编辑/补全；针对性 CLI 验证 | 原接口保留；相关 CLI/窗口/选择回归见最新记录，非 149 API 穷尽验证 |
| `api:importData`、`api:importTab`、`api:info`、`api:input`、`api:insert`、`api:isClipboard`、`api:loadTheme`、`api:logs` | 原 Scriptable/Proxy；实际 Quick 窗口/显式选择，新命令编辑/补全；针对性 CLI 验证 | 原接口保留；相关 CLI/窗口/选择回归见最新记录，非 149 API 穷尽验证 |
| `api:md5sum`、`api:menu`、`api:menuItems`、`api:monitorClipboard`、`api:monitoring`、`api:move`、`api:next`、`api:notification` | 原 Scriptable/Proxy；实际 Quick 窗口/显式选择，新命令编辑/补全；针对性 CLI 验证 | 原接口保留；相关 CLI/窗口/选择回归见最新记录，非 149 API 穷尽验证 |
| `api:onClipboardChanged`、`api:onClipboardUnchanged`、`api:onExit`、`api:onHiddenClipboardChanged`、`api:onItemsAdded`、`api:onItemsChanged`、`api:onItemsLoaded`、`api:onItemsRemoved` | 原 Scriptable/Proxy；实际 Quick 窗口/显式选择，新命令编辑/补全；针对性 CLI 验证 | 原接口保留；相关 CLI/窗口/选择回归见最新记录，非 149 API 穷尽验证 |
| `api:onOwnClipboardChanged`、`api:onSecretClipboardChanged`、`api:onStart`、`api:onTabSelected`、`api:open`、`api:pack`、`api:palette`、`api:paste` | 原 Scriptable/Proxy；实际 Quick 窗口/显式选择，新命令编辑/补全；针对性 CLI 验证 | 原接口保留；相关 CLI/窗口/选择回归见最新记录，非 149 API 穷尽验证 |
| `api:playSound`、`api:pointerPosition`、`api:popup`、`api:preview`、`api:previous`、`api:print`、`api:provideClipboard`、`api:provideSelection` | 原 Scriptable/Proxy；实际 Quick 窗口/显式选择，新命令编辑/补全；针对性 CLI 验证 | 原接口保留；相关 CLI/窗口/选择回归见最新记录，非 149 API 穷尽验证 |
| `api:queryKeyboardModifiers`、`api:read`、`api:remove`、`api:removeData`、`api:removeTab`、`api:renameTab`、`api:runAutomaticCommands`、`api:saveData` | 原 Scriptable/Proxy；实际 Quick 窗口/显式选择，新命令编辑/补全；针对性 CLI 验证 | 原接口保留；相关 CLI/窗口/选择回归见最新记录，非 149 API 穷尽验证 |
| `api:screenNames`、`api:screenshot`、`api:screenshotSelect`、`api:select`、`api:selectItems`、`api:selectedItemData`、`api:selectedItems`、`api:selectedItemsData` | 原 Scriptable/Proxy；实际 Quick 窗口/显式选择，新命令编辑/补全；针对性 CLI 验证 | 原接口保留；相关 CLI/窗口/选择回归见最新记录，非 149 API 穷尽验证 |
| `api:selectedTab`、`api:selection`、`api:separator`、`api:serverLog`、`api:setClipboardData`、`api:setCommands`、`api:setCurrentTab`、`api:setData` | 原 Scriptable/Proxy；实际 Quick 窗口/显式选择，新命令编辑/补全；针对性 CLI 验证 | 原接口保留；相关 CLI/窗口/选择回归见最新记录，非 149 API 穷尽验证 |
| `api:setEnv`、`api:setItem`、`api:setPointerPosition`、`api:setSelectedItemData`、`api:setSelectedItemsData`、`api:setTitle`、`api:settings`、`api:sha1sum` | 原 Scriptable/Proxy；实际 Quick 窗口/显式选择，新命令编辑/补全；针对性 CLI 验证 | 原接口保留；相关 CLI/窗口/选择回归见最新记录，非 149 API 穷尽验证 |
| `api:sha256sum`、`api:sha512sum`、`api:show`、`api:showAt`、`api:showDataNotification`、`api:sleep`、`api:source`、`api:stats` | 原 Scriptable/Proxy；实际 Quick 窗口/显式选择，新命令编辑/补全；针对性 CLI 验证 | 原接口保留；相关 CLI/窗口/选择回归见最新记录，非 149 API 穷尽验证 |
| `api:str`、`api:styles`、`api:synchronizeFromSelection`、`api:synchronizeToSelection`、`api:tab`、`api:tabIcon`、`api:toBase64`、`api:toUnicode` | 原 Scriptable/Proxy；实际 Quick 窗口/显式选择，新命令编辑/补全；针对性 CLI 验证 | 原接口保留；相关 CLI/窗口/选择回归见最新记录，非 149 API 穷尽验证 |
| `api:toggle`、`api:toggleConfig`、`api:unload`、`api:unpack`、`api:updateClipboardData`、`api:updateTitle`、`api:version`、`api:visible` | 原 Scriptable/Proxy；实际 Quick 窗口/显式选择，新命令编辑/补全；针对性 CLI 验证 | 原接口保留；相关 CLI/窗口/选择回归见最新记录，非 149 API 穷尽验证 |
| `api:write` | 原 Scriptable/Proxy；实际 Quick 窗口/显式选择，新命令编辑/补全；针对性 CLI 验证 | 原接口保留；相关 CLI/窗口/选择回归见最新记录，非 149 API 穷尽验证 |
| `api:enable`、`api:index`、`api:length`、`api:size` | 原 API 同声明别名；继续使用原参数/返回语义；managementActions / managementTabs 验证 size | 实现保留，别名逐项回归待补 |

| 内置插件 | 原接口 → 新入口 / 验证归属 | 当前状态 |
|---|---|---|
| `plugin:itemencrypted` | 原 ItemLoaderInterface → ClipboardItemPreview、新编辑/命令和 Settings Plugins；指定插件回归见最新记录 | 呈现/设置/数据/编辑/保存接口保持；源码接口未改，旧 ABI 和实际安装包待验收 |
| `plugin:itemfakevim` | 原 ItemLoaderInterface → ClipboardItemPreview、新编辑/命令和 Settings Plugins；指定插件回归见最新记录 | 呈现/设置/数据/编辑/保存接口保持；源码接口未改，旧 ABI 和实际安装包待验收 |
| `plugin:itemimage` | 原 ItemLoaderInterface → ClipboardItemPreview、新编辑/命令和 Settings Plugins；指定插件回归见最新记录 | 呈现/设置/数据/编辑/保存接口保持；源码接口未改，旧 ABI 和实际安装包待验收 |
| `plugin:itemnotes` | 原 ItemLoaderInterface → ClipboardItemPreview、新编辑/命令和 Settings Plugins；指定插件回归见最新记录 | 呈现/设置/数据/编辑/保存接口保持；源码接口未改，旧 ABI 和实际安装包待验收 |
| `plugin:itempinned` | 原 ItemLoaderInterface → ClipboardItemPreview、新编辑/命令和 Settings Plugins；指定插件回归见最新记录 | 呈现/设置/数据/编辑/保存接口保持；源码接口未改，旧 ABI 和实际安装包待验收 |
| `plugin:itemsync` | 原 ItemLoaderInterface → ClipboardItemPreview、新编辑/命令和 Settings Plugins；指定插件回归见最新记录 | 呈现/设置/数据/编辑/保存接口保持；源码接口未改，旧 ABI 和实际安装包待验收 |
| `plugin:itemtags` | 原 ItemLoaderInterface → ClipboardItemPreview、新编辑/命令和 Settings Plugins；指定插件回归见最新记录 | 呈现/设置/数据/编辑/保存接口保持；源码接口未改，旧 ABI 和实际安装包待验收 |
| `plugin:itemtext` | 原 ItemLoaderInterface → ClipboardItemPreview、新编辑/命令和 Settings Plugins；指定插件回归见最新记录 | 呈现/设置/数据/编辑/保存接口保持；源码接口未改，旧 ABI 和实际安装包待验收 |

非 .ui 辅助入口还包括 IconSelectDialog、密码/加密提示、ActionHandlerDialog、ActionHandler 进度、Notification、TrayMenu、ScriptableProxy 的 dialog()/input()/screenshotSelect()、FileDialog 和错误/确认提示；均已接入重新设计的承载。平台系统的文件/字体/颜色/确认对话框保留原生交互；密码、动作、输入和截图的内容布局实际重排，完整三端视觉仍待验收。第三方插件源码接口保持，旧二进制 ABI 尚未验证。

## 验收标准

| 编号 | 完成标准 | 必须保留的证据 |
|---|---|---|
| P5 | 从面板进入新管理窗口，原有功能均有有效新入口 | 基线功能映射、配置/脚本/CLI 比对、真实用户动作结果；不能只有菜单项存在。 |
| P6 | 面板和全部新界面符合固定样板，键盘/主题/DPI/多屏表现可用 | Windows/macOS 及声明的 Linux 环境截图、中文/英文布局、缩放/主题/可访问名称和焦点顺序验证。 |
| P7 | 记录/暂停/忽略/期限/容量/清理按规则生效 | 多 MIME、来源、固定项、未知时间旧记录、边界时刻及插件删除钩子的断言；真实监控行为单独验证。 |
| P12 | 管理、设置、命令、编辑、插件配置和辅助界面完成重做，八插件全部可用 | 新界面覆盖清单、富文本/FakeVim/加密/同步操作及旧数据往返；实际安装包的正式插件加载证据。 |

- 保留全部能力的结论以功能映射和行为验证为依据，不能仅靠构建成功或统一配色。
- 交接 Iteration3：可复用的编辑/表单/预览组件、设置持久化入口、完整功能清单、历史时间/来源规则、配置/主题/插件兼容结果、实际测试命令及剩余平台缺项。

## 验证计划

文档阶段执行 Iteration1 中的三项公共文档命令，lint 目标为整个 docs/astack/version，预期 0 errors/0 warnings。

本机 macOS 的配置与指定测试命令如下；每次运行通过隔离 runner 创建临时 session/config/data/state/GNUPGHOME，测试保存/恢复系统剪贴板，不使用日常历史。QML lint 与同目录构建顺序执行，避免并发修改 Ninja 元数据：

~~~bash
CMAKE_PREFIX_PATH='/opt/homebrew/opt/qt;/opt/homebrew/opt/qca;/opt/homebrew/opt/qtkeychain' \
  cmake --preset macOS-13-m1 -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DWITH_NATIVE_NOTIFICATIONS=OFF -DWITH_AUDIO=OFF
cmake --build build/copyq/macOS-13-m1 -j 6
cmake --build build/copyq/macOS-13-m1 --target copyq-palette-ui_qmllint
utils/run-isolated.sh build/copyq/macOS-13-m1/copyq-management-tests \
  multiSelectionIdentity bulkSelectionPerformance sourceLifetimeAndDisplay \
  transferAndDeleteProtection qmlSelectionAndActions nativeDropRoundTrip themeMapping \
  legacyStyleRules pluginPreviewBridge managementGeometry settingsDraftTransaction \
  commandDraftRoundTrip pluginSettingsDraft allPluginSettings historyTimeAndProtection historyCaptureFilters
utils/run-isolated.sh build/copyq/macOS-13-m1/copyq-tests \
  testCore:managementActions testCore:managementTabs testCore:managementGroups testCore:managementCommands \
  testCore:managementHistory testCore:managementHistoryCapture testCore:configPath \
  testCore:importExportTab testCore:commandConfig testCore:commandLoadTheme testCore:displayCommand \
  testCore:commandNotification testCore:commandScreenshot testItemImage:savePng \
  testItemPinned:keepPinnedIfMaxItemsChanges testItemEncrypted:encryptDecryptData \
  testItemSync:itemsToFiles testItemSync:filesToItems
utils/run-isolated.sh build/copyq/macOS-13-m1/copyq-tests \
  testCore:managementEditor testCore:managementDialogs
utils/run-isolated.sh build/copyq/macOS-13-m1/copyq-palette-tests \
  modelIdentity queryChanges displayCopiesAndPreview sourceDestructionAndReset \
  qmlKeyboardAndIme actionsAndCancellation explicitCommands standardPreviews pluginEditorAndSettings
~~~

macOS 的 managementEditor/managementDialogs 和另行指定的 nativeWindowIdentification 包含真实前台输入/激活，当前 loginwindow 会话有失败记录，不能用其余通过项代替。管理和面板的 IME 测试发送真实 QInputMethodEvent，但不启动系统输入法。

本机 Linux 复用 Iteration1 的 Colima `qclip-spec1-linux` 容器；先启动容器内 Xvfb :99 与 openbox，再执行下列命令。若它们已运行则复用现有显示会话，不并发启动第二个 :99。容器关闭缺失依赖不改变 CI 默认配置：

~~~bash
export DOCKER_CONTEXT=colima
docker exec qclip-spec1-linux cmake -S /workspace -B /workspace/build/linux -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DWITH_TESTS=ON \
  -DWITH_QCA_ENCRYPTION=OFF -DWITH_NATIVE_NOTIFICATIONS=OFF -DWITH_AUDIO=OFF -DPEDANTIC=ON
docker exec qclip-spec1-linux cmake --build /workspace/build/linux -j 6
docker exec qclip-spec1-linux cmake --build /workspace/build/linux --target copyq-palette-ui_qmllint
docker exec -e DISPLAY=:99 -e QT_QUICK_BACKEND=software qclip-spec1-linux \
  /workspace/utils/run-isolated.sh /workspace/build/linux/copyq-management-tests \
  multiSelectionIdentity bulkSelectionPerformance sourceLifetimeAndDisplay \
  transferAndDeleteProtection qmlSelectionAndActions nativeDropRoundTrip themeMapping \
  legacyStyleRules pluginPreviewBridge managementGeometry settingsDraftTransaction \
  commandDraftRoundTrip pluginSettingsDraft allPluginSettings historyTimeAndProtection historyCaptureFilters
docker exec -e DISPLAY=:99 -e QT_QUICK_BACKEND=software qclip-spec1-linux \
  /workspace/utils/run-isolated.sh /workspace/build/linux/copyq-tests \
  testCore:managementActions testCore:managementTabs testCore:managementGroups testCore:managementCommands \
  testCore:managementEditor testCore:managementHistory testCore:managementHistoryCapture testCore:managementDialogs \
  testCore:configPath testCore:searchItemsAndCopy testCore:keysAndFocusing testCore:importExportTab \
  testCore:commandConfig testCore:commandLoadTheme testCore:displayCommand testCore:commandDialog \
  testCore:commandDialogFitsContents testCore:commandDialogRestoreGeometry testCore:commandDialogCloseOnDisconnect \
  testCore:commandNotification testCore:commandScreenshot testItemImage:savePng testItemTags:searchTags \
  testItemPinned:keepPinnedIfMaxItemsChanges testItemEncrypted:encryptDecryptData testItemEncrypted:encryptDecryptItems \
  testItemFakeVim:paletteEditor testItemFakeVim:undoGroupsInsertSession testItemSync:itemsToFiles testItemSync:filesToItems \
  testCore:clipboardUriList testCore:paletteSearchAndCopy testCore:paletteCommands testCore:paletteClipboardFailure \
  testCore:paletteEditor testCore:palettePaste testCore:paletteMimeAndDisplayCommands testItemFakeVim:createItem
docker exec -e DISPLAY=:99 -e QT_QUICK_BACKEND=software qclip-spec1-linux \
  /workspace/utils/run-isolated.sh /workspace/build/linux/copyq-palette-tests \
  modelIdentity queryChanges displayCopiesAndPreview sourceDestructionAndReset \
  qmlKeyboardAndIme actionsAndCancellation explicitCommands standardPreviews nativeWindowIdentification pluginEditorAndSettings
~~~

本批入口机械检查为 `python3 utils/check-compatibility.py`。新增测试目标要求指定函数；CI 使用上述 16 个管理函数和指定服务/插件兼容函数，不直接运行全套。Windows runner 与新测试可执行文件已接入现有工作流和部署脚本，配置检查不等于 Windows 实机测试。

产品构建与原生隔离入口复用 Iteration1，按本段修改范围运行测试。以下是已存在的可执行测试选择；运行前必须完成 CLAUDE 的全部环境与显示会话准备：

~~~bash
build/copyq-tests "testCore:importExportTab" "testCore:commandConfig" "testCore:commandLoadTheme" "testCore:displayCommand"
build/copyq-tests "testItemImage:savePng" "testItemTags:searchTags" "testItemPinned:keepPinnedIfMaxItemsChanges"
build/copyq-tests "testItemEncrypted:encryptDecryptItems" "testItemFakeVim:undoGroupsInsertSession" "testItemSync:itemsToFiles" "testItemSync:filesToItems"
~~~

- 上述测试按改动选用，不能因“界面重做”重复运行所有测试；新增历史时间策略、配置取消、QML 页面动作和失效条目场景须有针对性测试。
- itemtext/itemnotes 还需新界面的格式/备注往返验证；组测试和测试插件环境不能代替安装包中八个正式插件的实测。
- 删除、清空、加密、迁移和同步只能使用隔离历史/配置/目录。Qt Quick 测试与真实 Win/mac 目标输入分别留证。
- 无某平台设备或旧主题兼容尚未收敛时保留未完成项，不把 Windows 结果外推成三端通过。

## 验证记录

以下最新记录针对本轮工作；其后的初始开发和 `4a6e1843` 提交前记录用于保留失败与修正过程，不能替代新鲜验证。pass 总数包含 initTestCase/cleanupTestCase。本机日志位于忽略的 build 目录，不随源码提交。

| 日期 | 命令 / 检查 | 结果与证据边界 |
|---|---|---|
| 2026-09-29 | 最终编辑/键盘收尾后：两平台 `cmake --build ... -j 6` 和 `copyq-palette-ui_qmllint`；本文管理/面板指定函数 | 构建/lint 退出 0；管理两端均 18 passed，面板 macOS 11 passed、Linux 12 passed，均 0 failed/0 skipped。日志 `build/spec2-last2-{mac,linux}-{build,qmllint}.log`、`build/spec2-last-{mac,linux}-{management,palette}.log`。同目录 build/lint 保持串行；已有 macOS minOS/元数据警告仍保留，未外推平台通过。 |
| 2026-09-29 | 本文 Linux `copyq-tests` 最终指定 38 个服务/插件函数 | 40 passed / 0 failed / 0 skipped，退出 0，日志 `build/spec2-complete-linux-service.log`。在原 30 个兼容用例之外补 clipboardUriList、面板查询/命令/失败保持/编辑/原生粘贴/MIME 显示副本和 FakeVim createItem，确保新管理主入口不破坏 SPEC1。createItem 保留全部原断言，并新增 Quick 新建条目 :w 连续保存只更新同一条目、删除源后保存不重新插入的断言。 |
| 2026-09-29 | macOS `copyq-tests testCore:paletteSearchAndCopy testCore:paletteCommands testCore:paletteClipboardFailure testCore:paletteMimeAndDisplayCommands` | 6 passed / 0 failed / 0 skipped，退出 0，日志 `build/spec2-ship-mac-palette-service.log`。面板中文合成输入/复制、命令、错误保持与完整 MIME/显示副本通过；先前 TEST_SELECTED 读取隐藏 Widgets 的失败由实际管理选择读取修复。不是系统输入法或外部前台粘贴验收。 |
| 2026-09-29 | 旧 FakeVim 新建/再次编辑补查及修正 | 首测 F2 未打开编辑器，保留 `build/spec2-ship-linux-palette-service.log`（9 passed / 1 failed）；等待实际编辑控件仍超时。定位为按管理快捷键时先匹配到 Editor_Save 的 F2，已仅在管理上下文排除编辑器专用动作。随后接续 save/invalidate，使 :w 保存后继续编辑、:wq/普通保存退出，并以持久索引和首次创建状态保护重复保存/源删除。失效源判断使用 document 的 modified 状态，不能用含有效 index 前提的 hasChanges。FakeVim 状态栏 Ex 输入以 Esc 返回正常模式后继续；保留原数据断言，最终 38 用例全部通过。临时诊断输出已移除。 |
| 2026-09-29 | 本文两平台 `cmake --build ... -j 6` 与 `copyq-palette-ui_qmllint` 最终校验 | 均退出 0；QML 规则 0 warnings。日志 `build/spec2-ship-{mac,linux}-{build,qmllint}.log`。macOS 仍有依赖 minOS 高于 13、重复静态库及 Ninja 元数据自动恢复警告；串行后仍出现元数据警告，原因未定位，不能证明 macOS 13 运行。Linux Qt 6.4.2/GCC 12.2、macOS Qt 6.11.2/AppleClang 21。 |
| 2026-09-29 | 本文 `copyq-management-tests` 指定 16 个函数，两平台隔离执行 | macOS/Linux 均 18 passed / 0 failed / 0 skipped。覆盖选择身份/5 万条批量、源/显示副本失效、删除保护/完整 MIME/拖放、真实 QML 操作/IME 事件、主题与 QSS 像素、几何、90 项设置草稿、命令完整往返、八插件设置、历史过滤/时间/序列化/加密/保护。5 万条全选加插入恢复：macOS 68ms、Linux 15ms，仅模型耗时。日志 `build/spec2-ship-{mac,linux}-management.log`；截图 `build/spec2-artifacts/management{,-compact}.png` 已检查，工具栏前景正确。 |
| 2026-09-29 | 本文 `copyq-palette-tests` 指定函数 | macOS 9 函数共 11 passed；Linux 10 函数共 12 passed；均 0 failed/0 skipped。覆盖共享模型/显示副本/源销毁、键盘/IME 事件、动作取消/命令、文本/图片/HTML/文件/损坏图片预览与原编辑/设置；Linux 额外通过 nativeWindowIdentification。日志 `build/spec2-ship-{mac,linux}-palette.log`。macOS 原生窗口激活仍保留历史失败，未用其他通过项替代。 |
| 2026-09-29 | 本文 Linux `copyq-tests` 指定 30 个服务/插件函数 | 32 passed / 0 failed / 0 skipped，日志 `build/spec2-ship-linux-service.log`。真实服务验证集合/组/动作/命令、富文本格式/搜索/撤销/保存/取消/失效源、类型/来源/敏感/暂停/重复采集/自动命令、最近范围清理/固定与手工集合保护、辅助布局、原脚本对话框几何/取消/断开、通知/截图、原配置/旧主题/旧数据、标签/图片/加密备注/FakeVim/双向文件同步。Xvfb/openbox 不等价于原生 Windows/macOS 前台验收。 |
| 2026-09-29 | 本文 macOS `copyq-tests` 指定 18 个服务/插件函数 | 19 passed / 1 failed / 0 skipped，退出 1；17 个实际用例通过，commandScreenshot 的 `screenshot().size() > 0` 返回 false，前台记录仍为 loginwindow，原因未完全确认；没有重试或放宽断言。管理/组/命令/采集/清理、配置/主题/旧数据、显示命令/通知、图片/固定、加密数据与双向同步通过；仍需可激活桌面补截图、编辑/辅助焦点和 FakeVim。日志 `build/spec2-ship-mac-service.log`。 |
| 2026-09-29 | `python3 utils/check-compatibility.py`；harness；目录级 SPEC lint；`git diff --check`；修改 shell 的 `bash -n`；PowerShell Parser；Windows 工作流 YAML 解析 | 均退出 0；登记 49 个动作/90 项配置/24 主表单/7 插件表单/149 API/8 插件，harness 全项通过，三份 SPEC errors=0/warnings=0。三端 CI 已加入 16 管理函数及指定兼容服务/插件函数；配置/语法通过不表示 Windows 构建或运行已通过。 |
| 2026-09-29 | 接续兼容回归发现的问题及针对性修正 | 组移至根路径曾触发 QString::chopped 的空前缀断言，已修复并通过 managementGroups。显示副本随插入重复执行脚本曾递归添加、Qt 6.4 崩溃（`build/spec2-display-core.log`）；改为持久索引缓存及关闭/查询/源数据失效后，displayCommand 和生命周期回归通过。Quick 可见性未接续 ItemSaver::setFocus 导致 filesToItems 失败，修复后两端双向同步通过。窗口析构可见性回调访问已销毁模型的 SIGABRT 经 gdb 定位（`build/spec2-palette-core.log`），先断开/销毁根/隐藏窗口后两端预览与关闭回归通过。 |
| 2026-09-29 | 测试承载与夹具修正 | macOS QCA provider 仅在安装阶段复制，构建态真实应用/单元测试找不到 provider；现在复制到产品 bundle 和独立 CopyQ-tests.app，真实八插件/加密断言通过。隔离 runner 的 GNUPGHOME 与 gpgconf 清理避免接触个人密钥。原备注用例按键先于编辑器取得焦点，补等待实际 palette_editor_text 后原加解密/备注断言通过（`build/spec2-encrypted-linux.log`，3 passed）。损坏图片预览从旧 Image.status 断言改为实际插件桥接/原始损坏 MIME 保留断言。工具栏黑字改为原 QToolButton 实际绘制；主题开启/关闭前景像素断言和截图均通过。 |
| 2026-09-29 | 历史策略初次编译及指定测试 | owner 改为标题/身份同队列后旧日志仍传 pair，受保护清理使用枚举需完整限定，均已修正并重新配置 GLOB 新源文件。两平台随后构建退出 0。Linux `historyTimeAndProtection` 与新增字段后的 `settingsDraftTransaction` 通过；过滤测试的受控 owner 回调把手动设置的忽略身份重置为允许身份，已改为同一可变采样身份，不修改产品过滤规则，复测待记。Qt 6.4 的 QVariantMap 属性 lint 通过 QML var 适配，保持数据字段不变。 |
| 2026-09-29 | 接续：两平台构建及 `copyq-palette-ui_qmllint`；隔离 `settingsDraftTransaction`、`commandDraftRoundTrip`、`pluginSettingsDraft`；macOS `testCore:managementGroups testCore:managementTabs` | 构建/lint 退出 0；设置 3 passed、命令/插件草稿 4 passed、macOS 集合服务 4 passed（均含 init/cleanup）。覆盖全部原配置名称、23 个可编辑命令字段和保留的本地化/InternalId、命令取消/保存/导入导出、语法检查不执行脚本、插件设置取消/应用。首次设置测试使用 QObject 查找未找到 Repeater 的视觉子项，改为等待并遍历实际 QQuickItem 树；原 INI 只保存 regex pattern，测试使用原格式支持的内联选项，没有修改存储格式。 |
| 2026-09-29 | Quick 主入口切换后的 Linux 服务回归 | `managementGroups/managementTabs/managementCommands` 通过；`managementActions` 的默认聚焦暴露了隐藏面板被记成粘贴目标的问题，失败记录 `build/spec2-entry-linux-test.log`。已修正目标采集也排除仍存活的隐藏自身 Quick 窗口，停止该段验收并安排复测。 |
| 2026-09-29 | 接续 I2-1 首次 macOS/Linux 编译 | 新 QFont QML 属性的 MOC 编译失败，缺完整类型头；停止该段验收并补充 QFont include，后续新鲜结果另记。 |
| 2026-09-29 | 接续 I2-1 `testCore:managementGroups` 首测 | 发现 CLI 新建非当前集合后侧栏仍保留旧列表，两平台均失败；已在原 createTab 完成后通知 Quick 集合列表，仅刷新列表，不重新加载源或改变选择/查询。后续复测另记。 |
| 2026-09-29 | 新设置窗口接入编译 | QML 注册类的 AppConfig 指针信号和设置根对象 QQuickItem 需要完整类型，已补相应头，停止该段验收并复测。组测试计数断言发现原生 QList 长度的脚本输出含换行，已修正测试预期，不改变产品输出。 |
| 2026-09-29 | `4a6e1843` 提交前（首批 I2-1）：两平台 `cmake --build ... -j 6` 与 `copyq-palette-ui_qmllint`，Linux 隔离 runner 的本文 7 个 management 函数及 SPEC1 12 个服务函数加 managementActions/managementTabs/managementCommands/managementEditor | 构建/lint 均退出 0；Linux 管理 9 passed / 0 failed、服务 18 passed / 0 failed（含 init/cleanup），完整 MIME、选择保护、真实键盘编辑/取消/失效源及相关面板回归通过。日志 `build/ship-linux-management.log`、`build/ship-linux-server.log`。用户本轮明确授权全部本地修改提交并推送 `origin/master`；SPEC1/SPEC2 保持开发中，不等于发布或完整验收。 |
| 2026-09-29 | `4a6e1843` 提交前 macOS 隔离 runner 的首批 7 个 management 函数 | 8 passed / 1 failed / 0 skipped：`nativeDropRoundTrip` 在 `QTest::qWaitForWindowExposed(&window)` 超时，尚未进入拖放断言；其余选择、保护、QML 操作、主题函数通过，5 万条全选/插入恢复 98ms。停止该平台 GUI 验收并保留 `build/ship-mac-management.log`，没有重试到通过或删改断言；先前通过记录不能替代本次结果。 |
| 2026-09-29 | `cmake --build build/copyq/macOS-13-m1 -j 6` 首次 I2-1 编译 | 失败：新管理控制文件缺 PlatformClipboard 完整类型 include，命令快捷键 QStringList 需要转换 QKeySequence；已停止验收并修复，后续新鲜结果另记。 |
| 2026-09-29 | I2-1 聚焦测试接入编译 | 发现 BROWSER 宏重复声明局部变量、新增 GLOB 测试文件需重新配置；已修复变量并重新运行原 macOS preset。首次 QML lint 的 3 条布局尺寸 warning 已修正。 |
| 2026-09-29 | `copyq-management-tests multiSelectionIdentity sourceLifetimeAndDisplay transferAndDeleteProtection qmlSelectionAndActions themeMapping` 首测 | 多选/生命周期/传输/实际 QML 点击及 IME 断言通过；主题断言错误地把 QQuickPalette 转成 QPalette，已改为检查真实 Quick palette.text。退出时发现 QML 绑定晚于模型析构，已在管理析构中先释放 QML 根对象；需复测。 |
| 2026-09-29 | 本文 macOS/Linux configure、`cmake --build ... -j 6` | 均退出 0；macOS 26.6.2 arm64/Qt 6.11.2/AppleClang 21，Linux Debian 12 arm64/Qt 6.4.2/GCC 12.2。macOS 有 Qt 库部署目标高于 macOS 13 的既有链接警告，不能证明 macOS 13 运行。构建日志为 `build/spec2-build.log`、`build/spec2-linux-build.log`；本轮没有安装、提交、推送或发布。 |
| 2026-09-29 | 两平台 `copyq-palette-ui_qmllint` | 均退出 0，QML 前端 0 warnings；Qt 6.4 不支持的 QStringList.indexOf/动态属性访问已改用类型明确的当前索引/属性。早期同目录并发 build/lint 时出现 Ninja 元数据截断，已改为串行；本机 macOS 末轮仍有 `premature end of file; recovering` 警告，自动恢复后目标退出 0，构建和测试结果通过，元数据问题原因尚未定位。 |
| 2026-09-29 | 本文两平台 `copyq-management-tests` 指定 7 个函数 | 各 9 passed / 0 failed / 0 skipped（含 init/cleanup），8 个正式插件均 enabled。覆盖持久选择、源销毁、固定/取消保护、完整 MIME 传输、实际 QML 表单/点击/IME 事件、原生 DropEvent 和主题数据。5 万条全选并插入后恢复选择：macOS 95ms、Linux 38ms，旧身份保留，新插入项未被选择；这是模型操作耗时，不是完整交互延迟。日志 `build/spec2-mac-management.log`、`build/spec2-linux-management.log`。 |
| 2026-09-29 | 本文 Linux `copyq-tests` 指定 8 个函数 | 10 passed / 0 failed / 0 skipped；真实服务验证多选批量动作/排序/移动、CLI 查询/暂停/选择、集合属性/顺序/卸载重载、命令选择/可用性/显示副本、编辑保存/取消/失效源保留输入及既有面板。日志 `build/spec2-linux-server.log`。 |
| 2026-09-29 | 本文 macOS `copyq-tests` 指定 6 个无前台键盘函数 | 8 passed / 0 failed / 0 skipped；管理动作/集合/命令和相关面板数据流通过，不能代替前台编辑/外部粘贴。日志 `build/spec2-mac-server.log`。 |
| 2026-09-29 | macOS `testCore:managementEditor` 首轮及 `copyq-palette-tests nativeWindowIdentification` 末轮 | 未通过：编辑测试无法把键盘送到 palette_editor_text，服务日志前台为 `loginwindow - Login`；原生窗口测试在 `otherNative->isActive()` 失败。后者同次指定 9 个函数共 10 passed / 1 failed，不能称整组通过。保留 `build/spec2-server.log` 与 `build/spec2-mac-palette.log`；没有修改测试以跳过失败，需原生可激活桌面补证。 |
| 2026-09-29 | 本文 Linux `copyq-palette-tests` 指定 9 个函数 | 11 passed / 0 failed / 0 skipped，覆盖共享适配器、查询/显示副本/销毁、QML 键盘及 IME 事件、命令、预览、原生窗口识别和插件编辑/设置原型。坏图片预览产生预期 provider warning。日志 `build/spec2-linux-palette.log`；不外推 Windows/macOS 前台验收。 |
| 2026-09-29 | `python3 utils/check-compatibility.py`；harness；三份 SPEC 的目录级 lint；`git diff --check`；修改的 shell 脚本 `bash -n`；Windows 工作流 YAML 解析 | 均退出 0；入口登记 49/87/24/7/149/8 与源码一致，SPEC errors=0/warnings=0。校验状态、链接、隔离入口和脚本语法，不代替产品行为或 Windows 运行验收。 |
| 2026-09-28 | 源码范围及既有测试名称核对 | 已确认八插件、配置/显示命令耦合、expire_tab 卸载语义及表中测试入口；产品源码未改，尚未运行应用/构建/产品测试。 |
| 2026-09-28 | 三份 SPEC 文档质量门 | harness、目录级 SPEC lint、git diff --check 均退出 0；三份 SPEC 0 errors/0 warnings，P1–P14 唯一归属及 18 个既有测试声明核对通过。仅文档/静态检查，产品验收仍待实施。 |

## 2026-10-03 毛玻璃与视觉重构

用户要求参考 [Maccy](https://github.com/p0deje/Maccy) 与 [DeskBox](https://github.com/Tianyu199509/DeskBox)，彻底优化界面并实现毛玻璃。沿用现有 Qt Quick、CopyQ 数据与插件桥接；本轮不改变历史、脚本、粘贴或存储契约。

设计：快捷面板采用独立搜索行、集合切换、图标/两行列表、独立预览与紧凑操作栏；次要操作收进菜单。管理、设置、命令、Snippet 共用柔和中性色、蓝色强调、细描边、圆角、统一控件与留白。支持现有主题颜色/字体/密度，自定义 QSS 仍经 ClipboardStyle 解释。材质由系统合成：Windows 11 Desktop Acrylic，macOS NSVisualEffectView behindWindow/popover；不支持的平台、关闭透明或高对比度时使用实色，避免只透明而无模糊。

影响面与文件清单（本轮必然跨十个以上文件）：

| 类型 | 文件 | 原因 |
|---|---|---|
| NEW | src/gui/clipboardwindow.h、clipboardwindow.cpp；src/platform/platformwindoweffects.h、win/winwindoweffects.cpp、mac/macwindoweffects.mm、dummy/dummywindoweffects.cpp | 共享 Quick 窗口材质生命周期与两端原生模糊、失败回退。 |
| MODIFY | src/gui/clipboardpalette.h、clipboardmanagement.h、clipboardsettings.h、clipboardcommands.h、clipboardsnippets.h | 五个窗口继承统一材质层，复用原控制器。 |
| NEW | src/gui/qml/GlassBackground.qml、ThemeTextField.qml、ThemeTextArea.qml、ThemeComboBox.qml、ThemeCheckBox.qml、ThemeDelegate.qml、ThemeDialog.qml | 共享材质表面、输入/选择/弹窗控件。 |
| MODIFY | src/gui/qml/Theme.qml、ThemeButton.qml、ThemeMenu.qml、ThemeMenuItem.qml、ClipboardPalette.qml、ManagementWindow.qml、Settings.qml、Commands.qml、Snippets.qml | 五个界面布局、主题与交互状态统一。 |
| MODIFY | src/gui/theme.h、theme.cpp、clipboardstyle.cpp、clipboarditempreview.h、clipboarditempreview.cpp；src/CMakeLists.txt、src/platform/platform.cmake、win/winplatform.cmake | 主题/QSS 兼容、插件预览深浅色、旧渲染入口现代化及 QML/原生源码注册。 |
| MODIFY | src/gui/clipboardpalette.cpp、clipboardmanagement.cpp、clipboardsettings.cpp、clipboardcommands.cpp | 已有 event override 转发共享材质生命周期；焦点/粘贴分支保持原行为。 |
| MODIFY | src/tests/tests_palette.cpp、tests_management.cpp、tests_snippets.cpp；utils/check-compatibility.py | 材质销毁/重建与深色预览断言、五窗口截图；修复清单脚本 Windows UTF-8/路径分隔符导致的误报。 |

验证计划：Windows 本机构建指定 QML lint 与 palette/management/snippet 聚焦测试；检查实际 QML 加载、搜索/选择/IME/主题/QSS 回归。通过隔离原生窗口截图检查浅色/深色布局和背景模糊，测试数据路径均隔离。命令：`cmake --build build/copyq/Windows --target qclip copyq-palette-tests copyq-management-tests copyq-snippet-tests copyq-palette-ui_qmllint`、`pwsh -NoProfile -File utils/check-harness.ps1`、`python utils/check-compatibility.py`、`git diff --check`。macOS 材质与 Linux 桌面回退须据实记录，Windows 截图不等于三端验收。

### 本轮验证记录

Windows 11 build 26300 / x64、Qt 6.10.3 / MSVC 19.51、150% DPI。使用仓库内 `build/toolchain/Qt/6.10.3/msvc2022_64` 和 VS 18 工具链。配置从 Windows preset 出发，本机开发构建关闭 QCA、Keychain、原生通知、音频与 sccache；不改变源码中正式构建默认开关，不作为正式发布包。

测试均通过 `utils/run-isolated.ps1 -Executable ... -Arguments @(...)` 运行；QtTest 报告使用 `-o ...txt,txt`，截图使用既有 `COPYQ_TESTS_ARTIFACT_DIR`。桌面模糊由原生 Acrylic 合成，窗口 framebuffer 请求 alpha=8，Windows 24H2+ 同时开启 redirection bitmap 的预乘 alpha；自定义 QSS 表面保持不透明。

| 日期 | 命令 / 范围 | 结果 |
|---|---|---|
| 2026-10-03 | `cmake --build build/copyq/Windows -j 8`；`cmake --build build/copyq/Windows --target copyq-palette-ui_qmllint` | 构建退出 0，QML 0 warnings；完整八插件产物存在。日志 `build/ui-build.log`、`build/ui-lint.log`。首次链接发现共享窗口被同时加入 common GLOB 与 QML 模块，已按现有模块的排除方式修正并重新构建。 |
| 2026-10-03 | `copyq-palette-tests glassWindowLifecycle qmlKeyboardAndIme explicitCommands standardPreviews` | 6 passed / 0 failed；覆盖原生表面销毁重建、可用时半透明/否则实色、深色预览实际 palette、搜索/IME、命令与插件预览。隔离窗口日志 `Native material available=true active=true`，截图 `build/ui-screenshots/palette-{light,dark}.png`。 |
| 2026-10-03 | `copyq-management-tests qmlSelectionAndActions themeMapping legacyStyleRules settingsDraftTransaction commandDraftRoundTrip pluginPreviewBridge managementGeometry` | 7 passed / 2 failed；管理点击/IME/800×520 布局、主题、自定义 QSS、设置草稿及插件桥接通过。`commandDraftRoundTrip` 在命令文本交换回环失败；`managementGeometry` 期望 QRect(100,160 900×600)，实际 QRect(107,190 885×563)。日志 `build/ui-management-results.txt`。 |
| 2026-10-03 | 修改前 `HEAD=51d9869d` 的独立 archive / 相同 Qt、MSVC、开关，仅运行 `commandDraftRoundTrip managementGeometry` | 两项出现完全相同失败（2 passed / 2 failed，含初始化/清理），证明本机既有失败；未删除断言或将失败记为通过。日志 `build/ui-baseline-results.txt`。 |
| 2026-10-03 | `copyq-snippet-tests qmlSnippets` | 3 passed / 0 failed；Snippet 浏览/详情/使用流程通过。截图检查修正默认未初始化 Theme 的无效颜色回退；五窗口截图保留在 `build/ui-screenshots/`，管理含正常/紧凑尺寸。 |
| 2026-10-03 | `python utils/check-compatibility.py`、`git diff --check` | 均退出 0，49 动作 / 90 配置 / 24 表单 / 7 插件表单 / 150 API / 8 插件。清单脚本原 Windows GBK 解码与反斜杠路径误报已修复。 |
| 2026-10-03 | `pwsh -NoProfile -File utils/check-harness.ps1`；本机/WSL spec-lint 路径发现 | harness 未通过：本 checkout 的 AGENTS.md 是 Git 在 `core.symlinks=false` 下生成的 `CLAUDE.md` 占位文件，而非原生链接；本机创建原生符号链接返回需要管理员权限。未改写治理入口或 Git 设置。astack spec-lint 脚本在已登记的本机/WSL 路径不可用，未声称执行通过。 |
| 2026-10-03 | `cmake --install ... --prefix build/ui-preview`、`windeployqt --release --no-translations --qmldir src/gui/qml`；补齐 Windows 部署脚本惯例的八插件和 themes；在预览目录运行上述 4 个 palette 方法 | 独立开发预览目录 `build/ui-preview/` 已部署 Qt/QML/八插件；不依赖工具链 PATH 的运行结果 6 passed / 0 failed，`qclip.exe --version` 退出 0。首次 install 不含 Windows 插件（该平台由部署流程复制），补齐后重新验证。可执行 SHA-256 `0f9a0382a7437052ee631aec58c45f4de07405f82777f1ec58f59dc00d3d12fc`。仅开发预览，未安装/设置自启动/提交/推送。 |

本轮覆盖 Windows 原生窗口的布局与材质接入，未重新验收外部应用真实粘贴、全局输入/IME、多屏或性能。macOS 原生材质源码与 Linux 不透明回退已接入，未在对应平台编译/实机验证；仍保持本 SPEC 开发中与三端总验收未完成。

### 2026-10-03 紧凑布局与对齐修订

用户根据本轮截图指出控件过于宽松、元素未对齐。将默认间距从 12 改为 8、外边距从 20 改为 12、历史行高从 64 改为 48；按钮和输入控件统一 32px 基准高度，字体放大时保留内容所需高度。快捷面板默认收紧为 740×440，搜索框 36px，History/Preview 两栏标题等高、列表和预览卡片顶部/底部对齐，图标按钮等宽。保留用户显式密度与字体、原生材质及原有操作。

影响文件：`Theme.qml`、`ThemeButton.qml`、`ThemeTextField.qml`、`ThemeTextArea.qml`、`ThemeComboBox.qml`、`ThemeCheckBox.qml`、`ThemeDelegate.qml`、`ThemeMenuItem.qml`、`ThemeDialog.qml` 调整共享尺寸；`ClipboardPalette.qml`、`ManagementWindow.qml`、`Settings.qml`、`Commands.qml`、`Snippets.qml` 统一局部留白、标题及列对齐；`theme.cpp`、`clipboardstyle.cpp` 同步默认密度与辅助 Widgets/绘制尺寸，`clipboardpalette.cpp` 同步原生面板尺寸。无新增设置、业务接口或功能变化。

验证计划：Windows 增量构建、`copyq-palette-ui_qmllint`；隔离运行既有 `glassWindowLifecycle qmlKeyboardAndIme explicitCommands standardPreviews`、管理布局/主题/设置/命令及 `qmlSnippets` 指定方法；重新截图检查浅深色、紧凑管理窗口、设置/命令/片段控件裁切与对齐，`python utils/check-compatibility.py` 和 `git diff --check`。

设置/命令的布尔选项改为左侧标签、右侧勾选框，减少重复的 Enabled 行；复选框取消默认左内边距，片段列表使用独立标题/关键字两行呈现，避免文本省略吞掉关键字。现有对象名称与控制器调用保留。

本次验证（同上 Windows/Qt/MSVC 环境）：

| 命令 / 范围 | 结果 |
|---|---|
| `cmake --build build/copyq/Windows -j 8`；`copyq-palette-ui_qmllint` | 均退出 0，QML 无警告；日志 `build/ui-density-build.log`、`build/ui-density-lint.log`。 |
| palette 指定上述四个方法 | 6 passed / 0 failed，原生 Acrylic 日志 available=true、active=true；`build/ui-density-palette-results.txt`。 |
| management：`qmlSelectionAndActions themeMapping legacyStyleRules settingsDraftTransaction commandDraftRoundTrip pluginPreviewBridge` | 7 passed / 1 failed；失败仍为前述 baseline 已复现的 `commandDraftRoundTrip`，未放宽或跳过断言；`build/ui-density-management-results.txt`。本次仅尺寸/呈现修订，未重复独立窗口位置存储测试。 |
| snippets：`qmlSnippets` | 3 passed / 0 failed；`build/ui-density-snippet-results.txt`。 |
| `cmake --install ... --prefix build/ui-preview` 后在部署目录运行 palette 四个方法，不注入 Qt 工具链 PATH | 6 passed / 0 failed；开发预览已更新，保持前述可选组件关闭配置；`build/ui-density-packaged-results.txt`。EXE SHA-256 `f94efffcf4bee1ca4623012e2fe1c3026fb3c1c54a6d2f2b55c45718d2664806`。 |
| 视觉检查 | `build/ui-density-screenshots/` 内浅深色 palette、management/management-compact、settings、commands、snippets 截图已检查：标题基线、两栏起止边界、控件高度与复选框对齐，片段关键字完整；未发现本轮收紧引起的裁切。仍非 macOS/Linux 或外部真实粘贴的验收。 |
| 清单/差异/治理 | `python utils/check-compatibility.py`、`git diff --check` 退出 0；harness 入口链接限制及本机 spec-lint 不可用仍如前述记录。 |

### 2026-10-03 全界面视觉精修与排查

用户指出集合下拉框的字形箭头未对齐，并要求以苹果应用的精细度全面审阅和美化。沿用 740×440 快捷面板与 32px 控件基准，保留用户显式字体/密度、全部操作及自定义 QSS。以统一线性图标、低噪声表面、精确中心对齐、明确文字层级、连续焦点反馈和完整禁用/悬停/选中状态为验收目标。

代码排查确认：下拉箭头/关闭/更多依赖字形基线；下拉 delegate 使用控件外宽，未扣除 popup padding；普通菜单和管理集合/历史仍使用旧绘制桥；SpinBox、滚动条、tooltip、分隔线未接入共享样式；设置/命令字段平铺、Snippet 次要设置挤在底部。修订必须覆盖这些共性问题，不能只改截图中的一个箭头。

影响面（跨十个以上文件的原因是公共样式需要被所有入口复用）：

| 类型 | 文件 | 原因 |
|---|---|---|
| NEW | src/gui/qml/ThemeIcon.qml、ThemeScrollBar.qml、ThemeSpinBox.qml、ThemeMenuSeparator.qml、ThemeToolTip.qml | 多入口共用几何图标、滚动/数字/菜单/提示控件，消除字体基线和默认 Basic 控件混搭。 |
| MODIFY | src/gui/qml/Theme.qml、ThemeButton.qml、ThemeComboBox.qml、ThemeCheckBox.qml、ThemeDelegate.qml、ThemeTextField.qml、ThemeTextArea.qml、ThemeDialog.qml、ThemeMenu.qml、ThemeMenuItem.qml、GlassBackground.qml | 统一尺寸、颜色、焦点/禁用/选中反馈；下拉几何对齐及弹出项边界。 |
| MODIFY | src/gui/qml/ClipboardPalette.qml、ManagementWindow.qml、Settings.qml、Commands.qml、Snippets.qml | 五窗口逐页修订图标、留白、导航/字段层级、表面与紧凑布局。 |
| NEW / MODIFY | src/images/chevron_*.svg、check*.svg、src/copyq.qrc、src/CMakeLists.txt | 辅助 Widgets 共享矢量箭头/检查标记，注册公共 QML 与资源。 |
| MODIFY | src/gui/theme.cpp、clipboarditempreview.cpp、clipboardsettings.cpp | 辅助 Widgets、插件预览与编辑器统一深浅色、数字/图标按钮和焦点；保留自定义 QSS 与主题编辑器预览。 |
| MODIFY | src/gui/clipboardpalette.cpp、configurationmanager.cpp | 弹出菜单/输入控件接收自身键盘事件；按实际页面归属恢复 Layout/Tray/Notifications 设置字段。 |
| MODIFY | src/gui/aboutdialog.cpp | 默认 About 富文本使用当前现代主题颜色及可读的链接色；显式 QSS 继续使用原颜色。 |
| MODIFY | src/gui/logdialog.cpp | 日志采用系统等宽字体，深色状态/引号/线程高亮使用可读前景与协调底色，去掉硬编码白色高亮块。 |
| MODIFY | src/tests/tests_palette.cpp、tests_management.cpp、tests_snippets.cpp | 增加真实控件几何/键盘/弹出选择验证及多页/深浅色/尺寸截图；保留既有业务断言。 |

验证计划：`cmd /c build\build-ui.cmd`、`cmd /c build\lint-ui.cmd`；通过 `utils/run-isolated.ps1` 运行 palette 的材质、控件及键盘/IME指定函数、management 的 QML/主题/QSS/设置/命令与视觉指定函数、snippet 的 `qmlSnippets`。截图覆盖所有主窗口、设置/命令各栏目、弹窗/菜单、深浅色与紧凑窗口，并检查实际生成的图像。随后运行 `python utils/check-compatibility.py`、`pwsh -NoProfile -File utils/check-harness.ps1`、`git diff --check`，部署验证后交付开发预览。现有 Windows 命令交换回环/原生位置存储失败须保留边界，不放宽断言；macOS/Linux 实机与外部粘贴不由视觉截图证明。

最后一轮辅助页审阅发现：深色快捷键表的交替行仍取系统浅色 palette；片段编辑器的隔离截图 fixture 未加载真实菜单项，导致工具栏空白。前者按实际主题表面推导默认颜色并统一图标按钮尺寸/导航，后者补齐 fixture 的菜单和快捷键后重新截图。透明预览内容的背景由滚动容器提供，验证应检查实际容器 Base 与文字颜色，不能要求透明内容自身 Window 角色为不透明背景。

辅助窗口深色 fixture 必须把主题写入隔离 Settings 后再构造真实窗口，不能在浅色窗口已经初始化后仅更换父 palette；恢复隔离配置由 scope guard 保证。控件 gallery 也须提供完整的编辑器配色，避免把浅色编辑颜色保留在深色测试地图中。About 的默认链接色另按现代 accent 修订，确保深色富文本可读。

#### 本轮验证记录（2026-10-03）

| 验证 | 结果与证据 |
|---|---|
| Windows 原生构建 / QML | `cmd /c build\build-ui.cmd` 与 `cmd /c build\lint-ui.cmd` 退出 0；Qt 6.10.3、MSVC 19.51、x64，QML lint 无告警；`build/ui-polish-build.log`、`build/ui-polish-qmllint.log`。沿用此前可选 QCA/Keychain/audio/通知/sccache 关闭的开发配置，不等同于正式发行构建。 |
| palette 指定方法 | 隔离运行 `glassWindowLifecycle themedControls collectionPopupNavigation qmlKeyboardAndIme explicitCommands standardPreviews`：8 passed / 0 failed（含 init/cleanup）；箭头几何中心、中文长集合名称、popup 宽度、数字键盘、下拉 Enter/Esc、IME、标准预览及材质生命周期通过；`build/ui-polish-palette-results.txt`。 |
| management 指定方法 | 隔离运行 `visualAudit qmlSelectionAndActions themeMapping legacyStyleRules settingsDraftTransaction commandDraftRoundTrip pluginPreviewBridge`：8 passed / 1 failed。视觉、点击/IME、主题/QSS、设置草稿和插件桥通过；`commandDraftRoundTrip` 仍为前述修改前 baseline 在相同工具链已复现的命令交换回环失败，未修改或放宽业务断言；`build/ui-polish-management-results.txt`。未重复既有独立窗口位置存储失败测试。 |
| snippets 指定方法 | 隔离运行 `qmlSnippets`：3 passed / 0 failed；包括浅深色正文编辑器与 720×480 布局；`build/ui-polish-snippet-results.txt`。 |
| 完整视觉审阅 | `build/ui-polish-screenshots/` 的 115 张窗口内容截图、`build/ui-polish-contact/` 的 13 张拼图已逐页检查：五个主窗口、设置七栏目及长表单底部、命令五栏目、管理三菜单/四弹窗、七插件设置页、Appearance/Shortcuts/Tabs、九类辅助窗口、两个编辑器，包含浅深色、紧凑窗口与 18px 控件字体。修复下拉字形错位、菜单宽度/隐藏项空行、短列表滚动条、深色预览白底/标题黑字/日志白色块、深色交替行/导航对比度与三类设置空页。 |
| 开发预览打包 | `cmake --install ... --prefix build/ui-preview`、`windeployqt` 退出 0，更新八插件。把同构建的 palette 测试程序和 QtTest 运行库放入预览目录，在无工具链 PATH 注入的隔离环境运行上述六个方法：8 passed / 0 failed；`build/ui-polish-packaged-results.txt`。`build/ui-preview/qclip.exe` SHA-256 `f3529471e861aeb8ee64688cad1b86789ba5d8f3a1c924afbe3351905b34918f`，与构建目录 EXE 一致。Qt 自身翻译 catalogs 未提供，打包保留其警告；本轮不是安装、签名或正式发布。 |
| 清单 / 差异 / 治理 | `python utils/check-compatibility.py` 退出 0（49 actions、90 options、24 forms、7 plugin forms、8 plugins、150 APIs）；`git diff --check` 退出 0。`check-harness.ps1` 仍在既有 `AGENTS.md` 链接占位文件处失败；本机及已知 WSL astack 路径均无 spec-lint 脚本，未声称两个治理门通过。 |

截图验证的是 Qt 窗口内容。Windows 材质测试报告 available/active 为 true，但原生屏幕抓取返回黑图，故不以 framebuffer 截图证明 Acrylic 的最终桌面合成效果。macOS/Linux 实机、真实中文输入法安装/候选窗、外部应用粘贴、多屏 DPI 切换、系统文件/字体/颜色选择器及所有用户自定义 QSS 组合未在本轮验收；保留相应已有实现和断言。视觉验证使用隔离会话，未启动日常会话监控或接触日常历史。用户随后明确授权提交本次改动并推送远程。

## 变更记录

| 日期 | 作者 | 内容 |
|---|---|---|
| 2026-09-29 | zerovoxxx | 接续落实 I2-1–I2-3 的集合树与主入口、90 项配置草稿、完整命令、编辑/辅助布局、插件呈现、历史过滤/时间/保护清理、旧主题映射和指定 CI；补齐显示副本生命周期、同步可见性与 macOS QCA provider，平台/复杂主题验收仍保持未完成。 |
| 2026-09-29 | zerovoxxx | 开始 I2-1，实现共享源的 QML 管理首批功能、显式多选/动作/命令/CLI、集合和传输、编辑数据保护及聚焦回归；登记完整入口清单，固定历史时间设计并明确尚未迁移界面和平台失败。SPEC/I2-1 保持开发中。 |
| 2026-09-28 | zerovoxxx | 从原总 SPEC 分出第二阶段，明确全界面和八插件保留、历史策略/主题兼容、三个实施里程碑及 P5/P6/P7/P12 的唯一归属。 |

SPEC3 新增脚本入口 `api:snippets`：打开 Snippet 浏览/编辑窗口，沿用现有代理及 CLI。实现与验证归属 SPEC3。
