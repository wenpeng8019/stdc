
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#include "stdc.h"

static log_cb                   g_cb_log = (log_cb)-1;              // 默认 (-1) stdout 输出
static bool                     g_tag_separate = false;
static char                     g_log_name[64] = {0};               // 日志名称（从可执行文件名提取）

#ifdef LOG_INSTRUMENT

// 典型以太网 MTU=1500，减去 IP(20) + UDP(8) 头部，保守取 1400
#define INST_UDP_MAX            1400
#define INST_HDR_SIZE           5                                   // rid(2) + seq(2) + type(1)
#define INST_PAYLOAD_MAX        (INST_UDP_MAX - INST_HDR_SIZE)      // 可用数据区
#define INST_WINDOW_SIZE        64                                  // 接收端滑动窗口大小（必须为 2 的幂）
#define INST_WINDOW_MASK        (INST_WINDOW_SIZE - 1)

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
} inst_slot_t;
static instrument_cb            g_inst_cb     = NULL;
static inst_slot_t              g_inst_win[INST_WINDOW_SIZE];
static uint16_t                 g_inst_next   = 0;                  // 下一个期望 seq
static bool                     g_inst_synced = false;              // 首包同步标记

// 选项 bitset
static uint8_t*                 g_inst_bits     = NULL;             // 动态分配
static uint16_t                 g_inst_bits_len = 0;                // 已分配字节数

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
                   def->req ? "\033[31m[required]\033[0m " : "[optional] ", def->desc);
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

    static __thread char        g_line[LOG_LINE_MAX];
    static __thread int         g_logging = -1;         // -1: 默认模式，0: 开启缓存模式（缓存内容为空），>0: 缓存模式且已写入内容
#ifndef LOG_FILE_DISABLED
    static void*                g_cache_head, *g_cache_rear;
    static uint32_t             g_line_count;
    static pthread_mutex_t      g_cache_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

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
            char* b = g_line + g_logging;
            strncpy(b, fmt + 2, LOG_LINE_MAX - 1 - g_logging); 
            g_line[LOG_LINE_MAX - 1] = '\0';
            g_logging += (int)strlen(b);
        }
        else g_logging += vsnprintf(g_line + g_logging, LOG_LINE_MAX - g_logging, fmt, params);
        return;
    }

    // 如果之前开启了缓存模式，且缓存内容为空，则关闭缓存模式，直接返回
    if (g_logging == 0) {
        g_logging = -1;
        return;
    }

    // 输出到 instrument（如果启用）
    #ifdef LOG_INSTRUMENT
    instrument_slot((uint8_t)level, tag, fmt, params);
    #endif

    int n;
    if (g_logging > 0) {                    // 对于缓存模式的最终输出
        n = g_logging; g_logging = -1;      // 关闭缓存模式
        if (n < LOG_LINE_MAX) { 
            if (*fmt == '%' && fmt[1] == ' ')
                n += snprintf(g_line+n, sizeof(g_line)-n, "%s", fmt + 2);
            else n += vsnprintf(g_line+n, sizeof(g_line)-n, fmt, params);
        }
    } else if (g_tag_separate) {
        if (*fmt == '%' && fmt[1] == ' ')
            n = snprintf(g_line, sizeof(g_line), "%s", fmt + 2);
        else n = vsnprintf(g_line, sizeof(g_line), fmt, params);
    } else if (*fmt == '%' && fmt[1] == ' ')  {
        n = snprintf(g_line, sizeof(g_line), "%s %s", tag, fmt + 2);
    } else {    
        n = snprintf(g_line, sizeof(g_line), "%s ", tag);
        if (n < LOG_LINE_MAX - 1) {
            n += vsnprintf(g_line + n, LOG_LINE_MAX - n, fmt, params);
        }
    }

    if (n >= LOG_LINE_MAX) { n = LOG_LINE_MAX - 1;
        g_line[n] = 0;
    }

    // 对于标准输出
    if (g_cb_log == (log_cb)-1) {
        if (g_line[--n] == '\n') g_line[n] = 0; // 移除末尾换行符（避免重复添加）
        printf("%s\n", g_line);                 // 标准输出自动添加换行符
    }
    // 对于回调（包括系统日志）
    else if (g_cb_log) {
        g_cb_log(level, g_tag_separate ? tag : NULL, g_line, n);
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
          fprintf(fp, "%s %s\n", tag, g_line);
          fclose(fp);

          log_append = true;
      }
      else {

#if LOG_TAG_SEPARATE
          void** item = (void**)malloc(n + strlen(tag) + 3/* ' ' + \r\0 */ + sizeof(void*)); *item = NULL;
          sprintf((char*)(item+1), "%s %s\n", tag, g_line);
#else
          void** item = (void**)malloc(len + 2/* \r\0 */ + sizeof(void*)); *item = NULL;
          sprintf((char*)(item+1), "%s\n", g_line);
#endif
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

static bool inst_init(void) {

    assert(g_inst_sock == P_INVALID_SOCKET);
    g_inst_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_inst_sock == P_INVALID_SOCKET) return false;

    int opt = 1;
    setsockopt(g_inst_sock, SOL_SOCKET, SO_BROADCAST, (const char*)&opt, sizeof(opt));
    setsockopt(g_inst_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

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

    // 广播目标地址
    memset(&g_inst_dest, 0, sizeof(g_inst_dest));
    g_inst_dest.sin_family      = AF_INET;
    g_inst_dest.sin_port        = htons(g_inst_port);
    g_inst_dest.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    // 生成随机 rid
    g_inst_rid = (uint16_t)(P_tick_us() ^ (uintptr_t)&g_inst_sock);

    // 设置接收超时（100ms），供线程周期性检查 g_inst_running
    P_sock_rcvtimeo(g_inst_sock, 100);

    // 启动接收线程（统一处理选项包）
    g_inst_running = true;
    if (P_thread(&g_inst_thread, inst_thread_proc, NULL, P_THD_BACKGROUND, 0) != E_NONE) {
        g_inst_running = false;
        P_sock_close(g_inst_sock);
        g_inst_sock = P_INVALID_SOCKET;
        return false;
    }

    atexit(inst_cleanup);
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

// 发送 type=1 包：offset(2) + byte(1)
static void inst_send_bits(uint16_t byte_idx) {
    if (g_inst_sock == P_INVALID_SOCKET) return;
    if (byte_idx >= g_inst_bits_len) return;

    uint8_t pkt[INST_HDR_SIZE + 3];
    nwrite_s(pkt, g_inst_rid);
    uint16_t seq = g_inst_seq++;
    nwrite_s(pkt + 2, seq);
    pkt[4] = 1;                                     // type=1 选项包
    nwrite_s(pkt + 5, byte_idx);
    pkt[7] = g_inst_bits[byte_idx];

    sendto(g_inst_sock, (const char*)pkt, sizeof(pkt), 0,
           (struct sockaddr*)&g_inst_dest, sizeof(g_inst_dest));
}

ret_t
instrument_enable(uint16_t idx, bool enable) {

    if (g_inst_sock == P_INVALID_SOCKET && !inst_init())
        return E_EXTERNAL(P_sock_errno());

    uint16_t byte_idx = idx / 8;
    uint8_t  bit_mask = (uint8_t)(1u << (idx % 8));

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
    uint16_t byte_idx = idx / 8;
    if (byte_idx >= g_inst_bits_len) return false;
    return (g_inst_bits[byte_idx] & (1u << (idx % 8))) != 0;
}

// ---- 发送端 ----

void
instrument_slot(uint8_t chn, const char* tag, const char* fmt, va_list params) {

    if (g_inst_sock == P_INVALID_SOCKET && !inst_init()) return;

    uint8_t pkt[INST_UDP_MAX];
    int n = INST_HDR_SIZE;

    // 包头: rid(2) + seq(2, big-endian) + type(1)
    nwrite_s(pkt, g_inst_rid);
    uint16_t seq = g_inst_seq++;
    nwrite_s(pkt + 2, seq);
    pkt[4] = 0;                                     // type=0 数据包

    // 负载: chn(1) + tag_len(1) + tag(N) + text(M)
    pkt[n++] = chn;
    int tag_len = tag ? (int)strlen(tag) : 0;
    if (tag_len > 255) tag_len = 255;
    pkt[n++] = (uint8_t)tag_len;
    if (tag_len > 0) { memcpy(pkt + n, tag, tag_len); n += tag_len; }

    int avail = (int)sizeof(pkt) - n;
    if (avail <= 0) return;

    int text_len;
    if (*fmt == '%' && fmt[1] == ' ')
        text_len = snprintf((char*)(pkt + n), avail, "%s", fmt + 2);
    else
        text_len = vsnprintf((char*)(pkt + n), avail, fmt, params);
    if (text_len < 0) text_len = 0;
    if (text_len >= avail) text_len = avail - 1;
    n += text_len;

    sendto(g_inst_sock, (const char*)pkt, n, 0,
           (struct sockaddr*)&g_inst_dest, sizeof(g_inst_dest));
}

// ---- 监听端 ----

ret_t
instrument_listen(instrument_cb cb) {

    if (g_inst_sock == P_INVALID_SOCKET && !inst_init())
        return E_EXTERNAL(P_sock_errno());

    g_inst_cb     = cb;
    memset(g_inst_win, 0, sizeof(g_inst_win));
    g_inst_synced = false;

    return E_NONE;
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

// 解析并交付一个数据包负载到回调
static void inst_deliver(uint8_t *payload, int len) {
    if (!g_inst_cb || len < 2) return;

    uint8_t chn     = payload[0];
    uint8_t tag_len = payload[1];
    if (2 + tag_len > len) return;

    char tag[256];
    memcpy(tag, payload + 2, tag_len);
    tag[tag_len] = '\0';

    char *text     = (char*)(payload + 2 + tag_len);
    int   text_len = len - 2 - tag_len;
    text[text_len] = '\0';                          // 安全：inst_slot_t.data 有 +1 余量

    g_inst_cb(chn, tag, text, text_len);
}

// 从 g_inst_next 开始连续交付已缓冲的包
static int inst_flush(void) {
    int count = 0;
    for (;;) {
        int idx = g_inst_next & INST_WINDOW_MASK;
        if (g_inst_win[idx].len == 0) break;

        uint8_t *pkt = g_inst_win[idx].data;
        int pkt_len  = g_inst_win[idx].len;
        if (pkt_len > INST_HDR_SIZE)
            inst_deliver(pkt + INST_HDR_SIZE, pkt_len - INST_HDR_SIZE);

        g_inst_win[idx].len = 0;
        g_inst_next++;
        count++;
    }
    return count;
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

        // 首包同步
        if (!g_inst_synced) { g_inst_synced = true;
            g_inst_next = seq; 
        }

        int16_t diff = (int16_t)(seq - g_inst_next);
        if (diff < 0) continue;                     // 旧包/重复

        // 超出窗口 → 强制交付已缓冲包，跳跃到新 seq
        if (diff >= INST_WINDOW_SIZE) {
            fprintf(stderr, "inst: lost %d packets (seq %u → %u)\n", (int)diff, g_inst_next, seq);
            for (int i = 0; i < INST_WINDOW_SIZE; i++) {
                int idx = (g_inst_next + i) & INST_WINDOW_MASK;
                if (g_inst_win[idx].len > 0) {
                    if (g_inst_win[idx].len > INST_HDR_SIZE)
                        inst_deliver(g_inst_win[idx].data + INST_HDR_SIZE,
                                     g_inst_win[idx].len  - INST_HDR_SIZE);
                    g_inst_win[idx].len = 0;
                }
            }
            g_inst_next = seq;
            diff = 0;
        }

        // 缓冲到窗口槽
        int idx = seq & INST_WINDOW_MASK;
        if (g_inst_win[idx].len == 0) {
            memcpy(g_inst_win[idx].data, buf, n);
            g_inst_win[idx].len = n;
        }

        // 顺序交付
        inst_flush();
    }
    return 0;
}

#endif

///////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic pop