<r><img src="https://steve372a.github.io/images/avatar.gif" alt></r>

# Pier Package Installer

Pier 是一个基于 **Batch (BAT)** 与 **MSHTA** 构建的轻量级包管理器。本项目是为了给 Windows 用户提供一个简单、透明、高兼容性（甚至支持 Windows XP）且具备安全审计能力的软件管理方案。

> **立项时间**：2022年8月  
> **当前版本**：2.1.0 / 2.2.0 Beta 

---

## 🚀 核心功能 (Features)

* **自动化下载与安装**：集成工具实现软件包自动获取、解压及部署。
* **图形化交互界面**：利用 MSHTA 技术，在纯脚本环境下提供直观的安装/卸载确认与风险提示窗口。
* **智能程序启动 (`pier o`)**：支持通过元数据定义的别名（Alias）或默认路径直接从命令行启动程序，无需手动配置系统环境变量。
* **安全审计与拦截**：内置安全过滤机制，自动识别并拦截安装脚本中可能存在的危险指令（如 `format`、`net user`、`rd /s` 等）。
* **多语言动态切换**：支持通过 `.ini` 配置文件动态加载语言包，实现界面文字本地化（默认支持中英双语）。
* **软件源管理 (`sources`)**：支持官方源检测及第三方源快速切换，内置换源安全确认逻辑。
* **静默模式支持**：提供 `-y` 参数，允许跳过所有 HTA 弹窗确认，实现完全自动化的静默安装。

## 🛠️ 命令使用 (Usage)

| 命令 | 说明 | 示例 |
| :--- | :--- | :--- |
| `pier install <包名>` | 安装软件包 | `pier install notepadplusplus` |
| `pier remove <包名>` | 卸载软件包 | `pier remove notepadplusplus` |
| `pier o <包名> [别名]` | **一键启动已安装程序** | `pier o code` 或 `pier o code -main` |
| `pier search <关键词>` | 在线搜索软件包 | `pier search python` |
| `pier list` | 查看当前源下的所有软件 | `pier list` |
| `pier sources` | 管理或快速切换软件源 | `pier sources` |
| `pier setlang <语言>` | 切换 Pier 界面语言 | `pier sl zh-CN` |

---

## ⚠️ 重要下载说明 (Download Tips)

**由于 Pier 运行高度依赖完整的二进制工具链与目录结构：**

1.  **切勿直接点击 GitHub 的 `Download ZIP` (Source Code)**：源码包仅包含脚本文件，缺少运行必需的二进制工具（如 `sed.exe`, `unzip.exe`）及 HTA 模块，直接运行会报错。
2.  **请前往 [Releases](https://github.com/steve372a/pier/releases) 页面**：下载最新的 `Pier-vX.X.X.zip` 压缩包，解压后即可开箱即用。

---

## 💻 兼容系统
* **Windows XP 及以上（Pier v1.0.1 版本在 Windows XP Service Pack 1 测试正常。）**（针对老旧运维环境进行了核心兼容性优化）。

## 许可证与第三方声明 (License)

* **第三方组件**：包含 `unzip.exe`、`uma-get(wget).exe` 等，遵循其各自的开源协议。
* **主程序**：Pier 采用 **MIT License** 开源。

## 🌐 官方资源
* **官方网站**: [steve372a.github.io/pier](https://steve372a.github.io/pier)
* **默认软件源（Raw）**: `https://steve372a.github.io/pier-repo`
* **默认软件源**: [https://steve372a.github.io/pier-repo](https://github.com/steve372a/pier-repo)

---

SED: https://www.gnu.org/software/sed/
WGET(UMA-GET): https://www.gnu.org/software/wget/ 
UNZIP: https://infozip.sourceforge.net/UnZip.html
7ZA: https://www.7-zip.org/
