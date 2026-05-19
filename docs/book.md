pier.c 是 Pier 的主程序入口。启动后先从可执行文件位置检测 PIER_ROOT，然后加载
etc/language.ini 获取语言目录，再读取语言目录下的 lang.ini 把翻译字符串加载到哈希表里。

接着调用 GetNativeSystemInfo 获取系统架构（x86/x64/arm/arm64），从 etc/sourceimage.ini
读取软件源 URL 和 PIES URL。一切就绪后遍历命令行参数检测 -y 标志支持自动确认。

输出欢迎信息后根据第一个参数路由到对应的命令处理函数：

  install → cmd_install()   把参数拼接成 "install root lang src pies 是否自动确认 架构 包名..."
  remove  → cmd_remove()    类似 install，传给 pier-pkg
  search  → cmd_search()    传给 pier-pkg
  list    → cmd_list()      传给 pier-pkg
  o       → cmd_open()      传给 pier-op
  sources → cmd_sources()   传给 pier-pkg
  setlang → cmd_setlang()   直接写 etc/language.ini，告诉用户改了语言
  help    → cmd_help()      输出帮助信息

所有这些命令处理函数都通过 execute_tool() 启动对应的 bin/ 下 exe。execute_tool 做的事情是：
用 CreateProcess 创建子进程，构造一个自定义环境块，在 PATH 最前面插入 bin/ 目录，这样
子进程可以直接调用 vecho.exe、pier-get.exe、unzip.exe 等同目录下的工具。环境块保留了
父进程的所有环境变量。进程创建后等它结束，返回退出码。

退出前输出一句感谢语（通过 vecho.exe 彩色输出），然后返回退出码。

pier.c 还内置了 read_ini_field() 函数来读取 INI 配置文件中的字段值，替代 sed。

---

pier-op.c 是 "o" 命令的处理器（打开/运行已安装软件）。它接收 PIER_ROOT、语言目录、软件源、
包名和可选别名参数。首先判断包名是否是第三方别名格式（user/package），如果是则优先检查本地
metadata/alias/ 目录下是否有缓存，没有就用 pier-get 从 ALIAS_SOURCE 下载别名模板。然后解析
别名模板中的 ToUse 字段找出实际包名，再从实际包名的 metadata.sque 中提取 [DefaultOpen] 程序
路径或 [Alias] 下指定别名的命令行。最后通过 CreateProcess 执行目标程序，执行前用
replace_placeholders 把命令行模板中的 $1 $2 等占位符替换为用户传递的额外参数，未消耗的
参数会拼接到命令末尾。ALIAS_SOURCE 硬编码为 https://steve372a.github.io/pier-repo，用户
无法修改。

---

pier-pkg.c 是包管理核心，处理 install、remove、search、list、sources 五条命令。通过 main
解析参数：install 需要 PIER_ROOT、语言目录、source_url、pies_url、是否自动确认、系统架构、
包名列表；remove 类似但不用 pies_url 和架构；search 接收四个参数后调用 search_packages
遍历远程 db.sque 返回匹配结果。

install 流程：download_metadata 调用 pier-get 下载 xxx.metadata → unzip 解压出 metadata.sque、
notice.sque、profile.sque → parse_metadata 解析 [PackageName][Version][OS][InstallerName]
[URL][Architecture][ProFile][ProFile_En][Author][Distributor][PackageSize][Notice]
等字段到 PackageInfo 结构体 → 展示包信息（通过 vecho.exe 彩色输出），用户确认后 download_package
调用 pier-get 下载 .pie 安装包并用 SHA256 校验，然后 install_package 用 unzip 解压到 app/目录，
写入 pierlist.sque 注册已安装状态，复制 metadata.sque 到 metadata/ 目录。

remove 流程：检查 pierlist.sque 是否有该包 → 显示包信息和使用空间 → 用户确认后删除 app/ 下
目录，从 pierlist.sque 移除条目，删除 metadata/ 下的元数据文件。

内置完整 SHA256 实现（支持 Windows XP），用于校验下载的 .pie 安装包完整性。支持多包批量操作，
pier install 7zip vim 会依次处理每个包。搜索功能通过远程服务器的 db.sque 文件检索包名匹配项，
用 vecho.exe 彩色显示结果。

scuts 快捷方式功能：install_package() 安装成功后自动在 %PIER_ROOT%\scuts\ 目录下创建
包名.bat 文件，内容为 @echo off / cd /d %~dp0.. / pier.exe o 包名 %*。%~dp0 动态定位
pier 根目录，挪动 pier 目录后快捷方式依然有效。创建 bat 后调用 check_path_has_pier() 读取
HKCU\Environment 注册表 PATH，若不含 pier 目录则提示用户运行 pier updpath。

remove_package() 卸载成功后会删除对应的 scuts/包名.bat，并输出提示。

pier updpath 命令：读取 HKCU\Environment 注册表 PATH，用 strstr 找到旧的 \scuts 路径段，
用 GetModuleFileName 获取当前 pier 根目录计算出新路径，memmove 替换后通过 RegSetValueExA
写回注册表。XP 兼容，无 Vista+ API。

---

pier-get.c 是多线程分段下载器（独立 EXE），替换 uma-get(wget) 和 CURL。
使用 LIBCURL.DLL (7.64.1 Schannel 后端) 实现 TLS 1.2+ HTTPS 下载，XP SP3 到 Win11 全兼容。

用法：pier-get.exe <url> <output_file> [threads]
  url       - 下载地址（支持 HTTP/HTTPS）
  output    - 输出文件完整路径
  threads   - 分段线程数（默认 4，最大 16）

核心流程：
1. get_content_length() 发送 HEAD 请求获取文件大小（CURLOPT_NOBODY + FOLLOWLOCATION）
2. 已知大小 → 计算每段 Range，创建多个线程并行下载，fseek 定位写入
3. 未知大小 → 单线程全量下载（回退模式）
4. 全局进度计数器用 InterlockedExchangeAdd 跨线程安全累加
5. 终端进度条：已知大小时显示 [====  ] 30% (1.5MB/5.1MB) 格式
   未知大小时显示旋转指示器 |/-\ 配合已下载字节数

技术要点：
- 无需 curl.h，全部 curl API 通过手工声明（C89 兼容）
- write_cb 回调返回 written * size 保证 libcurl 正确计数
- CURLOPT_FAILONERROR 使 HTTP 4xx/5xx 触发错误
- CURLOPT_TIMEOUT 60s / CURLOPT_CONNECTTIMEOUT 15s 防止卡死
- User-Agent "pier-get/2.4.0"
- TCC 编译需 libcurl.def 链接 LIBCURL.DLL

调用方：
- pier-pkg.c  download_metadata/download_package/search_packages
- pier-op.c   download_alias_template/download_metadata
- pier-upd.c  自更新下载
- pier-ver.c  远程版本检查（下载到临时文件读取）
