
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#include "stdc.h"

lang_cb                         P_lang;

static log_cb                   g_cb_log = (log_cb)-1;              // 默认 (-1) stdout 输出
static bool                     g_tag_separate = false;
static char                     g_log_name[64] = {0};               // 日志名称（从可执行文件名提取）

#ifdef LOG_INSTRUMENT

// 典型以太网 MTU=1500，减去 IP(20) + UDP(8) 头部，保守取 1400
#define INST_UDP_MAX            1400
#define INST_HDR_SIZE           7                                   // rid(2)+seq(2)+type(1)+chn(1)+tag_len(1)
#define INST_PAYLOAD_MAX        (INST_UDP_MAX - INST_HDR_SIZE)      // 可用数据区 (tag + text)
#define INST_WINDOW_SIZE        64                                  // 接收端滑动窗口大小（必须为 2 的幂）
#define INST_WINDOW_MASK        (INST_WINDOW_SIZE - 1)

// 组播地址：239.255.77.77 (自定义本地管理组播地址)
// - 239.0.0.0/8 为本地管理组播范围 (RFC 2365)
// - 使用组播而非单播的原因：SO_REUSEPORT 对单播是负载均衡（只有一个进程收到），
//   而组播可确保本机所有监听进程都能收到消息
#define INST_MCAST_ADDR         0xEFFF4D4D                          // 239.255.77.77

// 运行和状态
static volatile bool            g_inst_running = false;
static thd_t                    g_inst_thread  = 0;

// 单 socket 用于收发（bind 到 INSTRUMENT_PORT）
// 首次调用时自动初始化，进程退出时自动关闭
static uint16_t                 g_inst_rid    = 0;                  // 本节点随机 ID
static sock_t                   g_inst_sock   = P_INVALID_SOCKET;
static struct sockaddr_in       g_inst_dest;
static uint16_t                 g_inst_port   = INSTRUMENT_PORT;    // 可通过 instrument_port() 修改
static uint16_t                 g_inst_seq    = 0;

// 接收线程相关
typedef struct {
    uint8_t data[INST_UDP_MAX + 1];                  // +1 供交付时追加 '\0'
    int len;                                         // 0 = 空槽
    uint16_t rid;                                    // 发送方 rid
} inst_slot_t;

// Per-RID 序号追踪（支持多发送方）
#define INST_MAX_SENDERS        8
typedef struct {
    uint16_t rid;
    uint16_t next_seq;
    bool synced;
} inst_sender_t;
static inst_sender_t            g_inst_senders[INST_MAX_SENDERS];
static int                      g_inst_sender_cnt = 0;

static instrument_cb            g_inst_cb     = NULL;
static uint16_t                 g_inst_options_base = 0;
static TLS int                  g_inst_in_cb  = 0;                  // 防止回调递归
static inst_slot_t              g_inst_win[INST_WINDOW_SIZE];
static uint16_t                 g_inst_next   = 0;                  // 下一个期望 seq（全局，用于窗口索引）
static bool                     g_inst_synced = false;              // 首包同步标记（已废弃，保留兼容）

// 广播模式
typedef enum {
    INST_MODE_LOCAL  = 0,   // 只本地回调，不网络
    INST_MODE_HOST   = 1,   // 本地回调 + 127.0.0.1（默认，同一主机可见）
    INST_MODE_REMOTE = 2,   // 本地回调 + 局域网广播
} inst_mode_e;
static inst_mode_e              g_inst_mode   = INST_MODE_HOST;     // 默认主机模式
static uint32_t                 g_inst_keep_chn[8] = {0};           // 保留通道 bitset (256位)

// 选项 bitset
static uint8_t*                 g_inst_bits     = NULL;             // 动态分配
static uint16_t                 g_inst_bits_len = 0;                // 已分配字节数

// 前向声明
static void inst_send_buf(uint8_t chn, char* buf, int tag_len, int text_len);

#define LOG_HDR_RESERVE         INST_HDR_SIZE                       // g_line 预留的 header+tag 空间

#else
#define LOG_HDR_RESERVE         0
#endif

#ifdef printf
#undef printf
#endif

///////////////////////////////////////////////////////////////////////////////

/**
 * @brief                       规范化目录路径（原地修改）
 *                              1. 替换 ~ 为 HOME 目录
 *                              2. 移除末尾的 / (根目录除外)
 *                              3. 合并连续的 //
 *                              4. 处理 /./ 为 /
 * @note                        仅当 ~ 展开时分配新内存
 *                              注意，这里分配的内存，其生命周期与进程一致，所以无需释放
 */
static char* normalize_dir_path(char* path) {
    if (!path || !*path) return path;

    char* result = path;

    // 处理 ~ 开头的路径（需要分配新内存）
    if (path[0] == '~' && (path[1] == '/' || path[1] == '\0')) {
        const char* home = getenv("HOME");
        if (!home) home = "/tmp";  // fallback
        result = (char*)malloc(strlen(home) + strlen(path));  // len 已包含 ~，足够
        strcpy(result, home);
        if (path[1] != '\0') strcat(result, path + 1);
    }

    // 合并连续的 // 和处理 /./
    char *q = result, *p = result;
    while (*p) {
        if (*p == '/') {
            if (p[1] == '/') { ++p; continue; }                 // 跳过连续的 /
            if (p[1] == '.' && (p[2] == '/' || p[2] == '\0')) { // 跳过 /.
                p += 2; if (*p == '\0') break; continue;
            }
        }
        *q++ = *p++;
    }
    *q = '\0';

    // 移除末尾的 / (根目录除外)
    size_t len = q - result;
    while (len > 1 && result[--len] == '/') result[len] = '\0';

    return result;
}

int ARGS_ls_count(arg_var_st* var) {
    if (var->ls == NULL) return 0;
    return *(int*)&var->ls[-1];
}

static arg_def_st*  g_args_def = NULL;
static int          g_pos_count = 0;    // 位置参数数量
static bool         g_has_req = false;  // 是否有必选选项
static const char*  g_pos_desc = NULL;  // 位置参数描述（如 "<subcommand>"）
static const char*  g_usage_ex = NULL;  // 使用示例说明

void ARGS_usage(const char* pos_desc, const char* usage_ex) {
    g_pos_desc = pos_desc;
    g_usage_ex = usage_ex;
}

int ARGS_parse(int argc, char** argv, ...) {

    if (g_args_def) return 1;

    va_list args; va_start(args, argv);
    arg_def_st* def; int req_count = 0;  // 必选参数计数
    while ((def = va_arg(args, arg_def_st*)) != NULL) {
        def->next = g_args_def;
        g_args_def = def;
        if (def->req) req_count++;
    }
    va_end(args);
    g_has_req = (req_count > 0);

    int show_help = (argc == 1);
    g_pos_count = 0;
    char* pos_args[argc];  // 临时存储位置参数
    int w = 1;             // 写入位置（选项及其值前移到这里）

    for (int i = 1; i < argc; i++) { char* arg = argv[i];

        // 检查是否请求帮助
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            show_help = 1;
            break;
        }

        // -- 终止符：后续都是位置参数
        if (strcmp(arg, "--") == 0) {
            for (i++; i < argc; i++) {
                pos_args[g_pos_count++] = argv[i];
            }
            break;
        }

        // 长选项 --xxx
        if (arg[0] == '-' && arg[1] == '-') {
            for (def = g_args_def; def != NULL; def = def->next) {
                if (def->l && strcmp(def->l, arg + 2) == 0) break;
            }
            if (def == NULL) {
                fprintf(stderr, "Error: Unknown option '%s'\n", arg);
                exit(-1);
            }
            if (def->req) req_count--;

            argv[w++] = arg;  // 前移选项

            if (def->type == ARG_BOOL) {
                def->var->i64 = 1;
            } else if (def->type == ARG_LS) {
                if (i + 1 >= argc) {
                    fprintf(stderr, "Error: Option '%s' requires a value\n", arg);
                    exit(-1);
                }
                int count = 0;
                for (int k = i + 1; k < argc && argv[k][0] != '-'; k++) count++;
                def->var->ls = (const char**)&argv[w];
                *(int*)&argv[w - 1] = count;  // 重载选项位置为数量
                for (int k = 0; k < count; k++) {
                    argv[w++] = argv[++i];
                }
            } else {
                if (i + 1 >= argc) {
                    fprintf(stderr, "Error: Option '%s' requires a value\n", arg);
                    exit(-1);
                }
                argv[w++] = argv[++i];  // 前移选项值
                switch (def->type) {
                    case ARG_INT:   def->var->i64 = strtoll(argv[i], NULL, 10); break;
                    case ARG_FLOAT: def->var->f64 = strtod(argv[i], NULL); break;
                    case ARG_STR:   def->var->str = argv[i]; break;
                    case ARG_DIR:   def->var->str = normalize_dir_path((char*)argv[i]); break;
                    default: break;
                }
            }
        }
            // 短选项 -x 或 -abc 组合形式
        else if (arg[0] == '-' && arg[1] != '\0') {
            argv[w++] = arg;  // 前移选项

            for (int j = 1; arg[j] != '\0'; j++) { char c = arg[j];

                for (def = g_args_def; def != NULL; def = def->next) {
                    if (def->s == c) break;
                }
                if (def == NULL) {
                    fprintf(stderr, "Error: Unknown option '-%c'\n", c);
                    exit(-1);
                }
                if (def->req) req_count--;

                if (def->type == ARG_BOOL) {
                    def->var->i64 = 1;
                } else if (def->type == ARG_LS) {
                    if (i + 1 >= argc) {
                        fprintf(stderr, "Error: Option '-%c' requires a value\n", c);
                        exit(-1);
                    }
                    int count = 0;
                    for (int k = i + 1; k < argc && argv[k][0] != '-'; k++) count++;
                    def->var->ls = (const char**)&argv[w];
                    *(int*)&argv[w - 1] = count;
                    for (int k = 0; k < count; k++) {
                        argv[w++] = argv[++i];
                    }
                    break;
                } else {
                    const char* value;
                    if (arg[j + 1] != '\0') {
                        value = &arg[j + 1];
                    } else if (i + 1 < argc) {
                        argv[w++] = argv[++i];
                        value = argv[i];
                    } else {
                        fprintf(stderr, "Error: Option '-%c' requires a value\n", c);
                        exit(-1);
                    }
                    switch (def->type) {
                        case ARG_INT:   def->var->i64 = strtoll(value, NULL, 10); break;
                        case ARG_FLOAT: def->var->f64 = strtod(value, NULL); break;
                        case ARG_STR:   def->var->str = value; break;
                        case ARG_DIR:   def->var->str = normalize_dir_path((char*)value); break;
                        default: break;
                    }
                    break;
                }
            }
        }
            // 位置参数（非选项参数）
        else {
            pos_args[g_pos_count++] = arg;
        }
    }

    // 将位置参数放到选项之后
    for (int i = 0; i < g_pos_count; i++) {
        argv[w + i] = pos_args[i];
    }

    // 检查必选参数是否都已提供
    if (req_count > 0 && !show_help) {
        fprintf(stderr, "Error: Missing required options:\n");
        for (def = g_args_def; def != NULL; def = def->next) {
            if (def->req) {
                // 检查是否已设置（对于不同类型检查不同）
                bool missing = false;
                switch (def->type) {
                    case ARG_STR:
                    case ARG_DIR:   missing = (def->var->str == NULL); break;
                    case ARG_LS:    missing = (def->var->ls == NULL); break;
                    default:        missing = false; break;  // INT/FLOAT/BOOL 无法判断是否设置
                }
                if (missing) {
                    fprintf(stderr, "  --%s (-%c)\n", def->l, def->s);
                }
            }
        }
        exit(-1);
    }

    // 反转链表顺序（因为构建时是倒序插入的）
    arg_def_st* prev = NULL; def = g_args_def;
    while (def) {
        arg_def_st* next = def->next;
        def->next = prev;
        prev = def;
        def = next;
    }
    g_args_def = prev;

    // 解析完成后，如果需要帮助则遍历链表打印
    if (show_help) {
        ARGS_print(argv[0]);
        exit(argc == 1 ? -1 : 0);
    }

    return g_pos_count;
}

int ARGS_print(const char* arg0) {

    // 动态生成 Usage 信息
    const char* opt_str = g_args_def ? (g_has_req ? "OPTIONS" : "[OPTIONS]") : "";
    const char* arg_str = g_pos_desc ? g_pos_desc : (g_pos_count > 0 ? "ARGS..." : "");
    printf("Usage: %s%s%s%s%s\n", arg0,
           *arg_str ? " " : "", arg_str,
           *opt_str ? " " : "", opt_str);

    // 先输出位置参数/子命令说明
    if (g_usage_ex && *g_usage_ex) {
        putchar('\n');
        // 输出 usage_ex，遇到 $0 替换为程序名
        for (const char* p = g_usage_ex; *p; p++) {
            if (p[0] == '$' && p[1] == '0') {
                fputs(arg0, stdout);  // fputs 不会输出换行符，puts 才会
                p++;  // 跳过 '0'
            } else {
                putchar(*p);
            }
        }
    }

    // 再输出选项列表
    if (g_args_def) {

        // 先遍历计算最大长选项名长度
        int max_l_len = 0;
        for (arg_def_st* def = g_args_def; def != NULL; def = def->next) {
            int len = def->l ? (int)strlen(def->l) : 0;
            if (len > max_l_len) max_l_len = len;
        }

        printf("\nOptions:\n");
        for (arg_def_st* def = g_args_def; def != NULL; def = def->next) {
            const char* type_str = "";
            switch (def->type) {
                case ARG_INT:   type_str = "<int>"; break;
                case ARG_FLOAT: type_str = "<float>"; break;
                case ARG_BOOL:  type_str = ""; break;
                case ARG_STR:   type_str = "<string>"; break;
                case ARG_DIR:   type_str = "<dir>"; break;
                case ARG_LS:    type_str = "<list>"; break;
            }
            printf("  -%c, --%-*s  %-10s %s%s\n", def->s, max_l_len, def->l, type_str,
                   def->req ? "\033[31m[required]\033[0m " : "[optional] ", P_LA(def->desc));
        }
    }

    putchar('\n');
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// 系统日志（平台适配层）
///////////////////////////////////////////////////////////////////////////////

#if P_DARWIN
#include <os/log.h>
static os_log_t g_log_handle;
#elif defined(__ANDROID__)
#include <android/log.h>
#elif defined(__QNX__)
#include <sys/slog2.h>
static slog2_buffer_t g_log_handle;
#elif P_LINUX
#include <syslog.h>
static bool g_log_opened = false;
#endif

/**
 * @brief 获取日志名称（从可执行文件名提取基础名）
 * @return 日志名称字符串
 */
static const char* get_log_name(void) {
    if (g_log_name[0]) return g_log_name;
    
    // 获取可执行文件路径
    char exe_path[1024];
    if (!P_exe_file(exe_path, sizeof(exe_path)) || !exe_path[0]) {
        strcpy(g_log_name, "app");  // fallback
        return g_log_name;
    }
    
    // 查找最后一个路径分隔符
    const char* name = exe_path;
    for (const char* p = exe_path; *p; p++) {
        if (*p == '/' || *p == '\\') {
            name = p + 1;
        }
    }
    
    // 复制文件名，去除扩展名
    size_t len = 0;
    for (const char* p = name; *p && len < sizeof(g_log_name) - 1; p++) {
        if (*p == '.') break;  // 遇到扩展名停止
        g_log_name[len++] = *p;
    }
    g_log_name[len] = '\0';
    
    // 如果为空，使用默认值
    if (!g_log_name[0]) strcpy(g_log_name, "app");
    
    return g_log_name;
}

/**
 * @brief 初始化系统日志（内部函数，延迟初始化）
 */
static void log_init(void) {
#if P_DARWIN
    if (g_log_handle == NULL)
        g_log_handle = os_log_create(get_log_name(), "main_log");
#elif defined(__QNX__)
    if (g_log_handle == NULL) {
        slog2_buffer_set_config_t config;
        config.buffer_set_name = get_log_name();
        config.num_buffers = 1;
        config.verbosity_level = SLOG2_DEBUG2;
        config.buffer_config[0].buffer_name = "main_log";
        config.buffer_config[0].num_pages = 10;
        if (slog2_register(&config, &g_log_handle, 0) < 0) {
            fprintf(stderr, "Error registering slogger2 buffer(%d)!\n", errno);
        }
    }
#elif P_LINUX
    if (!g_log_opened) {
        openlog(get_log_name(), LOG_CONS | LOG_PID, LOG_USER);
        g_log_opened = true;
    }
#endif
}

/**
 * @brief 清理系统日志资源（内部函数）
 */
static void log_cleanup(void) {
#if P_DARWIN
    if (g_log_handle) {
        os_release(g_log_handle);
        g_log_handle = NULL;
    }
#elif P_LINUX
    if (g_log_opened) {
        closelog();
        g_log_opened = false;
    }
#endif
}

/**
 * @brief 写入系统日志（内部函数，平台适配）
 * @param level 日志级别
 * @param tag 日志标签
 * @param text 已格式化的日志文本
 * @param len 文本长度（未使用）
 */
static void log_write(log_level_e level, const char* tag, char* text, int len) {
    (void)len;  // unused
    
    if (!tag) tag = get_log_name();
    if (!text) return;

#if P_WIN

    char buffer[4096];
    snprintf(buffer, sizeof(buffer), "[%s] %s", tag, text);
    OutputDebugStringA(buffer);

#elif P_DARWIN

    static const os_log_type_t s_level[] = {
        OS_LOG_TYPE_DEBUG,   // VERBOSE
        OS_LOG_TYPE_DEBUG,   // DEBUG
        OS_LOG_TYPE_INFO,    // INFO
        OS_LOG_TYPE_DEFAULT, // WARN
        OS_LOG_TYPE_ERROR,   // ERROR
        OS_LOG_TYPE_FAULT    // FATAL
    };

    log_init();  // 延迟初始化
    if (g_log_handle) {
        os_log_with_type(g_log_handle, s_level[level], "%{public}s", text);
    }

#elif defined(__ANDROID__)

    static const android_LogPriority s_level[] = {
        ANDROID_LOG_VERBOSE, // VERBOSE
        ANDROID_LOG_DEBUG,   // DEBUG
        ANDROID_LOG_INFO,    // INFO
        ANDROID_LOG_WARN,    // WARN
        ANDROID_LOG_ERROR,   // ERROR
        ANDROID_LOG_FATAL    // FATAL
    };

    __android_log_write(s_level[level], tag, text);

#elif defined(__QNX__)

    static const int s_level[] = {
        SLOG2_DEBUG2,   // VERBOSE
        SLOG2_DEBUG1,   // DEBUG
        SLOG2_INFO,     // INFO
        SLOG2_WARNING,  // WARN
        SLOG2_ERROR,    // ERROR
        SLOG2_CRITICAL  // FATAL
    };

    log_init();  // 延迟初始化
    if (g_log_handle) {
        slog2c(g_log_handle, 0, s_level[level], text);
    }

#elif P_LINUX

    static const int s_level[] = {
        LOG_DEBUG,   // VERBOSE
        LOG_DEBUG,   // DEBUG
        LOG_INFO,    // INFO
        LOG_WARNING, // WARN
        LOG_ERR,     // ERROR
        LOG_CRIT     // FATAL
    };

    log_init();  // 延迟初始化
    syslog(s_level[level], "%s", text);

#else
#error "Unsupported platform"
#endif
}

///////////////////////////////////////////////////////////////////////////////

void log_output(log_cb cb_log, bool tag_separate) {
    
    log_cb old_cb = g_cb_log;
    
    // 如果设置为 NULL，清理资源并重置为默认
    if (cb_log == NULL) {
        if (old_cb == log_write) {
            log_cleanup();
        }
        g_cb_log = (log_cb)-1;
        g_tag_separate = false;
        return;
    }
    
    // 如果设置为 -2，转换为 log_write
    if (cb_log == (log_cb)-2) {
        cb_log = log_write;
    }
    
    // 如果从其他状态切换到 log_write，需要初始化
    if (cb_log == log_write && old_cb != log_write) {
        log_init();
    }
    // 如果从 log_write 切换到其他，需要清理
    else if (old_cb == log_write && cb_log != log_write) {
        log_cleanup();
    }
    
    g_cb_log = cb_log;
    g_tag_separate = tag_separate;
}

void log_slot(log_level_e level, const char *tag, const char *fmt, va_list params) {

    static __thread char        g_line[LOG_HDR_RESERVE + 256 + LOG_LINE_MAX];
    static __thread int         g_logging = -1;         // -1: 默认模式，0: 开启缓存模式（缓存内容为空），>0: 缓存模式且已写入内容
#ifndef LOG_FILE_DISABLED
    static void*                g_cache_head, *g_cache_rear;
    static uint32_t             g_line_count;
    static pthread_mutex_t      g_cache_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
    char* const                 buf = g_line + LOG_HDR_RESERVE + 256;     // 日志输出起始位置

    // 如果 tag 为空，表示输出日志到缓存
    if (!tag) {

        // 开启缓存模式，并 rewind 到开头
        if (!fmt) {
            g_logging = 0;
            return;
        }
        if (g_logging >= LOG_LINE_MAX - 1) { // 缓存已满，忽略后续内容
            fprintf(stderr, "W: log buffer full, ignore log content\n");
            return;
        }
        if (g_logging < 0) g_logging = 0;
        if (!*fmt) return;
        if (*fmt == '%' && fmt[1] == ' ') {
            char* b = buf + g_logging;
            strncpy(b, fmt + 2, LOG_LINE_MAX - 1 - g_logging); 
            buf[LOG_LINE_MAX - 1] = '\0';
            g_logging += (int)strlen(b);
        }
        else g_logging += vsnprintf(buf + g_logging, LOG_LINE_MAX - g_logging, fmt, params);
        return;
    }

    // 如果之前开启了缓存模式，且缓存内容为空，则关闭缓存模式，直接返回
    if (g_logging == 0) { g_logging = -1;
        return;
    }

    int tag_len = (int)strlen(tag);
    if (tag_len > 255 + LOG_LINE_MAX - 1)           // 限制 tag 长度，防止溢出
        tag_len = 255 + LOG_LINE_MAX - 1;
    char* out;                                      // 输出起始位置

    int n = 0;
    if (g_logging > 0) {                            // 对于缓存模式的最终输出
        n = g_logging; g_logging = -1;              // 关闭缓存模式
    }

    #ifdef LOG_INSTRUMENT
    {
        // 复制 tag
        int m = (tag_len < 256) ? tag_len : 255;
        out = buf - m - 1;
        memcpy(out, tag, m);
        out[m] = 0;

        // 格式化文本
        if (n < LOG_LINE_MAX) {
            if (*fmt == '%' && fmt[1] == ' ')
                n += snprintf(buf+n, LOG_LINE_MAX-n, "%s", fmt + 2);
            else n += vsnprintf(buf+n, LOG_LINE_MAX-n, fmt, params);
        }
        if (n >= LOG_LINE_MAX) buf[n = LOG_LINE_MAX - 1] = 0;

        inst_send_buf((uint8_t)level, out - INST_HDR_SIZE, m, n);

        if (g_tag_separate) out = buf; 
        else {
            if (tag_len >= 256) { int shift = tag_len - 255;
                int max_n = LOG_LINE_MAX - 1 - shift;       // 移动后的最大 text 长度 (-1 给 \0)
                if (max_n < 0) max_n = 0;
                if (n > max_n) buf[n = max_n] = 0;          // 截断 text
                memmove(buf + shift, buf, n + 1);           // 后移 text (+1 含 \0)
                memcpy(buf - 1, tag + 255, shift);          // 填入 tag 剩余部分 (从 out[255] 开始)
            }
            out[tag_len] = ' ';
        }
    }
    #else
    {
        char* text = buf;                                   // text 起始位置
        int text_max = LOG_LINE_MAX;                        // text 可用空间

        // 设置输出位置
        if (g_tag_separate) out = buf;
        else {
            int m = (tag_len < 256) ? tag_len : 255;
            out = buf - m - 1;
            memcpy(out, tag, m);
            if (tag_len >= 256) { int shift = tag_len - 255;
                text_max = LOG_LINE_MAX - 1 - shift;        // 移动后的最大 text 长度 (-1 给 \0)
                if (text_max < 0) text_max = 0;
                if (n > text_max) n = text_max;             // 截断缓存
                if (n > 0) memmove(buf + shift, buf, n);
                memcpy(buf - 1, tag + 255, shift);          // 填入 tag 剩余部分 (从 out[255] 开始)
                text = buf + shift;                         // text 起始位置改变
            }
            out[tag_len] = ' ';
        }

        // 格式化 fmt 到 text
        if (n < text_max) {
            if (*fmt == '%' && fmt[1] == ' ')
                n += snprintf(text+n, text_max-n, "%s", fmt + 2);
            else n += vsnprintf(text+n, text_max-n, fmt, params);
        }
        if (text_max > 0 && n >= text_max) text[n = text_max - 1] = 0;
    }
    #endif

    int total = (out == buf) ? n : tag_len + 1 + n;  // 总输出长度

    // 对于标准输出
    if (g_cb_log == (log_cb)-1) {
        if (total > 0 && out[total - 1] == '\n') out[--total] = 0; // 移除末尾换行符
        printf("%s\n", out);
    }
    // 对于回调（包括系统日志）
    else if (g_cb_log) {
        g_cb_log(level, g_tag_separate ? tag : NULL, out, total);
    }

#ifndef LOG_FILE_DISABLED

    pthread_mutex_lock(&g_cache_mutex);

      static bool log_append = false;
      static char log_fn[256];
      if (!log_append) {
          for(int i=0;;) {
              sprintf(log_fn, "%s/%s%d.txt", LOG_OUTPUT_PATH, get_log_name(), ++i);
              if (access(log_fn, F_OK) < 0) break;
          }
      }

      FILE* fp = fopen(log_fn, log_append?"a":"w");
      if (fp) {

          if (g_cache_head) {
              do {
                  void** item = (void**)g_cache_head;
                  fputs((char*)(item+1), fp);
                  g_cache_head = *item;
                  free(item);
              }
              while (g_cache_head);
              g_cache_rear = NULL;
              g_line_count = 0;
          }
          fprintf(fp, "%s\n", out);
          fclose(fp);

          log_append = true;
      }
      else {

          void** item = (void**)malloc(total + 2/* \n\0 */ + sizeof(void*)); *item = NULL;
          sprintf((char*)(item+1), "%s\n", out);
          if (g_cache_rear) *(void**)g_cache_rear = item;
          else g_cache_head = item;
          g_cache_rear = item;

          if (++g_line_count > 1000) {
              void* next = *(void**)g_cache_head;
              free(g_cache_head);
              g_cache_head = next;
          }
      }

      pthread_mutex_unlock(&g_cache_mutex);
#endif
}

///////////////////////////////////////////////////////////////////////////////
#ifdef LOG_INSTRUMENT

static void inst_cleanup(void) {
    g_inst_running = false;
    if (g_inst_thread) {
        P_join(g_inst_thread, NULL);
        g_inst_thread = 0;
    }
    if (g_inst_sock != P_INVALID_SOCKET) {
        P_sock_close(g_inst_sock);
        g_inst_sock = P_INVALID_SOCKET;
    }
    if (g_inst_bits) {
        free(g_inst_bits);
        g_inst_bits = NULL;
        g_inst_bits_len = 0;
    }
}

static int32_t inst_thread_proc(void *ctx);  // 前向声明

// 初始化 socket（仅网络，不启动线程）
// 前置条件：g_inst_sock == P_INVALID_SOCKET（调用处判断）
// 纯发送场景（instrument_enable/instrument_slot）只需调用此函数
static bool inst_init_sock(void) {

    assert(g_inst_sock == P_INVALID_SOCKET);

    g_inst_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_inst_sock == P_INVALID_SOCKET) return false;

    int opt = 1;
    setsockopt(g_inst_sock, SOL_SOCKET, SO_BROADCAST, (const char*)&opt, sizeof(opt));
    setsockopt(g_inst_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#ifdef SO_REUSEPORT
    setsockopt(g_inst_sock, SOL_SOCKET, SO_REUSEPORT, (const char*)&opt, sizeof(opt));
#endif

    // bind 到指定端口（收发共用）
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(g_inst_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(g_inst_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        P_sock_close(g_inst_sock);
        g_inst_sock = P_INVALID_SOCKET;
        return false;
    }

    // 组播设置：
    // - 加入组播组，确保能接收发往该组播地址的消息
    // - 启用组播回环，使本机发送的消息也能被本机其他进程收到
    // 注：单播 + SO_REUSEPORT 在 macOS/Linux 上是负载均衡（只有一个进程收到），
    //     组播是真正的一对多广播，所有加入组的进程都能收到
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = htonl(INST_MCAST_ADDR);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(g_inst_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq, sizeof(mreq));
    unsigned char loop = 1;
    setsockopt(g_inst_sock, IPPROTO_IP, IP_MULTICAST_LOOP, (const char*)&loop, sizeof(loop));

    // 目标地址：
    // - HOST 模式：组播地址（本机所有进程可见）
    // - REMOTE 模式：广播地址（局域网可见）
    memset(&g_inst_dest, 0, sizeof(g_inst_dest));
    g_inst_dest.sin_family      = AF_INET;
    g_inst_dest.sin_port        = htons(g_inst_port);
    g_inst_dest.sin_addr.s_addr = (g_inst_mode == INST_MODE_REMOTE)
                                    ? htonl(INADDR_BROADCAST)
                                    : htonl(INST_MCAST_ADDR);  // 组播地址

    // 生成随机 rid
    g_inst_rid = (uint16_t)(P_tick_us() ^ (uintptr_t)&g_inst_sock);

    // 设置接收超时（100ms），供线程周期性检查 g_inst_running
    P_sock_rcvtimeo(g_inst_sock, 100);

    atexit(inst_cleanup);
    return true;
}

// 启动接收线程（监听场景需要）
// 前置条件：g_inst_sock 已初始化，g_inst_thread == 0（调用处判断）
// instrument_listen/instrument_enabled 需要调用此函数以接收其他进程的消息
static bool inst_start_thread(void) {

    assert(g_inst_sock != P_INVALID_SOCKET);
    assert(g_inst_thread == 0);

    g_inst_running = true;
    if (P_thread(&g_inst_thread, inst_thread_proc, NULL, P_THD_BACKGROUND, 0) != E_NONE) {
        g_inst_running = false;
        return false;
    }
    return true;
}

void
instrument_port(uint16_t port) {
    assert(g_inst_sock == P_INVALID_SOCKET);        // 必须在首次使用前调用
    g_inst_port = port;
}

// ---- 选项 bitset ----

// 确保 bitset 能容纳指定字节偏移
static void inst_bits_reserve(uint16_t byte_idx) {
    uint16_t need = byte_idx + 1;
    if (need <= g_inst_bits_len) return;

    uint8_t *new_bits = (uint8_t*)realloc(g_inst_bits, need);
    if (!new_bits) return;

    // 新增部分初始化为 0
    memset(new_bits + g_inst_bits_len, 0, need - g_inst_bits_len);
    g_inst_bits = new_bits;
    g_inst_bits_len = need;
}

// 发送 type=1 包：header(7) + offset(2) + byte(1)
// 包格式与 type=0 统一使用 INST_HDR_SIZE header，便于接收端统一处理
static void inst_send_bits(uint16_t byte_idx) {
    if (g_inst_sock == P_INVALID_SOCKET) return;
    if (byte_idx >= g_inst_bits_len) return;

    uint8_t pkt[INST_HDR_SIZE + 3];
    nwrite_s(pkt, g_inst_rid);                      // rid
    nwrite_s(pkt + 2, g_inst_seq++);                // seq
    pkt[4] = 1;                                     // type=1 选项包
    pkt[5] = 0;                                     // chn（未使用，占位）
    pkt[6] = 0;                                     // tag_len（未使用，占位）
    // payload: offset(2) + byte(1)
    nwrite_s(pkt + INST_HDR_SIZE, byte_idx);
    pkt[INST_HDR_SIZE + 2] = g_inst_bits[byte_idx];

    sendto(g_inst_sock, (const char*)pkt, sizeof(pkt), 0,
           (struct sockaddr*)&g_inst_dest, sizeof(g_inst_dest));
}

ret_t
instrument_enable(uint16_t idx, bool enable) {

    // 发送操作：只需初始化 socket，不启动接收线程
    if (g_inst_sock == P_INVALID_SOCKET && !inst_init_sock())
        return E_EXTERNAL(P_sock_errno());

    idx += g_inst_options_base;
    uint16_t byte_idx = idx / 8; uint8_t  bit_mask = (uint8_t)(1u << (idx % 8));

    inst_bits_reserve(byte_idx);
    if (byte_idx >= g_inst_bits_len) return E_OUT_OF_MEMORY;

    if (enable)
        g_inst_bits[byte_idx] |= bit_mask;
    else
        g_inst_bits[byte_idx] &= ~bit_mask;

    inst_send_bits(byte_idx);
    return E_NONE;
}

bool
instrument_enabled(uint16_t idx) {

    // 监听操作：需要启动接收线程以接收其他进程的选项包
    if (g_inst_sock == P_INVALID_SOCKET && !inst_init_sock()) return false;
    if (g_inst_thread == 0 && !inst_start_thread()) return false;

    idx += g_inst_options_base;
    uint16_t byte_idx = idx / 8;
    if (byte_idx >= g_inst_bits_len) return false;
    return (g_inst_bits[byte_idx] & (1u << (idx % 8))) != 0;
}

// ---- 发送端 ----

// 内部函数：发送已格式化的文本
// buf: 缓冲区，tag + \0 + text 从 buf + INST_HDR_SIZE 开始
// tag_len: tag 长度（tag 已在 buf + INST_HDR_SIZE，后跟 \0）
// text_len: text 长度（text 从 tag + tag_len + 1 开始）
static void
inst_send_buf(uint8_t chn, char* buf, int tag_len, int text_len) {

    char* tag  = buf + INST_HDR_SIZE;
    char* text = tag + tag_len + 1;                 // 跳过 \0

    // 本地回调：tag 已有 \0 结尾，直接使用
    // 递归保护：防止回调中调用 print() 导致无限递归
    if (g_inst_cb && !g_inst_in_cb) {
        g_inst_in_cb = 1;
        g_inst_cb(0, chn, tag, text, text_len);
        g_inst_in_cb = 0;
    }

    // 本地模式：只发送保留通道
    if (g_inst_mode == INST_MODE_LOCAL) {
        if (!(g_inst_keep_chn[chn / 32] & (1u << (chn % 32)))) return;
    }
    
    // 发送操作：只需初始化 socket，不启动接收线程
    if (g_inst_sock == P_INVALID_SOCKET && !inst_init_sock()) return;

    // 写入固定 header (7 bytes)
    uint8_t *pkt = (uint8_t*)buf;
    nwrite_s(pkt, g_inst_rid);                      // rid
    nwrite_s(pkt + 2, g_inst_seq++);                // seq
    pkt[4] = 0;                                     // type=0 数据包
    pkt[5] = chn;                                   // chn
    pkt[6] = (uint8_t)tag_len;                      // tag_len

    // 协议: header + tag + \0 + text
    sendto(g_inst_sock, buf, INST_HDR_SIZE + tag_len + 1 + text_len, 0,
           (struct sockaddr*)&g_inst_dest, sizeof(g_inst_dest));
}

void
instrument_slot(uint8_t chn, const char* tag, const char* fmt, va_list params) {

    char buf[INST_UDP_MAX];

    // 先计算 tag_len，直接格式化到正确位置，避免 memmove
    int tag_len = tag ? (int)strlen(tag) : 0;
    if (tag_len > 255) tag_len = 255;

    char* tag_pos  = buf + INST_HDR_SIZE;
    char* text_pos = tag_pos + tag_len + 1;         // 跳过 tag + \0
    int text_max   = INST_PAYLOAD_MAX - tag_len - 1; // 最小 1393-255-1=1137
    int text_len;

    // "% " 前缀：直接拷贝字符串
    if (*fmt == '%' && fmt[1] == ' ') { const char *src = fmt + 2;
        text_len = (int)strlen(src);
        if (text_len > text_max) text_len = text_max;
        memcpy(text_pos, src, text_len);
    }
    // 格式化到 text 位置
    else {
        text_len = vsnprintf(text_pos, text_max, fmt, params);
        if (text_len < 0) text_len = 0;
        if (text_len >= text_max) text_len = text_max - 1;
    }

    // 写入 tag + \0
    if (tag_len > 0) memcpy(tag_pos, tag, tag_len);
    tag_pos[tag_len] = '\0';

    inst_send_buf(chn, buf, tag_len, text_len);
}

// ---- 监听端 ----

ret_t
instrument_listen(instrument_cb cb, uint16_t options_base) {

    // 监听操作：需要启动接收线程
    if (g_inst_sock == P_INVALID_SOCKET && !inst_init_sock())
        return E_EXTERNAL(P_sock_errno());
    if (g_inst_thread == 0 && !inst_start_thread())
        return E_EXTERNAL(P_sock_errno());

    g_inst_cb     = cb;
    memset(g_inst_win, 0, sizeof(g_inst_win));
    g_inst_synced = false;
    g_inst_options_base = options_base;

    return E_NONE;
}

void
instrument_local(int keep_chn, ...) {
    g_inst_mode = INST_MODE_LOCAL;
    
    // 清空保留列表
    memset(g_inst_keep_chn, 0, sizeof(g_inst_keep_chn));
    
    // 如果第一个参数为 0，表示关闭全部
    if (keep_chn == 0) return;
    
    // 设置保留通道
    g_inst_keep_chn[keep_chn / 32] |= (1u << (keep_chn % 32));
    
    va_list args;
    va_start(args, keep_chn);
    uint8_t chn;
    while ((chn = (uint8_t)va_arg(args, int)) != 0) {
        g_inst_keep_chn[chn / 32] |= (1u << (chn % 32));
    }
    va_end(args);
}

void
instrument_remote(void) {
    g_inst_mode = INST_MODE_REMOTE;
    // 如果 socket 已初始化，更新目标地址
    if (g_inst_sock != P_INVALID_SOCKET) {
        g_inst_dest.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    }
}

// ---- 监听端 ----

// 处理 type=1 选项包
static void inst_handle_bits(uint8_t *payload, int len) {
    if (len < 3) return;                            // offset(2) + byte(1)

    uint16_t byte_idx = nget_s(payload);
    uint8_t  byte_val = payload[2];

    inst_bits_reserve(byte_idx);
    if (byte_idx < g_inst_bits_len) {
        g_inst_bits[byte_idx] = byte_val;
    }
}

// 解析并交付一个数据包到回调
// pkt: 完整数据包（包含 header）
// len: 包长度
// 协议: header(7) = rid(2)+seq(2)+type(1)+chn(1)+tag_len(1)
//       payload   = tag + \0 + text
static void inst_deliver(uint16_t rid, uint8_t *pkt, int len) {
    assert(g_inst_cb);
    if (len < INST_HDR_SIZE + 2) return;  // 至少 header + tag(1) + \0

    uint8_t chn     = pkt[5];                       // header 中的 chn
    uint8_t tag_len = pkt[6];                       // header 中的 tag_len
    
    uint8_t *payload = pkt + INST_HDR_SIZE;
    int payload_len  = len - INST_HDR_SIZE;
    int text_len     = payload_len - tag_len - 1;   // -1 是 \0

    if (text_len < 0) return;                       // 数据不完整

    char *tag  = (char*)payload;                    // tag 已有 \0 结尾
    char *text = tag + tag_len + 1;                 // 跳过 \0
    text[text_len] = '\0';                          // 安全：inst_slot_t.data 有 +1 余量

    g_inst_cb(rid, chn, tag, text, text_len);
}

// 从 g_inst_next 开始连续交付已缓冲的包
static int inst_flush(void) {
    int count = 0;
    for (;;) {
        int idx = g_inst_next & INST_WINDOW_MASK;
        if (g_inst_win[idx].len == 0) break;

        uint8_t *pkt = g_inst_win[idx].data;
        int pkt_len  = g_inst_win[idx].len;
        uint16_t rid = g_inst_win[idx].rid;
        if (g_inst_cb && pkt_len > INST_HDR_SIZE)
            inst_deliver(rid, pkt, pkt_len);  // 传入完整包

        g_inst_win[idx].len = 0;
        g_inst_next++;
        count++;;
    }
    return count;
}

// 查找或创建 sender 条目
static inst_sender_t* inst_find_sender(uint16_t rid) {
    // 查找现有
    for (int i = 0; i < g_inst_sender_cnt; i++) {
        if (g_inst_senders[i].rid == rid) return &g_inst_senders[i];
    }
    // 创建新条目
    if (g_inst_sender_cnt < INST_MAX_SENDERS) {
        inst_sender_t *s = &g_inst_senders[g_inst_sender_cnt++];
        s->rid = rid;
        s->next_seq = 0;
        s->synced = false;
        return s;
    }
    // 满了，复用最旧的（LRU近似）
    inst_sender_t *s = &g_inst_senders[0];
    s->rid = rid;
    s->next_seq = 0;
    s->synced = false;
    return s;
}

// 接收线程：循环 recvfrom，按 seq 顺序交付到回调
static int32_t inst_thread_proc(void *ctx) {
    (void)ctx;
    uint8_t buf[INST_UDP_MAX + 1];

    while (g_inst_running) {
        int n = (int)recvfrom(g_inst_sock, (char*)buf, INST_UDP_MAX, 0, NULL, NULL);
        if (n < INST_HDR_SIZE + 2) continue;        // 超时/错误/包太小

        uint16_t rid = nget_s(buf);
        if (rid == g_inst_rid) continue;            // 过滤自己的包

        uint16_t seq = nget_s(buf + 2);
        uint8_t type = buf[4];

        // type=1 选项包：直接处理，不走顺序交付
        if (type == 1) {
            inst_handle_bits(buf + INST_HDR_SIZE, n - INST_HDR_SIZE);
            continue;
        }

        // type!=0 未知类型，丢弃
        if (type != 0) continue;

        // 获取该 rid 的序号追踪器
        inst_sender_t *sender = inst_find_sender(rid);

        // 首包同步
        if (!sender->synced) {
            sender->synced = true;
            sender->next_seq = seq;
        }

        int16_t diff = (int16_t)(seq - sender->next_seq);
        if (diff < 0) continue;                     // 旧包/重复

        // 超出窗口 → 警告并跳跃到新 seq
        if (diff >= INST_WINDOW_SIZE) {
            fprintf(stderr, "inst: rid=%u lost %d packets (seq %u → %u)\n",
                    rid, (int)diff, sender->next_seq, seq);
            sender->next_seq = seq;
            diff = 0;
        }

        // 直接交付（简化：不使用窗口缓冲，保持顺序由发送方保证）
        if (g_inst_cb && n > INST_HDR_SIZE) {
            inst_deliver(rid, buf, n);
        }
        sender->next_seq = seq + 1;
    }
    return 0;
}

#endif

///////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic pop
