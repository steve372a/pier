# Pier 软件源贡献指南

**Pier 软件源（pier-repo）** 是 Pier 包管理器的官方软件仓库。

你可以把它理解为 Pier 的"应用商店"。这个仓库里存放着所有可供 Pier 用户安装的软件包。

当你运行 `pier install 7zip` 时，Pier 会从这个仓库里找到 7zip 的元数据，然后下载并安装它。

这个仓库是开源的。任何人都可以为它贡献新的软件包。

为了让任何人不用仔细研究软件源的结构，我写了本手册。

---

## 仓库结构

在开始贡献之前，您可以快速理解一下代码结构。

```
pier-repo/
├── sources/              # 元数据目录（通过 GitHub Pages 分发）
│   ├── 7/                # 首字母索引（包名的首字母）
│   │   └── 7zip/         # 每个包一个独立目录
│   │       └── latest.metadata  # 指向最新版本的指针文件
│   ├── c/
│   │   └── czadb/
│   │       └── latest.metadata
│   └── ...
│
└── [GitHub Releases]     # 软件包存储
    ├── 7zip/             # Tag 名 = 包名
    │   ├── 7zip-26.00.pie      # 软件包文件（本质是 ZIP）
    │   └── 7zip-x64-26.00.pie
    └── czadb/
        └── czadb-4.2.3.pie
```

---

## 贡献流程

### Step 1: 准备软件包

1. 确认你的软件是 **绿色软件**：Pier 只支持解压即用的便携软件。如果你的软件需要写注册表、安装系统服务、或依赖复杂的安装程序，请先将其打包成便携版。此后可能会有有限的外部软件运行支持。
2. 将软件打包成 **ZIP**：确保解压后，软件的所有文件都在一个目录里，且双击即可运行。
3. 将 ZIP 重命名为 `{包名}-{版本}.pie`：例如 `7zip-26.00.pie`。

### Step 2: 创建元数据文件

元数据文件（.sque）描述了软件包的基本信息。格式如下：

```ini
[PackageName] # 包的显示名称（中文或英文）
7-Zip

[Version] # 版本号
26.00

[OS] # 软件包最低版本
XP

[InstallerName] # 包的唯一标识符（也是目录名和 Tag 名），只能用英文小写和数字，
7zip

[URL] # 下载文件名，可以用 `{version}` 占位符，自动填充
7zip-{version}.pie

[ProFile] # 包的简介
7-Zip 是一款适用于 Windows 系统的文件压缩软件。

[DefaultOpen] # 默认启动程序（可以用通配符）
7zFM.exe
::end

[Alias] # 命令别名模板，支持 `$1` 到 `$9999` 占位符
zip: 7z.exe a -tzip $1 $2
7z: 7z.exe a -t7z $1 $2
unzip: 7z.exe x $1 -o$2
::end

[Author] # 软件原作者
Igor Pavlov

[Distributor] # 打包者（你的名字）
你的名字

[PackageSize] # 包的大小，如 `4.14 MB`
4.14 MB

[Notice] # 许可证或注意事项，支持多行
遵循 GNU LGPL 协议。
```

#### 字段说明

| 字段 | 必填 | 说明 |
|---|---|---|
| `[PackageName]` | ✅ | 包的显示名称（中文或英文） |
| `[Version]` | ✅ | 版本号，格式如 `1.2.3` |
| `[OS]` | ✅ | 支持的操作系统，用逗号分隔 |
| `[InstallerName]` | ✅ | 包的唯一标识符（也是目录名和 Tag 名），只能用英文小写和数字 |
| `[URL]` | ✅ | 下载文件名，可以用 `{version}` 占位符 |
| `[ProFile]` | ✅ | 包的简介 |
| `[DefaultOpen]` | ⚠️ | 默认启动程序（如果这个包有 GUI 或命令行入口） |
| `[Alias]` | 可选 | 命令别名模板，支持 `$1` 到 `$9999` 占位符 |
| `[Author]` | 可选 | 软件原作者 |
| `[Distributor]` | ✅ | 打包者（你的名字） |
| `[PackageSize]` | ✅ | 包的大小，如 `4.14 MB` |
| `[Notice]` | 可选 | 许可证或注意事项，支持多行 |

#### 命名规范

- `InstallerName` 只能包含小写字母、数字、连字符（`a-z`、`0-9`、`-`）。例如：`7zip`、`python`、`czadb`。大写字母不敏感，会被转换为小写。
- 文件名中的版本号必须与 `[Version]` 字段一致。

### Step 3: 上传软件包

你需要将 `.pie` 文件上传到 pier-repo 仓库的 GitHub Releases 中。

1. 你需要 Fork pier-repo 仓库：https://github.com/steve372a/pier-repo
2. 点击 **"Draft a new release"**
3. Tag 名填写 `{InstallerName}`（例如 `7zip`，**不要**包含版本号）
4. Release title 填写包名（例如 `7-Zip`）
5. 将 `.pie` 文件拖入 "Attach binaries" 区域
6. 如果同一个包有多个架构（x86、x64），可以上传多个文件
7. 点击 **"Publish release"**

### Step 4: 创建元数据目录

在 `sources/` 目录下，按首字母创建子目录：

```
sources/
└── {首字母}/
    └── {InstallerName}/
        └── latest.metadata
```

例如，`7zip` 包应该放在 `sources/7/7zip/latest.metadata`。

`latest.metadata` 文件的内容：就是 Step 2 中你写的 `.sque` 文件。

### Step 5: 提交 Pull Request

1. Fork https://github.com/steve372a/pier-repo
2. 将你创建的元数据目录和文件推送到你的 fork 中
3. 创建 Pull Request，目标分支为 `main`
4. 等待审核

---

## 规则与注意事项

### 允许的软件类型

- ✅ 绿色便携软件（解压即用）
- ✅ 命令行工具
- ✅ 不写注册表、不安装系统服务的软件

### 禁止的软件类型

- ❌ 需要运行 `msiexec` 或 `.exe` 安装器的软件
- ❌ 需要写注册表的软件
- ❌ 需要安装系统服务的软件
- ❌ 破解版、盗版软件、商业软件（例如 Adobe 产品）
- ❌ 包含恶意代码的软件

### 版本更新

当你需要更新一个已存在的包时：

1. 上传新版 `.pie` 文件到**同一个 Tag**（例如 `7zip`）
2. 文件名使用新版本号（例如 `7zip-27.00.pie`）
3. 更新 `sources/{首字母}/{InstallerName}/latest.metadata` 文件中的 `[Version]` 和 `[PackageSize]` 等字段
4. 提交 Pull Request。

旧版本的文件不需要删除。Pier 客户端会自动选择最新版本。

### 命名冲突

`InstallerName` 是全局唯一的。在命名之前，请先检查 `sources/`、`release` 目录下是否已存在同名包。

---

## 常见问题

**Q: 我的软件不是绿色软件，但我还是想让它进 Pier，怎么办？**

A: 请先将它制作成便携版。搜索 `{软件名} portable`，通常可以找到官方或社区制作的便携版本。

**Q: 我的软件有 x86 和 x64 两个版本，怎么处理？**

A: 在同一个 Release 里上传两个文件：
- `7zip-26.00.pie`（x86）
- `7zip-x64-26.00.pie`（x64）

然后修改 `[URL]` 字段为：

```ini
[URL]
x86: 7zip-{version}.pie (default)
x64: 7zip-x64-{version}.pie
::end
```

**Q: 我上传的 .pie 文件很大，可以压缩吗？**

A: `.pie` 本身就是 ZIP 文件。请确保使用 ZIP 格式压缩，不要使用 RAR、7z 等其他格式。不能分卷。

**Q: 审核需要多长时间？**

A: 通常 1-3 天。如果超过一周没有回复，可以在 PR 评论区 @steve372a。

**Q: 我的软件元数据能显示，但是下载就错误，怎么回事？**

A: 一般情况下，维护者会拒绝你的 Pull Request，因为下载链接是错误的。但如果被错误合并了，建议你去再提交一次 Pull Request，并说明情况。

---

感谢你为 Pier 生态做出的贡献！你的每一个软件包，都在帮助更多的人用最简单的方式管理他们的软件。
