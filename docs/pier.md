
pier.c 确实存在一个极其严重的底层 Bug，这是导致你删除 7zip.metadata 后，uma-get 和 unzip 全部静默崩溃（从而提示找不到包）的罪魁祸首。同时还有传参丢弃、架构检测不准确等多个问题。
我已全局为你做了分析，以下是出问题的点以及修改方案：
1. 最致命的问题：pier.c 破坏了环境变量块
在 pier.c 的 execute_tool 函数中，试图给子进程（如 pier-pkg.exe）注入 PATH 环境变量。但构造环境块时，错误地使用了 strlen(env_block)：
code
C
// 错误代码
char *dst = env_block + strlen(env_block);
原因：环境块是一个以双 \0 结尾的字符串列表（形如 A=1\0B=2\0\0），strlen 遇到第一个 \0 就会停止。这导致后面的变量永远在覆盖第一个变量。最终子进程（pier-pkg）获取到的环境块是完全损坏的，丢失了所有环境（包括 PATH、SystemRoot）。当它尝试调用 uma-get.exe 或 system() 调用 cmd.exe 时，系统连 DLL 或 cmd 都找不到，直接静默失败，所以你啥都看不到。
修复方法（修改 pier.c 中的 execute_tool）：
code
C
/* 重新正确的构造环境块 */
        char *dst = env_block;
        int found_path = 0;
        for (i = 0; env[i] != '\0'; ) {
            int var_len = strlen(&env[i]);

            if (_strnicmp(&env[i], "PATH=", 5) == 0) {
                /* Modify PATH: prepend bin directory */
                snprintf(dst, env_size - (dst - env_block),
                         "PATH=%s\\bin;%s", g_pier_root, &env[i] + 5);
                found_path = 1;
            } else {
                strcpy(dst, &env[i]);
            }

            dst += strlen(dst) + 1; /* 安全地越过字符串及其 \0 */
            i += var_len + 1;
        }

        /* If no PATH found, add one */
        if (!found_path) {
            snprintf(dst, env_size - (dst - env_block),
                     "PATH=%s\\bin", g_pier_root);
            dst += strlen(dst) + 1;
        }
        *dst = '\0'; /* 补充双 \0 结尾 */
2. 参数被直接抛弃（导致多包和 -y 无效）
在 pier.c 的 cmd_install 和 cmd_remove 等命令中，只传了 argv[2] 给 pier-pkg，这就导致诸如 pier install 7zip vim -y 这类命令只有 7zip 生效，其他参数全被扔了。
修复方法（重写 pier.c 中的这三个函数）：
code
C
int cmd_install(int argc, char *argv[]) {
    char args[MAX_CMD_LEN];
    char full_source_url[MAX_PATH_LEN];
    char full_pies_url[MAX_PATH_LEN];
    int i, pos = 0;

    if (argc < 3) {
        printf("%s\n", get_lang("error_no_package"));
        return 2;
    }

    snprintf(full_source_url, sizeof(full_source_url), "%s/sources", g_source_url);
    snprintf(full_pies_url, sizeof(full_pies_url), "%s", g_pies_url);

    pos = snprintf(args, sizeof(args),
             "install \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"",
             g_pier_root, g_language_dir, full_source_url, full_pies_url,
             g_autoyes ? "y" : "n", g_sys_arch);

    for (i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-y") == 0 || strcmp(argv[i], "--yes") == 0) continue;
        if (pos < sizeof(args) - 1) {
            int w = snprintf(args + pos, sizeof(args) - pos, " \"%s\"", argv[i]);
            if(w > 0 && w < sizeof(args) - pos) pos += w;
            else pos = sizeof(args) - 1;
        }
    }
    return execute_tool("pier-pkg", args);
}

int cmd_remove(int argc, char *argv[]) {
    char args[MAX_CMD_LEN];
    char full_source_url[MAX_PATH_LEN];
    int i, pos = 0;

    if (argc < 3) {
        printf("%s\n", get_lang("error_no_package_remove"));
        return 2;
    }
    snprintf(full_source_url, sizeof(full_source_url), "%s/sources", g_source_url);

    pos = snprintf(args, sizeof(args), "remove \"%s\" \"%s\" \"%s\" \"%s\"",
             g_pier_root, g_language_dir, full_source_url, g_autoyes ? "y" : "n");

    for (i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-y") == 0 || strcmp(argv[i], "--yes") == 0) continue;
        if (pos < sizeof(args) - 1) {
            int w = snprintf(args + pos, sizeof(args) - pos, " \"%s\"", argv[i]);
            if(w > 0 && w < sizeof(args) - pos) pos += w;
            else pos = sizeof(args) - 1;
        }
    }
    return execute_tool("pier-pkg", args);
}

int cmd_open(int argc, char *argv[]) {
    char args[MAX_CMD_LEN];
    char full_source_url[MAX_PATH_LEN];
    int i, pos;

    if (argc < 3) {
        printf("%s\n", get_lang("error_no_package"));
        return 2;
    }
    snprintf(full_source_url, sizeof(full_source_url), "%s/sources", g_source_url);

    pos = snprintf(args, sizeof(args), "\"%s\" \"%s\" \"%s\" \"%s\" \"%s\"",
             g_pier_root, g_language_dir, g_source_url, full_source_url, argv[2]);

    for (i = 3; i < argc; i++) {
        if (pos < sizeof(args) - 1) {
            int w = snprintf(args + pos, sizeof(args) - pos, " \"%s\"", argv[i]);
            if(w > 0 && w < sizeof(args) - pos) pos += w;
            else pos = sizeof(args) - 1;
        }
    }
    return execute_tool("pier-op", args);
}
3. 架构识别在 64 位系统上误报
XP 下 32 位的 pier.exe 在 64 位机器上运行（WOW64），调用普通的 GetSystemInfo 会返回 x86，这会下错安装包。
修复方法（替换 pier.c 的 get_system_arch）：
code
C
void get_system_arch(void) {
    SYSTEM_INFO si;
    typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
    PGNSI pGNSI = (PGNSI)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetNativeSystemInfo");
    
    if (pGNSI) {
        pGNSI(&si);
    } else {
        GetSystemInfo(&si);
    }

    switch (si.wProcessorArchitecture) {
        case 9:  strcpy(g_sys_arch, "x64"); break;
        case 5:  strcpy(g_sys_arch, "arm"); break;
        case 12: strcpy(g_sys_arch, "arm64"); break;
        case 0:  
        default: strcpy(g_sys_arch, "x86"); break;
    }
}
4. pier-pkg.c 中调用 Unzip 创建了无权限的 NUL 句柄
在 pier-pkg.c 的 download_metadata 中，为了让 unzip 静默，创建了 NUL 句柄，但并没有给句柄赋予子进程继承属性，这使得 CreateProcess 虽然传了 TRUE，但子进程依然拿到了无效句柄而可能卡死/报错。
修复方法（找到 pier-pkg.c 中的 CreateFile("NUL"... 处，加上继承声明）：
code
C
SECURITY_ATTRIBUTES sa;
            sa.nLength = sizeof(SECURITY_ATTRIBUTES);
            sa.bInheritHandle = TRUE;
            sa.lpSecurityDescriptor = NULL;

            /* 使用 &sa 替换原来的 NULL */
            hNull = CreateFile("NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
5. 其他隐患补丁 (路径含空格断连问题)
pier.c 的 cmd_setlang 写死了相对路径，导致设置错乱：应改为 snprintf(lang_path, sizeof(lang_path), "%s\\etc\\language.ini", g_pier_root);，再由 fopen 打开该绝对路径。
pier_upd.c 中的 system() 语句路径漏了包引号，若是安装在带空格目录会导致自更新彻底崩溃，请找到其中下载指令处，改成使用短路径的写法：
code
C
char short_root[MAX_PATH_LEN];
    GetShortPathNameA(pier_root, short_root, MAX_PATH_LEN);
    snprintf(cmd, sizeof(cmd), "%s\\bin\\uma-get.exe -q --show-progress -P \"%s\" --no-check-certificate \"https://github.com/steve372a/pier/releases/download/v%s/pier-%s.zip\"", 
             short_root, temp_dir, new_version, new_version);
总结：你说的“完全不执行也不报错”的原因就在第1点环境块污染。修改掉 pier.c 中这几个错误之后，工具流链就可以正常流转了。