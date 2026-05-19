# Pier 软件包安装器使用教程

## 简介

Pier 是一个基于 Windows 批处理 (Batch) 与 C 语言构建的轻量级、自由的软件包管理器。它完美兼容 Windows XP/7/10/11 等系统，主打极致兼容性。还支持智能启动软件。

## 安装 Pier

### 方法一：下载 Release 版本

1. 访问 [GitHub Releases](https://github.com/steve372a/pier/releases)
2. 下载最新的 `Pier-vX.X.X.zip`
3. 解压到任意目录（如 `E:\Pier`）
4. 将解压目录添加到系统环境变量 PATH 中（可选）

### 方法二：使用其他包管理器

- **Winget**：`winget install pier`

## 基本命令

### 1. 安装软件包

```batch
# 基本安装
pier install <包名>

# 示例：安装 Notepad++
pier install notepadplusplus

# 自动安装（跳过确认）
pier install <包名> -y
```

安装过程：
1. 从软件源下载包元数据
2. 显示包信息（名称、版本、大小）(使用 `-y` 跳过)
3. 确认安装（使用 `-y` 跳过）
4. 下载并解压到 `app\<包名>` 目录
5. 创建桌面快捷方式（可选）

### 2. 卸载软件包

```batch
# 基本卸载
pier remove <包名>

# 示例：卸载 Notepad++
pier remove notepadplusplus

# 自动卸载（跳过确认）
pier remove <包名> -y
```

**注意**：卸载会删除 `app\<包名>` 目录及其所有内容，包括配置文件！

### 3. 运行已安装的软件

```batch
# 使用默认方式启动
pier o <包名>

# 示例：启动 VS Code
pier o vscode

# 使用别名启动特定版本
pier o <包名> <别名>

# 示例：使用别名启动
pier o sanakaprix/czadb

# 传递参数
pier o <包名> [别名] [参数...]
```

### 4. 搜索软件包

```batch
# 在线搜索
pier search <关键词>

# 示例：搜索 Python
pier search python
```

### 5. 列出可用软件包

```batch
# 从服务器获取完整列表
pier list
```

## 软件源管理

### 查看当前源

```batch
pier sources list
```

### 切换软件源

```batch
# 交互式切换
pier sources

# 直接切换到指定 URL
pier sources change <URL>
```

默认软件源：
- 官方源：`https://steve372a.github.io/pier-repo`

### 源配置文件

源配置保存在 `etc\sourceimage.ini`：
```ini
[package_source]
https://steve372a.github.io/pier-repo
[alias_source]
https://steve372a.github.io/pier-repo
```

## 语言设置

### 切换语言

```batch
# 切换到中文
pier --setlang set zh-CN

# 或简写
pier sl set zh-CN

# 切换到英文
pier sl set en-US
```

### 安装新语言包

```batch
pier --setlang install <语言代码>
```

### 重新安装语言包

```batch
pier --setlang reins <语言代码>
```

语言文件位于 `share\language\<语言代码>\lang.ini`。

## 别名系统（Alias）

### 什么是别名？

从前，一般的包管理器安装完软件后就不管了，要自己找软件来启动软件。后来 Scoop 引入了 Shim 系统，可以设置一个符号链接，实现随意地方启动软件，但参数还是要自己输入的。而 Pier 引入了别名模板系统，可以实现更智能的预设参数。
别名模板是包作者或第三方预设的快捷启动方式。它可以做到：

- 传递参数
- 通过占位符实现精准位置传递参数

### 使用别名

```batch
# 使用别名安装/运行第三方预设包
pier o sanakaprix(第三方别名模板作者)/czadb(包名称)

# 直接启动官方预设的别名模板
pier o czadb

# 别名对应的真实包在 [ToUse] 字段中指定
```
## 目录结构

```
pier-2.0.0-beta1/
├── bin/                    # 可执行文件
│   ├── pier-pkg.exe       # 包管理主程序
│   ├── pier-op.exe        # 程序启动器
│   ├── pier-ver.exe       # 版本检查
│   ├── vecho.exe          # 彩色输出工具
│   ├── sed.exe            # 文本处理
│   ├── unzip.exe          # 解压工具
│   └── uma-get.exe        # 下载工具
├── etc/                    # 配置文件
│   ├── language.ini       # 语言配置
│   └── sourceimage.ini    # 软件源配置
├── share/                  # 共享数据
│   ├── language/          # 语言包
│   │   ├── zh-CN/
│   │   │   └── lang.ini
│   │   └── en-US/
│   │       └── lang.ini
│   ├── cache/             # 缓存目录
│   └── metadata/          # 包元数据
│       └── alias/         # 别名模板
├── app/                    # 安装的软件包
│   └── <包名>/
├── piec.bat               # 主入口脚本
└── src/                    # 源代码
    ├── pier-pkg-simple.c
    ├── pier-op.c
    ├── pier-ver.c
    └── vecho.c
```

## 许可证

Pier 采用 MIT License 开源。

第三方组件：
- GNU sed (GPLv3): https://www.gnu.org/software/sed/
- GNU wget (GPLv3): https://www.gnu.org/software/wget/
- curl (curl license): https://curl.se/
- Info-ZIP (BSD-style): http://www.info-zip.org/

## 联系方式

- 作者：Sanakaprix
- 邮箱：steve372@foxmail.com
- GitHub：https://github.com/steve372a/pier/
- 网站：https://steve372a.github.io/pier/

---

**Thanks for using Pier Package Installer by Sanakaprix.**
