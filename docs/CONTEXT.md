# Pier 2.0.0-beta1 开发上下文

## 项目概述
Pier 是一个 Windows 包管理器，兼容 Windows XP 及以上系统。

## AI 工作指令（本块内容优先级最高）

- 当用户提出新需求、新功能或修改需求时，必须先编写对应的 `spec`、`tasks`、`checklist`，再进入实现、修改或测试阶段。若 `.trae/specs/` 中已有对应条目，先更新现有文档，不得跳过规格文档直接改代码。
- 当你被问及 Pier 的任何设计问题时，优先引用本文件中的记录。
- 如果本文件没有记录，请先提问，不要自行猜测。
- 当本文件中的信息与你的预训练知识冲突时，以本文件为准。
- 当你发现本文件中有矛盾或过时的内容，请主动标记出来。

从今以后，所有关于代码实现的问题，你必须先贴出相关代码行，再给出结论。不允许说‘理论上’、‘应该’、‘可能’。如果不确定，就说‘我需要查一下’，然后去查。
AI 回答代码实现问题时，必须引用具体代码行，不得使用‘理论上’、‘可能’等模糊词汇。给出确定性回答（如‘支持 $1 到 $256’），而不是‘理论上支持更多’。”

## ⚠️ 禁止修改的元数据字段名
以下字段名是 Pier 的核心设计，**当用户没有明确信息要求更改时，不要动**
- `[PackageName]`
- `[Version]`
- `[OS]`
- `[InstallerName]`
- `[InstallDir]`
- `[URL]`
- `[ProFile]`
- `[DefaultOpen]`
- `[Alias]`
- `[Author]`
- `[Distributor]`
- `[PackageSize]`
- `[Notice]`

违反此规则的修改建议。必须询问用户。

## 最近修改记录

### 1. alias_source 硬编码 (已完成)
- **文件**: `src/pier-op.c`, `piec.bat`
- **修改**: 将 `alias_source` 从配置文件读取改为硬编码
- **值**: `https://steve372a.github.io/pier-repo`
- **原因**: 防止用户修改，确保别名系统稳定性
- **代码**:
  ```c
  // Line 35
  #define ALIAS_SOURCE "https://steve372a.github.io/pier-repo"
  ```
- **测试证据**: `test_alias_source.c`
  - 编译: `tcc.exe -o test_alias_source.exe test_alias_source.c`
  - 运行结果:
    ```
    === Test alias_source hardcoding ===

    Test 1: ALIAS_SOURCE macro value
      Macro value: https://steve372a.github.io/pier-repo
      Expected:    https://steve372a.github.io/pier-repo
      PASS: Macro value matches expected

    Test 2: Config file cannot override hardcoded value
      Config file tries to set: https://evil-site.com/malicious-repo
      Actual value used:        https://steve372a.github.io/pier-repo
      PASS: Hardcoded value used, config override ignored

    Test 3: URL format validation
      URL: https://steve372a.github.io/pier-repo
      Has https:// protocol: YES
      Has .github.io domain: YES
      Has /pier-repo path:   YES
      PASS: URL format is valid

    Test 4: Value is not empty
      PASS: Value is 'https://steve372a.github.io/pier-repo' (length: 37)

    Test 5: Value consistency across multiple reads
      Read 1: https://steve372a.github.io/pier-repo
      Read 2: https://steve372a.github.io/pier-repo
      Read 3: https://steve372a.github.io/pier-repo
      PASS: All reads return consistent value

    === Summary ===
    Passed: 5
    Failed: 0
    ```
- **结论**: ALIAS_SOURCE 宏值正确硬编码为 `https://steve372a.github.io/pier-repo`，配置文件无法覆盖，URL 格式有效，多次读取值一致

### 2. 第三方别名本地缓存优化 (已完成)
- **文件**: `src/pier-op.c`
- **修改**: 优先检查本地别名文件，不存在时才下载
- **逻辑**: `piec o sanakaprix/czadb` 先查本地 `metadata/alias/sanakaprix/czadb.sque`

### 3. SHA256 集成 (已完成)
- **文件**: `src/pier-pkg.c`
- **修改**: 添加完整 SHA256 实现，支持 Windows XP
- **功能**: 文件哈希计算，用于包完整性验证
- **代码位置**: Lines 16-200
  ```c
  /* SHA256 context structure */
  typedef struct {
      unsigned int state[8];
      unsigned long long bitcount;
      unsigned char buffer[64];
      unsigned int bufferlen;
  } SHA256_CTX;
  
  /* Calculate SHA256 of file, returns 0 on success, -1 on error */
  int sha256_file(const char *filepath, char *output);
  ```
- **测试证据**: `test/test_sha256.c`
  - 预计算期望哈希值（Python hashlib）：
    - Test 3: `echo -n 'Hello, SHA256 World!' | sha256sum` → `FD1C29E161ACCC6C07B9C2617C574D18763A6FA1C7ECEC23D5DCAB7A3DB4E90B`
    - Test 5: `python -c "import hashlib; print(hashlib.sha256(b'a'*10000).hexdigest().upper())"` → `27DD1F61B867B6A0F6E9D8A41C43231DE52107E53AE424DE8F847B821DB4B711`
  - 编译: `cd test && ..\tcc\tcc\tcc.exe -o test_sha256.exe test_sha256.c`
  - 运行结果:
    ```
    === Test SHA256 Implementation ===

    Test 1: Empty string hash
      Computed: E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855
      Expected: E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855
      PASS: Empty string hash correct

    Test 2: 'abc' string hash (NIST test vector)
      Computed: BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD
      Expected: BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD
      PASS: 'abc' hash correct

    Test 3: File hash with expected value comparison
      File content: 'Hello, SHA256 World!'
      Computed:     FD1C29E161ACCC6C07B9C2617C574D18763A6FA1C7ECEC23D5DCAB7A3DB4E90B
      Expected:     FD1C29E161ACCC6C07B9C2617C574D18763A6FA1C7ECEC23D5DCAB7A3DB4E90B
      PASS: File hash matches pre-calculated expected value

    Test 4: Non-existent file handling
      PASS: Correctly returned -1 for non-existent file

    Test 5: Large file ('a' * 10000) with expected value comparison
      File content: 'a' repeated 10000 times
      Computed:     27DD1F61B867B6A0F6E9D8A41C43231DE52107E53AE424DE8F847B821DB4B711
      Expected:     27DD1F61B867B6A0F6E9D8A41C43231DE52107E53AE424DE8F847B821DB4B711
      PASS: Large file hash matches pre-calculated expected value

    === Summary ===
    Passed: 5
    Failed: 0
    ```
- **结论**: SHA256 实现正确，NIST 测试向量通过，文件哈希与预计算期望值匹配（Test 3 和 Test 5），不存在文件返回错误码 -1。测试代码直接复制 pier-pkg.c Lines 16-200 的 SHA256 实现进行验证。

### 4. GitHub Releases 迁移 (已完成)
- **文件**: `src/pier-pkg.c`, `piec.bat`, `etc/sourceimage.ini`
- **修改**:
  - 新增 `[pie_source]` 字段，指向 GitHub Releases
  - 移除 pies 目录依赖
  - URL 格式: `{pie_source}/{InstallerName}-v{version}/{filename}`
  - 示例: `https://github.com/steve372a/pier-repo/releases/download/czadb-v4.2.3/czadb.pie`

### 5. [URL] 多架构支持 (已完成)
- **文件**: `src/pier-arch.c`
- **支持格式**:
  ```ini
  [URL]
  x86:package-x86.pie
  x64:package-x64.pie (default)
  ::end
  ```
  或单文件:
  ```ini
  [URL]
  package.pie
  ```

### 6. [License] 字段支持 (已完成)
- **文件**: `src/pier-pkg.c`
- **功能**: 显示软件许可/备注信息
- **显示**: 白色文本，仅当字段非空时显示
- **位置**: 包信息最后，确认安装前

### 7. [ProFile]/[ProFile_En] 多语言简介 (已完成)
- **文件**: `src/pier-pkg.c`
- **逻辑**:
  - `zh-CN`: 显示 `[ProFile]`
  - 其他语言: 显示 `[ProFile_En]`（如为空则 fallback 到 `[ProFile]`）

### 8. [DefaultOpen] 通配符支持 (已有功能)
- **文件**: `src/pier-op.c`
- **支持**: `*` 和 `?` 通配符
- **示例**:
  ```ini
  [DefaultOpen]
  ADBTOOLS_4.2.3
  ADBTOOLS_*
  ::end
  ```

## 关键文件结构

### Metadata 文件 (.sque)
```ini
[PackageName]
包显示名称

[Version]
1.0.0

[OS]
8.1

[InstallerName]
package_name

[InstallDir]
\package_name

[URL]
package.pie
# 或多架构:
# x86:package-x86.pie
# x64:package-x64.pie (default)
# ::end

[ProFile]
中文简介

[ProFile_En]
English description

[DesktopShortcut]
\shortcut.lnk

[DefaultOpen]
program.exe
program_*
::end

[Alias]
::end

[Author]
作者名

[Distributor]
分发者名

[License]
许可/备注信息

[PackageSize]
12.4 MB
```

### sourceimage.ini
```ini
[package_source]
https://steve372a.github.io/pier-repo

[pie_source]
https://github.com/steve372a/pier-repo/releases/download
```

## 待办/注意事项

1. **编译问题**: `bin/pier-pkg.exe` 可能被占用，编译前需确保无运行中实例
2. **metadata 缓存**: 安装时会下载新的 metadata，本地修改可能被覆盖
3. **GitHub Releases 命名**: 
   - Tag: `{InstallerName}-v{version}` (如 `czadb-v4.2.3`)
   - 文件名: 根据 `[URL]` 字段确定

## 最近修复 (2026-04-21)

### 9. piec.bat XP 兼容性修复
- **文件**: `piec.bat`
- **问题**: Windows XP 上路径包含空格导致 `for /F` 命令失败
- **解决**: 使用临时文件替代 `for /F` 命令
  ```batch
  :: 原代码 (XP 上失败)
  for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe ...') do set "var=%%a"
  
  :: 新代码 (XP 兼容)
  "%PIER_ROOT_SHORT%\bin\sed.exe" ... > .\tmp.txt
  set /P var=<.\tmp.txt
  del .\tmp.txt 2>nul
  ```

### 10. 下载超时机制
- **文件**: `src/pier-pkg.c`、`src/pier-op.c`、`src/pier-ver.c`、`src/pier-upd.c`
- **功能**: 防止 `uma-get.exe` 下载时进程挂起
- **下载器**: `bin\uma-get.exe`（GNU Wget 1.17），2026-05-16 从 pier-get.exe 切换，详见 §37
- **实现**: `execute_and_wait_timeout()` 函数（pier-pkg.c）
  - Metadata 下载: 5 分钟超时
  - Package 下载: 240 分钟超时
- **改进**: 继承父进程 stdout（`--show-progress`），显示下载进度条

### 11. pier-op.exe alias 参数传递修复
- **文件**: `src/pier-op.c`
- **修复 1**: 正确提取程序名和参数
  - `7z.exe a -tzip $1 $2` → 程序名: `7z.exe`, 参数: `a -tzip $1 $2`
- **修复 2**: alias 未找到时，所有参数传递给 `[DefaultOpen]`
  - `piec o 7zip file.zip` → 执行 `7zFM "file.zip"`
- **改进**: 显示 alias 列表时同时显示具体命令

### 12. 输出格式优化
- **文件**: `src/pier-pkg.c`
- **调整**:
  - License 前空行移除
  - 确认提示 `(Y/N):` 前空行移除
  - 架构和 License 之间添加空行

### 13. 多语言支持 (2026-04-21)
- **文件**: `src/pier-pkg.c`
- **新增文件**: `profile.sque`, `notice.sque`
- **功能**:
  - 从 `LANGUAGE_DIR` 自动提取当前语言代码
  - `profile.sque`: 本地化 `PackageName`, `ProFile`, `Author`, `Distributor`
  - `notice.sque`: 本地化 `Notice` (支持多行)
- **Fallback 顺序**:
  1. 当前语言 (如 `zh-CN`)
  2. `en-US` (如果当前语言不存在)
  3. `metadata.sque` 默认内容

### 14. 元数据格式更新 (2026-04-21)
- **字段变更**:
  - `DisplayName` → `PackageName`
  - `Description` → `ProFile`
  - `License` → `Notice`
- **格式规范**:
  - `profile.sque`: `key: value` (单行)，可选字段，不写则使用 `metadata.sque` 默认值
  - `notice.sque`: 直接内容，支持多行，以 `::end` 结束
- **语言包安装**:
  - 固定安装到 `share\language\{InstallerName}`
  - 忽略 `[InstallDir]` 字段

### 15. 元数据文件结构 (2026-04-23)
- **`.metadata` 压缩包内容**:
  ```
  {package}.metadata (zip)
  ├── metadata.sque    # 主要元数据（中英文）
  ├── profile.sque     # 其他语言本地化（可选）
  └── notice.sque      # 其他语言许可证（可选）
  ```
- **metadata.sque 示例**:
  ```ini
  [PackageName]
  7-Zip

  [Version]
  26.00

  [ProFile]
  Windows 系统的文件压缩软件

  [Notice]
  MIT License
  Copyright (c) 2024
  ```
- **profile.sque 示例**:
  ```ini
  [ja-JP]
  PackageName: 7-Zip
  ProFile: Windows用ファイルアーカイバ

  [ko-KR]
  PackageName: 7-Zip
  ProFile: Windows용 파일 압축 프로그램
  ```
- **notice.sque 示例**:
  ```ini
  [ja-JP]
  MIT ライセンス
  著作権 (c) 2024
  ::end

  [ko-KR]
  MIT 라이선스
  저작권 (c) 2024
  ::end
  ```

## 核心组件

| 组件 | 功能 |
|------|------|
| `pier-pkg.exe` | 包安装/卸载 |
| `pier-op.exe` | 包启动 (支持别名) |
| `pier-ver.exe` | 版本检查 |
| `pier-arch.exe` | 架构检测、metadata解析 |
| `vecho.exe` | 彩色输出 |
| `sque.c` | SQUE 静态库（UTF-8/多行/本地化解析） |

## 别名系统

### 本地别名
- 位置: `metadata/{package}.sque`
- 使用: `piec o package`

### 第三方别名
- 位置: `metadata/alias/{first_letter}/{user}/{package}.sque`
- 使用: `piec o user/package`
- 下载: `alias_source/alias/{first_letter}/user/package.sque`

## 颜色代码 (vecho)
- `$white$` - 白色
- `$brightyellow$` - 亮黄色
- `$brightgreen$` - 亮绿色
- `$brightcyan$` - 亮青色
- `$brightwhite$` - 亮白色

### 16. MAX_ARGS 扩展至 9999 (2026-04-27)
- **文件**: `src/pier-op.c`
- **修改**: 将 `MAX_ARGS` 从 256 改为 9999
- **代码**:
  ```c
  // Line 31
  #define MAX_ARGS 9999
  ```
- **测试证据**: `test_replace_placeholders.c`
  - 编译: `tcc.exe -o test_replace_placeholders.exe test_replace_placeholders.c`
  - 运行结果:
    ```
    === Test replace_placeholders ===

    Test 1: $1 basic functionality
      PASS: Output='Value is first_arg'

    Test 2: $9999 high number
      PASS: Output='Last arg is arg_9999'

    Test 3: $0 should be rejected
      PASS: $0 correctly rejected, Output='Value is '

    Test 4: Multiple placeholders $1 and $9999
      PASS: Output='First=val_1, Last=val_9999'

    Test 5: $10000 should be rejected (beyond 9999 limit)
      PASS: $10000 correctly rejected (beyond limit)

    === Summary ===
    Passed: 5
    Failed: 0
    ```
- **结论**: $9999 能正确解析和替换，$1 功能未受影响，$0 和 $10000 被正确拒绝

## 技术债务与重构提案

### piec.bat 归档，主程序重构为 pier.exe (2026-04-27)

**状态**: `piec.bat` 已归档至 `archive/piec.bat.bak`，不再维护。新主程序为 `bin/pier.exe`。

**问题**: `piec.bat` 作为 800+ 行的 Windows Batch 脚本，存在以下不可维护的问题：
1. `for /F` 和 `set /P` 在路径含空格时行为异常
2. 临时文件（`tmp1.txt` ~ `tmp5.txt`）管理混乱
3. `sed` 外部依赖导致解析逻辑脆弱（换行符 CRLF/LF 问题）
4. 无错误码体系，流程控制依赖 `goto`
5. 参数传递受限（`%1`~`%9`），无法支持现代 CLI 体验

**方案**: 用纯 C 编写的 `pier.exe` 作为唯一主程序入口

#### 新架构

```
bin/pier.exe (唯一入口)
├── 初始化模块
│   ├── 自检测 PIER_ROOT（GetModuleFileName → 取父目录）
│   ├── 读取 etc/language.ini（原生 fopen，无 sed 依赖）
│   ├── 读取 etc/sourceimage.ini（原生 INI 解析）
│   └── 加载 lang.ini 字符串到内存哈希表
│
├── 命令路由模块
│   ├── install  → 调用 bin/pier-pkg.exe install ...
│   ├── remove   → 调用 bin/pier-pkg.exe remove ...
│   ├── search   → 调用 bin/pier-pkg.exe search ...
│   ├── list     → 下载 db.sque / dbm.sque，直接解析输出
│   ├── o        → 调用 bin/pier-op.exe（透传所有参数）
│   ├── sources  → 源管理（change / list）
│   ├── setlang  → 语言包管理
│   └── help     → 输出帮助文本
│
├── 工具函数库
│   ├── read_ini_field()      # 替代 sed -n "/\[field\]/{n:p}"
│   ├── download_file()       # 封装 uma-get.exe 调用
│   ├── execute_and_wait()    # 封装 CreateProcess
│   ├── color_printf()        # 替代 vecho.exe（内置 ANSI/WinAPI 着色）
│   └── confirm_prompt()      # Y/N 交互，支持 -y 自动确认
│
└── 输出模块
    ├── 本地化字符串查找（O(1) 哈希表，替代 30+ 次 sed 调用）
    └── 彩色输出（Windows Console API + ANSI escape sequences）
```

#### 关键改进

| 方面 | piec.bat (已归档) | pier.exe (新主程序) |
|------|------------------|---------------------|
| 路径空格 | 需 `PIER_ROOT_SHORT` 绕开 | 原生支持，无特殊处理 |
| INI 解析 | 30+ 次 `sed.exe` 子进程 | 单次加载到内存哈希表 |
| 临时文件 | 10+ 个 `tmp*.txt` | 零临时文件 |
| 错误处理 | `goto errorX` | 返回码 + 结构化错误枚举 |
| 参数数量 | 最多 `%9` | `argc/argv` 无限制 |
| 换行符敏感 | CRLF/LF 导致 `set /P` 异常 | `fgets` 统一处理两种换行 |
| 可测试性 | 无法单元测试 | 每个模块可独立测试 |

#### 实现计划

1. **Phase 1**: 提取 `piec.bat` 中所有 `sed` 调用，统计需支持的 INI 字段读取模式
2. **Phase 2**: 实现 `read_ini_field()` 和 `lang_hash` 模块，编写单元测试
3. **Phase 3**: 实现命令路由框架（`install`/`remove`/`list`/`o`/`sources`/`search`/`setlang`）
4. **Phase 4**: 集成 `color_printf()`，逐步替换 `vecho.exe` 调用
5. **Phase 5**: `pier.exe` 替换 `piec.bat`，`piec.bat` 归档至 `archive/piec.bat.bak`

#### 归档策略

- `piec.bat` → `archive/piec.bat.bak`（历史备份，不再维护）
- 所有现有子程序（`pier-pkg.exe`, `pier-op.exe`, `pier-arch.exe`, `uma-get.exe`）继续复用
- 配置文件格式（`sourceimage.ini`, `language.ini`, `lang.ini`）完全不变
- 用户调用方式：`pier install xxx`（直接运行 `bin/pier.exe`）

### 17. pier.exe 实现完成 (2026-04-27)
- **文件**: `src/pier.c`, `pier.exe` (根目录)
- **修改**: 用纯 C 实现 `pier.exe` 替换 `piec.bat`
- **功能**: 命令路由、INI 解析、语言字符串哈希表、彩色输出
- **代码位置**: `src/pier.c` (Lines 1-643)
- **关键函数**:
  ```c
  void detect_pier_root(void);                           /* GetModuleFileName 自检测 */
  int read_ini_field(const char *fp, const char *field, char *out, int size);  /* 替代 sed */
  int load_language_strings(const char *lang_dir);       /* 加载 lang.ini 到哈希表 */
  const char *get_lang(const char *key);                 /* O(n) 查找，替代 30+ sed 调用 */
  void color_printf(const char *color, const char *fmt, ...);  /* 替代 vecho.exe */
  int confirm_prompt(const char *message);               /* Y/N 交互，支持 -y */
  int execute_tool(const char *tool_name, const char *args);   /* CreateProcess，无 cmd/system */
  void get_system_arch(void);                            /* GetSystemInfo，无 pier-arch.exe */
  ```
- **关键改进**:
  - `execute_tool()`: 使用 `CreateProcess()` 直接启动子进程，**不经过 cmd.exe**，**不使用 system()**
  - `execute_tool()`: 临时将 `bin/` 目录添加到 PATH，确保子进程能找到 `unzip.exe` 等工具
  - `get_system_arch()`: 使用 `GetSystemInfo()` 直接获取架构，**不调用 pier-arch.exe**
  - `detect_pier_root()`: `pier.exe` 在根目录，只取一级父目录
  - `language.ini`: plain text 读取，非 INI 格式
- **Bug 修复**:
  - **问题**: `pier install 7zip` 报告 `错误：没找到该软件包: 7zip`
  - **根因**: `pier-pkg.exe` 内部使用 `system("unzip.exe ...")` 解压元数据，但 `unzip.exe` 在 `bin/` 目录下，不在系统 PATH 中
  - **修复**: `execute_tool()` 在 `CreateProcess()` 前将 `PIER_ROOT\bin` 添加到 PATH，子进程继承后能找到 `unzip.exe`
- **测试证据**: `test/test_pier.c`
  - 编译: `cd test && ..\tcc\tcc\tcc.exe -o test_pier.exe test_pier.c`
  - 运行结果: 9 PASSED, 0 FAILED
- **运行验证**:
  ```
  E:\backup\pier-2.0.0-beta1> pier ?
  pier - 软件包安装器 Windows(CMD) 版本 2.3.1
  ============================================================
    Package Installer by Sanakaprix
  ============================================================
  ... (help text) ...

  E:\backup\pier-2.0.0-beta1> pier install 7zip
  请稍候...
  包名: 7-Zip
  版本: 26.00
  包最低系统要求: Windows XP
  简介: 7-Zip 是一款适用于 Windows 系统的文件压缩软件。
  作者: Igor Pavlov.
  分发者: Sanakaprix
  架构: all
  是否继续安装该软件？ (Y/N):
  ```
- **归档**: `piec.bat` → `archive/piec.bat.bak`
- **结论**: pier.exe 完全替换 piec.bat，无 cmd.exe 依赖，无 system() 调用，无 sed 依赖，install 命令正常工作

---

## 设计原则

### 彩色输出
- **统一使用 `vecho.exe`**，所有 C 源文件（`pier.c`, `pier-pkg.c` 等）都通过 `CreateProcess` 调用 `vecho.exe` 进行彩色输出
- **不封装 `color_printf` 等函数**，直接调用 `vecho.exe`，例如：
  ```c
  // 正确做法 - 直接调用 vecho.exe
  snprintf(cmdline, sizeof(cmdline), "\"%s\\bin\\vecho.exe\" $brightgreen$%s: $brightwhite$%s", 
           g_pier_root, label, value);
  CreateProcess(...);
  
  // 错误做法 - 封装函数
  color_printf("brightgreen", "%s: ", label);  // 不要这样
  color_printf("brightwhite", "%s\n", value);  // 不要这样
  ```
- **一行内多颜色**：`vecho.exe` 支持一行内多次切换颜色，如 `$brightgreen$标签: $brightwhite$值`
- **例外**：`pier_debug.c` 保留原有实现，用于调试

---

## 项目文件树

```
pier-2.0.0-beta1/
├── CONTEXT.md              # 开发上下文文档
├── DEBUG_UPDATE.md         # 调试更新记录
├── LICENSE                 # 许可证文件
├── LICENSE.zip             # 许可证压缩包
├── NOTICE                  # 版权声明
├── README.md               # 中文说明文档
├── README_EN.md            # 英文说明文档
├── pier                    # 主程序入口脚本
├── pier.exe                # 主程序（C编译）
├── pier_verify.exe         # 验证工具
├── piercmd.bat             # 命令行入口
├── profile.sque.example    # 配置文件示例
├── tcc.zip                 # TCC编译器压缩包
│
├── app/                    # 已安装应用目录
│   ├── 7zip/               # 7-Zip 安装目录
│   ├── czadb/              # CZADB 工具目录
│   └── pier-ai/            # Pier AI 模块
│
├── archive/                # 归档文件
│   └── piec.bat.bak        # 旧版批处理备份
│
├── bin/                    # 工具程序目录
│   ├── pier-arch.exe       # 架构检测工具
│   ├── pier-hash.exe       # 哈希计算工具
│   ├── pier-op.exe         # 操作工具
│   ├── pier-pkg.exe        # 包管理工具
│   ├── pier-upd.exe        # 更新工具
│   ├── pier-ver.exe        # 版本工具
│   ├── uma-get.exe         # 下载工具
│   ├── unzip.exe           # 解压工具
│   ├── vecho.exe           # 彩色输出工具
│   ├── sed.exe             # 文本处理工具
│   └── ...                 # 其他依赖DLL
│
├── etc/                    # 配置目录
│   ├── hta.ini             # HTA配置
│   ├── language.ini        # 语言配置
│   ├── pierlist.sque       # 已安装包列表
│   └── sourceimage.ini     # 源镜像配置
│
├── metadata/               # 元数据目录
│   ├── alias/              # 别名映射
│   └── czadb/              # 包元数据缓存
│
├── share/                  # 共享数据目录
│   ├── cache/              # 下载缓存
│   ├── config/             # 用户配置
│   ├── language/           # 语言文件
│   │   ├── en-US/          # 英文语言包
│   │   └── zh-CN/          # 中文语言包
│   └── module/             # 功能模块
│
├── src/                    # 源代码目录
│   ├── pier.c              # 主程序源码
│   ├── pier-pkg.c          # 包管理源码
│   ├── pier-arch.c         # 架构检测源码
│   ├── pier-hash.c         # 哈希计算源码
│   ├── pier-op.c           # 操作工具源码
│   ├── pier-upd.c          # 更新工具源码
│   ├── pier-ver.c          # 版本工具源码
│   ├── pier_verify.c       # 验证工具源码
│   ├── sque.h              # SQUE 解析器头文件
│   ├── sque.c              # SQUE 解析器实现（静态库）
│   └── vecho.c             # 彩色输出源码
│
├── tcc/                    # TCC编译器目录
│   └── tcc/                # TCC子目录
│       ├── tcc.exe         # TCC编译器
│       ├── doc/            # 文档
│       ├── examples/       # 示例代码
│       ├── include/        # 头文件
│       │   ├── sys/        # 系统头文件
│       │   └── winapi/     # Windows API头文件
│       └── lib/            # 库文件
│
└── test/                   # 测试目录
    ├── test_pier.c         # 主程序测试
    └── test_sha256.c       # SHA256测试
```

---

## 18. UTF-8 全球语言支持 (2026-05-09)
- **文件**: `src/pier-pkg.c`, `src/pier-uni.c`
- **问题**: notice.sque 为 UTF-8 编码，但旧版 pier-pkg.exe 直接输出导致中文乱码；metadata.sque 为 GBK 编码，错误转换会损坏数据
- **诊断**: `debug2.c` hex dump 发现 metadata.sque 使用 `\r` 单字节行尾，ProFile 字段为 GBK 编码（`CA C7 D2 BB` = "是一款"）；notice.sque 为 UTF-8 编码
- **解决方案**: `is_valid_utf8()` 预检 + `utf8_to_acp()` 条件转换
  - UTF-8 数据 → `MultiByteToWideChar(CP_UTF8)` → `WideCharToMultiByte(CP_ACP)` → GBK 输出
  - GBK 数据 → `is_valid_utf8()` 返回 0 → 保持原样
- **代码位置**: `src/pier-pkg.c` Lines 282-335
  ```c
  /* Validate if a byte sequence is valid UTF-8 (C89, no external deps, XP safe) */
  int is_valid_utf8(const unsigned char *p) {
      while (*p) {
          if (*p < 0x80) {
              p++;
          } else if ((*p & 0xE0) == 0xC0) {
              if ((p[1] & 0xC0) != 0x80) return 0;
              p += 2;
          } else if ((*p & 0xF0) == 0xE0) {
              if ((p[1] & 0xC0) != 0x80) return 0;
              if ((p[2] & 0xC0) != 0x80) return 0;
              p += 3;
          } else if ((*p & 0xF8) == 0xF0) {
              if ((p[1] & 0xC0) != 0x80) return 0;
              if ((p[2] & 0xC0) != 0x80) return 0;
              if ((p[3] & 0xC0) != 0x80) return 0;
              p += 4;
          } else {
              return 0;
          }
      }
      return 1;
  }

  void utf8_to_acp(char *str, int max_len) {
      if (!str || !str[0]) return;
      if (max_len <= 1) return;
      if (!is_valid_utf8((const unsigned char *)str)) return;  // GBK 原样保持
      // ... UTF-8 → ACP 转换
  }
  ```
- **pier-uni.exe**: 新增工具，支持 stdin 管道模式和文件转换模式（类似 iconv）
  ```bash
  pier-uni.exe <file>   # 转换文件
  pier-uni.exe -        # 从 stdin 读取
  cmd | pier-uni.exe    # 管道模式
  ```
- **文件编码现状**:
  | 文件 | 编码 | 行尾 | 处理方式 |
  |------|------|------|---------|
  | metadata.sque | GBK | `\r` | `is_valid_utf8` 返回 0，原样保持 |
  | notice.sque | UTF-8 | `\r` | `is_valid_utf8` 返回 1，转换为 ACP |

## 19. SQUE 库深度重构 (2026-05-11)
- **文件**: `src/sque.h`, `src/sque.c`, `src/pier-pkg.c`, `src/pier-op.c`
- **问题**:
  1. **notice.sque 使用 `\r` 仅换行**（0x0D，无 0x0A），标准 `fgets` 不认→整文件作为一行→`sque_read` 只读到 `[zh-CN]`→返回-1→fallback 英文
  2. **UTF-8 乱码漏转**：`notice.sque` 为 UTF-8 编码，旧代码未做 UTF-8→ACP 转换
  3. **假分离**：`pier-pkg.c` / `pier-op.c` 中大量手动 `while(fgets) { if(strcmp(section,"xxx")...) }` 代码未复用 SQUE 解析器
- **修复**:
  1. `sque.c` 新增 `sque_fgets()` 自定义行读取器（字符级读取，同时支持 `\r` / `\n` / `\r\n`），替换全部 3 处 `fgets`+`trim_line`
  2. `is_valid_utf8` + `sque_utf8_to_acp` 从 `pier-pkg.c` 迁移至 `sque.c`（内部自动调用）
  3. `sque_read` 已支持多行拼接（`\n` 连接），直到 `::end`、下一 `[section]` 或 EOF
  4. `pier-pkg.c` `parse_metadata`: 11个字段改用 `sque_read(metadata_file, "Field", ...)` 统一读取
  5. `pier-pkg.c` `read_lang_string`: `fgets` 循环→ `sque_read(lang_file, key, ...)`
  6. `pier-pkg.c` 删除 `is_valid_utf8` / `utf8_to_acp`（已迁移至 sque.c）
  7. `pier-op.c` `read_lang_string` / `extract_alias_section` / `extract_defaultopen` / alias 列表显示：全部替换为 `sque_read`+`strtok` 内存解析
- **sque_fgets 实现**:
  ```c
  static char *sque_fgets(char *buf, int size, FILE *fp) {
      int c, pos = 0;
      while (pos < size - 1) {
          c = fgetc(fp);
          if (c == EOF) { if (pos == 0) return NULL; break; }
          if (c == '\r') {                      /* \r 停止 */
              c = fgetc(fp);
              if (c != '\n' && c != EOF) ungetc(c, fp);  /* 只吞 \r\n */
              break;
          }
          if (c == '\n') break;                 /* \n 停止 */
          buf[pos++] = (char)c;
      }
      buf[pos] = '\0';
      return buf;
  }
  ```
- **编译**: `tcc sque.c pier-pkg.c -o bin/pier-pkg.exe`, `tcc sque.c pier-op.c -o bin/pier-op.exe`
- **测试结果**: `pier install czadb` → notice 正确显示中文（3行）

## 20. `{version}` 占位符支持 (2026-05-11)
- **文件**: `src/pier-pkg.c` `get_pie_file()`
- **功能**: `[URL]` / `[pkgfile]` 字段中的 `{version}` 自动替换为实际版本号
- **示例**:
  ```ini
  [URL]
  x86: 7zip-{version}.pie (default)
  x64: 7zip-x64-{version}.pie
  ::end
  ```
  → `7zip-x64-26.00.pie`（x64系统）
- **完整解析逻辑**:
  1. 先读 `[pkgfile]`（向后兼容）
  2. 若不存在，读 `[URL]` 并按 `x86:`/`x64:`/`all:` 前缀匹配系统架构（使用 pier.exe 传入的 `g_sys_arch`）
  3. 移除 `(default)` 后缀
  4. 若都失败，fallback 到 `{name}.pie`
  5. 最后执行 `{version}` → 实际版本号替换

## 21. `[InstallDir]` 字段废止 (2026-05-10)
- **文件**: `src/pier-pkg.c`
- **变更**: 从 PackageInfo 结构体删除 `install_dir` 字段，所有安装路径使用 `InstallerName`
- **代码位置**: 结构体定义、`cmd_install`、`cmd_remove` 中的路径拼接

## 22. 退出消息移除 (2026-05-11)
- **文件**: `src/pier.c`, `share/language/zh-CN/lang.ini`, `share/language/en-US/lang.ini`
- **变更**: 完全移除退出时的告别消息（`vecho_line` + `get_lang("exit_message")`）
- pier.c `main()` 末尾不再输出任何退出消息
- 两个 lang.ini 中的 `[exit_message]` 段均已删除

## 23. `pier o` 中文显示修复 (2026-05-11)
- **文件**: `src/pier-pkg.c`, `src/pier-op.c`
- **问题**: `pier o czadb` 显示英文包名 "czawa ADB Tools" 而非中文 "czawa ADB 工具"
- **根因**: 
  - `parse_metadata()` 从 `share\cache\profile.sque` 和 `notice.sque` 读取中文内容并覆盖到 `g_packages[index]`，但从未写回磁盘
  - `download_metadata()` 和 `install_package()` 只复制了英文 `metadata.sque` 到 `metadata\{package}.sque`
  - `pier o` 读取 `metadata\{package}.sque` 得到的是纯英文内容
- **修复**:
  1. `pier-pkg.c` `parse_metadata()` 末尾增加代码：读取 `share\cache\metadata.sque`，将 `[PackageName]`、`[ProFile]`、`[ProFile_En]`、`[Notice]` 替换为已解析的中文值，写入 `metadata\{package}.sque`
  2. 移除 `download_metadata()` 和 `install_package()` 中的冗余 `CopyFileA`（会覆盖中文为英文）
  3. `pier-op.c` 下载 metadata 路径：读取 `profile.sque` 提取中文 PackageName/ProFile，读取 `notice.sque` 提取中文 Notice，合并写入 `metadata\{package}.sque`，并更新 `metadata_file` 指向合并后的文件

## 编译说明

所有源文件使用 **TCC (Tiny C Compiler)** 编译，编译器位于 `tcc/tcc/tcc.exe`。
所有 `.exe` 输出到项目根目录或 `bin/` 目录。

### 第一步：编译 SQUE 静态库
```batch
tcc\tcc\tcc.exe -o src\sque.obj -c src\sque.c
```

### 第二步：编译各组件
```batch
:: 主程序（依赖 sque.obj）
tcc\tcc\tcc.exe -o pier.exe src\pier.c src\sque.obj -lkernel32 -luser32 -lgdi32 -ladvapi32

:: 包管理器（依赖 sque.obj）
tcc\tcc\tcc.exe -o bin\pier-pkg.exe src\pier-pkg.c src\sque.obj -lkernel32 -luser32 -lgdi32 -ladvapi32

:: 操作工具（依赖 sque.obj）
tcc\tcc\tcc.exe -o bin\pier-op.exe src\pier-op.c src\sque.obj -lkernel32 -luser32 -lgdi32 -ladvapi32
```

**注意**：
- `sque.obj` 必须先编译，放在 `src/` 目录下
- `pier.exe` 放在项目根目录；`pier-pkg.exe`、`pier-op.exe` 放在 `bin/` 目录
- 编译前需确保目标 `.exe` 不被占用（`pier install` 进程未退出）

---

## 24. SQUE 条件语法解析引擎 & squecheck.exe (2026-05-11)
- **新增文件**: `src/sque_eval.h`, `src/sque_eval.c`, `src/squecheck.c`
- **新增数据文件**: `share/language/zh-CN/langsque.ini`, `share/language/en-US/langsque.ini`
- **功能**: 为 SQUE 配置文件引入单行条件语法 `if <条件> then <行为>[else <行为>];`
  - `sque_get_systemver()`: 通过 GetVersionExA 获取 Windows 版本（xp/7/8/8.1/10/11）
  - `sque_eval_line()`: 手写状态机解析条件行，支持 `==`/`!=`、集合比较 `!=("xp","7")`、else 分支
  - `sque_interpolate()`: 变量插值 `{version}` → 实际值
  - 全部大小写不敏感（关键字、变量名、引号内比较值）
  - `{systemver}` 和 `{os}` 为只读变量，`{version}` 和 `{packagesize}` 可被条件赋值
- **squecheck.exe**: 独立 SQUE 语法检查器
  - `pier sque -c <file>` 或 `pier sque check <file>` 路由调用
  - 自行解析 `langsque.ini`（UTF-8→ACP），不依赖 sque.h
  - 检测 7 类错误：缺分号、单等号、引号不匹配、括号不匹配、未定义变量、缺 then、只读赋值
  - 内置英文 fallback
- **编译**: `tcc -I src src/squecheck.c -lkernel32 -luser32 -o bin/squecheck.exe`

---

## 25. pier hash 命令 & SHA256 校验 (2026-05-11)
- **新增/修改文件**: `src/pier-hash.c`（重写），`src/pier.c`（添加路由），`src/pier-pkg.c`（HASH 校验），`src/sque.c`（match_key 改大小写不敏感）
- **新增数据文件**: `share/language/zh-CN/languext.ini`, `share/language/en-US/languext.ini`
- **功能**: `pier hash` 子命令，用于文件 SHA256 哈希计算和对比
  - `pier hash -g <file>` / `pier hash gene <file>`: 生成文件 SHA256
  - `pier hash -c <file1> <file2>` / `pier hash check <file1> <file2>`: 对比两个文件的 SHA256
  - 哈希匹配输出 `OK!`（绿色），不匹配输出 `FAIL!`（红色）
  - 对比使用 `_strnicmp`，大小写不敏感
  - 所有输出通过 vecho.exe（同 squecheck），中文消息通过 `vecho_acp()` 统一 UTF-8→ACP 转换
- **pier-hash.c**: 完整重写，集成 SHA256 实现 + vecho 输出 + languext.ini 本地化
  - `detect_pier_root()`: 自检测 pier 根目录
  - `vecho_line()` / `vecho_acp()`: CreateProcess 调用 vecho.exe
  - `load_lang_strings()`: 从 languext.ini 读取 7 个本地化 key
  - 无 printf/fprintf，全部通过 vecho 输出
- **languext.ini**: 新建扩展语言文件，包含 `hash_ok`、`hash_fail`、`hash_match`、`hash_mismatch`、`hash_gene`、`hash_error_open`、`hash_usage` 共 7 个 key
- **SQUE [HASH] 字段**: metadata.sque 支持 `[HASH]` 区段（`[Hash]`、`[hash]` 等大小写变体均可，sque.c 的 `match_key` 已改为 `_strnicmp`），格式 `sha256: <64位大写十六进制>`
  - pier-pkg 在 `parse_metadata()` 中读取 [HASH] 字段存储到 `PackageInfo.sha[65]`
  - 在 `install_package()` 解压前执行 SHA256 校验，不一致则中止安装
  - 无 [HASH] 字段时跳过校验（向后兼容旧包）
- **编译**:
  - `tcc -I src src/pier-hash.c -luser32 -o bin/pier-hash.exe`
  - sque.obj 重新编译（match_key 改为 _strnicmp）
- **修复 (2026-05-11)**: `cmd_hash()` 初始未传递 `g_language_dir` 给 pier-hash.exe，导致中文消息不生效。已添加 `--lang g_language_dir` 参数传递（[pier.c:L584-L587](file:///e:/backup/pier-2.0.0-beta1/src/pier.c#L584-L587)）

---

## 26. help.lang 更新 (2026-05-11)
- **修改文件**: `share/language/zh-CN/help.lang`, `share/language/en-US/help.lang`
- 中文版修复乱码问题（原文件编码错误）
- 两个语言版本均新增 `[Hash 命令]` 和 `[SQUE 语法检查]` 区段

---

## 27. XP TLS/SSL 兼容性修复 (2026-05-11, **已弃用 2026-05-16**)
- **状态**: pier-get.exe 已弃用，替换为 uma-get.exe（GNU Wget 1.17），详见 §37
- **修改文件**: `src/pier-get.c`, `src/curl_stubs.c`（新增）
- **问题**: Windows XP 的 Schannel 不支持 TLS 1.2 和 ALPN，连接 HTTPS 服务器时报 `SEC_E_UNSUPPORTED_FUNCTION`
- **最终方案**: **pier-get 改用 MinGW-w64 + 静态 OpenSSL 编译**，彻底解决
- **关键文件**:
  - `src/libcurl.a` (671KB) — curl 7.56.1，裁掉 HTTP/2/SSH/LDAP，只保留 HTTPS+OpenSSL
  - `src/libssl.a` + `src/libcrypto.a` — 来自 `mingw32/opt/lib/`
  - `src/curl_stubs.c` — MinGW ABI 桥接 + XP API 兼容桩
- **pier-get 编译** (MinGW-w64 i686，静态 + XP 目标):
  ```
  gcc -Isrc -static -static-libgcc -D_WIN32_WINNT=0x0501 -DWINVER=0x0501 \
      -L "mingw32\i686-w64-mingw32\lib" \
      src/pier-get.c src/sque-mingw.o src/curl_stubs.c \
      src/libcurl_xp.a mingw32/opt/lib/libssl.a mingw32/opt/lib/libcrypto.a \
      -lws2_32 -lwinmm -lcrypt32 -liphlpapi -lssp -lz \
      -lkernel32 -luser32 -lgdi32 -o bin/pier-get.exe
  ```
  - 产出: `bin/pier-get.exe` (~3MB)，**纯静态链接**
  - 系统 DLL 依赖：kernel32/user32/gdi32/advapi32/msvcrt/ws2_32/winmm
  - `-static -static-libgcc`：消除 libwinpthread-1.dll 依赖
  - `-D_WIN32_WINNT=0x0501 -DWINVER=0x0501`：防止源码生成 Vista+ API 引用
  - **注意**：`-D` 标志不能阻止静态库（libcurl_xp.a/libssl.a/libcrypto.a）中的引用——这些库编译时就已链接 Vista+ API。需要用 `curl_stubs.c` 提供桩函数覆盖（§35）。
  - **GetTickCount64 桩** (2026-05-13 更新)：
    ```c
    unsigned long long __stdcall GetTickCount64(void) {
        extern unsigned int __stdcall timeGetTime(void);
        return (unsigned long long)timeGetTime();
    }
    ```
    使用 `timeGetTime()`（winmm.dll, Win98+）代替 `GetTickCount()`（kernel32），避免 MinGW stdcall fixup 与 `--gc-sections` 冲突。
  - XP/Win7/Win10/Win11 全版本原生 TLS 1.2+
- **pier-get.c SSL 证书验证**:
  - 启动时自动检测 `bin\cacert.pem`（`find_ca_bundle()`）
  - 找到 → 使用 `CURLOPT_CAINFO`，完整证书链验证
  - 未找到 → 打印一次性警告，跳过验证（`VERIFYPEER=0`）
  - `bin\cacert.pem` 来源：`https://curl.se/ca/cacert.pem`（Mozilla CA 证书包）
- **pier-get.c 保留的特性**（对 OpenSSL 构建无害，向后兼容 Schannel 构建）:
  - ALPN 禁用 (`CURLOPT_SSL_ENABLE_ALPN = 0`)
  - XP 自动检测 + 兼容模式 (`is_windows_xp()`)
  - `-k`/`--insecure` 命令行参数
- **其他工具**: 继续用 TCC 编译（`tcc -Isrc ...`）

---

## 28. CREATE_NO_WINDOW XP 兼容性全面修复 (2026-05-12)
- **修改文件**: `src/pier-pkg.c`, `src/pier-op.c`, `src/pier-ver.c`, `share/language/zh-CN/lang.ini`, `share/language/en-US/lang.ini`
- **问题**: `CREATE_NO_WINDOW` (0x08000000) 是 Vista 新增标志，XP 不支持 → `CreateProcess` 返回 `ERROR_INVALID_PARAMETER`
- **影响范围**: 全项目 3 个文件共 6 处使用，导致：
  - `pier install` — unzip.exe 不启动，安装静默失败
  - `pier o/op/open` — pier-get.exe 不启动，无法获取元数据
  - `pier ver/version` — 无法检查版本更新

### pier-pkg.c 修复（3 处 + 安装验证）
| 位置 | 修复内容 |
|------|---------|
| `run_silent()` [L288](file:///e:/backup/pier-2.0.0-beta1/src/pier-pkg.c#L288) | 返回实际结果 + `ERROR_INVALID_PARAMETER` 时用 `0` 重试（XP fallback） |
| `capture_output()` [L347](file:///e:/backup/pier-2.0.0-beta1/src/pier-pkg.c#L347) | 同上 |
| `install_package()` [L1201](file:///e:/backup/pier-2.0.0-beta1/src/pier-pkg.c#L1201) | `CreateDirectoryA` 检查返回值 + `run_silent` 检查返回值 + `FindFirstFileA` 验证目录非空 + 先创建 `app/` 父目录 |
| SHA256 [L1219](file:///e:/backup/pier-2.0.0-beta1/src/pier-pkg.c#L1219) | 硬编码英文 → 语言字符串 |

### pier-op.c 修复（3 处）
| 位置 | 修复内容 |
|------|---------|
| `download_alias_template()` [L571](file:///e:/backup/pier-2.0.0-beta1/src/pier-op.c#L571) | pier-get 调用：XP fallback |
| `download_metadata()` pier-get [L609](file:///e:/backup/pier-2.0.0-beta1/src/pier-op.c#L609) | 同上 |
| `download_metadata()` unzip [L632](file:///e:/backup/pier-2.0.0-beta1/src/pier-op.c#L632) | unzip 调用：XP fallback |

### pier-ver.c 修复（1 处）
| 位置 | 修复内容 |
|------|---------|
| `check_version()` [L220](file:///e:/backup/pier-2.0.0-beta1/src/pier-ver.c#L220) | 版本检查 pier-get 调用：XP fallback |

### 新增语言字符串
- `error_install_dir_failed` — 无法创建安装目录
- `error_install_unzip_failed` — 解压失败
- `error_install_empty` — 安装目录为空
- `error_hash_failed` — 校验失败
- `error_verify_failed` — 安装验证失败
- zh-CN + en-US 均已添加

### 编译
```
tcc\tcc\tcc.exe -o bin\pier-pkg.exe src\pier-pkg.c src\sque.c -lkernel32 -luser32 -lgdi32 -ladvapi32
tcc\tcc\tcc.exe -o bin\pier-op.exe  src\pier-op.c  src\sque.c -lkernel32 -luser32 -lgdi32 -ladvapi32
tcc\tcc\tcc.exe -o bin\pier-ver.exe src\pier-ver.c src\sque.c -lkernel32 -luser32 -lgdi32 -ladvapi32
```

---

## 29. pier-get XP 偶尔崩溃 + 下载速度优化 (2026-05-12)
- **修改文件**: `src/pier-get.c`, `src/pier-pkg.c`
- **XP 崩溃（间歇性）**:
  - 导入表无 Vista+ API，崩溃为**多线程竞态**
  - 根因：`curl_easy_init()` 内部触发 `SSL_library_init()` 在 4 线程并发时非线程安全
  - 修复：`is_windows_xp()` 返回 true 时强制 `num_threads = 1`，消除竞态
- **下载速度**:
  - 新增 `-b` (batch) 模式：`pier-get.exe -b <lang> <url1> <out1> [<url2> <out2> ...]`
  - 同一进程内依次下载多个文件（减少进程创建 + TLS 握手的累积延迟）
  - `pier-pkg.c` 新增 `download_metadata_batch()`：install/remove 命令一次下载全部元数据
- **编译**:
  ```
  mingw32\bin\gcc.exe -Isrc -static -static-libgcc -D_WIN32_WINNT=0x0501 -DWINVER=0x0501 \
    -L "mingw32\i686-w64-mingw32\lib" \
    src\pier-get.c src\sque-mingw.o src\curl_stubs.c \
    src\libcurl_xp.a mingw32\opt\lib\libssl.a mingw32\opt\lib\libcrypto.a \
    -lws2_32 -lwinmm -lcrypt32 -liphlpapi -lssp -lz \
    -lkernel32 -luser32 -lgdi32 -o bin\pier-get.exe
  tcc\tcc\tcc.exe -o bin\pier-pkg.exe src\pier-pkg.c src\sque.c -lkernel32 -luser32 -lgdi32 -ladvapi32
  tcc\tcc\tcc.exe -o bin\pier-op.exe  src\pier-op.c  src\sque.c -lkernel32 -luser32 -lgdi32 -ladvapi32
  ```

---

## 30. 修复 pier o 命令 skip_section 逻辑缺陷 (2026-05-13)

### 问题
`pier o czadb` 报错 "无法获取包元数据"。

### 根因分析
- `pier-op.c` 和 `pier-pkg.c` 的 metadata 保存逻辑使用 `skip_section` 标志跳过已替换的 section 内容
- 当替换 `[PackageName]` 后，`skip_section` 被设为 1，等待遇到 `::end` 才重置
- **原始 metadata.sque（来自服务器）各 section 之间不使用 `::end` 分隔**，唯一的 `::end` 出现在很后面的 `[DefaultOpen]` section 末尾
- 导致从 `[PackageName]` 到 `[DefaultOpen]::end` 之间的所有内容（`[InstallerName]`、`[Version]`、`[OS]`、`[URL]` 等）全部被跳过
- 保存后的 `metadata/{package}.sque` 缺少 `[InstallerName]`，pier-op 无法读取安装路径

### 修复方案
- 修改 `skip_section` 重置条件：不仅是 `::end`，遇到 `[` 开头的行（新的 section header）也重置标志
- `pier-op.c` 和 `pier-pkg.c` 同步修复

### 修改文件
- `src/pier-op.c`: 第 302-314 行 skip_section 逻辑
- `src/pier-pkg.c`: 第 1175-1187 行 skip_section 逻辑

### 后续发现：parse_metadata 条件取反 bug（同一天）
修复 `skip_section` 后，`pier install czadb` 所有字段仍然为空。进一步排查发现 `pier-pkg.c` 主循环中调用 `parse_metadata()` 的条件写反了：

```c
// BUG: 成功下载的跳过，只对失败的调用 parse_metadata
if (g_packages[i].metadata_downloaded) continue;
parse_metadata(i);

// 修复：失败的跳过，只对成功下载的调用 parse_metadata
if (!g_packages[i].metadata_downloaded) continue;
parse_metadata(i);
```

`metadata_downloaded` 在 `download_metadata_batch()` 中设置为 1 表示下载成功，但条件把成功的跳过了，导致字段永远为空。两处均修复（install 流程第 1728 行 + remove 流程第 1784 行）。

---

## 31. 修复 pier o 缓存污染问题 (2026-05-13)

### 问题
`pier o 3` 打开了 `czadb` 程序，而不是报错 "包 3 不存在"。

### 根因分析
完整数据流：
1. `pier install czadb` → 用户取消 (N) → **未清理缓存** → `share\cache\metadata.sque` 残留 czadb 数据
2. `pier o 3` → 下载 `sources/3/3/latest.metadata` → HTTP 404
3. pier-get 把 404 HTML 当作文件保存 → 文件存在但非 zip
4. unzip 失败 → 新的 metadata.sque 未生成
5. **旧 czadb 的 metadata.sque 仍在 cache 中** → `GetFileAttributesA` 返回 TRUE → 误判下载成功
6. `metadata\3.sque` 被写入 czadb 的 InstallerName
7. `pier o 3` 打开 `app\czadb`

### 修复方案（三个层面）

**① 下载前清理旧缓存（pier-op.c）**
```c
remove("cache/metadata.sque");
remove("cache/notice.sque");
remove("cache/profile.sque");
```

**② 解压后验证内容有效性（pier-op.c + pier-pkg.c）**
```c
if (sque_read(metadata_file, "InstallerName", buf, sizeof(buf)) < 0 || strlen(buf) == 0) {
    return 0;
}
```

**③ 取消安装时清理缓存（pier-pkg.c）**
```c
if (!confirm_install()) {
    dir_remove(cache_dir);
    return 2;
}
```

### 修改文件
- `src/pier-op.c` `download_metadata()`: 缓存清理 + InstallerName 验证
- `src/pier-pkg.c` `download_metadata_batch()`: InstallerName 验证
- `src/pier-pkg.c` `main()`: 取消时缓存清理

---

## 32. pier-get 批处理 TLS 会话复用加速 (2026-05-13)

### 问题
`pier-get -b` 批处理模式宣称 "reusing TLS session"，但实际每次 `pier_get_download()` 都创建新 curl handle，TLS 会话缓存（per-handle）完全无法生效。"第一次慢，每一次慢"。

### 根因
libcurl 的 TLS 会话缓存绑定在 `CURL*` handle 上。同一 handle 多次请求同一 host 会自动复用。但当前代码每次 `curl_easy_init` → perform → `curl_easy_cleanup`。

### 修复: 持久化 curl handle + 强制 TLS 1.2

**① 新增 `pier_get_download_reuse()`**
- 接受外部持久化的 `CURL*` handle
- 直接用 GET + header callback 捕获 Content-Length（不再分离 HEAD+GET）
- 不调用 `curl_easy_cleanup`，handle 由调用者管理
- 第一个请求做 TLS 握手，后续全部复用会话

**② 批处理模式改造**
```c
shared_curl = curl_easy_init();
curl_easy_setopt(shared_curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2); // 跳过版本协商
// ... 设置公共 SSL/UA/FollowLocation 选项

for (i = 0; i < pair_count; i++) {
    pier_get_download_reuse(shared_curl, url, outfile, quiet);
}
curl_easy_cleanup(shared_curl);
```

### 测试结果
- 3 个 metadata 分离调用（3进程×2握手=6次）：**8.3s**
- 3 个 metadata 批处理+TLS复用（1次握手）：**1.9s**
- **加速 4.4 倍**

### 修改文件
- `src/pier-get.c`：新增 `#define CURL_SSLVERSION_TLSv1_2 6`、`pier_get_download_reuse()`、`main()` batch 分支重写

---

## 33. 核心设计原则：永远不调用第三方 uninstall.exe (2026-05-13)

### 规则
**Pier SHALL NOT invoke any third-party uninstall.exe during the remove/uninstall lifecycle.**

卸载流程由 pier 完全管理：直接删除目录、移除元数据、移除垫片。不允许委托给第三方卸载程序。

### 原因
- 第三方 uninstall.exe 可能执行不可控的副作用（注册表清理、对话框弹出、系统重启等）
- 与 pier 的声明式包管理理念冲突
- 用户体验不可预测

### 修改文件
- `src/pier-pkg.c` `remove_package()`：删除 `run_visible(uninstall_exe, "")` 调用块

---

## 34. pier remove 三个 bug 修复 (2026-05-13)

### 问题
1. **`$` 美元符号字面显示**：语言字符串中含 vecho 颜色码，与 C 代码 vecho_line 双重叠加
2. **释放空间 0.0 MB**：`calculate_folder_size()` 递归返回 MB，外层又 ×1048576 转字节，两次转换
3. **调用了 uninstall.exe**：详见 §33

### 修复
1. 去掉 `remove_space_usage`、`remove_space_usage_unit`、`remove_warning_text` 中的 `$color$` 码
2. 拆分 `calculate_folder_size_raw()`（返回 LONGLONG 字节）+ `calculate_folder_size()`（包装转 MB）
3. 删除 uninstall.exe 调用代码块

### 修改文件
- `share/language/zh-CN/lang.ini`、`share/language/en-US/lang.ini`
- `src/pier-pkg.c` `calculate_folder_size()` + `remove_package()`

---

## 35. run_silent 句柄不可继承导致 unzip 解压失败 (2026-05-13)

### 问题
`pier install 7zip` 下载成功但报 "解压失败：7-Zip"，而 `app\7zip\` 目录中文件实际已正确解压。

### 根因
`run_silent()` 中 `CreateFileA("NUL", ...)` 没有设置 `SECURITY_ATTRIBUTES`，句柄不可继承。但 `CreateProcess(..., TRUE, ...)` 指定了 `bInheritHandles=TRUE`，子进程拿到的 stdout/stderr 是无效句柄 → unzip 写入失败 → 非零退出码。

对比 metadata 解压代码和 `capture_output()` 都正确使用了 `sa.bInheritHandle = TRUE`。

### 修复
1. `run_silent()` [L304-306](file:///e:/backup/pier-2.0.0-beta1/src/pier-pkg.c#L304-L306)：增加 `SECURITY_ATTRIBUTES sa` 并设置 `sa.bInheritHandle = TRUE`
2. `install_package()` 两处 unzip 调用 [L1358](file:///e:/backup/pier-2.0.0-beta1/src/pier-pkg.c#L1358) [L1388](file:///e:/backup/pier-2.0.0-beta1/src/pier-pkg.c#L1388)：添加 `-q` 静默标志

### 修改文件
- `src/pier-pkg.c` `run_silent()` + `install_package()`

---

## 36. GetTickCount64 桩函数 — XP 缺失 + 链接器冲突修复 (2026-05-13)

### 问题
`pier-get.exe` 在 Windows XP 上报 "无法找到入口点 GetTickCount64"。

### 根因
`libcurl_xp.a` / `libssl.a` / `libcrypto.a` 静态库在编译时就引用了 `GetTickCount64`（Vista+ API），`-D_WIN32_WINNT=0x0501` 只能防止源码生成引用，无法消除已编译静态库中的引用。

此外，若桩函数内调用 `GetTickCount()`（同在 kernel32），MinGW 的 stdcall fixup 机制 (`_GetTickCount` → `_GetTickCount@0`) 会与链接器的 `--gc-sections` 冲突，导致 "defined in discarded section" 链接错误。

### 修复
`curl_stubs.c` 添加 `GetTickCount64` 桩，内部使用 `timeGetTime()`（winmm.dll，Win98+ 可用）而非 `GetTickCount()`：

```c
unsigned long long __stdcall GetTickCount64(void) {
    extern unsigned int __stdcall timeGetTime(void);
    return (unsigned long long)timeGetTime();
}
```

### 为何 timeGetTime 而非 GetTickCount
| 方案 | 问题 |
|------|------|
| 调用 `GetTickCount()` | MinGW stdcall fixup 被 `--gc-sections` 丢弃 → 链接失败 |
| 调用 `QueryPerformanceCounter` | 也在 kernel32，同样可能冲突 |
| 调用 `timeGetTime()` | 在 winmm.dll（已链接 `-lwinmm`），无 fixup 冲突 ✅ |

### 验证
```
objdump -x bin/pier-get.exe | Select-String "GetTick"
→ _GetTickCount64@0  (section 1, 本地函数, 不再是 IAT 导入)
→ __imp__GetTickCount@0 (kernel32 import, XP 支持)
→ 无 __imp__GetTickCount64 ✅
```

### 修改文件
- `src/curl_stubs.c` — 添加 `GetTickCount64` 桩（`timeGetTime` 实现）
- `CONTEXT.md` §27 — 更新 pier-get 编译说明和 DLL 依赖列表

---

## 37. 从 pier-get 切换到 uma-get (2026-05-16)

### 决策
pier-get.exe（curl + MinGW + OpenSSL 静态编译）在 Windows XP 上反复出现兼容性问题（GetTickCount64、TLS/Schannel、链接器符号冲突等），维护成本过高。决定**恢复使用 uma-get.exe**（GNU Wget 1.17）作为所有下载的后端。

### 理由
| | pier-get | uma-get |
|---|---|---|
| XP 兼容 | 需要桩函数、特殊编译、符号修复 | 原生支持（Win98+） |
| TLS | 依赖 OpenSSL 编译 + cacert.pem | 自带 OpenSSL（来自 wget 构建） |
| 编译 | MinGW-w64 i686 交叉编译，~30 个 .a 库 | 无需编译（预先构建的二进制文件） |
| 维护 | 每次安装新包都可能有兼容问题 | 稳定，多年来无问题 |
| 速度 | 多线程分段下载 | 单线程（约慢 2-3x） |

### 改动范围
- 4 个源文件，9 处调用全部切换
- `bin/pier-get.exe` 不再编译/使用
- `src/pier-get.c`、`src/curl_stubs.c`、`src/libcurl_xp.a` 保留不动
- 所有旧代码以 `/* OLD (pier-get): ... */` 注释备份
- `-D_WIN32_WINNT=0x0501 -DWINVER=0x0501` 编译标志仍有用（对 TCC 编译的其他文件）

### uma-get CLI 参数规范
```
uma-get.exe [-q|--show-progress] --timeout=N --tries=3 --no-check-certificate -O "输出" "URL"
```

| 标志 | 用途 |
|------|------|
| `-q` | 静默模式（metadata、db.sque、版本检查） |
| `-q --show-progress` | 静默 + 进度条（.pie 文件、更新包），`-q` 压掉 Length/Saving to 文字，`--show-progress` 强制显示进度条 |
| `--timeout=N` | 超时秒数：metadata 300、package 14400、version 15 |
| `--tries=3` | 失败重试 3 次 |
| `--no-check-certificate` | 跳过 SSL 证书验证 |
| `-O FILE` | **大写 O**，指定输出文件 |

### 修改文件
- `src/pier-pkg.c` — 5 处（download_metadata, download_metadata_batch→循环, download_package, search_packages/db.sque）
- `src/pier-op.c` — 2 处（download_alias, download_metadata）
- `src/pier-ver.c` — 1 处（版本检查）
- `src/pier-upd.c` — 1 处（更新下载）
- `CONTEXT.md` — §10、§27 更新

---

## 38. STARTF_USESTDHANDLES 缺失 hStdInput 导致 XP 上 unzip 崩溃 (2026-05-16)

### 问题
`pier install czadb` 在 XP 上下载成功但 unzip 解压失败（"解压失败：czawa ADB 工具"）。

### 根因
`run_silent()` 和 `download_metadata_batch()` 内联 unzip 代码中，`STARTF_USESTDHANDLES` 设置了 `hStdOutput`/`hStdError`，但 `si.hStdInput` 未赋值（NULL）。XP 上的 unzip.exe 启动时拿到无效 stdin 句柄 → 崩溃/非零退出。

**对比：** `download_metadata()` 内联 unzip（L921）早已正确设置了 `si_unzip.hStdInput = GetStdHandle(STD_INPUT_HANDLE);`，因此不受影响。

### 修复
2 处各加一行：
```c
si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
```

- [run_silent L314](file:///e:/backup/pier-2.0.0-beta1/src/pier-pkg.c#L314)
- [download_metadata_batch L1009](file:///e:/backup/pier-2.0.0-beta1/src/pier-pkg.c#L1009)

### 规则
**所有使用 `STARTF_USESTDHANDLES` 的地方必须同时设置三个句柄**：`hStdInput`、`hStdOutput`、`hStdError`，缺一不可。

### 修改文件
- `src/pier-pkg.c` — `run_silent()` + `download_metadata_batch()`

---

## 39. unzip 退出码 1 误报"解压失败"——退出码 + 文件存在双重检查 (2026-05-16)

### 问题
§38 修复 `hStdInput` 后，XP 上 unzip 不再崩溃，但 `pier install czadb` 仍提示"解压失败"。实际文件已正确解压（`unzip.exe` 返回退出码 1 表示成功但带非致命警告）。

### 根因
Info-ZIP `unzip.exe` 退出码约定：
| 退出码 | 含义 |
|--------|------|
| 0 | 成功，无警告 |
| 1 | 成功，有非致命警告（文件已全部解压） |
| 2 | 错误 |
| 3 | 严重错误 |

但调用方 `if (!run_silent(...))` 将任何非零退出码视为失败。

### 修复
两处解压调用点均改为"退出码 ≤ 1 + 文件存在验证"双重检查：

1. **应用包解压**：exit code > 1 才报失败，exit code 0/1 放行到已有的 `FindFirstFile` 文件验证（之前被 `return 0` 拦截永远到不了）
2. **语言包解压**：同上 + 新增文件存在验证（语言包原来完全没有）

```
unzip_ret = run_silent(unzip_exe, unzip_args);
if (unzip_ret > 1) { 报失败; return; }
if (!目录中有文件) { 报失败; return; }
// 通过 → 继续
```

### 修改文件
- `src/pier-pkg.c` — 语言包解压 + 应用包解压

### 规则
**unzip 退出码判断规则：0/1 = 成功，≥2 = 失败。同时必须验证目标目录中确实有文件。**

---

## 40. 为 uma-get (wget) 添加代理支持 (2026-05-16)

### 问题
用户设置 `http_proxy`/`https_proxy` 环境变量后，`pier install` 下载不稳定（有时 metadata 失败，有时软件包下载卡在 0%）。

### 根因
代码从未显式传递代理参数给 wget。虽然 GNU Wget 理论上会自动读取环境变量，但 Windows 版本通过 `CreateProcess` 继承环境时可能存在边界情况。

### 修复
新增 `build_wget_proxy_opts()` 函数，主动读取 `http_proxy`/`https_proxy` 环境变量，通过 wget 的 `-e` 参数显式传递。8 处 uma-get 调用点全部附加代理参数。

```c
void build_wget_proxy_opts(char *buf, int buf_size) {
    buf[0] = '\0';
    char *p = getenv("http_proxy");
    if (p && p[0]) snprintf(buf + strlen(buf), ..., " -e http_proxy=%s", p);
    p = getenv("https_proxy");
    if (p && p[0]) snprintf(buf + strlen(buf), ..., " -e https_proxy=%s", p);
}
```

示例（无代理时 `proxy_opts` 为空字符串，不影响原有行为）：
```
uma-get.exe -q --timeout=300 --tries=3 --no-check-certificate -e http_proxy=http://127.0.0.1:7897 -e https_proxy=http://127.0.0.1:7897 -O "..." "..."
```

### 修改文件
- `src/pier-pkg.c` — 4 处调用
- `src/pier-op.c` — 2 处调用
- `src/pier-ver.c` — 1 处调用
- `src/pier-upd.c` — 1 处调用

### 规则
**wget 代理必须显式传递**：依赖 wget 自动读取 Windows 环境变量不可靠，应主动通过 `getenv()` + `-e` 参数传递。

### 使用方法
用户只需照常设置环境变量即可：
```batch
set http_proxy=http://127.0.0.1:7897
set https_proxy=http://127.0.0.1:7897
pier install 7zip
```

---
## 41. build_wget_proxy_opts 添加 use_proxy=on (2026-05-16)

### 问题
用户设置 `http_proxy`/`https_proxy` 环境变量后，wget 仍不走代理，gh-proxy 公共反向代理不稳定（常卡 0%）。

### 根因
1. wget 的 `-e` 参数设置 `http_proxy` 时，需同时设置 `use_proxy=on` 才能启用代理。部分 wget 版本默认 `use_proxy=off`。
2. `etc/sourceimage.ini` 中 `pie_source` 配置了 `gh-proxy.org` 反向代理。gh-proxy 经常超时/不可用。

### 修复

#### 1. build_wget_proxy_opts 添加 use_proxy=on
4 个文件同步修改，设置代理时先执行 `-e use_proxy=on`：
```c
void build_wget_proxy_opts(char *buf, int buf_size) {
    char *http_proxy, *https_proxy;
    buf[0] = '\0';
    http_proxy = getenv("http_proxy");
    if (http_proxy && http_proxy[0]) {
        snprintf(buf + strlen(buf), buf_size - strlen(buf), " -e use_proxy=on -e http_proxy=%s", http_proxy);
    }
    https_proxy = getenv("https_proxy");
    if (https_proxy && https_proxy[0]) {
        if (buf[0] == '\0') {
            snprintf(buf + strlen(buf), buf_size - strlen(buf), " -e use_proxy=on");
        }
        snprintf(buf + strlen(buf), buf_size - strlen(buf), " -e https_proxy=%s", https_proxy);
    }
}
```
处理两种边界情况：
- 仅设 `http_proxy` — `use_proxy=on` 随 http_proxy 一起添加
- 仅设 `https_proxy`（无 http_proxy）— 先单独添加 `use_proxy=on`

#### 2. 移除 gh-proxy.org
`etc/sourceimage.ini` 中 `pie_source` 改为直连 GitHub Releases：
```ini
[pie_source]
https://github.com/steve372a/pier-repo/releases/download
```
代理走用户本地的 `http_proxy`/`https_proxy` 环境变量。

### 完整示例
```batch
set http_proxy=http://127.0.0.1:7897
set https_proxy=http://127.0.0.1:7897
pier install 7zip
```
实际执行的 wget 命令：
```
uma-get.exe -q --show-progress --timeout=14400 --tries=3 --no-check-certificate -e use_proxy=on -e http_proxy=http://127.0.0.1:7897 -e https_proxy=http://127.0.0.1:7897 -O "..." "..."
```

### 修改文件
- `src/pier-pkg.c` — `build_wget_proxy_opts()` 添加 `use_proxy=on`
- `src/pier-op.c` — 同上
- `src/pier-ver.c` — 同上
- `src/pier-upd.c` — 同上
- `etc/sourceimage.ini` — 移除 gh-proxy 前缀

### 规则
1. **`use_proxy=on` 是 wget 代理的必要开关**：仅设置 `http_proxy` 不够，需同时启用 `use_proxy=on`。
2. **最小修改原则**：不设代理时不产生额外参数，不影响原有行为。
3. **gh-proxy 不可用于生产**：公共反向代理服务不可靠，应使用用户自己的代理或直连。

*最后更新: 2026-05-16*

---

## 42. 移除主链路残留 system() 调用 (2026-05-19)

### 问题
`Pier` 主链路源码中仍残留多处 `system()`：
- `src/pier-op.c`：调用 `vecho.exe`，以及 `CreateProcessA()` 失败后的 `system(cmd)` fallback
- `src/pier-ver.c`：多处通过 `system()` 调用 `vecho.exe`
- `src/pier-upd.c`：`system("pause")` 和 `system("7za ... >nul 2>&1")`

这与 `pier.exe` 已确立的“无 `cmd.exe` / 无 `system()`”方向不一致，也会带来路径带空格、shell 转义和内建命令依赖问题。

### 修复

#### 1. `pier-op.c`
- 新增 `run_vecho()` helper，统一通过 `CreateProcessA()` 直接调用 `vecho.exe`
- 4 处 `system(vecho_cmd)` 全部替换为 `run_vecho()`
- `build_and_execute()` 删除 `system(cmd)` fallback，改为直接输出 `CreateProcessA()` 错误码

#### 2. `pier-ver.c`
- 新增 `run_vecho()` helper
- `print_highlight_ver()` 和所有版本提示/警告输出改为 `CreateProcessA()` 直接调用 `vecho.exe`

#### 3. `pier-upd.c`
- 新增 `pause_for_enter()`，替换所有 `system("pause")`
- 新增 `run_process_silent()`，通过 `CreateProcessA()` + `STARTF_USESTDHANDLES` + `NUL` 句柄重定向实现静默执行
- `7za.exe` 解压不再依赖 `>nul 2>&1`

### 验证
- `rg -n "\\bsystem\\s*\\(" src`
  - 结果仅剩 `src/pier.c` 注释中的 `system()` 文本，不再有实际调用
- 编译通过：
  - `tcc\tcc\tcc.exe -o bin\pier-ver.exe src\pier-ver.c -lkernel32 -luser32 -lgdi32 -ladvapi32`
  - `tcc\tcc\tcc.exe -o bin\pier-op.exe src\pier-op.c src\sque.c -lkernel32 -luser32 -lgdi32 -ladvapi32`
  - `tcc\tcc\tcc.exe -o bin\pier-upd.exe src\pier-upd.c -lkernel32 -luser32 -lgdi32 -ladvapi32`

### 规则
1. **主链路不再允许新增 `system()` 调用**
2. **调用外部程序优先使用 `CreateProcessA()`**
3. **`pause` / `cls` 等 shell 内建命令不得通过 `system()` 依赖 `cmd.exe`，应改为纯 C 或 Win32 API**
