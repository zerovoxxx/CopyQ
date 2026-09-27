# Iteration1：Windows 剪贴板快捷面板

> **文档信息**
>
> | 字段 | 值 |
> |---|---|
> | 文档类型 | SPEC |
> | 文档状态 | 待实施 |
> | 创建日期 | 2026-09-28 |
> | 最后更新 | 2026-09-28 |
> | 作者 | zerovoxxx |
> | 关联文档 | [迭代索引](../INDEX.md)、[项目入口](../../../CLAUDE.md)；本轮用户需求讨论 |
> | 一句话目标 | 在保留 CopyQ 全部现有功能的基础上，以独立快捷面板完成 Windows 上的历史查找、选择和向原窗口粘贴。 |

## 目标与非目标

### 已确认的目标

- 用户优先在本机 Windows 使用，未来可能面向其他用户发布。
- 整体产品目标是复现 Alfred 的剪贴板能力和产品交互；CopyQ 已有功能全部保留。
- 呼出剪贴板后选择记录，按回车自动粘贴到呼出前正在使用的应用、具体窗口和输入位置。
- 用户指定本机 astack 的 harness 流程管理此次改造，需求、设计和验证结果写入本命名空间。

### 本迭代建议交付边界

先完成“呼出 → 输入搜索 → 选择 → 回车粘贴 → 回到原应用”的完整链路，并保留完整管理窗口入口。这是面向整体目标的第一个可验收阶段；需求讨论中的高级能力按下文候选清单安排，不因为未进入本迭代而移除 CopyQ 已有功能。

本轮初始化只交付治理文件、需求基线和机械校验，不把下文规划的产品功能记为已实现。Qt Widgets 是基于当前代码的技术决策；具体主题、热键和呈现细节尚有待明确项。

### 非目标

- 本迭代不实现 Alfred 的应用启动器、全盘文件搜索、工作流编辑器或账号服务。
- 不替换 CopyQ 的历史存储，不创建第二份独立历史，不删除标签页、插件、脚本、编辑、导入导出等功能。
- Snippet 自动展开、连续复制合并、跨设备同步和 OCR 不在首个面板迭代的实施范围；前三者中与 Alfred 剪贴板相关的能力继续保留在候选清单。
- 初始化阶段只处理治理文件与需求基线。后续用户已另行授权启动原始 UI 预览和提交、推送当前改动；定制应用构建与对外发布仍未开展。
- macOS 的字体栅格化、系统材质和系统 Quick Look 不作为 Windows 的逐像素一致保证；可对照窗口结构、间距、颜色和操作流程验收。

### 后续待明确项

| 项目 | 已知情况与暂定处理 |
|---|---|
| Alfred 外观 | 本轮已核对的版本为 5.8.1；用户尚未提供主题、截图或录屏。暂以官方现代深色外观作为草案，最终视觉验收前补齐参照，不把它称为已确认的逐像素样板。 |
| 全局热键 | 沿用 CopyQ 的可配置快捷键机制；具体默认组合在本机冲突检查后确定，尚未指定组合。 |
| 粘贴格式 | 建议保留历史项原始 MIME 数据，另提供纯文本动作；默认纯文本还是保留格式需要在产品验收前确定，不能在读取或展示时丢弃原数据。 |
| 快捷键映射 | Enter 的自动粘贴语义已确认；复制、预览、删除、数字直选等 Windows 键位在交互草图中明确。 |
| Windows 环境 | 系统版本、缩放、多显示器布局及常用目标应用尚未核查；在原生测试准备任务中从本机确认，不凭历史项目推断。 |

上述事项不阻止 harness 初始化与基础结构设计。与这些选择有关的视觉或行为实现，应先在 SPEC 中更新决定及验收口径。

## 变更范围

### 本轮已授权的初始化文件

| 文件 | 操作 | 职责 |
|---|---|---|
| [CLAUDE.md](../../../CLAUDE.md) | MODIFY | astack 导航模板、原有 CopyQ 约束、真实质量门、活跃迭代入口 |
| [AGENTS.md](../../../AGENTS.md) | MODIFY | 共享治理内容；本机采用 NTFS 硬链接 |
| [.claude/settings.json](../../../.claude/settings.json) | MODIFY | 补齐隔离状态路径，令 Qt 平台配置与原有 X11 约束一致 |
| [INDEX.md](../INDEX.md) | NEW | 迭代状态和短变更记录 |
| 本 SPEC | NEW | 已确认需求、设计决定、待明确项和验收依据 |
| `docs/astack/plan/.gitkeep` | NEW | 让计划目录随仓库保存；尚未生成产品实现 PLAN |
| [utils/check-harness.ps1](../../../utils/check-harness.ps1) | NEW | 检查治理入口、Windows 链接适配、文档链接、状态同步和环境配置 |

### 产品实施涉及的现有位置

| 位置 / 语义锚点 | 计划影响与原因 |
|---|---|
| [src/gui/mainwindow.h](../../../src/gui/mainwindow.h)、[mainwindow.cpp](../../../src/gui/mainwindow.cpp)：`showWindow`、`activateCurrentItemHelper`、`updateFocusWindows` | 接入快捷面板与完整窗口入口；复用原应用窗口记录及粘贴流程，防止面板将自身覆盖为粘贴目标 |
| [src/gui/clipboardbrowser.h](../../../src/gui/clipboardbrowser.h)、[src/item/clipboardmodel.h](../../../src/item/clipboardmodel.h) | 复用同一历史数据；确认对象生命周期、行索引变化、模型变更通知，避免保存第二套历史 |
| [src/gui/filterlineedit.cpp](../../../src/gui/filterlineedit.cpp)、[src/item/itemdelegate.h](../../../src/item/itemdelegate.h) | 复用筛选与内容访问能力，独立绘制紧凑结果行；是否能直接复用现有 delegate 在实施前评估 |
| `src/gui/` 中拟新增面板文件 | 承载搜索、单选列表、键盘导航、空结果和关闭行为；命名先检索同类实现再确定 |
| [src/platform/win/winplatformwindow.cpp](../../../src/platform/win/winplatformwindow.cpp)：`raiseWindow`、`pasteFromClipboard` | 恢复原窗口，按已有配置发送粘贴事件，处理目标失效和焦点恢复失败 |
| [src/common/globalshortcutcommands.cpp](../../../src/common/globalshortcutcommands.cpp)、[src/common/appconfig.h](../../../src/common/appconfig.h) | 在既有命令/配置机制中接入面板，不另建全局热键系统 |
| [src/CMakeLists.txt](../../../src/CMakeLists.txt)、[src/ui/mainwindow.ui](../../../src/ui/mainwindow.ui)、[shared/themes](../../../shared/themes) | 注册新增源码及样式资源，维护完整管理窗口入口 |
| [src/tests/tests.h](../../../src/tests/tests.h)、[src/tests/tests.cpp](../../../src/tests/tests.cpp) | 为新面板与现有搜索、历史激活行为提供聚焦验证；实际 Windows 粘贴单独验证 |

表中产品源码在本轮初始化未修改。跨文件接入前，PLAN 要明确历史数据生产方、面板消费方、原窗口记录者和粘贴执行者之间的契约。

## 外部参照与设计取舍

| 参照 | 借鉴点 | 本项目的取舍 |
|---|---|---|
| [Alfred Clipboard History](https://www.alfredapp.com/help/features/clipboard/) | 输入搜索、历史类型、回车粘贴、保留期限、忽略应用、Snippet、合并 | 以实际剪贴板流程作为整体产品参照；首个迭代优先完整呼出与粘贴链路 |
| [Alfred Changelog](https://www.alfredapp.com/changelog/) | 本轮核对版本 5.8.1；5.1 已加入历史项 Quick Look 和直接打开文件/URL | 预览与打开能力保留为后续对齐项目，不能误称 5.8.1 新增；以后更新参照版本需重新核对 |
| [Alfred Appearance](https://www.alfredapp.com/help/appearance/) | 可变主题、结果数量、多屏位置以及面板焦点模式 | “最新版”不能唯一确定像素外观；Windows 应验证自身焦点链路，不能直接照搬 macOS 非激活面板实现 |
| [本仓库 CMake](../../../CMakeLists.txt) 与 Windows 平台实现 | C++17、Qt 6 Widgets、既有模型及 SendInput 粘贴 | Qt Widgets 新增独立面板；保持既有存储和平台路径，避免引入外部进程协议 |
| 本机 astack `astack-marketplace/plugins/astack-workflow/skills/harness-init` | 主入口、迭代索引与单一 SPEC | 保留模板核心原则；本机 NTFS 硬链接适配见项目入口，不宣称通过原脚本的最终软链检查 |

| UI 方案 | 取舍 |
|---|---|
| Qt Widgets 独立快捷面板（采用） | 与现有 Qt 模型、事件循环和 Windows 实现直接集成；列表行、窗口形状和键盘状态需要专门实现 |
| Qt Quick 面板 | 适用于后续明确的复杂动效需求；当前会增加 UI 桥接和运行依赖，尚无证据表明必要 |
| WinUI 3 前端 | Windows 控件集可用，但需拆分服务边界或维护跨框架接口；不符合当前复用已有项目的最小改造范围 |

## 实现说明

### 产品入口

新增日常使用的快捷面板，同时让托盘或明确动作可进入完整 CopyQ 管理窗口。面板与管理窗口访问同一历史；标签、插件、脚本、备注、编辑、导入导出保持可达。

界面建议采用搜索输入区、紧凑结果列表和必要的快捷键提示。历史为空与搜索无结果应有不同文案。主题细节以待补充样板为准。

### 回车粘贴契约

1. 面板获得焦点前记录实际前台窗口；同一应用的不同窗口必须区分，不能只记应用名。
2. 输入文字改变筛选；上下键改变选择。中文输入法正在组词时，Enter 先用于确认候选，不得同时激活历史项。
3. 用户确认后取得所选条目的原始数据。历史更新导致行号变化时，仍需指向原选择内容。
4. 等待剪贴板写入完成，隐藏面板并恢复原窗口，再经 CopyQ 的 Windows 粘贴链路执行。
5. Esc 或失焦关闭不触发复制/粘贴；只浏览和选择也不更改系统剪贴板。
6. 目标窗口已销毁、焦点恢复失败或目标不可粘贴时，不向其他窗口盲发粘贴事件；提供与实际结果一致的反馈，不能把“已写入剪贴板”描述为“已粘贴成功”。

“仅复制”动作与“回车自动粘贴”各自明确。具体键位待交互草图确定；默认粘贴格式按上文待明确项收敛。

### 兼容与验证边界

- Windows 原生进程的焦点、中文输入法、多屏缩放与目标应用兼容性是首轮风险，不能通过改颜色或静态源码检查来证明。
- 测试必须使用项目约定的隔离路径、测试会话与指定函数；后台真实历史不得作为测试数据。
- 原有 `build/copyq`、`build/copyq-tests` 命令继续遵守全部环境变量和 Xvfb/openbox 要求。原生 Windows `.exe` 测试需另行落实隔离启动入口；Linux xcb 配置不能原样用于 Windows 程序。
- 构建、普通测试、插件验证与实际 Windows 用户体验分别记录。禁用插件的测试配置不能单独证明插件兼容。

### 后续候选能力

- 历史项文本、图片、单文件预览；直接打开 URL/文件。
- 历史记录保存为 Snippet、Snippet 分类浏览及自动展开。
- 连续复制合并、纯文本粘贴快捷动作。
- 在既有 CopyQ 能力上优化保留期限、容量和忽略应用的入口。

这些能力来自讨论建议和 Alfred 功能参照，尚未全部获得明确实施优先级。进入后续迭代时单独定义行为与验收，不扩张本次初始化范围。

## 验收标准

### 初始化验收（本轮）

- H1：`CLAUDE.md` 包含模板核心原则、原有有效约束、质量门与活跃迭代；Windows 可读取两个入口且内容共享。
- H2：`INDEX.md` 与本 SPEC 均为“待实施”，不存在把产品规划标成已完成的记录；本地文档链接有效。
- H3：原测试 session、配置/数据/状态路径、密码、日志、主题、显示会话要求全部保留；构建命令语法修正有说明。
- H4：SPEC lint 和项目 harness 校验成功，变更无空白错误；没有修改生产代码。

### 产品验收（开发阶段）

| 编号 | 用户操作与预期 | 实现位置 | 所需证据 |
|---|---|---|---|
| P1 | 在目标应用输入框呼出面板，直接输入中文/英文即筛选；空记录与无匹配有清晰状态 | `src/gui/` 面板、筛选与历史模型 | 界面级筛选断言及真实中文输入法录屏 |
| P2 | 上下键选择后 Enter 粘贴到呼出前的同一窗口与输入位置；测试同一应用的两个窗口 | 面板控制、`updateFocusWindows`、Windows 平台窗口 | 两个窗口内容前后对照；实际目标输入内容断言或截图 |
| P3 | 中文候选确认、Esc、失焦、目标窗口关闭均不把历史内容粘贴到错误位置 | 面板键盘/关闭事件、Windows 焦点链路 | 包含负向场景的界面测试和 Windows 实测 |
| P4 | 仅浏览不更改剪贴板；显式仅复制动作写入所选内容；搜索时新增历史不导致选错内容 | 模型选择、数据提取、剪贴板写入 | 隔离剪贴板内容断言与面板操作验证 |
| P5 | 从快捷面板可进入完整 CopyQ 管理窗口，原标签、编辑、脚本、插件与导入导出入口可用 | 主窗口、快捷入口与既有功能 | 有影响功能的聚焦回归；插件使用适用测试配置单独验证 |
| P6 | 按确定后的样板对照布局与交互，实际使用的缩放/显示器下文字清晰，窗口不越界 | 窗口布局、样式、屏幕定位 | 指定主题与缩放下的对照截图；外观样板缺失时不得标为通过 |

## 验证计划

### 本轮可直接执行

在 Windows 仓库根目录执行：

```powershell
pwsh -NoProfile -File utils/check-harness.ps1
git diff --check
wsl.exe -d Ubuntu --cd /mnt/d/CodeSpace/Github/CopyQ -- bash /home/alexk/data/codebase/github/astack/astack-marketplace/plugins/astack-workflow/skills/spec/scripts/spec-lint.sh docs/astack/version/Iteration1_ClipboardPalette_SPEC.md
```

预期：全部退出 0；SPEC 校验 errors=0、warnings=0。项目校验包含针对性检查模板残留、失效本地链接、SPEC/INDEX 状态不同步和测试环境配置冲突。

### 产品开发时的验证入口

- 构建：按项目入口完成依赖配置，Windows 执行 `cmake --preset Windows` 和 `cmake --build --preset Windows`。此阶段尚未验证本机 MSVC/Qt 依赖，不声明构建可用。
- 现有聚焦回归：严格完成项目入口的所有环境与 X11 会话准备后，执行 `build/copyq-tests "testCore:configPath" "testCore:searchItemsAndCopy" "testCore:keysAndFocusing"`。它们用于配置、旧搜索及焦点回归，不能代替新面板测试。
- 新面板测试与原生 Windows 隔离启动命令在实现 PLAN 中以实际新增入口填写。现阶段不存在对应程序或测试，不杜撰可执行命令或通过结果。
- P1–P6 必须在实际呈现层留下证据；测试数据写入成功不代表面板展示或原窗口粘贴已经正确。

## 验证记录

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

## 变更记录

| 日期 | 作者 | 内容 |
|---|---|---|
| 2026-09-28 | zerovoxxx | 从既有需求讨论创建首个 SPEC；区分已确认需求、技术决定和待明确项，记录源码影响面、验收映射、astack 流程与 Windows 链接适配。 |
| 2026-09-28 | zerovoxxx | 补充原始 UI 预览的构建来源与验证边界；按用户要求交付当前 harness 和需求文件，产品状态保持待实施。 |
