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

| 顺序 | 本段交付 | 完成条件 |
|---|---|---|
| I2-1 功能清单与管理窗口 | 补齐旧入口/配置/命令/插件映射、主题方案，迁移管理和标签/批量操作 | 有可审阅的界面样板和未迁移项列表；管理使用真实历史并通过相关动作测试。 |
| I2-2 设置、命令、编辑与插件 | 迁移所有设置、脚本编辑/补全、内容编辑及八个插件，完成辅助窗口 | 原入口均有新入口，插件数据/显示/设置/编辑均验证；撤销/取消和显示命令有效。 |
| I2-3 历史策略与兼容收尾 | 类型/期限/容量/忽略/清理、旧主题/配置/数据往返、三端布局与交互 | P5/P6/P7/P12 全部有对应证据，未覆盖项清零；新 Snippet 功能明确交给 Iteration3。 |

### I2-1 实施决策（2026-09-29）

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

I2-1 首批管理功能已实现，I2-1 与本 SPEC 继续保持开发中；P5/P6/P7/P12 均未通过完整验收。入口清单已登记 49 个内置动作、87 个 AppConfig 配置、24 个主程序 UI 表单、7 个插件 UI 表单、149 个文档 API 名称（含别名）和 8 个正式插件。机械检查仅证明登记覆盖，不证明这些入口全部迁移或行为兼容。

- 管理窗口可从面板 Manage 和托盘进入，复用原历史和显示命令；支持独立查询、多选、批量复制/排序/四向移动/删除、单集合增删改/排序/属性、跨集合复制/移动，以及行/集合的原生拖放。拖放和传输保留完整 MIME，固定保护和取消删除不被绕过。
- 自定义命令收到管理的显式选中项，CLI 选择/查询/当前项/可见状态接入新管理视图。条目编辑沿用原 ItemEditorWidget；纯文本与备注保存保留未编辑的 MIME，新建取消不插入，源条目删除/卸载后保存失败保留输入并提示错误。Linux 已验证真实键盘保存/取消；macOS 前台编辑验收失败，见验证记录。
- 管理属性表单复用原容量、持久化和密码有效期字段；有效期保留秒单位及原范围，覆盖 172800 秒（两天）的真实 QML 表单输入。源集合在打开表单/删除确认时固定，期间 CLI 切换集合不会修改另一集合。Quick 源可见性接入原卸载判断，强制卸载后选择失效，重新点击原集合可重载。
- 主题目前复用旧配色和字体，800×520 紧凑布局及 1100×740 布局有实际 QML 窗口截图。原始截图位于本机 `build/spec2-artifacts/management.png`、`build/spec2-artifacts/management-compact.png`；截图和合成输入不能替代真实中文输入法、多屏/DPI 与前台粘贴验收。

| 下一段 | 尚未交付的范围与接入位置 |
|---|---|
| I2-1 收尾 | 真正集合树及组级重命名/删除/排序、图标新界面、完整上下文动作、设置/命令/编辑/辅助页面的可审阅样板；当前集合以完整路径平铺。部分按钮仍调用旧设置/图标/辅助窗口，不能据此关闭全界面验收。 |
| I2-2 | 同一 ConfigurationManager 配置源的所有设置页及 Apply/Cancel/默认值；命令条件/脚本编辑、补全/错误、默认激活命令和外部粘贴；完整富文本/FakeVim 编辑承载、全部插件设置/显示/编辑、辅助窗口。管理基础 CLI 已接入，但 `showAt` 的旧 Widgets 几何、预览/对话框与依赖旧控件的 API 仍待逐项迁移；149 名称登记不等于 149 行为通过。 |
| I2-3 | 历史类型/时间/来源/忽略/期限/清理的实现、旧主题 QSS 等价能力和数据/配置往返；本轮仅固定时间规则，未新增时间字段或清理逻辑。任意 QSS、第三方插件 ABI、真实加密到期/同步行为不能凭插件加载或配色测试宣称兼容。 |
| 平台收尾 | Windows 已接入构建/隔离测试脚本，尚无原生运行证据；macOS 前台日志为 `loginwindow - Login`，原生焦点测试失败，提交前另有拖放测试窗口暴露超时；前台编辑、真实输入法、外部粘贴和多屏/DPI 待补。Linux 仅 Debian 12 arm64/Xvfb，QCA/原生通知/音频关闭，Wayland 未验证。 |

继续实施前先核对下节逐项状态与对应测试，不删除旧控制代码。管理窗口状态入口在 `mainwindow_management.cpp`；选择/显示复用 `ClipboardPaletteModel`；编辑保存保护在 `MainWindow::openItemEditor`；Quick 可见源的缓存判断在 `ClipboardBrowserPlaceholder`。新增页面继续复用原配置/插件/脚本数据流，不另建配置或历史。

### I2-1 逐项兼容基线

本清单的“待迁移”表示新入口尚未通过行为验证；旧入口仍可用于对照。`python3 utils/check-compatibility.py` 核对源码入口与本文登记的集合，不能替代运行验收。配置与 API 按名称逐项登记，旧参数、错误和输出继续以原实现为准。

| 范围 | 登记数 | 当前职责 |
|---|---|---|
| 菜单/编辑动作 | 49 | I2-1 登记，I2-2/I2-3 逐项验证 |
| 主工程表单 | 24 | I2-1 登记，I2-2/I2-3 逐项验证 |
| 插件设置表单 | 7 | I2-1 登记，I2-2/I2-3 逐项验证 |
| AppConfig 配置 | 87 | I2-1 登记，I2-2/I2-3 逐项验证 |
| 文档化脚本函数（重载合并，包含同声明别名） | 149 | I2-1 登记，I2-2/I2-3 逐项验证 |
| 内置插件 | 8 | I2-1 登记，I2-2/I2-3 逐项验证 |

| 旧入口标识 | 新入口 / 验证归属 | 当前状态 |
|---|---|---|
| `action:Edit_CopySelectedItems` | ManagementWindow 复制 / managementActions | macOS/Linux 已通过 |
| `action:Edit_FindItems` | 管理搜索和原 filter() / qmlSelectionAndActions、managementActions | QML/CLI 已通过；实机快捷键待补 |
| `action:Edit_PasteItems` | ManagementWindow 粘贴到集合 / managementActions | macOS/Linux 已通过 |
| `action:Edit_ReverseSelectedItems` | 原 reverseItems 语义 / managementActions | macOS/Linux 已通过 |
| `action:Edit_SortSelectedItems` | 原 sortItems 语义 / managementActions | macOS/Linux 已通过 |
| `action:Editor_Background` | 重新设计的编辑承载窗口 / editorRoundTrip | 待迁移 |
| `action:Editor_Bold` | 重新设计的编辑承载窗口 / editorRoundTrip | 待迁移 |
| `action:Editor_Cancel` | 共用 ItemEditorWidget 承载窗口 / managementEditor | Linux 保存/取消通过；完整编辑迁移待 I2-2 |
| `action:Editor_EraseStyle` | 重新设计的编辑承载窗口 / editorRoundTrip | 待迁移 |
| `action:Editor_Font` | 重新设计的编辑承载窗口 / editorRoundTrip | 待迁移 |
| `action:Editor_Foreground` | 重新设计的编辑承载窗口 / editorRoundTrip | 待迁移 |
| `action:Editor_Italic` | 重新设计的编辑承载窗口 / editorRoundTrip | 待迁移 |
| `action:Editor_Redo` | 重新设计的编辑承载窗口 / editorRoundTrip | 待迁移 |
| `action:Editor_Save` | 合并编辑 MIME、失效时保留输入 / managementEditor | Linux 已通过；macOS 前台焦点未通过 |
| `action:Editor_Search` | 重新设计的编辑承载窗口 / editorRoundTrip | 待迁移 |
| `action:Editor_Strikethrough` | 重新设计的编辑承载窗口 / editorRoundTrip | 待迁移 |
| `action:Editor_Underline` | 重新设计的编辑承载窗口 / editorRoundTrip | 待迁移 |
| `action:Editor_Undo` | 重新设计的编辑承载窗口 / editorRoundTrip | 待迁移 |
| `action:File_Commands` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `action:File_Exit` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `action:File_Export` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `action:File_Import` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `action:File_New` | 管理新建编辑窗口 / managementEditor | Linux 保存/取消通过；完整编辑迁移待 I2-2 |
| `action:File_Preferences` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `action:File_ProcessManager` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `action:File_ShowClipboardContent` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `action:File_ShowPreview` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `action:File_ToggleClipboardStoring` | 管理暂停/恢复 / managementActions | macOS/Linux 已通过；类型/期限/忽略待 I2-3 |
| `action:Help_About` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `action:Help_Help` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `action:Help_ShowLog` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `action:ItemMenu` | 管理 Actions 菜单及原自定义命令 / managementCommands | 显式多选和 matchCmd 通过；键位/菜单定制待补 |
| `action:Item_Action` | ManagementWindow → ClipboardManagement 显式选择动作 / managementActions | 待迁移 |
| `action:Item_Edit` | 共用 ItemEditorWidget、其他 MIME 保留 / managementEditor | Linux 已通过；完整编辑/FakeVim 待 I2-2 |
| `action:Item_EditNotes` | 备注独立合并保存 / managementEditor | Linux 已通过；macOS 前台焦点未通过 |
| `action:Item_EditWithEditor` | ManagementWindow → ClipboardManagement 显式选择动作 / managementActions | 待迁移 |
| `action:Item_MoveDown` | 原 move / managementActions | macOS/Linux 已通过 |
| `action:Item_MoveToBottom` | 原 move / managementActions | macOS/Linux 已通过 |
| `action:Item_MoveToClipboard` | 原 moveToClipboard 显式选择 / managementActions、qmlSelectionAndActions | 复制/置顶通过；默认动作及外部粘贴待 I2-2 |
| `action:Item_MoveToTop` | 原 move / managementActions | macOS/Linux 已通过 |
| `action:Item_MoveUp` | 原 move / managementActions | macOS/Linux 已通过 |
| `action:Item_Remove` | 原 removeIndexes 及插件/脚本保护 / managementActions、transferAndDeleteProtection | macOS/Linux 已通过 |
| `action:Item_ShowContent` | ManagementWindow → ClipboardManagement 显式选择动作 / managementActions | 待迁移 |
| `action:Tabs_ChangeTabIcon` | ManagementWindow → ClipboardManagement 显式选择动作 / managementActions | 待迁移 |
| `action:Tabs_NewTab` | QML 集合表单、原 createTab / managementTabs | 增改删/路径式分组通过；组级交互待补 |
| `action:Tabs_NextTab` | 原 nextTab / managementTabs | macOS/Linux 已通过 |
| `action:Tabs_PreviousTab` | 原 previousTab / managementTabs | macOS/Linux 已通过 |
| `action:Tabs_RemoveTab` | QML 确认、原 removeTab / managementTabs | 单集合删除通过；组级删除待补 |
| `action:Tabs_RenameTab` | QML 来源快照、原 renameTab / managementTabs、qmlSelectionAndActions | 单集合更名通过；组级更名待补 |
| `form:src/ui/aboutdialog.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/actiondialog.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/actionhandlerdialog.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/addcommanddialog.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/clipboarddialog.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/commanddialog.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/commandedit.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/commandwidget.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/configtabappearance.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/configtabgeneral.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/configtabhistory.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/configtablayout.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/configtabnotifications.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/configtabtray.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/configurationmanager.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/importexportdialog.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/itemorderlist.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/logdialog.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/mainwindow.ui` | ManagementWindow、共享真实历史 / managementActions、managementTabs、managementCommands | 核心管理通过；其余入口仍待 I2-2 |
| `form:src/ui/pluginwidget.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/shortcutdialog.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/shortcutswidget.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `form:src/ui/tabdialog.ui` | QML 创建/更名/删除确认 / managementTabs、qmlSelectionAndActions | 单集合通过；组级/图标对话框待补 |
| `form:src/ui/tabpropertieswidget.ui` | QML 集合容量/存盘/密码期限 / managementTabs、qmlSelectionAndActions | 保存/重载、跨来源确认通过；密码过期实测待补 |
| `plugin-form:plugins/itemencrypted/itemencryptedsettings.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `plugin-form:plugins/itemfakevim/itemfakevimsettings.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `plugin-form:plugins/itemimage/itemimagesettings.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `plugin-form:plugins/itemnotes/itemnotessettings.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `plugin-form:plugins/itemsync/itemsyncsettings.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `plugin-form:plugins/itemtags/itemtagssettings.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |
| `plugin-form:plugins/itemtext/itemtextsettings.ui` | I2-2 对应设置/命令/编辑/辅助页面；I2-3 主题往返 | 待迁移 |

| 原配置（名称保持） | 新入口 / 验证归属 | 当前状态 |
|---|---|---|
| `option:activate_closes` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:activate_focuses` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:activate_item_with_single_click` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:activate_pastes` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:always_on_top` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:autocompletion` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:autostart` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:check_clipboard` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:check_selection` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:clipboard_mime_size_limit` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:clipboard_notification_lines` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:clipboard_tab` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:close_on_unfocus` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:close_on_unfocus_delay_ms` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:close_on_unfocus_extra_delay_ms` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:command_history_size` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:confirm_exit` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:copy_clipboard` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:copy_selection` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:disable_tray` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:edit_ctrl_return` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:editor` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:encrypt_tabs` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:expire_encrypted_tab_seconds` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:expire_tab` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:filter_case_insensitive` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:filter_regular_expression` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:frameless_window` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:hide_main_window` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:hide_main_window_in_task_bar` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:hide_tabs` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:hide_toolbar` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:hide_toolbar_labels` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:item_data_threshold` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:item_popup_interval` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:max_process_manager_rows` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:maxitems` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:move` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:native_menu_bar` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:native_notifications` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:native_tray_menu` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:navigation_style` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:notification_horizontal_offset` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:notification_maximum_height` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:notification_maximum_width` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:notification_position` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:notification_vertical_offset` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:number_search` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:open_windows_on_current_screen` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:prevent_screen_capture` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:restore_geometry` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:row_index_from_one` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:run_selection` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:save_delay_ms_on_item_added` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:save_delay_ms_on_item_edited` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:save_delay_ms_on_item_modified` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:save_delay_ms_on_item_moved` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:save_delay_ms_on_item_removed` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:save_filter_history` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:save_on_app_deactivated` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:script_paste_delay_ms` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:show_advanced_command_settings` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:show_simple_items` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:show_tab_item_count` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:style` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:tab_tree` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:tabs` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:terminate_action_timeout_ms` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:text_tab_width` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:text_wrap` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:transparency` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:transparency_focused` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:tray_commands` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:tray_images` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:tray_item_paste` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:tray_items` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:tray_menu_open_on_left_click` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:tray_tab` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:tray_tab_is_current` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:update_clipboard_owner_delay_ms` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:use_key_store` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:window_key_press_time_ms` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:window_paste_with_ctrl_v_regex` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:window_wait_after_raised_ms` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:window_wait_before_raise_ms` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:window_wait_for_modifier_released_ms` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |
| `option:window_wait_raised_ms` | I2-2 设置及原 config() / commandConfig；I2-3 保存/取消/默认值比对 | 待迁移 |

| 原脚本 API（名称及参数保持） | 新入口 / 验证归属 | 当前状态 |
|---|---|---|
| `api:abort`、`api:action`、`api:add`、`api:addCommands`、`api:afterMilliseconds`、`api:change`、`api:clearClipboardData`、`api:clipboard` | 原 Scriptable/Proxy；I2-1 显式动作快照，I2-2 命令编辑/补全；针对性 CLI 验证 | 实现保留，新界面接入待验证 |
| `api:clipboardFormatsToSave`、`api:commands`、`api:config`、`api:copy`、`api:copySelection`、`api:count`、`api:currentItem`、`api:currentPath` | 原 Scriptable/Proxy；I2-1 显式动作快照，I2-2 命令编辑/补全；针对性 CLI 验证 | 实现保留，新界面接入待验证 |
| `api:data`、`api:dataFormats`、`api:dateString`、`api:dialog`、`api:disable`、`api:edit`、`api:editItem`、`api:env` | 原 Scriptable/Proxy；I2-1 显式动作快照，I2-2 命令编辑/补全；针对性 CLI 验证 | 实现保留，新界面接入待验证 |
| `api:escapeHtml`、`api:eval`、`api:execute`、`api:exit`、`api:exportData`、`api:exportTab`、`api:fail`、`api:filter` | 原 Scriptable/Proxy；I2-1 显式动作快照，I2-2 命令编辑/补全；针对性 CLI 验证 | 实现保留，新界面接入待验证 |
| `api:focusPrevious`、`api:focused`、`api:forceUnload`、`api:fromBase64`、`api:fromUnicode`、`api:getItem`、`api:hasClipboardFormat`、`api:hasData` | 原 Scriptable/Proxy；I2-1 显式动作快照，I2-2 命令编辑/补全；针对性 CLI 验证 | 实现保留，新界面接入待验证 |
| `api:hasSelectionFormat`、`api:help`、`api:hide`、`api:hideDataNotification`、`api:iconColor`、`api:iconTag`、`api:iconTagColor`、`api:ignore` | 原 Scriptable/Proxy；I2-1 显式动作快照，I2-2 命令编辑/补全；针对性 CLI 验证 | 实现保留，新界面接入待验证 |
| `api:importData`、`api:importTab`、`api:info`、`api:input`、`api:insert`、`api:isClipboard`、`api:loadTheme`、`api:logs` | 原 Scriptable/Proxy；I2-1 显式动作快照，I2-2 命令编辑/补全；针对性 CLI 验证 | 实现保留，新界面接入待验证 |
| `api:md5sum`、`api:menu`、`api:menuItems`、`api:monitorClipboard`、`api:monitoring`、`api:move`、`api:next`、`api:notification` | 原 Scriptable/Proxy；I2-1 显式动作快照，I2-2 命令编辑/补全；针对性 CLI 验证 | 实现保留，新界面接入待验证 |
| `api:onClipboardChanged`、`api:onClipboardUnchanged`、`api:onExit`、`api:onHiddenClipboardChanged`、`api:onItemsAdded`、`api:onItemsChanged`、`api:onItemsLoaded`、`api:onItemsRemoved` | 原 Scriptable/Proxy；I2-1 显式动作快照，I2-2 命令编辑/补全；针对性 CLI 验证 | 实现保留，新界面接入待验证 |
| `api:onOwnClipboardChanged`、`api:onSecretClipboardChanged`、`api:onStart`、`api:onTabSelected`、`api:open`、`api:pack`、`api:palette`、`api:paste` | 原 Scriptable/Proxy；I2-1 显式动作快照，I2-2 命令编辑/补全；针对性 CLI 验证 | 实现保留，新界面接入待验证 |
| `api:playSound`、`api:pointerPosition`、`api:popup`、`api:preview`、`api:previous`、`api:print`、`api:provideClipboard`、`api:provideSelection` | 原 Scriptable/Proxy；I2-1 显式动作快照，I2-2 命令编辑/补全；针对性 CLI 验证 | 实现保留，新界面接入待验证 |
| `api:queryKeyboardModifiers`、`api:read`、`api:remove`、`api:removeData`、`api:removeTab`、`api:renameTab`、`api:runAutomaticCommands`、`api:saveData` | 原 Scriptable/Proxy；I2-1 显式动作快照，I2-2 命令编辑/补全；针对性 CLI 验证 | 实现保留，新界面接入待验证 |
| `api:screenNames`、`api:screenshot`、`api:screenshotSelect`、`api:select`、`api:selectItems`、`api:selectedItemData`、`api:selectedItems`、`api:selectedItemsData` | 原 Scriptable/Proxy；I2-1 显式动作快照，I2-2 命令编辑/补全；针对性 CLI 验证 | 实现保留，新界面接入待验证 |
| `api:selectedTab`、`api:selection`、`api:separator`、`api:serverLog`、`api:setClipboardData`、`api:setCommands`、`api:setCurrentTab`、`api:setData` | 原 Scriptable/Proxy；I2-1 显式动作快照，I2-2 命令编辑/补全；针对性 CLI 验证 | 实现保留，新界面接入待验证 |
| `api:setEnv`、`api:setItem`、`api:setPointerPosition`、`api:setSelectedItemData`、`api:setSelectedItemsData`、`api:setTitle`、`api:settings`、`api:sha1sum` | 原 Scriptable/Proxy；I2-1 显式动作快照，I2-2 命令编辑/补全；针对性 CLI 验证 | 实现保留，新界面接入待验证 |
| `api:sha256sum`、`api:sha512sum`、`api:show`、`api:showAt`、`api:showDataNotification`、`api:sleep`、`api:source`、`api:stats` | 原 Scriptable/Proxy；I2-1 显式动作快照，I2-2 命令编辑/补全；针对性 CLI 验证 | 实现保留，新界面接入待验证 |
| `api:str`、`api:styles`、`api:synchronizeFromSelection`、`api:synchronizeToSelection`、`api:tab`、`api:tabIcon`、`api:toBase64`、`api:toUnicode` | 原 Scriptable/Proxy；I2-1 显式动作快照，I2-2 命令编辑/补全；针对性 CLI 验证 | 实现保留，新界面接入待验证 |
| `api:toggle`、`api:toggleConfig`、`api:unload`、`api:unpack`、`api:updateClipboardData`、`api:updateTitle`、`api:version`、`api:visible` | 原 Scriptable/Proxy；I2-1 显式动作快照，I2-2 命令编辑/补全；针对性 CLI 验证 | 实现保留，新界面接入待验证 |
| `api:write` | 原 Scriptable/Proxy；I2-1 显式动作快照，I2-2 命令编辑/补全；针对性 CLI 验证 | 实现保留，新界面接入待验证 |
| `api:enable`、`api:index`、`api:length`、`api:size` | 原 API 同声明别名；继续使用原参数/返回语义；managementActions / managementTabs 验证 size | 实现保留，别名逐项回归待补 |

| 内置插件 | 原接口 → 新入口 / 验证归属 | 当前状态 |
|---|---|---|
| `plugin:itemencrypted` | ItemLoaderInterface 呈现/匹配/数据/保存/编辑/设置/命令 → 管理预览、编辑窗口、插件设置；验证见“八个插件”表 | 全能力待迁移 |
| `plugin:itemfakevim` | ItemLoaderInterface 呈现/匹配/数据/保存/编辑/设置/命令 → 管理预览、编辑窗口、插件设置；验证见“八个插件”表 | 全能力待迁移 |
| `plugin:itemimage` | ItemLoaderInterface 呈现/匹配/数据/保存/编辑/设置/命令 → 管理预览、编辑窗口、插件设置；验证见“八个插件”表 | 全能力待迁移 |
| `plugin:itemnotes` | ItemLoaderInterface 呈现/匹配/数据/保存/编辑/设置/命令 → 管理预览、编辑窗口、插件设置；验证见“八个插件”表 | 全能力待迁移 |
| `plugin:itempinned` | ItemLoaderInterface 呈现/匹配/数据/保存/编辑/设置/命令 → 管理预览、编辑窗口、插件设置；验证见“八个插件”表 | 全能力待迁移 |
| `plugin:itemsync` | ItemLoaderInterface 呈现/匹配/数据/保存/编辑/设置/命令 → 管理预览、编辑窗口、插件设置；验证见“八个插件”表 | 全能力待迁移 |
| `plugin:itemtags` | ItemLoaderInterface 呈现/匹配/数据/保存/编辑/设置/命令 → 管理预览、编辑窗口、插件设置；验证见“八个插件”表 | 全能力待迁移 |
| `plugin:itemtext` | ItemLoaderInterface 呈现/匹配/数据/保存/编辑/设置/命令 → 管理预览、编辑窗口、插件设置；验证见“八个插件”表 | 全能力待迁移 |

非 .ui 辅助入口还包括 IconSelectDialog、密码/加密提示、ActionHandlerDialog、ActionHandler 进度、Notification、TrayMenu、ScriptableProxy 的 dialog()/input()/screenshotSelect()、FileDialog 和错误/确认提示；全部交给 I2-2 的独立入口迁移，不能以统一 QSS 代替完成。第三方插件源码接口保持，旧二进制 ABI 尚未验证。

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

I2-1 本机 macOS 的配置与指定测试命令如下；每次运行通过隔离 runner 创建临时 session/config/data/state，测试保存/恢复系统剪贴板，不使用日常历史。QML lint 与同目录构建顺序执行，避免并发修改 Ninja 元数据：

~~~bash
CMAKE_PREFIX_PATH='/opt/homebrew/opt/qt;/opt/homebrew/opt/qca;/opt/homebrew/opt/qtkeychain' \
  cmake --preset macOS-13-m1 -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DWITH_NATIVE_NOTIFICATIONS=OFF -DWITH_AUDIO=OFF
cmake --build build/copyq/macOS-13-m1 -j 6
cmake --build build/copyq/macOS-13-m1 --target copyq-palette-ui_qmllint
utils/run-isolated.sh build/copyq/macOS-13-m1/copyq-management-tests \
  multiSelectionIdentity bulkSelectionPerformance sourceLifetimeAndDisplay \
  transferAndDeleteProtection qmlSelectionAndActions nativeDropRoundTrip themeMapping
utils/run-isolated.sh build/copyq/macOS-13-m1/copyq-tests \
  testCore:managementActions testCore:managementTabs testCore:managementCommands \
  testCore:configPath testCore:paletteSearchAndCopy testCore:paletteMimeAndDisplayCommands
utils/run-isolated.sh build/copyq/macOS-13-m1/copyq-tests testCore:managementEditor
utils/run-isolated.sh build/copyq/macOS-13-m1/copyq-palette-tests \
  modelIdentity queryChanges displayCopiesAndPreview sourceDestructionAndReset \
  qmlKeyboardAndIme explicitCommands standardPreviews nativeWindowIdentification pluginEditorAndSettings
~~~

最后两条包含真实前台输入/激活，当前环境有失败记录，不能用其余通过项代替。管理的 IME 测试发送真实 QInputMethodEvent，但不启动系统输入法。

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
  transferAndDeleteProtection qmlSelectionAndActions nativeDropRoundTrip themeMapping
docker exec -e DISPLAY=:99 -e QT_QUICK_BACKEND=software qclip-spec1-linux \
  /workspace/utils/run-isolated.sh /workspace/build/linux/copyq-tests \
  testCore:managementActions testCore:managementTabs testCore:managementCommands testCore:managementEditor \
  testCore:configPath testCore:paletteSearchAndCopy testCore:paletteMimeAndDisplayCommands testCore:paletteEditor
docker exec -e DISPLAY=:99 -e QT_QUICK_BACKEND=software qclip-spec1-linux \
  /workspace/utils/run-isolated.sh /workspace/build/linux/copyq-palette-tests \
  modelIdentity queryChanges displayCopiesAndPreview sourceDestructionAndReset \
  qmlKeyboardAndIme explicitCommands standardPreviews nativeWindowIdentification pluginEditorAndSettings
~~~

本批入口机械检查为 `python3 utils/check-compatibility.py`。新增测试目标要求指定函数；CI 使用上述管理函数，不直接运行全套。Windows runner 与新测试可执行文件已接入现有工作流和部署脚本，配置检查不等于 Windows 实机测试。

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

| 日期 | 命令 / 检查 | 结果与证据边界 |
|---|---|---|
| 2026-09-29 | 全部改动提交前：两平台 `cmake --build ... -j 6` 与 `copyq-palette-ui_qmllint`，Linux 隔离 runner 的本文 7 个 management 函数及 SPEC1 12 个服务函数加 managementActions/managementTabs/managementCommands/managementEditor | 构建/lint 均退出 0；Linux 管理 9 passed / 0 failed、服务 18 passed / 0 failed（含 init/cleanup），完整 MIME、选择保护、真实键盘编辑/取消/失效源及相关面板回归通过。日志 `build/ship-linux-management.log`、`build/ship-linux-server.log`。用户本轮明确授权全部本地修改提交并推送 `origin/master`；SPEC1/SPEC2 保持开发中，不等于发布或完整验收。 |
| 2026-09-29 | 提交前 macOS 隔离 runner 的本文 7 个 management 函数 | 8 passed / 1 failed / 0 skipped：`nativeDropRoundTrip` 在 `QTest::qWaitForWindowExposed(&window)` 超时，尚未进入拖放断言；其余选择、保护、QML 操作、主题函数通过，5 万条全选/插入恢复 98ms。停止该平台 GUI 验收并保留 `build/ship-mac-management.log`，没有重试到通过或删改断言；先前通过记录不能替代本次结果。 |
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

## 变更记录

| 日期 | 作者 | 内容 |
|---|---|---|
| 2026-09-29 | zerovoxxx | 开始 I2-1，实现共享源的 QML 管理首批功能、显式多选/动作/命令/CLI、集合和传输、编辑数据保护及聚焦回归；登记完整入口清单，固定历史时间设计并明确尚未迁移界面和平台失败。SPEC/I2-1 保持开发中。 |
| 2026-09-28 | zerovoxxx | 从原总 SPEC 分出第二阶段，明确全界面和八插件保留、历史策略/主题兼容、三个实施里程碑及 P5/P6/P7/P12 的唯一归属。 |
