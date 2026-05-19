# Contributing to Pier

欢迎！Pier 是一个轻量级 Windows 包管理器。

Pier 使用了 AI 辅助开发

## 🧭 项目定位

**给 Windows XP 用的（也不只是）轻量级无依赖包管理器。**

- 目标用户：Windows XP/7 用户、工控机、老电脑
- 核心竞争力：XP 兼容 + 别名生态 + 轻量

## ✅ 欢迎什么样的贡献

| 类型 | 说明 |
|---|---|
| 🐛 Bug 修复 | 任何时候都欢迎 |
| 📖 文档改进 | README、拼写、示例 |
| 🧩 别名模板 | 提交到 pier-repo |
| ⬆️ 包更新 | 更新软件源里的包版本 |
| 📦 贡献包！ | 贡献软件包、元数据到 pier-repo |
| 🧩 贡献翻译 | 贡献万国的元数据包、软件、文档的翻译！ |
| 💡 Issue 讨论 | 提建议、报需求 |

## ❌ 不接受什么

以下改动违背 Pier 的设计哲学，会被拒绝：

- ❌ **违反设计规范** — 
  - 放弃 SQUE、引入 JSON/YAML/XML 等配置格式
- ❌ **引入 .NET / MSVC 新运行时依赖** — 纯 Win32 API + C89/C90
- ❌ **放弃 Windows XP 兼容** — 目前仍需要支持 Windows XP

## 🚀 快速开始

### 环境

- 宿主机使用 Windows XP/7/10/11 或 Wine
- 编译器：TCC（推荐），pier-get 如果要编译，使用 mingw
- 构建方式见文档

### 提 PR 流程

1. Fork 本仓库
2. 从 `radical` 分支创建你的功能分支
3. 修改代码
4. 提交 PR 到 `radical` 分支
5. 等待审核！

> radical 是开发分支，main 是稳定分支。
> PR 合入 radical 后，经维护者测试再合并到 main。

## 🧠 代码风格

- C89/C90 标准
- 纯 Windows API（无 .NET / CRT 新版本依赖）
- 欢迎重写！

## ⚖️ License

Pier 主程序采用 MIT License。
