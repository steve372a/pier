<div align="center">
  <img src="https://github.com/steve372a/steve372a/blob/main/2.svg" width="300">
  <img src="https://github.com/steve372a/steve372a/blob/main/cat.svg" width="300">
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

**本仓库仅通过 URL 引用 Piprlulu 图片，不直接分发图片文件。**

Pier 是一个基于 **Batch (BAT)、C语言** 构建的轻量级包管理器。本项目是为了给 Windows 用户提供一个简单、透明、易用、自由、高兼容性（支持 Windows XP）的包管理器。

**本项目使用 AI Agent 辅助。**

> **立项时间**：2022年8月  
> **当前版本**：2.3.1
> **下一个版本**：2.4.0

>  本仓库的插画 Piprlulu 属于 [@steve372a](https://github.com/steve372a)，其形象受到 CC BY-NC-SA 4.0 协议保护。可以参见 [Piprlulu使用规则](https://steve372a.github.io/piprlulu/license)
> piprlulu 图片引用仅作为项目展示（Demonstration），不构成软件运行的必要依赖（Dependency）
---

<img src="https://github.com/steve372a/steve372a/blob/6223e2c39a5e59abe9afbdd009721c2f84bdd84c/pier01.png" width="400">

### 无边界多语言
* 天生无需翻译器。
* 无论你说中文、English、日本語、한국어、Español……Pier 的界面、提示信息、甚至软件包的描述，一切的一切————都会用你的母语展示。

<div align="center">
<img src="https://github.com/steve372a/steve372a/blob/185b5026b9e19a17618564b54bc33e9ecc3a651d/diandianearth.png" alt="diandianearth.png" width="150">
</div>

### 神奇“Pier O“

*   **忘掉那些你永远记不住的长参数。**  
    比如 `ffmpeg -i input.mp4 -c:v libx264 -preset fast -crf 23 output.mp4`  
    在 Pier 里，你只需要记住：  
    `pier o ffmpeg mp4 input.mp4 output.mp4`

*   **一切都可以快速封装。**  
    任何复杂的命令行，都可以被封装成一个好记的“别名”。  
    无论是 `qemu`、`ffmpeg`，还是你自己的工作脚本——一次写好，终身受用。

*   **直接使用别人的别名模板。**  
    不用从零开始。你完全可以使用社区他人的别名模板，一行命令即可下载使用：  
    `pier o user/package`

> **Pier O 不是简化命令，是消灭命令。**  
> 你记住的，不再是参数，而是你想做什么。

<div align="center">
<img src="https://github.com/steve372a/steve372a/blob/d24cd56bc94603d110d073b61457ceec23c531af/diandianmagic.png" alt="diandianearth.png" width="150">
</div>

---

<img src="https://github.com/steve372a/steve372a/blob/996a4f11d6a821803effab95813b1be9ab864c89/56f07bde8a2076062290862ffb5b6d5b.png" width="400">

<div align="center">
<img src="https://github.com/steve372a/steve372a/blob/63258ba2edba5d3fde9044023deb09d703abf185/b310f6c9bfce6aa0507dd10d799d2b78.png" width="200">
</div>

**如何开始？**

2.  **直接前往 [Releases](https://github.com/steve372a/pier/releases) 页面**：下载最新的 `Pier-vX.X.X.zip` 压缩包，解压后即可开箱即用。

---


<img src="https://github.com/steve372a/steve372a/blob/9f3bd43ea6a767402f5fdea9139c560c6d78ec34/d6f6a166d07400ec590643ee6b30e5d3.png" width="400">

| 命令 | 说明 | 示例 |
| :--- | :--- | :--- |
| `pier install <包名>` | 安装软件包 | `pier install notepadplusplus` |
| `pier remove <包名>` | 卸载软件包 | `pier remove notepadplusplus` |
| `pier o <包名> [别名]` | **一键启动已安装程序，支持占位符！别名模板生态！** | `pier o code` 或 `pier o code -main` |
| `pier search <关键词>` | 在线搜索软件包 | `pier search python` |
| `pier list` | 查看当前源下的所有软件 | `pier list` |
| `pier sources` | 管理或快速切换软件源 | `pier sources` |
| `pier setlang <语言>` | 切换 Pier 界面语言 | `pier sl zh-CN` |

---

<img src="https://github.com/steve372a/steve372a/blob/4b953a76ce07de2daab01429d3d1667f40f3be82/b2394de3bd9491f0b0452552765b5cd0.png" width="400">

* **Windows XP 及以上（Pier v2.2.0 版本在 Windows XP Service Pack 3 测试正常。）**（针对老旧运维环境进行了核心兼容性优化）。
  
<img width="1280" height="720" alt="b921d5b8359ac1e57d1ed24972241820" src="https://github.com/user-attachments/assets/bca3185e-a407-4d3b-b034-344042714a32" />


<img src="https://github.com/steve372a/steve372a/blob/eeff9306cc601a12bb2900f937951a2f00f2d65d/5a4830003332aae887485fc6f0a01dc4.png" width="400">

* **第三方组件**：包含 `unzip.exe`、`uma-get(wget).exe` 等，遵循其各自的开源协议。你可以参照Release中的NOTICE文件。
* **主程序**：Pier 采用 **MIT License** 开源。
* **IP 形象小规则**：你可以在 https://steve372a.github.io/piprlulu/license 中查看到原文哦。
  piprlulu 家族不受 MIT 约束。本形象是基于 CC BY-NC-SA 4.0 协议保护的。
  FORK 仓库后什么也不需要移除哦！移除也是可以的。

<div align="center">
<img src="https://pic1.imgdb.cn/item/69ee0a5f6dcdaf678a8bdb57.png" alt="pipicry.png" width="200">
</div>

* (★ ω ★)...

<div align="center">
<img src="https://github.com/steve372a/steve372a/blob/4a418db9b13fba5945608c9b7507258c7b8e8958/3d642f0851637e18eeb2aab295600ff9.png" width="600">
</div>

## 🌐 官方资源
* **官方网站**: [steve372a.github.io/pier](https://steve372a.github.io/pier)
* **默认软件源（Raw）**: `https://steve372a.github.io/pier-repo`
* **默认软件源**: [https://steve372a.github.io/pier-repo](https://github.com/steve372a/pier-repo)

---

SED: https://www.gnu.org/software/sed/
WGET(UMA-GET): https://www.gnu.org/software/wget/ 
UNZIP: https://infozip.sourceforge.net/UnZip.html
7ZA: https://www.7-zip.org/
