# QClip — 跨平台剪贴板体验重构

> 本文件是项目的导航入口。详细方案以 `docs/astack/version/*_SPEC.md` 为准。

## 1. 项目定位

QClip 基于官方 CopyQ 二次深度开发并开源。首版重做全部界面，剪贴板交互与能力完整对标 Alfred，深度整合 CopyQ 底层并保留所有现有能力。持续支持 Windows、macOS、Linux 三端桌面，Windows/macOS 优先，Linux 次之。详细范围、技术取舍及未验证边界见活跃 SPEC。

## 2. 核心开发原则

### 1. 🚀 编码哲学 (Karpathy-Inspired Coding Guidelines)

> "Tradeoff: These guidelines bias toward caution over speed. For trivial tasks, use judgment."

#### 1. Think Before Coding
- **State your assumptions explicitly.** If uncertain, ask.
- **Don't assume. Don't hide confusion. Surface tradeoffs.**
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- **If something is unclear, stop. Name what's confusing. Ask.**

#### 2. Simplicity First
- **Minimum code that solves the problem. Nothing speculative.**
- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- **If you write 200 lines and it could be 50, rewrite it.**

#### 3. Surgical Edits
- **Touch only what you must. Clean up only your own mess.**
- Avoid "while I'm here" refactoring of unrelated code.
- Keep diffs small and focused.
- If a change requires touching 10 files, explain why before doing it.
- Don't reformat code unless it's the primary task.

#### 4. Goal-Driven Execution
- **Define success criteria. Loop until verified.**
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make them pass"
- "Refactor X" → "Ensure tests pass before and after"
- **Validation is the only path to finality.**

### 2. Harness 核心准则

1. Spec 驱动：有明确行为变化时，先写或更新对应 SPEC。
2. 单一权威：目标、边界、设计、验收和重要结论优先写在 SPEC 本文。
3. 影响面可解释：跨模块、接口、数据流或运行流程变更必须先写清涉及文件和原因。
4. 机械校验优先：能用 lint / test / script 检查的规则，不靠人工记忆。
5. 文档维护：默认不维护 sidecar 文档，除非用户明确要求专项报告。

## 3. 扩展原则

### 3.1 项目与改造约束

- 现有代码事实：C++17、Qt 6 Widgets、CMake；Windows 构建入口见 `CMakePresets.json` 的 `Windows` preset 和 `.github/workflows/build-windows.yml`。不得套用 astack 应用自身的 Node/pnpm 构建命令。
- 用户目标：QClip 开源、三端桌面支持、Windows/macOS 优先；首版重做全部界面，剪贴板交互与能力完整对标 Alfred，保留 CopyQ 所有能力。预览、Snippet、自动展开和连续复制合并已升级为正式需求。已确认与待确认事项见活跃 SPEC。
- 目标技术方案：C++17 + Qt 6 Quick/QML + Qt Quick Controls + CMake，在现有 Qt 服务进程内重做界面，复用历史、插件、脚本和三端平台实现；Widgets 依赖按功能迁移。插件呈现、Quick 窗口焦点与 QML 打包需先验证，不能以保留旧管理窗口代替全界面验收。
- 测试使用隔离 session、配置、状态和数据路径；运行应用或测试前必须满足下文环境与显示会话约束。不得用日常剪贴板历史验证删除、清空或迁移。
- 仅运行与变更相关的指定测试，不运行全量测试。文档初始化只验证文档和入口关系，不启动剪贴板监控或粘贴。
- 用户界面验收要覆盖真实窗口、输入法和粘贴结果。WSL/Xvfb 结果不等价于 Windows 前台窗口粘贴结果；未验证的边界必须写入 SPEC。
- 发布依据 `RELEASE.md`、`shared/copyq.iss` 和现有 CI；本机初始化不代表已发布。提交、推送、安装与实际 UI 验证分别记录证据。

### 3.2 项目质量门

从仓库根目录执行。环境准备与具体命令见下文，不以另一技术生态的默认命令代替。

| 质量门 | 项目命令与预期 |
|---|---|
| 文档 / harness | `pwsh -NoProfile -File utils/check-harness.ps1` 与 `git diff --check`，均退出 0；检查入口、链接、SPEC/INDEX 状态、模板残留和测试环境配置 |
| SPEC 创建或更新 | 使用本机 astack 的 `spec/scripts/spec-lint.sh` 检查 `docs/astack/version/`，errors=0、warnings=0；完整命令见活跃 SPEC |
| Linux 编译 | 完成下文 CMake 配置后运行 `cmake --build build`，退出 0；环境依赖未就绪时据实记录 |
| Windows 编译 | 在已配置 MSVC、Qt、依赖路径和 sccache 的环境中运行 `cmake --preset Windows`，再运行 `cmake --build --preset Windows`，均退出 0；本机工具链尚未验证 |
| macOS 编译 | 在已配置依赖的原生 macOS 主机执行对应架构的 `cmake --preset macOS-13` / `cmake --build --preset macOS-13` 或 `macOS-13-m1` 对应命令，均退出 0；本轮尚未验证 |
| 聚焦测试 | 完成下文 Xvfb/openbox 与所有环境变量后，运行 `build/copyq-tests "testCore:configPath" "testCore:searchItemsAndCopy" "testCore:keysAndFocusing"`；按实际影响面裁剪或新增具体测试 |
| 完整测试 | 不适用：项目要求始终指定测试函数，禁止直接跑全量套件 |
| Windows 用户体验 | 原生 `.exe` 的隔离测试入口须在开发阶段落实；按 SPEC 的实际应用、焦点、中文输入及 DPI 场景留证，不把 Linux 测试当作 Windows 验收 |

上游入口的两个 build/install 示例已规范为 CMake 的 `--build build` 语法，构建目标不变。其余环境、测试筛选、显示会话和脚本约束保留如下。

### 3.3 上游命令与目录约定

#### Commands

Always use the following environment variables for all `build/copyq` and
`build/copyq-tests` commands:

    export COPYQ_SESSION_NAME="test"
    export COPYQ_SETTINGS_PATH="build/copyq-test-conf"
    export COPYQ_ITEM_DATA_PATH="build/copyq-test-data"
    export COPYQ_STATE_PATH="build/copyq-test-conf"
    export COPYQ_PLUGINS=""
    export COPYQ_DEFAULT_ICON="1"
    export COPYQ_SESSION_COLOR="#f90"
    export COPYQ_THEME_PREFIX="$PWD/shared/themes"
    export COPYQ_PASSWORD="TEST123"
    export COPYQ_LOG_LEVEL="DEBUG"
    export QT_LOGGING_RULES="*.debug=true;qt.*.debug=false"
    export QT_QPA_PLATFORM="xcb"

Run CMake to configure build:

    cmake -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
      -DCMAKE_INSTALL_PREFIX=$PWD/build/install \
      -DCMAKE_CXX_FLAGS="-ggdb -fdiagnostics-color" \
      -DWITH_TESTS=ON \
      -DPEDANTIC=ON .

Build: `cmake --build build`

Install: `cmake --build build --target install`

Tests and the app require a running X11 or Wayland session with a window
manager. **You MUST start Xvfb and openbox (or a Wayland compositor) before
running any `build/copyq` or `build/copyq-tests` command**, otherwise the
process will crash (exit code 134 or SIGSEGV). X11 setup (once per session):

    Xvfb :99 -screen 0 1280x1024x24 &
    sleep 1
    export DISPLAY=:99
    openbox &
    sleep 1

Then export `DISPLAY=:99` alongside the other environment variables for every
command. For Wayland, start a compositor and set `QT_QPA_PLATFORM=wayland`
instead of `xcb`.

Avoid running all tests, always specify a list of test functions to run.

Run tests after build: `build/copyq-tests $TEST_FUNCTIONS`

Run a specific test by group and tag: `build/copyq-tests "testCore:configPath"`

Run all tests for a plugin group: `build/copyq-tests testItemSync`

List test group names: `build/copyq-tests -functions`

List all individual test methods: `build/copyq-tests -datatags`

Filter tests by name substring: `COPYQ_TESTS_FILTER=clipboard build/copyq-tests`

Start the server process: `build/copyq`

In case any process exits with exit code 11 (SIGSEGV) use `coredumpctl` utility
to find the root cause.

Stop the server process: `build/copyq exit`

List server and client logs (server process does not need to run): `build/copyq logs`

Run a script - requires server to be running:

    build/copyq source script.js

    # the above command is equivalent to
    build/copyq 'source("script.js")'

Scripting API documentation is in @docs/scripting-api.rst. After changing it,
run @utils/script_docs_to_cpp.py to update the completion popup in the GUI.

Useful scripts (omit the `tab(...)` call to use the default tab):

- `tab('TAB1'); add('ITEM')` - prepend ITEM text item to the TAB1 tab
- `tab('TAB1'); size()` - item count in the TAB1 tab
- `tab('TAB1'); read(0,1,2)` - read items at indexes 0, 1 and 2 in the TAB1 tab
- `config()` - list configuration options with current value and description
- `config('check_clipboard', 'false')` - set an option

#### Project structure

- @plugins - code for various plugins build as dynamic modules loaded optionally by the app
- @src - main app code
- @src/app - wrappers for QCoreApplication object
- @src/common - common functionality, client/server local socket handling, logging
- @src/gui - GUI widgets and some helper modules
- @src/item - tab and item data handling, serialization code
- @src/platform - platform-specific code
- @src/scriptable - scripting capabilities
- @src/tests - tests for the main app
- @src/ui - Qt widget definition files (XML)
- @qxt - code to handle global system-wide shortcuts

### 3.4 Windows 上的入口链接

`CLAUDE.md` 是治理主入口。astack 标准结构使用 `AGENTS.md -> CLAUDE.md` 符号链接。

Windows checkout 的原生符号链接创建需要管理员权限，WSL 在 NTFS 上创建的 Linux 符号链接无法被 Windows 读取，因此该环境使用 **NTFS 硬链接**，两个文件共享同一内容。2026-09-29 的原生 macOS checkout 在确认内容相同后恢复 `AGENTS.md -> CLAUDE.md` 标准符号链接。只编辑 `CLAUDE.md`；会采用临时文件替换保存的编辑器可能断开硬链接，编辑后运行 `utils/check-harness.ps1` 验证。

Git 不保留硬链接身份；原 Windows 交付将两入口记录为普通文件，本机 macOS 恢复后 `AGENTS.md` 记录为符号链接。首次迁移将本仓库局部 `core.symlinks` 设为 `true`，全局设置未改。新 Windows checkout 中先比较两个文件内容；完全一致时可恢复硬链接。若 Git 因不支持软链而生成仅包含 `CLAUDE.md` 的链接占位文件，也可按已确认的目标恢复。其他内容差异先合并有效规则，禁止直接覆盖。

```powershell
$isLinkPlaceholder = (Get-Content -LiteralPath AGENTS.md -Raw).Trim() -eq 'CLAUDE.md'
if (-not $isLinkPlaceholder -and (Get-FileHash -LiteralPath AGENTS.md).Hash -ne (Get-FileHash -LiteralPath CLAUDE.md).Hash) {
    throw '两个治理入口内容不同，请先合并'
}
Remove-Item -LiteralPath AGENTS.md
New-Item -ItemType HardLink -Path AGENTS.md -Target CLAUDE.md
pwsh -NoProfile -File utils/check-harness.ps1
```

支持符号链接的 Linux checkout 可在确认两份内容一致后恢复标准符号链接。原始 `init-harness.sh` 严格要求符号链接，不识别本机硬链接适配；在本 checkout 维护 scaffold 使用项目校验器，不反复运行原脚本覆盖入口。

## 4. 权威文档

- `docs/astack/INDEX.md` — 版本 / 迭代 / SPEC 索引
- `docs/astack/version/Iteration<N>_<Slug>_SPEC.md` — 迭代设计文档，目标与边界的唯一权威
- `docs/astack/plan/Iteration<N>_<Slug>_PLAN.md` — 仅复杂任务使用的执行计划

## 5. Spec 工作流

默认只使用四个核心流程：

```text
/astack-workflow:spec  →  /astack-workflow:plan  →  /astack-workflow:dev  →  /astack-workflow:ship
想清楚要做什么              拆开复杂任务               开始执行                  验证、提交、推送
```

- `/astack-workflow:spec`：创建或更新 SPEC，并维护 `docs/astack/INDEX.md`。每个 SPEC 必须包含验证计划。
- `/astack-workflow:plan`：仅在复杂任务时，把 SPEC 拆成可执行步骤和验证点，输出到 `docs/astack/plan/`。
- `/astack-workflow:dev`：按 SPEC 或 PLAN 实施代码变更，运行验证命令并把证据写回 SPEC。
- `/astack-workflow:ship`：做最终新鲜验证、状态流转、提交并推送。

## 5.1 设计与影响面

当用户要求需求设计、方案设计、影响面分析或跨组件变更时，先完成设计再编码。小范围文档或单文件修正可以裁剪，但不得跳过假设、影响面和验证目标。

SPEC 至少写清：

- 背景与目标：做什么、不做什么、为什么现在做。
- 假设与约束：哪些来自用户，哪些来自代码搜索，哪些是推断。
- 影响面：涉及哪些组件、文件、接口或数据流，以及为什么必然涉及。
- 改动清单：按文件标注 `NEW` / `MODIFY`，避免无关扩散。
- 验证计划：至少 1 条可执行命令；纯文档变更可写 `git diff --check`。

## 5.2 验证门

1. SPEC 阶段写清楚至少 1 条机械验证命令。
2. DEV 阶段每完成一个可独立交付的任务，运行对应验证命令；失败则停止并记录失败原因。
3. SHIP 阶段只接受本轮新鲜验证结果，不用早先的“应该通过”或局部检查替代完整证据。
4. 验证记录只写命令、结果、日期和必要备注，不维护额外 review / retro 文档。

## 5.3 编码红线

- 不自造命名；新增文件名、类名、函数名、字段名、常量名前先 `rg` 同类实现。
- 不为未来扩展新增配置项、Handler、Filter 或抽象。
- 不修改无关代码的格式、注释或逻辑。
- 复用已有数据流；上游已经取得的数据，不重复请求或跨层查询。
- 复杂逻辑必须有针对性测试；没有新鲜验证，不声明完成。

## 6. 当前活跃迭代

首版只设三个实施 SPEC，按 1 → 2 → 3 推进；三个阶段共同满足全界面重做、Alfred 剪贴板完整对齐和 CopyQ 全能力保留，内部阶段产物不能替代完整首版。公共架构与接入契约以 Iteration1 为准。

- [Iteration1_ClipboardPalette_SPEC.md](docs/astack/version/Iteration1_ClipboardPalette_SPEC.md) — **开发中**：核心、快捷面板、插件/输入原型及三端构建测试入口已实现；macOS/Linux 已有聚焦证据，Windows 和剩余实机验收待补。
- [Iteration2_CopyQCompatibility_SPEC.md](docs/astack/version/Iteration2_CopyQCompatibility_SPEC.md) — **开发中**：I2-1 的 QML 管理、显式多选/命令、单集合和批量操作已有 macOS/Linux 聚焦证据；组级管理与其他页面样板收尾后继续 I2-2，全部界面/历史策略及三端验收待完成。当前开发入口。
- [Iteration3_AlfredDesktopRelease_SPEC.md](docs/astack/version/Iteration3_AlfredDesktopRelease_SPEC.md) — **待实施**：Alfred 增强能力与三端发布，依赖前两阶段。
- [docs/astack/INDEX.md](docs/astack/INDEX.md) 记录需求/验收归属和 GPT-6 Sol xhigh 执行入口。2026-09-29 已实施 SPEC1 及 SPEC2 首批开发任务；详细验证与未通过边界写在对应 SPEC，三端总验收尚未通过。

各 SPEC 已包含 3–4 个实施里程碑。执行时先核对当前里程碑的代码接口和相关验证，完成后把结果与交接内容写回所属 SPEC；确有必要再增加局部 PLAN，不默认拆更多 SPEC 或旁路报告。
