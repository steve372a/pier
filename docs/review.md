

## 快捷方式功能提案（待评审）

### 需求描述

在安装时就为每个已安装软件生成一个快捷 `.bat` 文件到 `%PIER_ROOT%\scuts\` 目录，
用户执行一次 `setx PATH "%PATH%;%PIER_ROOT%\scuts"` 后，即可直接通过包名运行软件：

```
pier install python
# 安装时自动生成 %PIER_ROOT%\scuts\python.bat，内容：
#   @echo off
#   cd /d %~dp0..
#   pier.exe o python %*

# 用户只需执行一次（一次性操作）：
#   setx PATH "%PATH%;%PIER_ROOT%\scuts"

# 之后任意位置都能直接运行：
#   python script.py
#   7z file.zip
```

注：setx 仅作为界面提示，用户需手动执行一次。

### 优点

1. **极简用户体验**：安装后直接 `python` / `vim` / `git`，无需记忆 `pier o` 前缀
2. **隔离性强**：所有快捷方式集中在 `scuts/` 目录，setx 后不影响系统其他命令
3. **可逆性好**：删除 `%PIER_ROOT%\scuts\` 中的 .bat 文件即可撤销某个快捷方式
4. **永久生效**：setx 修改注册表 PATH，重启 CMD/PowerShell 后依然有效
5. **实现简单**：install_package() 加几行 fopen，remove_package() 加 remove() 即可

### 缺点

1. **setx 一次性门槛**：用户需手动执行一次 setx 命令
2. **全局命名空间冲突**：pier 的 python.bat 和系统 python.exe 可能冲突
3. **pier 需在固定位置**：bat 用 `cd /d %PIER_ROOT%` 跳回 pier 根目录执行
4. **remove 时需同步删 .bat**：目前 remove_package() 尚未实现此逻辑

### 可行性分析

| 方面 | 评估 |
|------|------|
| 技术难度 | 低：install_package() 加几行 fopen，remove_package() 加 remove() 即可 |
| XP 兼容性 | 高：setx 是 XP 内置命令，cd /d 也支持 |
| 命名冲突 | 中：需文档说明优先级概念 |
| 多用户 PC | 低：setx 改 HKCU PATH，不影响其他用户 |
| 卸载清理 | 低：remove_package() 需同步删除 scuts/包名.bat |
| pier 迁移 | 中：挪动 pier 目录后需重新 setx |

### bat 内容设计

```
@echo off
cd /d %~dp0..
pier.exe o 包名 %*
```

- `%~dp0` 是 bat 自己所在的目录（`scuts\`），`cd /d %~dp0..` 跳到 pier 根目录
- 不依赖 PIER_ROOT 环境变量，pier 挪位置后快捷方式依然有效
- `%*` 把所有参数透传给 pier o
- 不封装函数，直接写最直观的代码

### 实现计划

1. `install_package()` 中：创建 `scuts/` 目录，写入 `scuts/包名.bat`
2. `remove_package()` 中：同步删除 `scuts/包名.bat`
3. 安装成功后 vecho 提示用户：已创建快捷方式、检测 PATH 是否含 pier 目录，若无则提示可运行 pier updpath
4. `pier updpath` 命令：读取注册表 PATH，替换旧的 pier 路径为当前 pier 路径，用 GetModuleFileName 获取新路径，用 setx 写回注册表

### 工作流程

#### 安装时
```
pier install python
→ 创建 scuts/python.bat
→ vecho: "已创建快捷方式：python"
→ 检测注册表 PATH 是否含 pier 目录：
  → 若有：无声
  → 若无：vecho: "快捷方式已创建，但 PATH 中未检测到 pier 目录，如需使用请运行 pier updpath"
```

#### 卸载时
```
pier remove python
→ 删除 app/python/
→ 删除 scuts/python.bat
→ vecho: "已删除快捷方式：python"
```

#### 更新 PATH（挪动 pier 目录后）
```
pier updpath
→ 读取 HKCU\Environment 的 PATH
→ 找到旧的 pier 路径，替换为当前 GetModuleFileName 获得的新路径
→ setx PATH "新PATH"
→ vecho: "PATH 已更新，请关闭当前窗口再重新打开使新 PATH 生效"
→ 若 PATH 中本无 pier 目录：vecho: "未在 PATH 中找到 pier 目录，无需更新"
```

### lang.ini 新增条目

| key | 说明 |
|-----|------|
| `scut_created` | 已创建快捷方式： |
| `scut_not_in_path` | 快捷方式已创建，但 PATH 中未检测到 pier 目录... |
| `scut_removed` | 已删除快捷方式： |
| `updpath_done` | PATH 已更新，请关闭当前窗口再重新打开使新 PATH 生效。 |
| `updpath_not_found` | 未在 PATH 中找到 pier 目录，无需更新。 |