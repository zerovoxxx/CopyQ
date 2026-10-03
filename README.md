# QClip

**把复制过的内容，变成随时可用的工具。**

QClip 是面向 Windows 和 macOS 的开源剪贴板管理器。它基于 [CopyQ](https://github.com/hluk/CopyQ) 的历史记录、插件和脚本能力，重新设计桌面界面，提供快捷搜索面板、集合管理、持久文本片段（Snippet）、动态模板和可选的关键词自动展开。

QClip is an open-source clipboard manager built on CopyQ, with a Qt Quick interface, searchable history, collections, snippets and dynamic templates.

[下载 v1.0.0](https://github.com/zerovoxxx/QClip/releases/tag/v1.0.0) · [完整使用说明](docs/USER_GUIDE.md) · [报告问题](https://github.com/zerovoxxx/QClip/issues) · [构建与发布](RELEASE.md)

## 能做什么

- **找回复制过的内容**：保存文本、HTML、图片、文件链接等剪贴板格式，通过搜索和预览快速定位。
- **用键盘完成操作**：呼出快捷面板、输入关键词、选择条目并粘贴；支持仅复制、纯文本粘贴和自定义动作。
- **整理常用资料**：用集合、分组、标签、备注和固定条目管理历史，批量编辑、移动、导入和导出。
- **保存常用回复**：Snippet 独立于普通历史保存，支持集合、富文本、关键词、动态占位符和光标位置。
- **减少重复输入**：显式启用关键词自动展开或连续两次复制合并；可设置应用例外和合并分隔符。
- **按自己的方式工作**：保留 CopyQ 的命令行、脚本、快捷键、主题和八个内置插件，包括图片、标签、固定、同步和加密。

数据保存在本机。自定义命令和同步插件可以访问网络或外部文件，行为取决于你启用的配置。

## 下载与安装

从 [GitHub Releases](https://github.com/zerovoxxx/QClip/releases) 下载适合设备的文件。

| 系统 | 文件 | 安装方式 |
|---|---|---|
| Windows 10 1903 / Windows 11，x64 | `qclip-1.0.0-setup.exe` | 运行安装器，按提示安装 |
| Windows，x64 | `qclip-1.0.0.zip` | 完整解压到可写目录，运行 `qclip.exe` |
| macOS 13+，Intel | `QClip-1.0.0-macos-13.dmg` | 打开 DMG，将 QClip 拖入 Applications |
| macOS 13+，Apple Silicon | `QClip-1.0.0-macos-13-m1.dmg` | 打开 DMG，将 QClip 拖入 Applications |

macOS 可在 **Apple menu → About This Mac** 查看芯片类型。当前发行包未提供 Windows 商业代码签名或 Apple Developer ID 公证；首次启动可能需要按系统提示确认来源。macOS 的自动粘贴和后台展开需要相关系统权限，详见[安装说明](docs/USER_GUIDE.md#安装与首次启动)。

本次发行提供 Windows/macOS 二进制。Linux 源码支持仍保留，可参考[源码构建说明](RELEASE.md#从源码构建)。

## 第一次使用

1. 启动 QClip，保持托盘或菜单栏中的程序运行。
2. 在其他应用里复制一段普通文字，再复制另一段文字。
3. 打开 QClip，通过 **Manage** 进入管理窗口；快捷面板也可用 `qclip palette` 呼出。
4. 输入关键词搜索，使用 ↑ / ↓ 选择条目，按 Enter 粘贴到先前的窗口；点击 **Copy** 则只写入剪贴板。
5. 在 **Settings… → Shortcuts…** 配置适合自己的全局快捷键。需要直接呼出快捷面板时，可创建命令 `copyq: palette()` 并设置全局快捷键。

`copyq:` 是兼容的脚本解释器前缀。实际产品与可执行文件名为 QClip / `qclip`。

常用回复可以从选中历史条目的 **Save snippet** 保存，再在 **Snippets…** 编辑。自动展开初始关闭，应在普通文本编辑器中验证关键词、权限和应用例外后启用。

## 命令行与自动化

Windows 在程序目录使用 `./qclip.exe`；macOS 使用 `/Applications/QClip.app/Contents/MacOS/QClip`。以下示例以加入 PATH 的 `qclip` 为例，大部分命令需要主程序已经运行。

```sh
qclip --help
qclip palette
qclip show
qclip add -- '一段常用文字'
qclip read 0
qclip clipboard
qclip snippets
```

完整操作与模板示例见[使用说明书](docs/USER_GUIDE.md)。兼容脚本接口参考仓库中的[脚本 API](docs/scripting-api.rst)，以及明确标注为上游参考的 [CopyQ API 文档](https://copyq.readthedocs.io/en/latest/scripting-api.html)。

## 当前边界

自动展开会在无法确认的输入法组合输入、安全输入或缺少权限时暂停。不同应用的粘贴、管理员权限窗口、多屏 DPI、旧主题样式和复杂 CopyQ 配置迁移仍存在平台差异；详见[说明书中的限制](docs/USER_GUIDE.md#已知限制)。QClip 不提供 Alfred Workflow 编辑器，也不承诺直接导入 `.alfredsnippets`。

构建、隔离聚焦测试和安装包依赖审计的执行方式见 [RELEASE.md](RELEASE.md)。完整产品场景的实施与验收记录见[项目 SPEC](docs/astack/INDEX.md)。

## 开发与贡献

技术栈：C++17、Qt 6 Widgets / Quick / QML、CMake。Widgets 仍承载部分插件及辅助编辑界面，Qt Quick 承载快捷面板和主要管理界面。

提交问题时请附上系统版本、芯片/架构、QClip 版本、复现步骤和去除个人数据的日志。开发前阅读 [CLAUDE.md](CLAUDE.md) 和[迭代索引](docs/astack/INDEX.md)，并使用隔离配置验证剪贴板行为。

## 许可与致谢

QClip 是 CopyQ 的修改版本，采用 **GNU GPL v3 或更新版本（GPL-3.0-or-later）**。完整协议见 [LICENSE](LICENSE)。原有版权声明、[AUTHORS](AUTHORS)、源码中的 SPDX 标识与第三方许可继续保留；发布包附带许可文本，并提供与版本标签对应的完整项目源码。

- **CopyQ**：由 Lukáš Holeček 和贡献者开发，提供 QClip 的剪贴板引擎、历史存储、脚本与插件基础。[上游仓库](https://github.com/hluk/CopyQ) · [上游作者](AUTHORS) · [保留的上游变更记录](docs/UPSTREAM-CHANGES.md)。
- **QClip**：由 zerovoxxx 和 QClip 贡献者维护本分支的界面与增强功能；问题反馈请提交到 [QClip](https://github.com/zerovoxxx/QClip/issues)。
- **第三方组件**：Qt、KDE Frameworks、QCA、QtKeychain、OpenSSL、ICU、LibQxt、FakeVim、Font Awesome 和 miniaudio 等，遵循各自许可。清单与文本见 [THIRD-PARTY-NOTICES](shared/THIRD-PARTY-NOTICES.txt) 和 [shared/licenses](shared/licenses)。

Alfred 是交互设计的参考产品；QClip 与 Alfred 无隶属关系。
