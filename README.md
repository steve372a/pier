<div align="center">

<div align="center">
  <img src="https://github.com/steve372a/steve372a/blob/main/2.svg" width="200">
  <img src="https://github.com/steve372a/steve372a/blob/main/cat.svg" width="200">
  <h1>Pier Package Installer</h1>
  <img src="https://github.com/steve372a/steve372a/blob/34af5582e2e13447abab73d31d616f23b05f38b3/pier_newworld_cn.png" width="600">
</div>

<p align="center" style="white-space: nowrap;">
  <a href="README.md" style="margin: 0 40px;">
    <img src="https://github.com/steve372a/steve372a/blob/9ac4fe356ef0a6036f96f6dcd85e7c6a725b44ef/cn.png" width="125" alt="中文">
  </a>
  <a href="README.en-US.md" style="margin: 0 40px;">
    <img src="https://github.com/steve372a/steve372a/blob/9ac4fe356ef0a6036f96f6dcd85e7c6a725b44ef/en.png" width="125" alt="English">
  </a>
</p>

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Windows-XP%2F7%2F10%2F11-blue)](https://github.com/steve372a/pier)
[![Version](https://img.shields.io/badge/version-2.4.0-brightgreen)]()
[![Stars](https://img.shields.io/github/stars/steve372a/pier?style=social)]()

</div>

---

## ✨ 速看

| 🚀 一行命令装软件 | 💾 XP/7 完美兼容 | 🧠 别名生态！ |
|---|---|---|
| `pier install notepadplusplus` | 工控机、老电脑、内网系统 | `pier o ffmpeg mp4 input.mp4 output.mp4` |

---

## 🚀 快速上手

### 安装

前往 [Releases](https://github.com/steve372a/pier/releases) 下载最新 `Pier-v2.4.0.zip`，解压即用。无需配置、无需依赖。

### 基础用法

| 命令 | 说明 | 示例 |
|---|---|---|
| `pier install <包名>` | 安装软件包 | `pier install notepadplusplus` |
| `pier remove <包名>` | 卸载软件包 | `pier remove notepadplusplus` |
| `pier o <包名> [参数]` | **一键启动，支持别名模板** | `pier o code` |
| `pier search <关键词>` | 在线搜索软件包 | `pier search python` |
| `pier list` | 查看所有可用软件 | `pier list` |
| `pier sources` | 管理软件源 | `pier sources` |
| `pier setlang <语言>` | 切换界面语言 | `pier sl zh-CN` |
| `pier sque check` | 检查文件 SQUE Script 语法 | `pier sque check <文件路径>` |
| `pier updpath` | 更新注册表永久 PATH | `pier updpath` |

---

## 🌍 无边界多语言

Pier 里，无论你说中文、English、日本語、한국어、Español——界面、提示、包描述，**一切用你的母语展示**。

<div align="center">
<img src="https://github.com/steve372a/steve372a/blob/185b5026b9e19a17618564b54bc33e9ecc3a651d/diandianearth.png" alt="diandianearth.png" width="100">
</div>

---

## 🪄 Pier O — 消灭命令参数

忘掉那些永远记不住的长参数：

```bash
# 传统方式
ffmpeg -i input.mp4 -c:v libx264 -preset fast -crf 23 output.mp4

# Pier 方式
pier o ffmpeg mp4 input.mp4 output.mp4

# 意想不到的垫片...
fm mp4 input.mp4 output.mp4
```

**一切都可以快速封装。** 任何复杂的命令行，都可以被封装成一个好记的"别名"。无论是 `qemu`、`ffmpeg`，还是你自己的工作脚本——一次写好，终身受用。

**直接使用别人的别名模板。** 一行命令即可下载使用社区共享的别名：

```bash
pier o user/package
```

> **Pier O 不是简化命令，是消灭命令。**
> 你记住的，不再是参数，而是你想做什么。

<div align="center">
<img src="https://github.com/steve372a/steve372a/blob/d24cd56bc94603d110d073b61457ceec23c531af/diandianmagic.png" alt="diandianearth.png" width="100">
</div>

---

## 💪 兼容性

- **Windows XP Service Pack 3 及以上**（v2.2.0 在 XP SP3 实测正常，XPSP1 也可以尝试。）
- Windows 7 / 8 / 10 / 11
- 针对老旧运维环境进行了核心兼容性优化

---

## 📦 包含组件

| 组件 | 协议 |
|---|---|
| Pier 主程序 | **MIT License** |
| `unzip.exe` | Info-ZIP License |
| `uma-get(wget).exe` | GPL License |
| `7za.exe` | LGPL License |
| `LIBCURL` | CURL License |

详情参见 Release 中的 `NOTICE` 文件。

> IP 形象 Piprlulu 基于 **CC BY-NC-SA 4.0** 协议保护，不受 MIT 约束。
> Fork 仓库后可移除，也可保留。[查看完整使用规则](https://steve372a.github.io/piprlulu/license)

---

## 🌐 官方资源

- **官方网站**: [steve372a.github.io/pier](https://steve372a.github.io/pier)
- **软件源仓库**: [github.com/steve372a/pier-repo](https://github.com/steve372a/pier-repo)
- **默认软件源**: `https://steve372a.github.io/pier-repo`

---

## 🤝 贡献

Pier 完全由 AI 辅助开发，为此，我写了个手册，帮助你彻底认识 Pier Package Installer，欢迎一切形式的贡献：

- 💡 提建议 → [Issues](https://github.com/steve372a/pier/issues)
- 🐛 报 Bug → [Bug Report](https://github.com/steve372a/pier/issues/new)
- 🔧 提 PR → [Pull Requests](https://github.com/steve372a/pier/pulls)

---

<div align="center">

**用 Pier，给老系统一个新生命 🦞**

[⭐ Star](https://github.com/steve372a/pier) · [📥 下载](https://github.com/steve372a/pier/releases) · [🌐 官网](https://steve372a.github.io/pier)

</div>
