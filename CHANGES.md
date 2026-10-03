# 1.0.0

QClip 首个独立开源版本，基于 CopyQ 16 的剪贴板引擎与插件体系。

- 统一 QClip 项目、应用、安装器、帮助入口和发布资产品牌；使用独立配置、会话和应用 ID。
- Qt Quick 快捷面板与管理窗口：搜索、预览、复制/粘贴、集合树、批量操作和统一外观。
- 持久 Snippet、富文本、集合交换、动态模板、关键词自动展开与连续复制合并。
- 保留命令行/脚本、标签/备注/固定、磁盘同步、加密及八个内置插件。
- 重写 README，提供完整中文使用说明，包内保留 CopyQ 作者、GPL 与第三方许可。
- 发布 Windows x64 安装器/便携包及 macOS Apple Silicon（M 系列）DMG，附对应源码与 SHA-256。

自动展开默认关闭。Windows 包无商业代码签名，macOS 为 ad-hoc 签名且未经 Developer ID 公证。输入法、安全输入、权限、多屏 DPI、复杂旧配置/主题迁移仍存在未完成的实机验收；详见 docs/USER_GUIDE.md 和项目 SPEC。

CopyQ 原始版本历史保留在 [docs/UPSTREAM-CHANGES.md](docs/UPSTREAM-CHANGES.md)。
