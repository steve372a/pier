<r><img src="https://steve372a.github.io/images/avatar.gif" alt></r>

# Pier Package Installer

Pier 是一个基于 **Batch (BAT)** 与 **MSHTA** 构建的轻量级包管理器。本项目目前仍在完善中，旨在为 Windows 用户提供简单、透明且兼容性极强的软件管理方案。2022年8月立项。

## 可以做到的事

* **自动化下载与安装**：通过集成工具实现软件包的自动获取、解压及安装部署。
* **图形化交互界面**：利用 MSHTA 技术，在纯脚本环境下提供直观的安装确认、风险提示及操作界面。
* **安全审计与拦截**：内置安全过滤机制，能够识别并拦截安装脚本中可能存在的危险指令（如 `format`、`net user`、`reg add` 等），保护系统安全。
* **多语言动态切换**：支持通过 `.ini` 配置文件加载不同语言包，实现界面文字的本地化展示。
* **静默模式支持**：提供 `-y` 参数选项，允许用户跳过所有 HTA 弹窗确认，实现完全自动化的静默安装。
* **软件卸载管理**：支持对已安装包进行清理与卸载，并具备语言包保护等逻辑防止误删核心组件。
* **环境清理与维护**：运行前后自动清理临时文件（如 `pier_choice.tmp`、`pier_env.tmp`），保持系统环境整洁。

## 兼容系统
* **Windows XP 起**（建议 Windows XP Service Pack 3 及以上版本）。

---

## 许可证与第三方声明 (License & Third-Party Components)

**Third-Party Components:** This distribution contains third-party binaries (e.g., `unzip.exe`, `uma-get(wget).exe`) for convenience. These components are governed by their respective licenses.

**Main License:**
Pier itself is licensed under the **MIT License**.

## 官方网站
[Pier 官方网络站点](https://steve372a.github.io/pier)
