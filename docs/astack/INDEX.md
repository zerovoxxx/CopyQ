# 迭代状态总表

> 所有迭代的状态追踪。由 `/astack-workflow:spec` 和 `/astack-workflow:ship` 维护。
>
> SPEC 默认放在 `docs/astack/version/`，复杂任务 PLAN 默认放在 `docs/astack/plan/`。

| 迭代 | 标题 | 状态 | 文档 | 创建日期 |
|------|------|------|------|---------|
| Iteration1 | Windows 剪贴板快捷面板 | 待实施 | [SPEC](version/Iteration1_ClipboardPalette_SPEC.md) | 2026-09-28 |

本次已完成 harness 接入与需求基线沉淀。上述状态表示产品功能尚待实施，不表示已构建或已交付 Alfred 式界面。复杂实现的 PLAN 在进入开发前按 SPEC 拆解，当前 `plan/` 仅保留目录。

## 变更记录

| 日期 | 版本 | 作者 | 摘要 |
|------|------|------|------|
| 2026-09-28 | 初始化 / Iteration1 | zerovoxxx | 按本机 astack harness-init 迁移治理入口，登记已确认需求、Qt Widgets 方案、验收场景及后续候选；Windows 使用 NTFS 硬链接适配，保留上游测试隔离规则。 |
| 2026-09-28 | 初始化交付 | zerovoxxx | 补充原始 UI 预览来源与验证边界，按用户要求提交当前治理文件；产品功能仍待实施，SPEC 归档检查为 no-op。 |
