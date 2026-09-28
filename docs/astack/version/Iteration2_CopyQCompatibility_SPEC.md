# Iteration2：QClip 全界面重做与 CopyQ 能力保留

> **文档信息**
>
> | 字段 | 值 |
> |---|---|
> | 文档类型 | SPEC |
> | 文档状态 | 待实施 |
> | 创建日期 | 2026-09-28 |
> | 最后更新 | 2026-09-28 |
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

本次仅新增 SPEC。未来全界面迁移涉及十个以上文件，原因是多个独立窗口、插件与脚本可见行为均属于用户确认范围；按下表和里程碑分批，禁止同时无边界改写全部 GUI。

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
| 2026-09-28 | 源码范围及既有测试名称核对 | 已确认八插件、配置/显示命令耦合、expire_tab 卸载语义及表中测试入口；产品源码未改，尚未运行应用/构建/产品测试。 |
| 2026-09-28 | 三份 SPEC 文档质量门 | harness、目录级 SPEC lint、git diff --check 均退出 0；三份 SPEC 0 errors/0 warnings，P1–P14 唯一归属及 18 个既有测试声明核对通过。仅文档/静态检查，产品验收仍待实施。 |

## 变更记录

| 日期 | 作者 | 内容 |
|---|---|---|
| 2026-09-28 | zerovoxxx | 从原总 SPEC 分出第二阶段，明确全界面和八插件保留、历史策略/主题兼容、三个实施里程碑及 P5/P6/P7/P12 的唯一归属。 |
