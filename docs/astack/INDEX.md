# 迭代状态总表

> QClip 首版只设三个实施 SPEC。按 1 → 2 → 3 推进，共同满足首版完整范围。
>
> 各能力的目标、契约、验收及证据写在所属 SPEC；本索引负责导航和执行顺序。

| 迭代 | 标题 | 状态 | 文档 | 创建日期 |
|------|------|------|------|---------|
| Iteration1 | QClip 核心接入与快捷面板 | 开发中 | [SPEC](version/Iteration1_ClipboardPalette_SPEC.md) | 2026-09-28 |
| Iteration2 | QClip 全界面重做与 CopyQ 能力保留 | 开发中 | [SPEC](version/Iteration2_CopyQCompatibility_SPEC.md) | 2026-09-28 |
| Iteration3 | QClip Alfred 增强能力与三端发布 | 待实施 | [SPEC](version/Iteration3_AlfredDesktopRelease_SPEC.md) | 2026-09-28 |

用户已确认：QClip、公开开源、Windows/macOS 优先且支持 Linux、首版重做全部界面、Alfred 剪贴板和相关 Snippet 完整对齐、CopyQ 全部能力保留。技术方向为 C++17 + Qt 6 Quick/QML + Qt Quick Controls + CMake，并复用现有原生平台实现。

已完成需求分析、官方代码同步核对和三份 SPEC 拆分；2026-09-29 已实施 Iteration1 的核心/面板、插件和平台原型、隔离测试及 QML 部署入口。macOS/Linux 聚焦验证已有证据；Windows、真实中文输入法、多屏及部分应用/预览场景仍待验收，Iteration1 总状态保持开发中。阶段产物属于内部开发成果，不能替代完整首版。原总 SPEC 路径保留在 Iteration1，历史讨论/验证证据继续保存在其中。

2026-09-29 已接续实施 Iteration2 的 I2-1–I2-3：集合树/组操作、Quick 主入口、设置草稿、完整命令与编辑/辅助承载、八插件桥接、历史采集/时间/保护清理及 QSS 兼容入口均已落地。登记 49 个动作、90 项配置、24 个主表单、7 个插件表单、149 个文档 API 和八插件；最新验证及失败修复写在 SPEC2。两端构建和指定回归已有新鲜证据；macOS 截图/前台焦点、任意旧结构 QSS 的替代审阅及三端实机/安装包验收仍未完成，SPEC2 保持开发中。

## 为什么这样拆分

| 阶段 | 主要解决的问题 | 交付给下一阶段 |
|---|---|---|
| 1 核心接入与面板 | QML 如何读真实历史、正确过滤/选择、调用旧动作并恢复外部焦点；插件/输入能否迁移 | 稳定模型/动作/窗口契约、可运行面板、插件/平台原型、隔离和 QML 构建部署入口 |
| 2 全界面与兼容 | 全部旧功能在新界面如何可达，插件/编辑/脚本/主题及数据如何保持，历史策略缺口如何补齐 | 完整界面与功能映射、共用编辑/设置组件、历史策略和兼容证据 |
| 3 Alfred 增强与发布 | Snippet/展开/合并的数据及时序，品牌/迁移、实际性能和三端安装包 | 完整首版发布候选及三端行为/性能/兼容结果 |

按完整用户流程推进，每份只有 3–4 个里程碑；平台工作从第一阶段开始验证。实现某一里程碑时只修改必要模块，完成相关验证后继续。缺设备可推进独立工作，平台结果保持未通过；不把第三阶段当成第一次发现 macOS 或插件问题的时点。

## 原需求与验收归属

保留原编号，确保拆分不减少范围。跨阶段能力按明确子责任分配，具体验收只有一个所属 SPEC。

| 原需求 | 所属阶段及边界 |
|---|---|
| A1 搜索/选择/回车/仅复制 | Iteration1 |
| A2 多格式/预览/打开 | Iteration1；插件特有显示/编辑迁移由 Iteration2 承接 |
| A3 历史记录/期限/容量/忽略/清理 | Iteration2 |
| A4 Snippet 集合/编辑/浏览/导入导出 | Iteration3，复用 Iteration2 编辑组件 |
| A5 自动展开/动态内容/光标/应用例外 | Iteration3；平台可行性原型前置到 Iteration1 |
| A6 连续复制合并 | Iteration3 |
| A7 外观/位置/焦点/跨端交互 | Iteration1 窗口与面板；Iteration2 全局样式/主题；Iteration3 最终平台验收 |
| C1 标签/管理/编辑/批量操作 | Iteration2 |
| C2 脚本/CLI/命令/快捷键/托盘 | Iteration2 保留行为；Iteration1 不破坏动作链路；Iteration3 验证品牌/CLI/session 迁移 |
| C3 八个内置插件与插件能力 | Iteration2，关键原型前置到 Iteration1 |
| C4 主题/配置/持久化/旧数据 | Iteration2 主题和旧格式兼容；Iteration3 品牌路径/迁移与安装包验证 |

| 原验收编号 | 唯一所属 SPEC |
|---|---|
| P1、P2、P3、P4、P8 | Iteration1 |
| P5、P6、P7、P12 | Iteration2 |
| P9、P10、P11、P13、P14 | Iteration3 |

## GPT-6 Sol xhigh 执行入口

建议执行配置为 **GPT-6 Sol / xhigh**。每份 SPEC 已包含实施顺序，不必为了开工再增加一套相同 PLAN；复杂局部任务确实需要 PLAN 时只引用所属 SPEC 的契约。

首次开发可以直接使用以下任务说明：

~~~text
使用 GPT-6 Sol，推理强度 xhigh，在 D:/CodeSpace/Github/CopyQ 实施 QClip。
先读取 CLAUDE.md、docs/astack/INDEX.md 和
docs/astack/version/Iteration1_ClipboardPalette_SPEC.md。
从 I1-1 开始，按本 SPEC 里程碑依次推进；Iteration2/3 本次先读取依赖边界，
不同时开始全面 UI 迁移或 Snippet 业务。
核对当前 Git 状态和 SPEC 指向的代码，保留未提交工作。
每段先列出实际修改路径、将复用的接口、行为契约和相关验证，再实施。
复用 CopyQ 模型、配置、命令/脚本、插件和原生粘贴，不保存第二套历史。
新增命名先搜索现有实现；模型 final、筛选隐藏行、QWidget 插件和焦点耦合
按 SPEC 处理，不能假设 QML 可以直接替换。
测试使用隔离 session/配置/历史和指定函数；满足显示会话要求后才运行。
完成一段便执行相关验证，把结果、实际命令、接口和剩余项写回当前 SPEC，
然后继续下一段。仅文档检查通过不表示产品通过。
缺少原生平台环境时说明具体未验证项并继续独立工作。
存在必须改变用户范围的障碍时，先形成具体可审阅方案，再说明取舍。
最终报告完成的里程碑、行为变化、新鲜验证和剩余边界。
~~~

后续执行 Iteration2 或 Iteration3 时，将目标文件和起始里程碑改为对应值，并读取前一阶段的交接结果；公共架构仍以 Iteration1 为准。状态只在实际开工/完成时更新，不能因 SPEC 编写完毕而改成产品“已完成”。

## 文档质量门

从仓库根目录执行，三项均须退出 0，lint 必须检查三份 SPEC：

~~~powershell
pwsh -NoProfile -File utils/check-harness.ps1
git diff --check
wsl.exe -d Ubuntu --cd /mnt/d/CodeSpace/Github/CopyQ -- bash /home/alexk/data/codebase/github/astack/astack-marketplace/plugins/astack-workflow/skills/spec/scripts/spec-lint.sh docs/astack/version
~~~

## 变更记录

| 日期 | 版本 | 作者 | 摘要 |
|------|------|------|------|
| 2026-09-28 | 初始化 / Iteration1 | zerovoxxx | 按本机 astack harness-init 迁移治理入口，登记已确认需求、Qt Widgets 方案、验收场景及后续候选；Windows 使用 NTFS 硬链接适配，保留上游测试隔离规则。 |
| 2026-09-28 | 初始化交付 | zerovoxxx | 补充原始 UI 预览来源与验证边界，按用户要求提交当前治理文件；产品功能仍待实施，SPEC 归档检查为 no-op。 |
| 2026-09-28 | 产品方向与上游同步 | zerovoxxx | 确定 QClip、开源、三端优先级、首版全界面重做、Alfred 剪贴板完整对齐与 CopyQ 全能力保留；推荐 Qt Quick/QML，更新能力/验收矩阵。添加 upstream 并核对已包含官方最新 master `1cddb851`，保留 harness 提交，产品状态仍待实施。 |
| 2026-09-28 | 三阶段实施拆分 | zerovoxxx | 保留原 SPEC 路径并收敛公共架构/面板阶段，新增全界面兼容、Alfred 增强与三端发布两份 SPEC；A1–A7/C1–C4/P1–P14 完整映射，补充 GPT-6 Sol xhigh 执行入口。 |
| 2026-09-29 | Iteration1 开发 | zerovoxxx | 实施快捷面板与核心动作链路，建立插件/原生输入原型、三端 QML 构建部署和隔离聚焦测试；证据及后续接口集中于 SPEC1，未验证的平台门保持未通过。 |
