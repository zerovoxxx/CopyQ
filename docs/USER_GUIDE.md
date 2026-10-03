# QClip 使用说明书

适用版本：**QClip 1.0.0**。覆盖 Windows 和 macOS 的安装、日常使用、文本片段、自动化与数据维护。部分界面使用英文，下面同时给出英文控件名称，便于定位。

## 目录

- [安装与首次启动](#安装与首次启动)
- [五分钟上手](#五分钟上手)
- [快捷面板](#快捷面板)
- [管理窗口与集合](#管理窗口与集合)
- [历史记录与隐私](#历史记录与隐私)
- [Snippet 文本片段](#snippet-文本片段)
- [动态模板](#动态模板)
- [关键词自动展开](#关键词自动展开)
- [连续复制合并](#连续复制合并)
- [命令与快捷键](#命令与快捷键)
- [插件与外观](#插件与外观)
- [备份与恢复](#备份与恢复)
- [从 CopyQ 迁移](#从-copyq-迁移)
- [命令行](#命令行)
- [升级退出与卸载](#升级退出与卸载)
- [故障排查](#故障排查)
- [已知限制](#已知限制)

## 安装与首次启动

### Windows

要求 Windows 10 1903（build 18362）或更新版本，64 位 x64 系统。

1. 打开 [QClip Releases](https://github.com/zerovoxxx/QClip/releases)，下载 `qclip-1.0.0-setup.exe`。
2. 运行安装器，选择安装目录、插件、翻译以及是否创建桌面快捷方式/开机启动。
3. 从开始菜单打开 **QClip**。关闭管理窗口后，程序可继续在托盘运行。
4. 若 Windows 提示来源未知，先确认来自本仓库，并与发行页的 SHA-256 校验文件核对；当前版本没有商业代码签名。

便携版：完整解压 `qclip-1.0.0.zip` 到可写目录，然后运行 `qclip.exe`。不要从压缩包内直接启动，也不要只复制 EXE；`qml`、`plugins`、`platforms` 和 DLL 是运行依赖。可写的便携目录会自动创建 `config` 与 `logs` 子目录，数据随程序目录保存。

校验下载文件（PowerShell）：

```powershell
Get-FileHash ./qclip-1.0.0-setup.exe -Algorithm SHA256
```

### macOS

要求 macOS 13 或更新版本，芯片为 Apple M 系列（Apple Silicon）。在 **Apple menu → About This Mac** 中确认芯片，下载 `macos-13-m1.dmg`；M1、M2 及后续 M 系列使用同一 arm64 版本。本次不提供 Intel 安装包。

1. 下载对应 DMG 并打开。
2. 把 **QClip.app** 拖入 **Applications**，再从 Applications 启动；不要长期从 DMG 直接运行。
3. 如果系统拦截未公证应用，在核对下载来源后，按系统支持的 **System Settings → Privacy & Security → Open Anyway** 流程打开。当前包采用本地 ad-hoc 签名，未通过 Apple Developer ID 公证。
4. 自动粘贴与后台输入功能需要权限。在 **System Settings → Privacy & Security → Accessibility** 中允许 QClip；使用自动展开/复制合并时，同时检查 **Input Monitoring**。授权后完全退出并重新启动 QClip。
5. 移动或替换应用后若权限失效，在上述列表重新添加位于 Applications 的 QClip。

校验文件（Terminal）：

```sh
shasum -a 256 QClip-1.0.0-macos-13-m1.dmg
```

### 认识四个入口

| 入口 | 用途 |
|---|---|
| 托盘 / 菜单栏 | 常驻运行、打开窗口、暂停记录、退出 |
| 快捷面板 | 临时搜索、预览、复制或粘贴历史内容 |
| 管理窗口（Manage） | 集合、批量操作、编辑、设置与数据管理 |
| Snippets… | 保存和编辑长期使用的模板及展开设置 |

首次启动只看到托盘图标时，从托盘打开窗口，或执行 `qclip show` / `qclip palette`。请按[命令与快捷键](#命令与快捷键)配置自己的全局呼出快捷键。

## 五分钟上手

1. 在普通文本编辑器中复制 `第一次使用 QClip`，再复制 `第二条内容`。
2. 打开 QClip，历史里应出现两条记录。若没有，检查是否暂停记录或复制了被忽略/标记敏感的内容。
3. 在目标文本框中放好光标，然后用全局快捷键或 `qclip palette` 呼出快捷面板。
4. 输入 `第一次`，按 ↑ / ↓ 选择，查看右侧预览。
5. 按 Enter 粘贴到原窗口；如只想复制，点击 **Copy**，再回目标应用手动粘贴。
6. 点击 **Manage**，新建常用集合，把需要保留的条目移入集合或保存成 Snippet。

普通历史遵循容量、期限与清理设置。长期模板应保存为 Snippet。关闭窗口不停止后台记录，停止记录请使用 **Pause recording**。

## 快捷面板

顶部下拉列表切换集合，搜索框过滤该集合的记录。左侧是结果，右侧预览文本、图片、链接或其他可识别格式。

| 操作 | Windows | macOS |
|---|---|---|
| 上下选择 | ↑ / ↓ | ↑ / ↓ |
| 执行默认动作 | Enter | Return |
| 仅复制 | Ctrl+Enter | ⌘+Return |
| 纯文本粘贴 | Shift+Enter | ⇧+Return |
| 绕过默认命令直接粘贴 | Alt+Enter | ⌥+Return |
| 关闭面板 | Esc | Esc |

默认动作通常为粘贴；如果配置了 Enter 命令，底部按钮会显示对应命令名称。此时可用 Alt/Option+Enter 直接粘贴。中文输入法处于组合输入时，先确认候选，再操作记录。

**Open** 打开受支持链接；**Preview file** 预览仍存在的文件；**Edit…** 编辑条目。剪贴板里的文件链接不是完整文件备份，原文件移动或删除后链接可能失效。

**Actions…** 显示适用于选中条目的命令。顶部 **More options → Save snippet** 将选中记录复制到独立模板存储；**Snippets…** 打开模板管理。找不到原粘贴窗口时，用 **Copy** 后手动粘贴。

## 管理窗口与集合

通过 **Manage** 或 `qclip show` 打开。左侧是集合树，中间为条目列表，右侧为预览；顶部可暂停记录和打开设置。

### 建立集合与分组

1. 左侧点击 **New**，输入名称并创建集合。
2. 用 `工作/常用回复` 这样的路径组织分组，组下可放多个集合。
3. 左侧 **More…** 提供重命名、组内新建、属性、图标、移动、排序和删除。
4. **Properties…** 调整集合属性；先确认选中的是集合还是分组。

### 处理记录

点击条目查看内容；Ctrl（macOS 上 ⌘）或 Shift 配合选择用于多选，也可点击 **Select all**。使用 **Copy**、**Edit…**、**Delete** 或 **Actions…**。操作菜单提供移动/复制到集合、标签、备注、固定等功能；拖放效果取决于目标与插件。

编辑文本后保存，原记录随之更新。图片、富文本和其他格式使用适用的插件编辑器。固定条目用于保护重要内容，但不能替代备份。删除集合、批量删除或清理前先导出重要数据。

## 历史记录与隐私

在 **Settings… → History** 调整数量/保存时间、内容过滤与应用忽略。配置以界面说明为准；设置窗口使用草稿，**Apply / OK** 保存，**Cancel** 放弃未保存更改。

**Pause recording / Resume recording** 控制后续采集，不清空已有历史。**More options → Clear history…** 可按时间范围清理；检查是否包含受保护或时间未知条目后再确认。

默认历史保存在本机，未启用加密时以未加密数据存储。密码管理器等提供的敏感剪贴板标记通常会被忽略，但并非所有应用都会设置标记。复制机密资料时可以暂停记录，或配置应用/内容过滤。

加密与密码保护见[密码保护说明](password-protection.rst)。不要丢失密码；同步目录、显式导出、截图和日志有独立的隐私边界。

## Snippet 文本片段

Snippet 用于签名、邮箱地址、固定回复、文案与代码模板。它有独立存储，普通历史清理不会删除它。

### 新建与使用

1. 打开 **Snippets…**，点击 **New collection…** 建立例如 `常用回复` 的集合。
2. 选中集合，点击 **New snippet**。
3. 填写标题（Title）、关键词（Keyword），用 **Save details** 保存字段，再用 **Edit body…** 编辑正文并保存。
4. 按需启用单条模板的展开选项。标题用于查找，关键词用于匹配。
5. 选择模板，在右侧检查输出预览；**Copy** 仅复制，**Paste to original window** 粘贴到原目标。

历史 **Save snippet** 保存的是内容副本；以后编辑/删除历史不修改或删除模板。

### 集合和关键词

双击集合可修改名称、Keyword prefix、Keyword suffix 和启用状态。有效关键词是 **集合前缀 + 条目关键词 + 集合后缀**。例如前缀 `;;` 加上条目关键词 `mail`，得到 `;;mail`。

关键词区分大小写。重复有效关键词显示冲突并停止自动展开/引用；消除冲突后再使用。没有关键词的模板仍可手动复制或粘贴。

### 导入与导出

选中集合后 **Export collection…** 导出 `.qcs`；**Import collection…** 导入。集合导出是明文，包含模板正文，不使用整体备份密码保护。来源或格式不合法时不要导入。

QClip 的 `.qcs` 与 Alfred 的 `.alfredsnippets` 不同，当前不承诺直接互通。

## 动态模板

占位符写在正文中，每次使用时生成新值，原模板不变。先在预览中检查结果。

| 写法 | 用途 |
|---|---|
| `{date}` / `{time}` / `{datetime}` | 当前日期、时间或两者，按地区设置格式化 |
| `{isodate}` | ISO 日期，例如 `2026-10-03` |
| `{date:yyyy-MM-dd}` | 自定义日期格式 |
| `{date +1D:yyyy-MM-dd}` | 明天的日期 |
| `{clipboard}` | 当前剪贴板的纯文本 |
| `{clipboard:0}` | 历史中的第一条文本，索引从 0 开始 |
| `{clipboard.uppercase}` | 剪贴板文本转大写 |
| `{clipboard.trim.lowercase}` | 去掉两端空白，再转小写 |
| `{random:1..100}` | 范围内的随机整数 |
| `{random:早上好,下午好,晚上好}` | 从候选中选择一个 |
| `{random:UUID}` | 生成 UUID |
| `{cursor}` | 输出后的光标位置，模板内只放一个 |
| `{snippet:;;sig}` | 引用有效关键词为 `;;sig` 的模板 |

日期运算单位：`Y` 年、`M` 月、`D/d` 日、`h` 小时、`m` 分钟、`s` 秒。引用只展开一层。未知占位符保留原文，已知占位符写法不合法会报错。

```text
您好，

已收到您的来信，我们会在 {date +1D:yyyy-MM-dd} 前回复。
{cursor}

此致
客服团队
```

文本修饰支持 `uppercase`、`lowercase`、`trim`、`capitals` / `capitalcase`、`reverse`、`stripdiacritics`、`stripnonalphanumeric`。历史/引用文本必须存在。富文本会保留格式，实际粘贴效果也取决于接收应用。

## 关键词自动展开

初始关闭。开启后监听受支持的键盘输入，匹配有效关键词并替换为正文。

1. 创建简单模板，关键词 `;;hello`，正文 `您好！`。
2. 确认集合和单条模板允许展开、关键词无冲突。
3. 在 Snippets 底部启用 **Automatically expand**。
4. macOS 完成 Accessibility / Input Monitoring 授权，检查底部监听状态。
5. 在普通文本编辑器中切换英文直接输入，键入有效关键词并验证替换。

**Match anywhere** 允许在单词内匹配；默认检查词边界。**Restore clipboard** 在展开后尝试恢复先前剪贴板；其间复制了新内容时，会避免覆盖新剪贴板。

**Excluded application IDs** 用分号分隔应用标识，用于禁用相应应用的展开/合并。标识不是窗口标题：macOS 一般为 bundle ID，Windows 由平台接口提供进程路径，请根据实际身份配置。

组合输入、安全输入、无权限、目标改变或键盘状态不明确会暂停/取消展开。正文可包含中文，但不等于所有中文输入法触发流程均已验证。遇到问题用手动 Copy/Paste，或关闭 Automatically expand。

Windows 自动展开请切换到普通键盘布局（例如英语 US）；中文、日文、韩文输入 locale 及带 IMM 文件的输入法会保守暂停，输入法中的英文模式也可能暂停。手动复制和粘贴仍可使用中文正文。

## 连续复制合并

在 Snippets 底部启用 **Merge double copy**。先复制一段文字，再选择下一段，按住 Ctrl（macOS 为 ⌘）快速连续按两次 C；程序尝试合并前后两段。

**Merge separator** 可选 Newline（换行）、Space（空格）、None（无分隔）；**Sound** 控制反馈音。双按检测窗口约 400ms。不需要开启自动展开，但同样依赖后台输入权限。

仅文本适用。非文本、被忽略应用、敏感内容或状态不明确时不合并。先在普通编辑器中练习并检查剪贴板结果。

## 命令与快捷键

**Settings… → Shortcuts…** 调整操作键位。命令管理保留快捷键、自动命令、显示命令与脚本，可从管理窗口操作菜单进入（默认 F6，macOS 可能需要 Fn）。

设置全局快捷面板键：

1. 新建命令，命名为 `打开 QClip 快捷面板`。
2. Command 填写 `copyq: palette()`。
3. 在 Global Shortcut 录入组合，例如 Windows 的 Ctrl+Alt+V、macOS 的 ⌘+⇧+V。
4. 保存/应用后，在其他应用中测试；与其他程序冲突时换一个组合。

这些快捷键是自选示例。`copyq:` 是兼容脚本解释器前缀，不改成 `qclip:`。

命令可访问文件、网络、剪贴板或其他程序，导入社区命令前检查代码。Enter 默认命令会改变面板的 Enter 行为，不确定时用明确的 Copy 按钮。

## 插件与外观

通过 **Settings… → Plugins** 或 **Plugin settings…** 配置：

| 插件 | 用途 |
|---|---|
| Text | 文本显示与高亮 |
| Images | 图片预览和导出 |
| Notes | 条目备注 |
| Tags | 标签与搜索 |
| Pinned Items | 固定重要条目 |
| Synchronize | 与磁盘文件/目录同步 |
| Encryption | 加密相关功能 |
| FakeVim | 类 Vim 编辑操作 |

同步是磁盘文件同步，不是云账户服务。使用同步/加密前检查外部文件、密码与导出格式。

Encryption 插件的条目加密需要另行安装 [GnuPG](https://gnupg.org/download/) 并让 `gpg` 或 `gpg2` 可从 PATH 找到；QClip 不附带 GnuPG。安装后重启 QClip，先在临时集合中验证密钥、加密和解密，并保管密钥备份。它与设置中的整个存储密码、Snippet 存储加密是不同的功能。

**Settings… → Appearance…** 调整外观，Layout 调整布局。新 Qt Quick 控件不能完整复刻任意旧 Widgets QSS；升级旧主题后逐项检查，必要时恢复默认主题。

## 备份与恢复

1. 从管理窗口的操作菜单使用整体导出，选择历史、设置和命令等内容。
2. 确认包含 Snippet 和迁移资产的整体备份成功写出，另存到其他磁盘。
3. 密码保护下整体备份遵循加密导出路径；`.qcs` 集合导出仍是明文。
4. 恢复前先备份当前配置，通过导入选择备份并检查导入范围。
5. 核对集合、Snippet、命令与外部文件后再继续工作。

操作菜单中的 **Export / Import** 名称随语言而异，详情见[备份参考](backup.rst)。不要在运行时直接覆盖数据文件；原文件链接与同步目录应独立备份。

## 从 CopyQ 迁移

QClip 使用独立配置、会话和 IPC，不自动接管原配置。

1. 退出 CopyQ，完整复制配置/历史/关联文件到隔离目录，保留原目录。
2. 备份 QClip 当前数据。
3. **Snippets… → Import CopyQ profile…** 选择隔离副本，其中应只有一个待导入配置并包含历史资产。
4. 加密源需要先启用并解锁 QClip 加密，再输入原 CopyQ 密码。
5. 成功后核对历史、集合、命令及外部路径。受支持资产复制到 QClip 独立目录，源保留。

目录外引用、缺失文件或多个候选配置可能导致拒绝导入。复杂插件私有配置、外部脚本和同步路径需要人工核对，确认完整前不要删除原数据。

## 命令行

大部分命令需要主程序运行。Windows 在 EXE 目录打开 PowerShell；macOS 使用 Terminal。

```powershell
# Windows
./qclip.exe --help
./qclip.exe palette
./qclip.exe show
./qclip.exe snippets
./qclip.exe add -- '常用文字'
./qclip.exe read 0
./qclip.exe clipboard
```

```sh
# macOS
/Applications/QClip.app/Contents/MacOS/QClip --help
/Applications/QClip.app/Contents/MacOS/QClip palette
```

加入 PATH 后可直接用 `qclip`（macOS 可自行设置别名）。`add --` 的 `--` 让参数按原文处理，否则 `\n` 等可能解释为转义。`read 0` 是最新记录。

`--session NAME` 使用独立会话。`COPYQ_*` 环境变量、脚本 API、MIME 保留兼容原名。开发/测试用仓库隔离脚本，避免操作日常历史。

参考[本仓库 API](scripting-api.rst)和[上游 CopyQ API](https://copyq.readthedocs.io/en/latest/scripting-api.html)。上游下载/界面示例可能不同，QClip 安装和新界面以本说明书为准。

## 升级退出与卸载

升级前导出整体备份并退出 QClip。Windows 便携升级保留自己的 `config`，替换运行文件；macOS 替换 Applications 中的应用后检查权限。

从托盘/菜单栏选择 **Exit** 或执行 `qclip exit` 完全退出；仅关闭窗口通常继续记录。

Windows 安装版在 **Settings → Apps → Installed apps → QClip → Uninstall** 卸载；macOS 退出后移除 Applications 中的 QClip。配置/历史可能保留，备份后再手动删除。便携配置位于便携目录，删除整个目录会同时删除这些数据。

## 故障排查

| 现象 | 优先检查 |
|---|---|
| 无新增历史 | 是否运行、Pause recording、忽略规则、敏感剪贴板标记 |
| 找不到旧记录 | 集合/搜索、不同会话、便携 config、历史清理 |
| 快捷键无效 | Global Shortcut 是否保存、键位冲突、macOS 权限 |
| 只能复制不能粘贴 | 原窗口、Accessibility、Windows 目标是否为管理员进程 |
| 自动展开无效 | 总开关/集合/条目开关、关键词冲突、Input Monitoring、输入法/安全输入 |
| 新界面启动失败 | 包是否完整、qml/DLL 是否缺失、运行文件是否混用版本 |
| 图片/插件失效 | 是否安装插件、插件目录、数据格式 |
| 迁移失败 | 隔离副本完整性、配置数量、加密密码、目录外文件 |
| macOS 阻止启动 | 架构/系统版本、来源/哈希、Privacy & Security 的提示 |

普通权限的 Windows 程序可能无法向管理员窗口注入粘贴，优先用 Copy 后手动粘贴。

向 [QClip Issues](https://github.com/zerovoxxx/QClip/issues) 提供系统、架构、版本与最小复现。关于页的 Diagnostics 提供构建信息；日志为 `qclip logs`。先去除账号、剪贴板内容和个人路径，不上传完整历史数据库。

## 已知限制

- Windows 包为 x64，不提供原生 ARM64 包。
- macOS 本次仅提供 Apple Silicon（M 系列），采用 ad-hoc 签名，没有 Developer ID 公证。
- 自动展开/合并依赖输入监控；输入法、安全输入、权限、多屏 DPI 与第三方应用场景仍需实机验证。
- 不提供 Alfred Workflow 编辑器，不承诺 `.alfredsnippets` 互通。
- 任意旧 QSS、复杂加密 CopyQ 配置和插件外部路径不能保证完全互通。
- Linux 源码继续维护，本次无 Linux 二进制；Wayland 不支持新增的全局输入监听/展开。
- CI 聚焦测试和打包审计不代表所有输入法、应用和设备均通过完整产品验收。

QClip 基于 CopyQ，采用 [GPL-3.0-or-later](../LICENSE)。作者与第三方信息见 [README](../README.md#许可与致谢)、[AUTHORS](../AUTHORS) 和 [THIRD-PARTY-NOTICES](../shared/THIRD-PARTY-NOTICES.txt)。
