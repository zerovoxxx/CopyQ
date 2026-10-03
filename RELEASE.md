# QClip 构建与发布

## v1.0.0 发行内容

| 平台 | 发行文件 | 最低系统 |
|---|---|---|
| Windows x64 | qclip-1.0.0-setup.exe、qclip-1.0.0.zip | Windows 10 1903 |
| macOS Apple Silicon | QClip-1.0.0-macos-13-m1.dmg | macOS 13 |
| 对应源码 | QClip-1.0.0.tar.gz | 按源码依赖构建 |
| 校验 | checksums-sha256.txt | 校验所有上述文件 |

本次不发布 Linux 二进制。Windows 无商业代码签名；macOS 使用 ad-hoc 签名，未提供 Developer ID 或公证。自动展开初始关闭，安装和授权见 [使用说明书](docs/USER_GUIDE.md)。

## 从源码构建

代码使用 C++17、Qt 6 Widgets/Quick/QML、CMake 与 Ninja。正式发行固定 Qt 6.10.3，KDE Frameworks 6.29.0、QCA 2.3.10、QtKeychain 0.17.0；依赖下载与构建参数以对应 workflow 和 utils/github/build-*-deps.sh 为准。

```sh
git clone https://github.com/zerovoxxx/QClip.git
cd QClip
git checkout v1.0.0
```

### Windows

在 MSVC x64 开发者终端中安装 Qt 与依赖，准备 Ninja/sccache，设置 CMAKE_PREFIX_PATH，然后执行：

```sh
cmake --preset Windows
cmake --build --preset Windows
cmake --build build/copyq/Windows --target copyq-palette-ui_qmllint
```

Windows workflow 使用 utils/github/deploy-windows.sh 部署 Qt、QML、插件、通知和加密运行库，并使用 shared/qclip.iss 生成 Inno Setup 安装器。正式包包含 QML 目录；禁用 QCA/Keychain/通知/音频的本地开发包不等价于完整发行包。

### macOS

安装 Xcode 命令行工具、Qt、Ninja/ccache 和依赖。主程序与所有第三方依赖设置 MACOSX_DEPLOYMENT_TARGET=13.0。本次仅构建 Apple Silicon（M 系列），使用 macOS-13-m1：

```sh
cmake --preset macOS-13-m1
cmake --build --preset macOS-13-m1
cmake --build build/copyq/macOS-13-m1 --target copyq-palette-ui_qmllint
cd build/copyq/macOS-13-m1
cpack
```

部署脚本收集 Qt/QML 与第三方库，修复依赖路径，最后执行 ad-hoc codesign。审计会检查每个内嵌二进制的最低系统版本、内部链接和完整签名；设置主程序的最低系统版本不能代替依赖审计。

### Linux 源码

安装 Qt 6 开发组件、ICU、X11/Wayland、QCA、QtKeychain 与对应 KDE Frameworks；配置构建和测试约束见 [CLAUDE.md](CLAUDE.md)。本次发行不提供 Linux 安装包。

## 验证

发布必须对应同一个 Git 提交与版本标签。先执行 git diff --check、utils/check-harness.ps1、QML lint，并运行 Windows/macOS workflows 中明确列出的隔离聚焦测试。禁止直接执行全量套件，也不使用日常历史测试清理/迁移。

安装布局审计：

```sh
python utils/check-release.py --platform windows --root qclip-1.0.0 --source QClip-1.0.0.tar.gz
python3 utils/check-release.py --platform macos --root /Volumes/QClip-1.0.0/QClip.app --source QClip-1.0.0.tar.gz
```

平台 workflow 输出依赖/许可/对应源码审计 JSON；发布前核对 errors 为空。CI 测试不证明全部第三方应用、输入法、权限、多屏、旧主题和数据迁移已完成验收；这些边界继续登记在 [SPEC](docs/astack/INDEX.md)。

## 发布流程

发布目的地固定为 [zerovoxxx/QClip](https://github.com/zerovoxxx/QClip)。GitHub CLI 必须使用显式 --repo / -R，避免 fork 的默认仓库落到上游。

1. 更新 src/version.cmake、CHANGES.md、应用元数据和使用文档。插件 ABI 独立版本化，产品版本重置不修改已有 ABI。
2. 通过上述检查后提交代码，为目标提交创建 v1.0.0 标签并推送 master 和标签。
3. 标签触发 Windows/macOS 原生构建；也可先从 Actions 手动运行两套 workflow。只使用与发行标签完全相同提交、版本号匹配的成功运行产物。
4. 在 Git Bash 或原生 Unix shell 执行 utils/github/draft-release.sh 1.0.0；脚本下载已成功构建的资产，生成 git archive 对应源码和 SHA-256，创建草稿并上传。
5. 审阅资产、版本、哈希、许可和发行说明后，执行 gh release edit v1.0.0 --repo zerovoxxx/QClip --draft=false --latest 发布。不得将缺失平台或失败运行当作发布完成。

脚本支持复用同一个 release-1.0.0 目录继续下载。SHA-256 校验不是数字签名；没有提供签名凭据时不得声称官方代码签名或公证。

## 许可与对应源码

保留 LICENSE、AUTHORS、THIRD-PARTY-NOTICES.txt 和 licenses/ 中的组件许可，README 与 USER_GUIDE.md 也随包提供。QClip 源码以 tag 的 git archive 归档，公开链接与二进制放在同一发行页。

Qt/KDE 等组件使用各自许可和源码目的地址，见 [第三方清单](shared/THIRD-PARTY-NOTICES.txt)。部署同时收录匹配 SDK 和依赖的许可文本；不把通用许可文件当作完整第三方组件清单。
