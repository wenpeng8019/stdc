#ifndef STDC_H_
#define STDC_H_

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma ide diagnostic ignored "OCUnusedMacroInspection"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "UnreachableCallsOfFunction"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <float.h>
#include <inttypes.h>           /* PRIu64, ... */
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <time.h>

//-----------------------------------------------------------------------------

// defined(_WIN32) 也包含 defined(_WIN64)
#if defined(_WIN32)
#define P_WIN 1
#else
#define P_WIN 0
#endif

// __APPLE__ 说明是 Apple 平台; __MACH__ 说明内核是 Darwin / Unix
#if defined(__APPLE__) && defined(__MACH__)
#define P_DARWIN 1
#else
#define P_DARWIN 0
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define P_BSD 1
#else
#define P_BSD 0
#endif

// 其他常见 Unix-like 系统标识：__ANDROID__/__QNX__/__CYGWIN__/__MINGW32__/__MINGW64__/__EMSCRIPTEN__/__WASM__

// Unix-like (macOS, BSD, etc.), __unix 是旧 gcc 兼容
#if defined(__unix__) || defined(__unix)
#define P_UNIX 1
#else
#define P_UNIX 0
#endif

// __linux 是旧 gcc 兼容
#if defined(__linux__) || defined(__linux)
#define P_LINUX 1
#else
#define P_LINUX 0
#endif

#if P_WIN
#   include <windows.h>
#endif
#if P_DARWIN
#   include <Availability.h>  /* 用于检查 macOS 版本、以及系统 API 可用性 */
#endif
#if !P_WIN || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__)
#   include <unistd.h>
#   include <errno.h>
#endif

// POSIX (Linux, macOS, BSD, etc.)，该宏依赖 unistd.h 头文件
#if defined(_POSIX_VERSION)
#define P_POSIX 1
#else
#define P_POSIX 0
#endif

// MINGW 并不是一个真正的 POSIX 环境，但它提供了部分 POSIX API 的兼容实现
#if P_POSIX || defined(__MINGW32__) || defined(__MINGW64__)
#define P_POSIX_LIKE 1
#else
#define P_POSIX_LIKE 0
#endif

//-----------------------------------------------------------------------------
// 编译器一致性： __GNUC__ 包含 clang 和 gcc; _MSC_VER 包含 Visual Studio

#if P_POSIX_LIKE
#define P_SEP '/'
#else
#define P_SEP '\\'
#endif

#if P_WIN
    #define P_IS_SEP(c) ((c) == '/' || (c) == '\\')
#else
    #define P_IS_SEP(c) ((c) == '/')
#endif

#ifndef __FILE_NAME__
#define __FILE_NAME__  (strrchr(__FILE__, P_SEP) ? strrchr(__FILE__, P_SEP) + 1 : __FILE__)
#endif

// c++ 优先使用 thread_local，它是 c++ 11 的标准
#if defined(__cplusplus)
#define TLS thread_local
#elif defined(_MSC_VER)
#define TLS __declspec(thread)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define TLS _Thread_local
#elif defined(__GNUC__) || defined(__clang__)
#define TLS __thread
#else
#error "TLS not supported on this compiler"
#endif

#ifdef __cplusplus
extern "C" {
#endif
///////////////////////////////////////////////////////////////////////////////

#ifdef NDEBUG
#define P_check(expr, ...) { if (!(expr)) { print("E: %s@%d(%d)", __FUNCTION__, __LINE__, (int)(long)(expr)); __VA_ARGS__ ;} }
#else
#define P_check(expr, ...) assert(expr); if (!(expr)) exit(-1);
#endif

typedef char*                   str_t;
typedef const char*             cstr_t;

typedef int32_t                 ret_t;
#define E_NONE                  0
#define E_UNKNOWN               (-1)                /* 未知错误 */
#define E_INVALID               (-2)                /* 指定的参数或操作无效。不满足规范或规定 */
#define E_CONFLICT              (-3)                /* 冲突指的是排它。即运行我的前提是没有它；或者对于单例资源来说，现在正在被（别人）使用，或者之前的使用没有被正确释放。 */
#define E_OUT_OF_RANGE          (-4)                /* 请求的范围超出限制 */
#define E_OUT_OF_MEMORY         (-5)                /* 内存不足（最基本的资源不足） */
#define E_OUT_OF_SUPPLY         (-6)                /* （通用的）请求数量大于供给能力，或者数据下溢（underflow） */
#define E_OUT_OF_CAPACITY       (-7)                /* （通用的）数据量超出设计容量；或者数据溢出（overflow） */
#define E_NONE_EXISTS           (-8)                /* 访问项不存在 */
#define E_NONE_CONTEXT          (-9)                /* 不具备可运行的状态。常见的就是没有初始化或启动 */
#define E_NONE_RELEASED         (-10)               /* 当前（资源）项未被释放，无法被析构或再次分配 */
#define E_BUSY                  (-11)               /* 当前无法运行或答复，需要稍后重试 */
#define E_TIMEOUT               (-12)               /* 超时 */
#define E_NO_PERMISSION         (-13)               /* 无权限访问 */
#define E_NO_SUPPORT            (-14)               /* 程序不支持（未设计）该功能 */
#define E_CUSTOM(e)             (-100-e)            /* 自定义错误 */

#define E_EXTERNAL(e)           (-((e)<<8))         /* 外部（其他系统）定义的错误 */
#define E_EXT_CODE(e)           ((-(e))>>8)         /* 获取外部（其他系统）定义的错误 */

typedef union {
    bool                    b;
    int8_t                  i8;
    uint8_t                 u8;
    int16_t                 i16;
    uint16_t                u16;
    int32_t                 i32;
    uint32_t                u32;
    int64_t                 i64;
    uint64_t                u64;
    float                   f32;
    double                  f64;
    str_t                   s;
    cstr_t                  cs;
    void*                   p;
    const void*             cp;
}                               var_t;

// 使用 P_xxx 操作有利于 hook 操作，后续可替换为自定义内存分配器
#ifndef P_alloc
#define P_alloc(size)           malloc(size)
#endif
#ifndef P_realloc
#define P_realloc(ptr, size)    realloc(ptr, size)
#endif
#ifndef P_free
#define P_free(ptr)             free(ptr)
#endif

typedef const char*(*lang_cb)(const char* cstr);
extern lang_cb                  P_lang;
#define P_LA(cstr)              (P_lang ? P_lang(cstr) : (cstr))

///////////////////////////////////////////////////////////////////////////////

#ifndef LOG_INSTRUMENT
#ifndef NDEBUG
#define LOG_INSTRUMENT
#endif
#endif

#ifndef ROOT_TAG
#define ROOT_TAG            ""                      /* 必须是预定义常量字符串 */
#endif
#ifndef MOD_TAG
#define MOD_TAG             ""                      /* 可以是一个字符串函数 */
#endif

#ifndef LOG_DEF
#define LOG_DEF             LOG_SLOT_DEBUG
#endif

#ifndef LOG_TAG_MAX
#define LOG_TAG_MAX         (-24)                   /* tag 格式化布局, abs 大小为 tag 宽度
                                                     * >0: 固定宽度，左对齐, 右侧补空格;
                                                     * <0: 固定宽度，右对齐, 左侧补空格;
                                                     * =0: 非固定宽度输出, tag 最大 128 字符 */
#endif
#ifndef LOG_TAG_L
#define LOG_TAG_L           "["
#endif
#ifndef LOG_TAG_R
#define LOG_TAG_R           "]"
#endif

#ifndef LOG_TAG_P
#define LOG_TAG_P           false
#endif

#define LOG_LINE_MAX        2048

typedef enum {
    LOG_SLOT_NONE = 0,
    LOG_SLOT_FATAL,                                 /* 导致程序无法再继续运行的错误 */
    LOG_SLOT_ERROR,                                 /* 不应该发生的错误，但不影响程序继续运行 */
    LOG_SLOT_WARN,                                  /* 警告信息。既可能会产生问题，或无法达到预期 */
    LOG_SLOT_INFO,                                  /* 运行状态 */
    LOG_SLOT_DEBUG,                                 /* 用于程序调试 */
    LOG_SLOT_VERBOSE,                               /* 任何信息（常用于输出说明文档） */
} log_level_e;

// 这里的 txt 是可以写入变更的，但需要确保编辑的区域 <= len+1，例如增加一个 \n，或移除末尾的 \n（如果存在）
typedef void(*log_cb)(log_level_e level, const char* tag, char *txt, int len);

/**
 * @brief 定义日志输出目标
 * @note 几个特殊值：
 *   - (log_cb)-1: 输出到 stdout（默认）
 *   - (log_cb)-2: 输出到系统日志（os_log/syslog/OutputDebugString等）
 */
#ifndef LOG_CALLBACK
#define LOG_CALLBACK        ((log_cb)-1)
#endif

#ifndef LOG_LEVEL
#ifndef NDEBUG
#define LOG_LEVEL           LOG_SLOT_DEBUG
#else
#define LOG_LEVEL           LOG_SLOT_INFO
#endif
#endif

void
log_output(cstr_t filename, uint32_t sz_max);

/**
 * @brief 日志输出
 *        该操作的主要场景是被 print() 自动调用。应用层应该首选 print()，而不是直接调用 log_slot()
 *        该操作的内部会自动根据不同平台、或配置来选择目标输出（stdout、系统日志、自定义回调、或文件）
 * @param level 日志级别
 * @param tag 日志标签（模块名、文件名等）
 * @param fmt 格式字符串（支持 printf 风格）
 * @note  这里的 fmt 支持一个特殊用法：
 *        即如果 fmt 以 '% ' 开头，即 '%' + 空格，则视为 fmt 是一个非格式化字符串
 *        此时会直接将 fmt 中的内容原样输出，相当于 printf("%s", fmt + 2)
 * @param ... 可变参数
 * @param pre_tag 将 tag 作为日志内容的一部分，先于日志文本输出。而不是单独作为标签参数传递给回调函数
 */
void
log_slot(log_level_e level, const char* tag, const char* fmt, va_list params, log_cb cb_log, bool pre_tag);

//-----------------------------------------------------------------------------

/**
 * @brief                       instrument 消息回调接口定义
 * @param rid                   发送方节点 ID（本地触发时为 0）
 * @param chn                   消息通道 (当前用于传输日志时对应 log_level_e)
 * @param tag                   消息标签
 * @param txt                   消息文本 (已追加 '\0' 终止符)
 * @param len                   消息文本长度 (不含 '\0')
 */
typedef void(*instrument_cb)(uint16_t rid, uint8_t chn, const char* tag, char *txt, int len);

#ifdef LOG_INSTRUMENT

#ifndef INSTRUMENT_PORT
#define INSTRUMENT_PORT         1980
#endif

#ifndef INSTRUMENT_CTRL
#define INSTRUMENT_CTRL         255
#endif

#ifndef INSTRUMENT_OPT_BASE
#define INSTRUMENT_OPT_BASE     0
#endif

#define INST_PORT_MAX           32                                  // 端口/标识名称最大长度

/**
 * @brief                       设置 instrument 通信端口
 * @param port                  UDP 端口号
 * @note                        必须在首次调用其他 instrument_* 函数之前调用
 *                              不执行该操作，则默认端口为 INSTRUMENT_PORT
 */
void instrument_port(uint16_t port);

/**
 * @brief                       设置 instrument 控制通道
 * @param chn                   通道号
 * @note                        必须在首次调用其他 instrument_* 函数之前调用
 *                              不执行该操作，则默认通道为 INSTRUMENT_CTRL
 */
void instrument_ctrl(uint16_t chn);

/**
 * @brief                       设置 instrument 内部日志回调。
 *                              默认不会输出内部日志
 *                              -1 表示输出到 stdout（默认），-2 表示输出到系统日志
 * @param cb                    日志回调函数
 * @note                        目前 instrument 内部日志主要就是接收线程的调试输出
 */
void instrument_loggable(log_cb cb/* nullable */);

/**
 * @brief                       设置进程本地模式
 * @param keep_chn              保留的通道列表，以 0 结尾
 * @note                        调用后 instrument_slot 只触发本地 instrument_cb 回调，
 *                              且仅保留通道的消息会发送到网络
 *                              默认为主机模式（host）：本地回调 + 127.0.0.1 回环地址
 * @example
 *                              instrument_local(0);         // 关闭全部网络发送
 *                              instrument_local('x', 0);    // 保留 'x' 通道
 *                              instrument_local('x', 'y', 0); // 保留 'x' 和 'y' 通道
 */
void instrument_local(int keep_chn, ...);

/**
 * @brief                       设置远程广播模式
 * @note                        调用后 instrument_slot 向局域网广播消息
 *                              默认为主机模式（host）：本地回调 + 127.0.0.1 回环地址
 */
void instrument_remote(void);

/**
 * @brief                       发送 instrument 消息包 (内部调用)
 * @param chn                   消息通道
 * @param tag                   消息标签
 * @param fmt                   格式化字符串
 * @param params                可变参数列表
 * @note                        以 UDP 广播方式发送到局域网
 *                              首次调用时自动初始化 socket，进程退出时自动关闭
 */
void instrument_slot(uint8_t chn, const char* tag, const char* fmt, va_list params);

/**
 * @brief                       启动 instrument 监听
 * @param cb                    消息回调函数，按 seq 顺序交付
 * @param id                    本方标识（用于 instrument_req 定向匹配）
 *                              NULL=不接受任何请求，空串=接受所有请求，非空=精确匹配
 * @return                      E_NONE 成功，否则返回错误码
 * @note                        内部启动接收线程，处理乱序和丢包
 *                              同时初始化发送端 socket（监听端也可发送）
 *                              丢包时会输出 stderr 警告信息
 */
ret_t instrument_listen(instrument_cb cb, cstr_t id/* nullable */);

/**
 * @brief                       启用/禁用指定的 instrument 选项
 * @param idx                   选项索引 (0-based)
 * @param enable                true=启用, false=禁用
 * @return                      E_NONE 成功，否则返回错误码
 * @note                        选项状态通过 UDP 广播同步到所有节点
 *                              可用于远程控制功能开关
 */
ret_t instrument_set(uint16_t idx, bool enable);

/**
 * @brief                       查询指定 instrument 选项是否启用
 * @param idx                   选项索引 (0-based)
 * @return                      true=已启用, false=未启用或索引越界
 */
bool instrument_get(uint16_t idx);

#define instrument_option(idx)       (instrument_get(INSTRUMENT_OPT_BASE + (idx)))
#define instrument_enable(idx, en)   (instrument_set(INSTRUMENT_OPT_BASE + (idx), en))

/**
 * @brief                       阻塞等待对端调用 instrument_continue
 * @param port                  本方名称（广播给对端，作为 WAIT 消息内容）
 * @param from                  期望的应答方名称（NULL 或空串表示任意方均可中断）
 * @param timeout_ms            超时时间（毫秒），0 表示无限等待
 * @return                      E_NONE 收到 continue，E_TIMEOUT 超时
 * @note                        内部定期广播 WAIT 包，收到匹配的 CONTINUE 后返回
 *                              WAIT 消息通过 instrument_cb(rid, 255, NULL, waiting, len) 通知监听方
 */
ret_t instrument_wait(cstr_t port, cstr_t from, uint32_t timeout_ms);

/**
 * @brief                       发送 CONTINUE 应答，中断对端的 instrument_wait
 * @param to                    目标方名称（WAIT 方的 waiting 参数）
 * @param from                  本方名称（WAIT 方的 from 参数匹配用）
 * @return                      E_NONE 成功
 */
ret_t instrument_continue(cstr_t to, cstr_t from);

/**
 * @brief                       向目标发送请求并同步等待响应
 * @param id                    目标方标识（匹配对方 instrument_listen 注册的 id）
 * @param timeout_ms            超时时间（毫秒），0 表示无限等待
 * @param msg                   请求标签（非 NULL，通过 instrument_cb 的 tag 参数传递）
 * @param buffer                入参：请求内容；出参：响应内容
 * @param bufsz                 buffer 大小
 * @return                      E_NONE 收到响应，E_TIMEOUT 超时
 * @note                        同步阻塞，不支持并发（所有远程调用串行执行）
 *                              接收方通过 instrument_cb(rid, ctrl, msg, content, len) 收到请求
 *                              tag 非 NULL 是与 WAIT 的区别标识
 */
ret_t instrument_req(cstr_t id, uint32_t timeout_ms, 
                     cstr_t msg, char *buffer, size_t bufsz);

/**
 * @brief                       响应 instrument_req 请求
 * @param rid                   请求方的 rid（来自 instrument_cb 的 rid 参数）
 * @param reply                 响应文本（写入请求方的 buffer）
 * @return                      E_NONE 成功
 */
ret_t instrument_resp(uint16_t rid, cstr_t reply);


extern int64_t instrument_tick;                  // <=0: 累计等待时长(us)取反; >0: wait中冻结的 tick_us

#else
#define instrument_port(...)     ((void)0)
#define instrument_ctrl(...)     ((void)0)
#define instrument_local(...)    ((void)0)
#define instrument_remote()      ((void)0)
#define instrument_slot(...)     ((void)0)
#define instrument_loggable(...) ((void)0)
#define instrument_listen(...)   ((ret_t)((volatile int){E_NONE}))
#define instrument_set(...)      ((ret_t)((volatile int){E_NONE}))
#define instrument_get(...)      ((volatile bool){false})
#define instrument_enable(...)   ((ret_t)((volatile int){E_NONE}))
#define instrument_option(...)   ((volatile bool){false})
#define instrument_wait(...)     ((ret_t)((volatile int){E_NONE}))
#define instrument_continue(...) ((ret_t)((volatile int){E_NONE}))
#define instrument_req(...)      ((ret_t)((volatile int){E_NONE}))
#define instrument_resp(...)     ((ret_t)((volatile int){E_NONE}))
#define instrument_tick          ((volatile int64_t){0})
#endif

//-----------------------------------------------------------------------------

static inline void print(const char* fmt, ...) {

    // +3: "[]\0"
#   if LOG_TAG_MAX > 0
    static char s_tag[LOG_TAG_MAX + 3];
#   elif LOG_TAG_MAX < 0
    static char s_tag[-(LOG_TAG_MAX) + 3];
#   else
    static char s_tag[128 + 3];
#   endif
    // 初始化 mod tag，只执行一次
    if (!*s_tag) {

        int n = sizeof(ROOT_TAG);                               // 注意 sizeof 得到的长度(即数组长度), 包括了 \0, 也就是 n = strlen + 1
        // 左对齐固定输出（右侧补空格）
#       if LOG_TAG_MAX > 0
        if (n > LOG_TAG_MAX)                                    // ROOT TAG 太长，大于等于 TAG_MAX，则只显示 ROOT TAG
            sprintf(s_tag, LOG_TAG_L "%.*s" LOG_TAG_R, LOG_TAG_MAX, ROOT_TAG);    // 限制最大输出（从后截断）
        else { int m = strlen(MOD_TAG);
            if (n + m <= LOG_TAG_MAX + 1)                       // 总长度小于等于固定宽，则全输出，右侧补空格
                sprintf(s_tag, LOG_TAG_L "%s%s%-*s" LOG_TAG_R, ROOT_TAG, MOD_TAG, LOG_TAG_MAX + 1 - n - m, "");
            else {                                              // 否则 ROOT 全输出，MOD 仅输出后部分，中间用 ".." 间隔
                int offset = (LOG_TAG_MAX - 1 - n); assert(offset >= -2);
                if (offset <= 0) sprintf(s_tag, LOG_TAG_L "%s%.*s" LOG_TAG_R, ROOT_TAG, 2+offset, "..");
                else sprintf(s_tag, LOG_TAG_L "%s..%s" LOG_TAG_R, ROOT_TAG, (char*)MOD_TAG + m - offset);
            }
        }
        // 右对齐固定输出（左侧补空格）
#       elif LOG_TAG_MAX < 0
        if (n > -(LOG_TAG_MAX))                                 // ROOT TAG 太长，大于等于 TAG_MAX，则只显示 ROOT TAG
            sprintf(s_tag, LOG_TAG_L "%.*s" LOG_TAG_R, -(LOG_TAG_MAX), ROOT_TAG); // 限制最大输出（从后截断）
        else { int m = strlen(MOD_TAG);
            if (n + m <= -(LOG_TAG_MAX) + 1)                    // 总长度小于等于固定宽，则 MOD 全输出、ROOT 固定宽度，宽度为总长度去除 MOD 长度，前面补空格
                sprintf(s_tag, LOG_TAG_L "%*s%s" LOG_TAG_R, -(LOG_TAG_MAX) - m, ROOT_TAG, MOD_TAG);
            else {                                              // 否则 ROOT 全输出，MOD 仅输出后部分，中间用 ".." 间隔
                int offset = (-(LOG_TAG_MAX) - 1 - n); assert(offset >= -2);
                if (offset <= 0) sprintf(s_tag, LOG_TAG_L "%s%.*s" LOG_TAG_R, ROOT_TAG, 2+offset, "..");
                else sprintf(s_tag, LOG_TAG_L "%s..%s" LOG_TAG_R, ROOT_TAG, (char*)MOD_TAG + m - offset);
            }
        }
        // 非固定宽度输出，则仅限制最大输出尺寸
#       else
        if (n > 128) sprintf(s_tag, LOG_TAG_L "%.*s" LOG_TAG_R, 128, ROOT_TAG);   // ROOT TAG 太长，大于等于 128，则只显示 ROOT TAG
        else sprintf(s_tag, LOG_TAG_L "%s%.*s" LOG_TAG_R, ROOT_TAG, 128 + 1 - n, (char*)MOD_TAG);
#       endif
    }

#ifndef NDEBUG
    // DEBUG 模式下，fmt 为空作为一种标识，即将文件名和行号作为 tag 输出，同时将 ... 中的第一个参数作为 fmt
    va_list args; va_start(args, fmt);
    const char* tag = s_tag;
    uint8_t chn = (uint8_t)LOG_DEF;
    static TLS char s_file[256];
    if (!fmt || !*fmt) {
        snprintf(s_file, sizeof(s_file), "%s:%d", va_arg(args, const char*), va_arg(args, int));
        tag = s_file;
        fmt = va_arg(args, const char*);
        chn = (uint8_t)LOG_SLOT_DEBUG;
    }
#else
    // RELEASE 模式下，会忽略这一形式的输出
    if (!fmt || !*fmt) return;
    va_list args; va_start(args, fmt);
    const char* tag = s_tag;
    uint8_t chn = (uint8_t)LOG_DEF;
#endif

    static TLS bool s_begin = false;

    if (*fmt == ':') {
        // 如果以双 ':' 开头，则在调整状态下，直接输出到标准输出
        if (*++fmt == ':') {
#ifndef NDEBUG
            fmt += fmt[1] == ' ' ? 2 : 1;               // 忽略 1 个且只忽略 1 个空格（即允许多个空格作为缩进）
            if (!*fmt) fmt = va_arg(args, const char*); // 支持 print("I:", fmt, ...) 形式的调用
            vprintf(fmt, args);
#endif
            va_end(args);
            return;
        }
        // 否则如果以单 ':' 开头，则开启缓存模式
        // + 注意，换成模式不支持 print(":", fmt, ...) 形式的调用
        //   因为 print(":") 被视为只开启缓存模式，但不输出任何内容

        if (s_begin) log_slot(LOG_SLOT_NONE, NULL, NULL, args, (log_cb)LOG_CALLBACK, LOG_TAG_P); // 如果之前已经开启缓存，则清空缓存，从头开始
        else { s_begin = true;
            if (*fmt == ' ') ++fmt;                     // 忽略 1 个且只忽略 1 个空格（即允许多个空格作为缩进）
            log_slot(LOG_SLOT_NONE, NULL, fmt, args, (log_cb)LOG_CALLBACK, LOG_TAG_P);
        }
        va_end(args);
        return;
    }

    if (fmt[1] == ':') {
        s_begin = false;                                // 只要指定 chn 标识，就会关闭缓存模式
        const char *q="FEWIDV", *p = strchr(q, *fmt);
        if (p) { chn = (uint8_t)(p - q) + 1;
            fmt+=2; if (*fmt == ' ') ++fmt;             // 忽略 1 个且只忽略 1 个空格（即允许多个空格作为缩进）
            if (!*fmt) fmt = va_arg(args, const char*); // 支持 print("I:", fmt, ...) 形式的调用
        }
        else {
#ifdef LOG_INSTRUMENT
            chn = (uint8_t)(*fmt);
            fmt+=2; if (*fmt == ' ') ++fmt;
            if (!*fmt) fmt = va_arg(args, const char*); // 支持 print("E:", fmt, ...) 形式的调用
            instrument_slot(chn, tag, fmt, args);       // 如果指定的 chn 不合法，则视为 instrument 输出
#endif
            va_end (args);
            return;
        }
    }

    if (s_begin) log_slot(LOG_SLOT_NONE, NULL, fmt, args, (log_cb)LOG_CALLBACK, LOG_TAG_P);
    else if (chn <= LOG_LEVEL)
        log_slot(chn, tag, fmt, args, (log_cb)LOG_CALLBACK, LOG_TAG_P);
#ifdef LOG_INSTRUMENT
    else instrument_slot(chn, tag, fmt, args);
#endif

    va_end (args);
}

#ifndef NDEBUG
#define printf(...)     print(NULL, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define instrument      print
#else
#define printf(...)     ((void)0)
#define instrument(...) ((void)0)
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 * args 使用示例:
 * -------------------------------------------------------------------------
 *
 * // 定义参数（全局作用域）
 * ARGS_B(false, verbose, 'v', "verbose", "Enable verbose output");
 * ARGS_S(true,  input,   'i', "input",   "Input file path");
 * ARGS_I(false, count,   'n', "count",   "Number of iterations");
 *
 * int main(int argc, char** argv) {
 *
 *     // 设置帮助信息（可选），usage_ex 中的 $0 会被替换为程序名
 *     ARGS_usage("<file>...",
 *         "Examples:\n"
 *         "  $0 -i input.txt -n 10 file1 file2\n"
 *         "  $0 --verbose -i data.bin");
 *
 *     // 解析参数
 *     int pos_count = ARGS_parse(argc, argv,
 *         &ARGS_DEF_verbose,
 *         &ARGS_DEF_input,
 *         &ARGS_DEF_count,
 *         NULL);
 *
 *     // 使用参数
 *     if (ARGS_verbose.i64) printf("Verbose mode\n");
 *     printf("Input: %s\n", ARGS_input.str);
 *     printf("Count: %lld\n", ARGS_count.i64);
 *
 *     // 访问位置参数
 *     for (int i = 0; i < pos_count; i++) {
 *         printf("Positional[%d]: %s\n", i, argv[argc - pos_count + i]);
 *     }
 *     return 0;
 * }
 * -------------------------------------------------------------------------
 */

typedef enum arg_type {
    ARG_FLOAT = -3,
    ARG_INT,
    ARG_BOOL,
    ARG_STR,
    ARG_DIR,                    // 目录路径：自动规范化：展开~、移除末尾/、合并//、处理/./
    ARG_LS,                     // 字符串列表：逗号分隔的字符串数组。此时 ARGS_xxx.ls[i] 可获取列表项; ARGS_ls_count(&ARGS_xxx) 可获取列表数量
    ARG_PRE,                    // 预处理回调：在解析命令行参数过程中触发。常用于指定语言选项，可在执行 show_help 之前执行
                                // 回调接口定义：void prev_cb(char* argv)
} arg_type_e;

/**
 * 参数值联合体
 *
 * 各类型的默认值（选项未出现在命令行时）：
 *   ARG_BOOL   -> i64 = 0
 *   ARG_INT    -> i64 = 0
 *   ARG_FLOAT  -> f64 = 0.0
 *   ARG_STR    -> str = NULL
 *   ARG_DIR    -> str = NULL
 *   ARG_LS     -> ls  = NULL
 *
 * 各类型对空字符串值（如 --opt ""）的处理：
 *   ARG_INT    -> 0   (strtoll 返回 0)
 *   ARG_FLOAT  -> 0.0 (strtod 返回 0.0)
 *   ARG_STR    -> ""  (指向空字符串)
 *   ARG_DIR    -> ""  (指向空字符串)
 *
 * @note 选项后缺少值时（如 -n 后无参数）会报错退出
 * @note INT/FLOAT 标记为必选时，无法区分"未设置"和"设置为0"
 */
typedef union arg_var {
    const char*  str;           // ARG_STR, ARG_DIR
    int64_t      i64;           // ARG_BOOL, ARG_INT
    double       f64;           // ARG_FLOAT
    const char** ls;            // ARG_LS，用 ARGS_ls_count() 获取数量
} arg_var_st;

typedef struct arg_def {
    const char* name;
    const char* desc;
    arg_type_e  type;
    char        s;
    const char* l;
    bool        req;
    arg_var_st* var;
    struct arg_def* next;
} arg_def_st;

#define ARGS_DEF(req, name, type, s_cmd, l_cmd, desc)   \
extern arg_var_st ARGS_##name;                          \
arg_var_st ARGS_##name;                                 \
static arg_def_st ARGS_DEF_##name = {                   \
    #name, desc, ARG_##type, s_cmd, l_cmd, req,         \
    &ARGS_##name, NULL                                  \
}
#define ARGS_B(req, name, s_cmd, l_cmd, desc)   ARGS_DEF(req, name, BOOL, s_cmd, l_cmd, desc)
#define ARGS_I(req, name, s_cmd, l_cmd, desc)   ARGS_DEF(req, name, INT, s_cmd, l_cmd, desc)
#define ARGS_F(req, name, s_cmd, l_cmd, desc)   ARGS_DEF(req, name, FLOAT, s_cmd, l_cmd, desc)
#define ARGS_S(req, name, s_cmd, l_cmd, desc)   ARGS_DEF(req, name, STR, s_cmd, l_cmd, desc)
#define ARGS_D(req, name, s_cmd, l_cmd, desc)   ARGS_DEF(req, name, DIR, s_cmd, l_cmd, desc)
#define ARGS_L(req, name, s_cmd, l_cmd, desc)   ARGS_DEF(req, name, LS, s_cmd, l_cmd, desc)

#define ARGS_DFT(init, name, type, s_cmd, l_cmd, desc)  \
extern arg_var_st ARGS_##name;                          \
arg_var_st ARGS_##name = init;                          \
static arg_def_st ARGS_DEF_##name = {                   \
    #name, desc, ARG_##type, s_cmd, l_cmd, false,       \
    &ARGS_##name, NULL                                  \
}
#define ARGS_Bv(dft, name, s_cmd, l_cmd, desc)  ARGS_DFT({.i64=(dft)}, name, BOOL, s_cmd, l_cmd, desc)
#define ARGS_Iv(dft, name, s_cmd, l_cmd, desc)  ARGS_DFT({.i64=(dft)}, name, INT, s_cmd, l_cmd, desc)
#define ARGS_Fv(dft, name, s_cmd, l_cmd, desc)  ARGS_DFT({.f64=(dft)}, name, FLOAT, s_cmd, l_cmd, desc)
#define ARGS_Sv(dft, name, s_cmd, l_cmd, desc)  ARGS_DFT({.str=(dft)}, name, STR, s_cmd, l_cmd, desc)
#define ARGS_Dv(dft, name, s_cmd, l_cmd, desc)  ARGS_DFT({.str=(dft)}, name, DIR, s_cmd, l_cmd, desc)
#define ARGS_Lv(dft, name, s_cmd, l_cmd, desc)  ARGS_DFT({.ls=(dft)}, name, LS, s_cmd, l_cmd, desc)

#define ARGS_PRE(cb_pre, name, s_cmd, l_cmd, desc)      \
static arg_def_st ARGS_DEF_##name = {                   \
    #name, desc, ARG_PRE, s_cmd, l_cmd, false,          \
    (arg_var_st*)cb_pre, NULL                           \
}

#define ARGS(name) extern arg_var_st ARGS_##name

/**
 * 设置命令行帮助信息
 * @param pos_desc  位置参数描述，显示在 Usage 行中，如 "<subcommand>" 或 "<file>..."
 *                  传 NULL 时自动根据解析结果生成（有位置参数则显示 "ARGS..."）
 * @param usage_ex  额外的帮助说明，显示在选项列表之后，如子命令说明或使用示例
 *                  支持 $0 占位符，输出时会被替换为程序名（argv[0]）
 *                  传 NULL 或空字符串时不显示
 */
void ARGS_usage(const char* pos_desc, const char* usage_ex);

/**
 * 解析命令行参数
 * @param argc      main() 的 argc
 * @param argv      main() 的 argv，解析后会重排：选项在前，位置参数在后
 * @param ...       参数定义列表，以 NULL 结尾，每项为 &ARGS_DEF_xxx
 * @return          位置参数数量（可通过 argv[argc - 返回值] 访问位置参数）
 *
 * @note 遇到 -h/--help 或无参数时自动打印帮助并退出
 * @note 必选参数缺失时打印错误并退出
 */
int ARGS_parse(int argc, char** argv, .../* end with NULL */);

/**
 * 打印命令行帮助信息
 * @param arg0      程序名（通常传 argv[0]）
 * @return          始终返回 0
 *
 * @note 通常由 ARGS_parse 自动调用，也可手动调用
 */
int ARGS_print(const char* arg0);

/**
 * 获取列表类型参数的元素数量
 * @param var       ARGS_L 定义的参数变量指针
 * @return          列表元素数量，若未设置返回 0
 */
int ARGS_ls_count(arg_var_st* var);

///////////////////////////////////////////////////////////////////////////////
// 内存对齐
///////////////////////////////////////////////////////////////////////////////

// 返回类型的对齐要求（字节数）
#if defined(__cplusplus)
#   define ALIGN_OF(type)   alignof(type)
#elif __STDC_VERSION__ >= 201112L
#   define ALIGN_OF(type)   _Alignof(type)
#elif defined(_MSC_VER)
#   define ALIGN_OF(type)   __alignof(type)
#else
#   define ALIGN_OF(type)   __alignof__(type)
#endif

// 平台最大对齐要求（malloc/calloc 保证的对齐）
// + C11: stddef.h 定义了 max_align_t
// + 旧标准：手动定义为包含所有最大对齐类型的 union
#if __STDC_VERSION__ >= 201112L
    // C11 already defines max_align_t in stddef.h (included above)
#   define MAX_ALIGN        ALIGN_OF(max_align_t)
#elif defined(__cplusplus) && __cplusplus >= 201103L
    // C++11 has std::max_align_t
#   include <cstddef>
    typedef std::max_align_t max_align_t;
#   define MAX_ALIGN        ALIGN_OF(max_align_t)
#else
    // Pre-C11: define manually
    typedef union {
        long long ll;
        long double ld;
        void *p;
        double d;
    } max_align_t;
#   define MAX_ALIGN        ALIGN_OF(max_align_t)
#endif

// 声明对齐属性
// + 用法: ALIGN_AS(16) uint8_t buf[64];
// + 如果直接使用对象类型（如结构体）定义，则默认会对齐为类型的自然对齐，即无需显式通过 ALIGN_AS 指定对齐
#if defined(_MSC_VER)
#   define ALIGN_AS(n)      __declspec(align(n))
#else
#   define ALIGN_AS(n)      __attribute__((aligned(n)))
#endif

// 检查指针是否按指定字节数对齐
// + 用法: if (IS_ALIGNED(ptr, 4)) { ... }
#define IS_ALIGNED(ptr, n)  (((uintptr_t)(ptr) & ((n) - 1)) == 0)

// 主机字节序的未对齐安全读写（不做字节序转换，仅解决对齐问题）
// + 前提：数据字节序与本机一致（本机存、本机取）
// read_x/write_x: 通过指针读写，read_s(&result, bytes) / write_s(bytes, value)
// get_x: 返回值版本，uint16_t val = get_s(bytes)
// + memcpy 会被编译器优化为单条 load/store 指令
static inline uint16_t get_s(const void *src)  { uint16_t v; memcpy(&v, src, 2); return v; }
static inline uint32_t get_l(const void *src)  { uint32_t v; memcpy(&v, src, 4); return v; }
static inline uint64_t get_ll(const void *src) { uint64_t v; memcpy(&v, src, 8); return v; }
#define read_s(sp, bytes)   memcpy((sp), (bytes), 2)
#define read_l(lp, bytes)   memcpy((lp), (bytes), 4)
#define read_ll(llp, bytes) memcpy((llp), (bytes), 8)
#define write_s(bytes, s)   memcpy((bytes), &(s), 2)
#define write_l(bytes, l)   memcpy((bytes), &(l), 4)
#define write_ll(bytes, ll) memcpy((bytes), &(ll), 8)

///////////////////////////////////////////////////////////////////////////////
// 字节序
///////////////////////////////////////////////////////////////////////////////

#if P_WIN
#   define LITTLE_ENDIAN   1234
#   define BIG_ENDIAN      4321
#   define BYTE_ORDER      LITTLE_ENDIAN
    // 参考 winsock2.h 条件判断逻辑
    // + Windows: SDK 在 Windows 8+ 或定义 INCL_EXTRA_HTON_FUNCTIONS 时提供 htonll/ntohll
#   if !defined(INCL_EXTRA_HTON_FUNCTIONS) && \
       (!defined(NTDDI_VERSION) || NTDDI_VERSION < 0x06020000) /* NTDDI_WIN8 */
    static inline uint64_t htonll(uint64_t x) {
        return (((uint64_t)htonl((uint32_t)(x & 0xFFFFFFFFULL))) << 32) | (uint64_t)htonl((uint32_t)(x >> 32));
    }
    static inline uint64_t ntohll(uint64_t x) { return htonll(x); }
#   endif
#elif P_DARWIN
#   include <machine/endian.h>
#   include <libkern/OSByteOrder.h>
#   ifndef htonll
#       define htonll(x) OSSwapHostToBigInt64(x)
#   endif
#   ifndef ntohll
#       define ntohll(x) OSSwapBigToHostInt64(x)
#   endif
#elif P_LINUX || P_BSD
#   include <endian.h>
#   ifndef htonll
#       define htonll(x) htobe64(x)
#   endif
#   ifndef ntohll
#       define ntohll(x) be64toh(x)
#   endif
#else
#   include <sys/param.h>
    static inline uint64_t htonll(uint64_t x) {
        const uint32_t hi = htonl((uint32_t)(x >> 32));
        const uint32_t lo = htonl((uint32_t)(x & 0xFFFFFFFFULL));
        return ((uint64_t)lo << 32) | hi;
    }
    static inline uint64_t ntohll(uint64_t x) { return htonll(x); }
#endif

#if BYTE_ORDER == BIG_ENDIAN
#   define IS_BIG_ENDIAN 1
#   define IS_LITTLE_ENDIAN 0
#elif BYTE_ORDER == LITTLE_ENDIAN
#   define IS_BIG_ENDIAN 0
#   define IS_LITTLE_ENDIAN 1
#else
#error "Unknown byte order"
#endif

// 网络字节序（大端）的未对齐安全读写（同时完成字节序转换）
// nread/nwrite: 通过指针读写，nread_s(&result, bytes) / nwrite_s(bytes, value)
// nget: 返回值版本，uint16_t port = nget_s(bytes)
// + 避免未对齐访问，适用于从网络包 payload 直接读取/构造
#if IS_BIG_ENDIAN
#   define nread_s(bytes, result)   memcpy((result), (bytes), 2)
#   define nread_l(bytes, result)   memcpy((result), (bytes), 4)
#   define nread_ll(bytes, result)  memcpy((result), (bytes), 8)
#   define nwrite_s(h, bytes)       memcpy((bytes), &(h), 2)
#   define nwrite_l(h, bytes)       memcpy((bytes), &(h), 4)
#   define nwrite_ll(h, bytes)      memcpy((bytes), &(h), 8)
#   define nget_s(b)                get_s(b)
#   define nget_l(b)                get_l(b)
#   define nget_ll(b)               get_ll(b)
#else
#   define nread_s(sp, bytes)       (*(sp) = ((uint16_t)(bytes)[0] << 8) | (uint16_t)(bytes)[1])
#   define nread_l(lp, bytes)       (*(lp) = ((uint32_t)(bytes)[0] << 24) | \
                                             ((uint32_t)(bytes)[1] << 16) | \
                                             ((uint32_t)(bytes)[2] << 8) | \
                                             (uint32_t)(bytes)[3])
#   define nread_ll(llp, bytes)     (*(llp) = ((uint64_t)(bytes)[0] << 56) | \
                                              ((uint64_t)(bytes)[1] << 48) | \
                                              ((uint64_t)(bytes)[2] << 40) | \
                                              ((uint64_t)(bytes)[3] << 32) | \
                                              ((uint64_t)(bytes)[4] << 24) | \
                                              ((uint64_t)(bytes)[5] << 16) | \
                                              ((uint64_t)(bytes)[6] << 8) | \
                                              (uint64_t)(bytes)[7])
#   define nwrite_s(bytes, s)       ((bytes)[0] = (uint8_t)((s) >> 8), \
                                     (bytes)[1] = (uint8_t)((s) & 0xFF))
#   define nwrite_l(bytes, l)       ((bytes)[0] = (uint8_t)((l) >> 24), \
                                     (bytes)[1] = (uint8_t)((l) >> 16), \
                                     (bytes)[2] = (uint8_t)((l) >> 8), \
                                     (bytes)[3] = (uint8_t)((l) & 0xFF))
#   define nwrite_ll(bytes, ll)     ((bytes)[0] = (uint8_t)((ll) >> 56), \
                                     (bytes)[1] = (uint8_t)((ll) >> 48), \
                                     (bytes)[2] = (uint8_t)((ll) >> 40), \
                                     (bytes)[3] = (uint8_t)((ll) >> 32), \
                                     (bytes)[4] = (uint8_t)((ll) >> 24), \
                                     (bytes)[5] = (uint8_t)((ll) >> 16), \
                                     (bytes)[6] = (uint8_t)((ll) >> 8), \
                                     (bytes)[7] = (uint8_t)((ll) & 0xFF))
#   define nget_s(b)                (((uint16_t)(b)[0] << 8) | (uint16_t)(b)[1])
#   define nget_l(b)                (((uint32_t)(b)[0] << 24) | ((uint32_t)(b)[1] << 16) | \
                                     ((uint32_t)(b)[2] << 8)  | (uint32_t)(b)[3])
#   define nget_ll(b)               (((uint64_t)(b)[0] << 56) | ((uint64_t)(b)[1] << 48) | \
                                     ((uint64_t)(b)[2] << 40) | ((uint64_t)(b)[3] << 32) | \
                                     ((uint64_t)(b)[4] << 24) | ((uint64_t)(b)[5] << 16) | \
                                     ((uint64_t)(b)[6] << 8)  | (uint64_t)(b)[7])
#endif

static inline bool is_little_endian(void) { int i = 1; return *(char*)&i; }

///////////////////////////////////////////////////////////////////////////////
// 随机数生成
///////////////////////////////////////////////////////////////////////////////

/*
 * P_rand_init()   - 初始化随机数生成器（可选，仅降级方案需要）
 * P_rand32()      - 生成 32 位随机数（加密安全）
 * P_rand64()      - 生成 64 位随机数（加密安全）
 * P_rand_bytes()  - 填充随机字节
 *
 * 平台实现：
 *   - macOS/BSD:  arc4random() (ChaCha20, CSPRNG) - 无需初始化
 *   - Windows:    rand_s() (RtlGenRandom, CSPRNG) - 无需初始化
 *   - Linux:      /dev/urandom (内核 CSPRNG) - 随用随开，无需初始化
 *   - 降级方案:   srand(time) + rand() (自动初始化，仅用于测试)
 *
 * 性能考虑：
 *   - Linux 平台采用"随用随开"策略，每次打开/关闭 /dev/urandom
 *   - 现代内核的 open() 开销很小，适合中低频调用
 *   - 避免长期占用文件描述符，简化生命周期管理
 *
 * 返回值：非零随机数（0 保留为无效值）
 */

#if P_DARWIN || P_BSD
    // macOS/BSD: arc4random() 无需初始化
    static inline void P_rand_init(void) { /* 无操作 */ }

    static inline uint32_t P_rand32(void) {
        uint32_t r = arc4random();
        return r ? r : 1;  // 避免返回 0
    }

    static inline uint64_t P_rand64(void) {
        uint64_t r = ((uint64_t)arc4random() << 32) | arc4random();
        return r ? r : 1;
    }

#elif P_WIN
    // Windows: rand_s() 在 <stdlib.h> 中，需要定义 _CRT_RAND_S
    #ifndef _CRT_RAND_S
        #define _CRT_RAND_S
    #endif

    #if !(defined(_MSC_VER) && _MSC_VER >= 1400)
        static bool g_rand_initialized = false;
    #endif

    static inline void P_rand_init(void) {
        #if defined(_MSC_VER) && _MSC_VER >= 1400
            /* rand_s() 无需初始化 */
        #else
            /* 降级方案：初始化 rand() */
            if (!g_rand_initialized) {
                srand((unsigned int)time(NULL));
                g_rand_initialized = true;
            }
        #endif
    }

    static inline uint32_t P_rand32(void) {
        uint32_t r;
        #if defined(_MSC_VER) && _MSC_VER >= 1400
            if (rand_s(&r) != 0) r = (uint32_t)time(NULL);
        #else
            if (!g_rand_initialized) P_rand_init();
            r = (uint32_t)rand();  // 降级方案
        #endif
        return r ? r : 1;
    }

    static inline uint64_t P_rand64(void) {
        uint64_t r;
        #if defined(_MSC_VER) && _MSC_VER >= 1400
            uint32_t hi, lo;
            while (rand_s(&hi) != 0 || rand_s(&lo) != 0) { /* retry */ }
            r = ((uint64_t)hi << 32) | lo;
        #else
            if (!g_rand_initialized) P_rand_init();
            // 降级方案：组合多个 rand() 调用
            r = ((uint64_t)rand() << 48) ^ ((uint64_t)rand() << 32) ^
                ((uint64_t)rand() << 16) ^ (uint64_t)rand();
        #endif
        return r ? r : 1;
    }

#else
    // Linux/其他 POSIX: 使用 /dev/urandom（随用随开，避免长期占用文件描述符）
    static bool g_rand_initialized = false;

    static inline void P_rand_init(void) {
        if (!g_rand_initialized) {
            /* 初始化 rand() 作为降级方案 */
            srand((unsigned int)time(NULL));
            g_rand_initialized = true;
        }
    }

    static inline uint32_t P_rand32(void) {
        uint32_t r;
        FILE *fp = fopen("/dev/urandom", "rb");
        
        if (fp && fread(&r, sizeof(r), 1, fp) == 1) {
            fclose(fp);
            /* 成功从 /dev/urandom 读取 */
        } else {
            if (fp) fclose(fp);
            /* 降级方案：使用 rand() */
            if (!g_rand_initialized) P_rand_init();
            r = (uint32_t)rand();
        }
        return r ? r : 1;
    }

    static inline uint64_t P_rand64(void) {
        uint64_t r;
        FILE *fp = fopen("/dev/urandom", "rb");
        
        if (fp && fread(&r, sizeof(r), 1, fp) == 1) {
            fclose(fp);
            /* 成功从 /dev/urandom 读取 */
        } else {
            if (fp) fclose(fp);
            /* 降级方案：组合多个 rand() */
            if (!g_rand_initialized) P_rand_init();
            r = ((uint64_t)rand() << 48) ^ ((uint64_t)rand() << 32) ^
                ((uint64_t)rand() << 16) ^ (uint64_t)rand();
        }
        return r ? r : 1;
    }
#endif


/*
 * P_rand_bytes() - 填充缓冲区为随机字节
 * @param buf  目标缓冲区
 * @param len  字节数
 */
static inline void P_rand_bytes(void *buf, size_t len) {
    if (!buf || len == 0) return;
    
    uint8_t *p = (uint8_t *)buf;
    
    // 每次填充 4 字节，利用 P_rand32()
    while (len >= 4) {
        uint32_t r = P_rand32();
        memcpy(p, &r, 4);
        p += 4;
        len -= 4;
    }
    
    // 处理剩余的 1-3 字节
    if (len > 0) {
        uint32_t r = P_rand32();
        memcpy(p, &r, len);
    }
}

///////////////////////////////////////////////////////////////////////////////
// 系统时间和时钟
///////////////////////////////////////////////////////////////////////////////

#if P_POSIX_LIKE
typedef struct timespec P_clock;
// 新版本 MINGW 已经支持 clock_gettime 和 CLOCK_MONOTONIC，但这里统一用自己的实现，避免兼容性问题
#if !P_WIN
static inline ret_t P_clock_now(P_clock* clock) {
    if (!clock) return E_INVALID;
    return clock_gettime(CLOCK_MONOTONIC, clock) == 0 ? E_NONE : E_NO_SUPPORT;
}
#endif
#else
typedef struct timespec {
    time_t tv_sec;              /* seconds */
    long   tv_nsec;             /* nanoseconds */
} P_clock;
#endif

#if P_WIN
static inline ret_t P_time_now(P_clock* clock) {
        if (!clock) return E_INVALID;

        FILETIME ft;
        typedef VOID (WINAPI *P_GetSystemTimePreciseAsFileTime)(LPFILETIME);
        static P_GetSystemTimePreciseAsFileTime pGetSystemTimePreciseAsFileTime = NULL;
        static bool s_checked = false;
        if (!s_checked) {
            HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
            if (hKernel32)
                pGetSystemTimePreciseAsFileTime = (P_GetSystemTimePreciseAsFileTime)GetProcAddress(hKernel32, "GetSystemTimePreciseAsFileTime");
            s_checked = true;
        }
        if (pGetSystemTimePreciseAsFileTime)
            pGetSystemTimePreciseAsFileTime(&ft);
        else
            GetSystemTimeAsFileTime(&ft);

        uint64_t t100ns = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
        t100ns -= 116444736000000000ULL;
        clock->tv_sec = (time_t)(t100ns / 10000000ULL);
        clock->tv_nsec = (long)((t100ns % 10000000ULL) * 100ULL);
        return E_NONE;
    }
    static inline ret_t P_clock_now(P_clock* clock) {
        if (!clock) return E_INVALID;

        static LARGE_INTEGER ticks_per_sec = {0};
        if (!ticks_per_sec.QuadPart) {
            if (!QueryPerformanceFrequency(&ticks_per_sec) || !ticks_per_sec.QuadPart)
                return E_NO_SUPPORT;
        }

        LARGE_INTEGER ticks = {0};
        if (!QueryPerformanceCounter(&ticks)) return E_NO_SUPPORT;
        const uint64_t freq = (uint64_t)ticks_per_sec.QuadPart;
        const uint64_t cnt = (uint64_t)ticks.QuadPart;
        clock->tv_sec = (time_t)(cnt / freq);
        clock->tv_nsec = (long)((cnt % freq) * 1000000000ULL / freq);
        return E_NONE;
    }
//#   include "timeapi.h"
//#   include "sysinfoapi.h"
//#   include "processthreadsapi.h"
    static inline ret_t P_cost_now(P_clock* clock, bool bProcessOrThread) {
        if (!clock) return E_INVALID;

        FILETIME createTime, exitTime, kernelTime, userTime;
        BOOL ok;
        if (bProcessOrThread)
            ok = GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &kernelTime, &userTime);
        else
            ok = GetThreadTimes(GetCurrentThread(), &createTime, &exitTime, &kernelTime, &userTime);
        if (!ok)
            return E_EXTERNAL(GetLastError());

        ULARGE_INTEGER li_user, li_kernel;
        li_user.LowPart = userTime.dwLowDateTime;
        li_user.HighPart = userTime.dwHighDateTime;
        li_kernel.LowPart = kernelTime.dwLowDateTime;
        li_kernel.HighPart = kernelTime.dwHighDateTime;

        // Windows FILETIME: 100ns intervals since 1601-01-01
        // POSIX timespec: seconds/nanoseconds since 1970-01-01
        uint64_t total_100ns = (li_user.QuadPart + li_kernel.QuadPart);
        clock->tv_sec = (time_t)((total_100ns - 116444736000000000LL) / 10000000);
        clock->tv_nsec = (long)((total_100ns % 10000000) * 100);
        return E_NONE;
    }
#elif P_DARWIN
#   include <sys/time.h>
static inline ret_t P_time_now(P_clock* clock) {
    if (!clock) return E_INVALID;
#   if (defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101200) || \
    (defined(__IPHONE_OS_VERSION_MIN_REQUIRED) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 100000) || \
    (defined(__TV_OS_VERSION_MIN_REQUIRED) && __TV_OS_VERSION_MIN_REQUIRED >= 100000) || \
    (defined(__WATCH_OS_VERSION_MIN_REQUIRED) && __WATCH_OS_VERSION_MIN_REQUIRED >= 30000)
    if (clock_gettime(CLOCK_REALTIME, clock) == 0) return E_NONE;
    if (errno != ENOSYS && errno != EINVAL) return E_UNKNOWN;
#   endif
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) return E_UNKNOWN;
    clock->tv_sec = tv.tv_sec;
    clock->tv_nsec = (long)tv.tv_usec * 1000L;
    return E_NONE;
}
#   include <mach/mach.h>
#   include <mach/clock.h>
static inline ret_t P_cost_now(P_clock* clock, bool bProcessOrThread) {
    clock_serv_t cclock;
    host_get_clock_service(mach_host_self(), bProcessOrThread ? SYSTEM_CLOCK : CALENDAR_CLOCK, &cclock);
    mach_timespec_t mts;
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    clock->tv_sec = mts.tv_sec;
    clock->tv_nsec = mts.tv_nsec;
    return E_NONE;
}
#else /* other POSIX */
static inline ret_t P_time_now(P_clock* clock) {
        if (!clock) return E_INVALID;
        if (clock_gettime(CLOCK_REALTIME, clock) == 0) return E_NONE;
        return (errno == ENOSYS || errno == EINVAL) ? E_NO_SUPPORT : E_UNKNOWN;
    }
    #define P_cost_now(clock, bProcessOrThread) clock_gettime(bProcessOrThread ? CLOCK_PROCESS_CPUTIME_ID : CLOCK_THREAD_CPUTIME_ID, clock);
#endif

#if P_WIN
#define P_usleep(us) Sleep((DWORD)(((uint64_t)us + 999ULL) / 1000ULL))
#else
#define P_usleep(us) usleep(us)
#endif

#define clock_s_f(a)     ((a).tv_sec+(a).tv_nsec/1000000000.0)
#define clock_ms_f(a)    ((a).tv_sec*1000.0+(a).tv_nsec/1000000.0)
#define clock_us_f(a)    ((a).tv_sec*1000000.0+(a).tv_nsec/1000.0)
#define clock_s(a)       ((a).tv_sec+((a).tv_nsec+500000000)/1000000000)
#define clock_ms(a)      ((a).tv_sec*1000+((a).tv_nsec+500000)/1000000)
#define clock_us(a)      ((a).tv_sec*1000000+((a).tv_nsec+500)/1000)
// 注意，不要统一转换为 ns 进行计算，因为根据 long 类型精度计算，以 ns 表示的 s 最多只能表示 2s
#define clock_s_diff(a,b)     (((a).tv_sec-(b).tv_sec)+((a).tv_nsec-(b).tv_nsec+((a).tv_nsec>=(b).tv_nsec?500000000:-500000000))/1000000000)
#define clock_ms_diff(a,b)    (((a).tv_sec-(b).tv_sec)*1000+((a).tv_nsec-(b).tv_nsec+((a).tv_nsec>=(b).tv_nsec?500000:-500000))/1000000)
#define clock_us_diff(a,b)    (((a).tv_sec-(b).tv_sec)*1000000+((a).tv_nsec-(b).tv_nsec+((a).tv_nsec>=(b).tv_nsec?500:-500))/1000)
#define clock_dec(a,b,v) {                      \
    (v).tv_sec = (a).tv_sec - (b).tv_sec;       \
    (v).tv_nsec = (a).tv_nsec - (b).tv_nsec;    \
    if ((v).tv_nsec < 0) {                      \
        (v).tv_sec -= 1;                        \
        (v).tv_nsec += 1000000000;              \
    }                                           \
}
#define clock_inc(a,b,v) {                      \
    (v).tv_sec = (a).tv_sec + (b).tv_sec;       \
    (v).tv_nsec = (a).tv_nsec + (b).tv_nsec;    \
    if ((v).tv_nsec >= 1000000000) {            \
        (v).tv_sec += 1;                        \
        (v).tv_nsec -= 1000000000;              \
    }                                           \
}
#define clock_gt(a,b)      ((a).tv_sec>(b).tv_sec || (a).tv_sec==(b).tv_sec && (a).tv_nsec>(b).tv_nsec)
#define clock_ge(a,b)      ((a).tv_sec>(b).tv_sec || (a).tv_sec==(b).tv_sec && (a).tv_nsec>=(b).tv_nsec)

static uint64_t P_tick_s(void)  { if (instrument_tick > 0) return (uint64_t)instrument_tick / 1000000; P_clock _clk; P_clock_now(&_clk); return clock_s(_clk)  + (uint64_t)(instrument_tick / 1000000); }
static uint64_t P_tick_ms(void) { if (instrument_tick > 0) return (uint64_t)instrument_tick / 1000;    P_clock _clk; P_clock_now(&_clk); return clock_ms(_clk) + (uint64_t)(instrument_tick / 1000); }
static uint64_t P_tick_us(void) { if (instrument_tick > 0) return (uint64_t)instrument_tick;           P_clock _clk; P_clock_now(&_clk); return clock_us(_clk) + (uint64_t)instrument_tick; }

#define tick_diff(now, nlast)  ((now)>(nlast) ? (now)-(nlast) : 0)

// 循环序比较：判断 a 是否比 b 更新
static bool uint8_circle_newer(uint8_t a, uint8_t b) { uint8_t diff = (uint8_t)(a - b); return diff != 0 && diff < 128; }
static bool uint16_circle_newer(uint16_t a, uint16_t b) { int32_t diff = (int32_t)((int16_t)(a - b)); return diff > 0; }
static bool uint32_circle_newer(uint32_t a, uint32_t b) { uint32_t diff = a - b; return diff != 0 && diff < 0x80000000u; }

///////////////////////////////////////////////////////////////////////////////
// 文件系统访问
///////////////////////////////////////////////////////////////////////////////

#if P_WIN
#   include <direct.h>              // _mkdir, _getcwd, _get_pgmptr
#   include <io.h>                  // _access, _stat64
#   include <shlobj.h>              // SHGetKnownFolderPath
    typedef struct _stat64 stat_t;
#elif P_DARWIN
#   include <mach-o/dyld.h>         // _NSGetExecutablePath
#   include <sysdir.h>              // sysdir_start_search_path_enumeration
    typedef struct stat stat_t;
#elif P_BSD
#   include <sys/sysctl.h>          // sysctl
    typedef struct stat stat_t;
#else
    typedef struct stat64 stat_t;
#endif
#if P_POSIX_LIKE
#   include <dirent.h>              // opendir, readdir, closedir
#   include <sys/stat.h>            // stat, mkdir
#   include <pwd.h>                 // getpwuid
#endif

#ifndef PATH_MAX
#   if defined(FILENAME_MAX)
#       define PATH_MAX FILENAME_MAX
#   else
#       define PATH_MAX 4096
#   endif
#endif



//-----------------------------------------------------------------------------

static inline ret_t P_work_dir(char buffer[], uint32_t size) {
#if P_WIN
    if (!GetCurrentDirectory(size, buffer)) return E_EXTERNAL(GetLastError());
#else
    if (!getcwd(buffer, size)) return E_EXTERNAL(errno);
#endif
    return E_NONE;
}

static inline ret_t P_home_dir(char buffer[], uint32_t size) {
#if P_WIN
    // Windows: 使用 SHGetKnownFolderPath 获取用户配置文件目录
    PWSTR wpath = NULL;
    HRESULT hr = SHGetKnownFolderPath(&FOLDERID_Profile, 0, NULL, &wpath);
    if (FAILED(hr)) return E_EXTERNAL(hr);
    int len = WideCharToMultiByte(CP_UTF8, 0, wpath, -1, buffer, (int)size, NULL, NULL);
    CoTaskMemFree(wpath);
    if (len == 0) return E_OUT_OF_CAPACITY;
#else
    // POSIX: 使用 getpwuid 获取用户 home 目录
    struct passwd* pw = getpwuid(getuid());
    if (!pw || !pw->pw_dir || !pw->pw_dir[0]) return E_UNKNOWN;
    size_t len = strlen(pw->pw_dir);
    if (len >= size) return E_OUT_OF_CAPACITY;
    memcpy(buffer, pw->pw_dir, len + 1);
#endif
    return E_NONE;
}

static inline ret_t P_download_dir(char buffer[], uint32_t size) {
#if P_WIN
    // Windows: 使用 SHGetKnownFolderPath 获取用户设置的下载目录
    PWSTR wpath = NULL;
    HRESULT hr = SHGetKnownFolderPath(&FOLDERID_Downloads, 0, NULL, &wpath);
    if (FAILED(hr)) return E_EXTERNAL(hr);
    int len = WideCharToMultiByte(CP_UTF8, 0, wpath, -1, buffer, (int)size, NULL, NULL);
    CoTaskMemFree(wpath);
    if (len == 0) return E_OUT_OF_CAPACITY;
    return E_NONE;
#elif P_DARWIN
    // macOS: 使用 sysdir API 获取下载目录
    sysdir_search_path_enumeration_state state = sysdir_start_search_path_enumeration(
        SYSDIR_DIRECTORY_DOWNLOADS, SYSDIR_DOMAIN_MASK_USER);
    state = sysdir_get_next_search_path_enumeration(state, buffer);
    if (state == 0) {
        // fallback to ~/Downloads
        ret_t r = P_home_dir(buffer, size);
        if (r != E_NONE) return r;
        size_t len = strlen(buffer);
        if (len + 10 >= size) return E_OUT_OF_CAPACITY;
        memcpy(buffer + len, "/Downloads", 11);
    }
    // sysdir 返回的路径可能包含 ~ ，需要展开
    if (buffer[0] == '~') {
        char home[PATH_MAX];
        ret_t r = P_home_dir(home, sizeof(home));
        if (r != E_NONE) return r;
        size_t home_len = strlen(home);
        size_t rest_len = strlen(buffer + 1);  // 跳过 ~
        if (home_len + rest_len >= size) return E_OUT_OF_CAPACITY;
        memmove(buffer + home_len, buffer + 1, rest_len + 1);
        memcpy(buffer, home, home_len);
    }
    return E_NONE;
#else
    // Linux: 优先使用 XDG_DOWNLOAD_DIR，否则回退到 ~/Downloads
    const char* xdg_download = getenv("XDG_DOWNLOAD_DIR");
    if (xdg_download && xdg_download[0]) {
        size_t len = strlen(xdg_download);
        if (len >= size) return E_OUT_OF_CAPACITY;
        memcpy(buffer, xdg_download, len + 1);
        return E_NONE;
    }
    // fallback to ~/Downloads
    ret_t r = P_home_dir(buffer, size);
    if (r != E_NONE) return r;
    size_t len = strlen(buffer);
    if (len + 10 >= size) return E_OUT_OF_CAPACITY;
    memcpy(buffer + len, "/Downloads", 11);
    return E_NONE;
#endif
}

static inline ret_t P_exe_file(char buffer[], uint32_t size) {
#if P_WIN
    char *path;
    if (_get_pgmptr(&path) != 0) {
        buffer[0] = 0;
        return E_EXTERNAL(errno);
    }
    size_t len = strlen(path);
    if (len >= size) return E_OUT_OF_CAPACITY;
    memcpy(buffer, path, len + 1);
#elif P_DARWIN
    if (_NSGetExecutablePath(buffer, &size) != 0) {
        buffer[0] = 0;
        return E_EXTERNAL(errno);
    }
    char *canonicalPath = realpath(buffer, NULL);
    if (!canonicalPath) {
        buffer[0] = 0;
        return E_EXTERNAL(errno);
    }
    size_t len = strlen(canonicalPath);
    if (len >= size) {
        free(canonicalPath);
        buffer[0] = 0;
        return E_OUT_OF_CAPACITY;
    }
    memcpy(buffer, canonicalPath, len + 1);
    free(canonicalPath);
#elif P_BSD
    int mib[4];  mib[0] = CTL_KERN;  mib[1] = KERN_PROC;  mib[2] = KERN_PROC_PATHNAME;  mib[3] = -1;
    if (sysctl(mib, 4, buffer, &size, NULL, 0) != 0) {
        buffer[0] = 0;
        return E_EXTERNAL(errno);
    }
#elif P_LINUX
    ssize_t len = readlink("/proc/self/exe", buffer, size);
    if (len == -1) {
        buffer[0] = 0;
        return E_EXTERNAL(errno);
    }
    if ((size_t)len >= size) {
        buffer[0] = 0;
        return E_OUT_OF_CAPACITY;
    }
    buffer[len] = '\0';
#elif defined(__QNX__)
    FILE* fp = fopen("/proc/self/exefile", "r");
    if (!fp) {
        buffer[0] = 0;
        return E_EXTERNAL(errno);
    }
    size_t read_size = fread(buffer, 1, size - 1, fp);
    fclose(fp);
    buffer[read_size] = 0;
    // 移除末尾换行符
    if (read_size > 0 && buffer[read_size - 1] == '\n') buffer[read_size - 1] = 0;
#else
#error "Unsupported platform"
#endif
    return E_NONE;
}

/*
 * 生成一个临时文件路径
 * - 使用系统临时目录
 * - 生成唯一的文件名
 * - 不实际创建文件，只返回路径
 */
static inline ret_t P_tmp_file(char buffer[], uint32_t size) {
#if P_WIN
    // GetTempPathA 直接写入 buffer
    DWORD len = GetTempPathA(size, buffer);
    if (len == 0 || len >= size) return E_EXTERNAL(GetLastError());
    
    // 在后面追加唯一文件名: tmp_<pid>_<tick>_<counter>.tmp
    static volatile long counter = 0;
    long cnt = InterlockedIncrement(&counter);
    int n = snprintf(buffer + len, size - len, "tmp_%lu_%lu_%ld.tmp", 
                     (unsigned long)GetCurrentProcessId(), 
                     (unsigned long)GetTickCount(), cnt);
    if (n < 0 || (uint32_t)n >= size - len) return E_OUT_OF_CAPACITY;
#else
    // 获取临时目录
    const char* tmp_dir = getenv("TMPDIR");
    if (!tmp_dir || !tmp_dir[0]) tmp_dir = getenv("TMP");
    if (!tmp_dir || !tmp_dir[0]) tmp_dir = getenv("TEMP");
    if (!tmp_dir || !tmp_dir[0]) tmp_dir = "/tmp";
    
    // 构造模板并用 mkstemp 生成唯一文件名
    int n = snprintf(buffer, size, "%s/tmp_XXXXXX", tmp_dir);
    if (n < 0 || (uint32_t)n >= size) return E_OUT_OF_CAPACITY;
    int fd = mkstemp(buffer);  // 创建并打开文件
    if (fd < 0) return E_EXTERNAL(errno);
    close(fd);                 // 关闭文件（保留路径）
#endif
    return E_NONE;
}

/*
 * 计算路径的根目录长度
 * - Unix: / 返回 1
 * - Windows: C:\ 返回 3，C: 返回 2，\\server\share 返回 share 结束位置
 */
static inline size_t P_root(const char* path) {
    
#if P_WIN
    size_t len = strlen(path);
    if (len >= 2 && P_IS_SEP(path[0]) && P_IS_SEP(path[1])) {
        // UNC: \\server\share
        const char* p = path + 2;
        while (*p && !P_IS_SEP(*p)) p++;  // 跳过 server
        if (*p) {
            p++;
            while (*p && !P_IS_SEP(*p)) p++;  // 跳过 share
            return (size_t)(p - path);
        }
        return len;  // 只有 \\server，整个都是根
    } else if (len >= 2 && ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':') {
        // 驱动器号: "C:" 或 "C:\"
        return (len >= 3 && P_IS_SEP(path[2])) ? 3 : 2;
    }
    return 0;
#else
    return P_IS_SEP(path[0]) ? 1 : 0;
#endif
}

/*
 * 返回 path 的父目录
 * - 如果 path 是文件，返回文件所在目录
 * - 如果 path 是目录，返回上级目录
 * - 不会检测 path 是否真实存在，仅根据字符串处理
 * - 返回值：成功返回 root_len（根目录长度），失败返回 -1
 */
static inline int P_base(char buffer[], uint32_t size, const char* path) {
    
    size_t len = strlen(path);
    if (len >= size) return -1;
    memcpy(buffer, path, len + 1);
    
    size_t root_len = P_root(buffer);
    
    // 移除末尾分隔符（但保留根目录）
    while (len > root_len && P_IS_SEP(buffer[len - 1])) len--;
    // 回退到上一个分隔符
    while (len > root_len && !P_IS_SEP(buffer[len - 1])) len--;
    // 移除尾部分隔符（但保留根目录）
    while (len > root_len && P_IS_SEP(buffer[len - 1])) len--;
    
    // 相对路径回退到空，返回 "."
    if (len == 0) buffer[len++] = '.';
    buffer[len] = 0;
    return (int)root_len;
}

/*
 * 返回 path 的目录部分
 * - 如果 path 以分隔符结尾，视为目录，返回 path 本身
 * - 如果 path 不以分隔符结尾，则检测：
 *   - 如果是目录，返回 path 本身
 *   - 如果是文件，返回文件所在目录
 * - 返回的路径会去除末尾分隔符（保留根目录）
 * - 返回值：成功返回 root_len（根目录长度），失败返回 -1
 */
static inline int P_dir(char buffer[], uint32_t size, const char* path) {
    
    size_t len = strlen(path);
    if (len >= size) return -1;
    memcpy(buffer, path, len + 1);
    
    size_t root_len = P_root(buffer);

    // 如果不以分隔符结尾，需要检测是目录还是文件，如果是文件，回退到父目录
    if (!P_IS_SEP(buffer[len - 1])) {
#if P_WIN
        DWORD attr = GetFileAttributesA(buffer);
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            while (len > root_len && !P_IS_SEP(buffer[len - 1])) len--;
        }
#else
        struct stat st; // 注: 这里只使用 st_mode 判断类型，不关注是否为 stat64
        if (stat(buffer, &st) == 0 && !S_ISDIR(st.st_mode)) {            
            while (len > root_len && !P_IS_SEP(buffer[len - 1])) len--;
        }
#endif
    }
    
    // 移除末尾分隔符（保留根目录）
    while (len > root_len && P_IS_SEP(buffer[len - 1])) len--;
    
    // 相对路径回退到空，返回 "."
    if (len == 0) buffer[len++] = '.';
    buffer[len] = 0;

    return (int)root_len;
}

/*
 * 根据 base 和 path 构造全路径
 * - 前置条件：buffer != NULL, size > 0, path != NULL, path[0] != 0
 * - 如果 path 是绝对路径，直接使用 path
 * - 如果 base 为空，使用当前工作目录
 * - base 如果是文件则取其目录部分
 * - 处理 path 中的 .. 回退
 * - 返回 true 表示成功，false 表示缓冲区不足
 * 注意：
 * - path 会被规范化（处理 ./ 和 ../），但 base 不会，调用方应确保 base 中不含 ./ 或 ../
 * - 支持 file:// URI 格式的输入，但输出始终为普通文件系统路径（用于系统调用）
 */
static inline bool P_file(char buffer[], uint32_t size, const char* base, cstr_t path) {

    // 处理 path 的 file:// 前缀
    // file:///path (Unix) → /path
    // file:///C:/path (Windows) → C:/path
    // file://server/share (Windows UNC) → \\server\share
    if (strncmp(path, "file://", 7) == 0) {
        path += 7;
#if P_WIN
        if (path[0] != '/') {
            // file://server/share → UNC 路径
            if (size < 3) return false;
            buffer[0] = '\\';
            buffer[1] = '\\';
            size_t i = 2;
            while (*path && i < size - 1) {
                buffer[i++] = (*path == '/') ? '\\' : *path;
                path++;
            }
            buffer[i] = 0;
            return i < size;
        }
        path++;  // 跳过第一个 /，变成 C:/path
#endif
    }

    // 处理 base 的 file:// 前缀
    if (base && strncmp(base, "file://", 7) == 0) {
        base += 7;
#if P_WIN
        // Windows: file:///C:/path → /C:/path → C:/path
        if (base[0] == '/') base++;
#endif
        // Unix: file:///path → /path (保留 / 作为绝对路径)
    }

    // 如果 path 是绝对路径，直接使用
#if P_WIN
    bool is_abs = (path[0] == '/' || path[0] == '\\' || (path[0] && path[1] == ':'));
#else
    bool is_abs = (path[0] == '/');
#endif
    if (is_abs) {
        size_t len = strlen(path);
        if (len >= size) return false;
        memcpy(buffer, path, len + 1);
        return true;
    }

    // 获取 base 目录到 buffer
#if P_WIN
    char sep = '\\';  // Windows 默认分隔符，后续根据 base 中实际使用的分隔符调整
#endif
    int root_len;
    if (!base || !base[0]) {
        // base 为空，使用当前工作目录
        if (P_work_dir(buffer, size) != E_NONE) return false;
        root_len = (int)P_root(buffer);
    } else {
        // 使用 P_dir 获取 base 的目录部分
        root_len = P_dir(buffer, size, base);
        if (root_len < 0) return false;
#if P_WIN
        // 检测 base 中使用的分隔符风格
        if (strchr(base, '/') && !strchr(base, '\\')) sep = '/';
#endif
    }
    size_t len = strlen(buffer);

    // 处理 path 中的 .. 和拼接
    const char* p = path;
    while (*p) {
        // 跳过分隔符
        while (P_IS_SEP(*p)) p++;
        if (!*p) break;

        // 找到下一段
        const char* seg = p;
        while (*p && !P_IS_SEP(*p)) p++;
        size_t seg_len = p - seg;

        // 对于 ../ 回退上一层
        if (seg_len == 2 && seg[0] == '.' && seg[1] == '.') {
            while (len > (size_t)root_len && !P_IS_SEP(buffer[len-1])) len--;
            while (len > (size_t)root_len && P_IS_SEP(buffer[len-1])) len--;
            buffer[len] = 0;
        }
        // 如果不是 ./ 则构建路径 seg
        else if (seg_len != 1 || seg[0] != '.') { 
            
            // 有前缀路径，加分隔符
            if (len > 0) {
                if (len + 1 + seg_len >= size) return false;
#if P_WIN
                buffer[len++] = sep;
#else
                buffer[len++] = '/';
#endif
            }
            // 否则路径回退到空，则直接从头开始
            else if (seg_len >= size) return false;
            
            memcpy(buffer + len, seg, seg_len);
            len += seg_len;
            buffer[len] = 0;
        }
    }

    return true;
}

//-----------------------------------------------------------------------------

static inline bool P_access(cstr_t csFilePathName, bool bAsReadable, bool bAsWritable) {
#if P_WIN
    DWORD dwAttr = GetFileAttributes(csFilePathName);
    if (dwAttr == INVALID_FILE_ATTRIBUTES) return false;
    // 只读文件也可以读，所以 bAsReadable 不需要判断 FILE_ATTRIBUTE_READONLY
    if (bAsWritable && (dwAttr & FILE_ATTRIBUTE_READONLY)) return false;
    return true;
#else
    int mode = F_OK; if (bAsReadable) mode |= R_OK; if (bAsWritable) mode |= W_OK;
    return access(csFilePathName, mode) == 0;
#endif
}

static inline ret_t P_stat(cstr_t csFilePathName, stat_t* st) {
#if P_WIN
    if (_stat64(csFilePathName, st) != 0) return E_EXTERNAL(errno);
#elif P_DARWIN || P_BSD
    if (stat(csFilePathName, st) != 0) return E_EXTERNAL(errno);
#else
    if (stat64(csFilePathName, st) != 0) return E_EXTERNAL(errno);
#endif
    return E_NONE;
}

static inline int64_t P_size(cstr_t csFilePathName) {
    stat_t st; ret_t ret = P_stat(csFilePathName, &st);
    return ret ? ret : st.st_size;
}

static inline ret_t P_remove(cstr_t csFilePathName) {
#if P_WIN
    if (DeleteFile(csFilePathName)) return E_NONE;
   return E_EXTERNAL(GetLastError());
#else
    if (remove(csFilePathName) == 0) return E_NONE;
    return E_EXTERNAL(errno);
#endif
}

//-----------------------------------------------------------------------------

static inline ret_t P_is_dir(const char* path, bool writeable) {
#if P_WIN
    DWORD dwAttr = GetFileAttributes(path);
    if (dwAttr == INVALID_FILE_ATTRIBUTES) {
        DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) return E_NONE_EXISTS;
        return E_EXTERNAL(err);
    }
    if (!(dwAttr & FILE_ATTRIBUTE_DIRECTORY)) return E_NONE_CONTEXT;
    if (writeable && (dwAttr & FILE_ATTRIBUTE_READONLY)) return E_NO_PERMISSION;
#else
    struct stat st; // 注: 这里只使用 st_mode 判断类型，不关注是否为 stat64
    if (stat(path, &st) != 0) {
        if (errno == ENOENT) return E_NONE_EXISTS;
        return E_EXTERNAL(errno);
    }
    if (!S_ISDIR(st.st_mode)) return E_NONE_CONTEXT;
    if (writeable && access(path, W_OK) != 0) {
        if (errno == EACCES || errno == EPERM) return E_NO_PERMISSION;
        return E_EXTERNAL(errno);
    }
#endif
    return E_NONE;
}

static inline ret_t P_mkdir(cstr_t path) {
#if P_WIN
    if (_mkdir(path) == 0) return E_NONE;
   return E_EXTERNAL(GetLastError());
#else
    if (mkdir(path, 0777) == 0) return E_NONE;
    return E_EXTERNAL(errno);
#endif
}

static inline ret_t P_rmdir(cstr_t csDirPathName) {
#if P_WIN
    if (_rmdir(csDirPathName) == 0) return E_NONE;
    return E_EXTERNAL(GetLastError());
#else
    if (rmdir(csDirPathName) == 0) return E_NONE;
    return E_EXTERNAL(errno);
#endif
}

#if P_WIN
    typedef struct {
        char path[MAX_PATH * 2];    // first dir_len chars = directory path, rest for full path construction
        int dir_len;                // length of directory path
        HANDLE hFind;
        WIN32_FIND_DATAA findData;
        int first;
    } dir_t;
#else
    typedef struct {
        char path[PATH_MAX * 2];    // first dir_len chars = directory path, rest for full path construction
        int dir_len;                // length of directory path
        DIR* dp;
        struct dirent* ent;
        stat_t st;                  // cached stat result for current entry
    } dir_t;
#endif

// 打开目录，用户传入缓冲区
static inline ret_t P_open_dir(dir_t* dir, const char* path) {
#if P_WIN
    if (!dir) return E_INVALID;
    strncpy(dir->path, path, sizeof(dir->path) - 1);
    dir->path[sizeof(dir->path) - 1] = '\0';
    dir->dir_len = (int)strlen(dir->path);
    char pattern[MAX_PATH * 2];
    snprintf(pattern, sizeof(pattern), "%s/*", path);
    dir->hFind = FindFirstFileA(pattern, &dir->findData);
    dir->first = 1;
    if (dir->hFind == INVALID_HANDLE_VALUE) {
        return E_EXTERNAL(GetLastError());
    }
    return E_NONE;
#else
    if (!dir) return E_INVALID;
    strncpy(dir->path, path, sizeof(dir->path) - 1);
    dir->path[sizeof(dir->path) - 1] = '\0';
    dir->dir_len = (int)strlen(dir->path);
    dir->dp = opendir(path);
    dir->ent = NULL;
    dir->st.st_mode = 0;
    if (!dir->dp) return E_EXTERNAL(errno);
    return E_NONE;
#endif
}

// 关闭目录句柄
static inline void P_close_dir(dir_t* dir) {
if (!dir) return;
#if P_WIN
    if (dir->hFind != INVALID_HANDLE_VALUE) FindClose(dir->hFind);
#else
    if (dir->dp) closedir(dir->dp);
#endif
}

// 获取下一个目录项，返回文件名字符串，遍历结束返回 NULL
static inline const char* P_dir_next(dir_t* dir) {
#if P_WIN
    if (!dir) return NULL;
    dir->path[dir->dir_len] = '\0'; /* clear fullname cache */
    if (dir->first) {
        dir->first = 0;
        return dir->findData.cFileName;
    }
    if (FindNextFileA(dir->hFind, &dir->findData)) {
        return dir->findData.cFileName;
    }
    return NULL;
#else
    if (!dir || !dir->dp) return NULL;
    dir->path[dir->dir_len] = '\0'; /* clear fullname cache */
    dir->st.st_mode = 0; /* clear stat cache */
    dir->ent = readdir(dir->dp);
    return dir->ent ? dir->ent->d_name : NULL;
#endif
}

static inline const char* P_dir_name(const dir_t* dir) {
#if P_WIN
    return dir->findData.cFileName;
#else
    return dir->ent ? dir->ent->d_name : NULL;
#endif
}
static inline const char* P_dir_fullname(dir_t* dir) {
    if (dir->path[dir->dir_len] == '\0') {
        snprintf(dir->path + dir->dir_len, sizeof(dir->path) - dir->dir_len, "/%s", P_dir_name(dir));
    }
    return dir->path;
}
static inline int64_t P_dir_size(dir_t* h) {
#if P_WIN
    return ((uint64_t)h->findData.nFileSizeLow | ((uint64_t)h->findData.nFileSizeHigh << 32));
#else
    if (h->st.st_mode == 0 && stat(P_dir_fullname(h), &h->st) != 0) return -1;
    return h->st.st_size;
#endif
}
static inline bool P_dir_is_dir(dir_t* h) {
#if P_WIN
    return (h->findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    if (h->st.st_mode == 0 && stat(P_dir_fullname(h), &h->st) != 0) return false;
    return S_ISDIR(h->st.st_mode);
#endif
}

///////////////////////////////////////////////////////////////////////////////
// 并行和同步
///////////////////////////////////////////////////////////////////////////////

typedef volatile int            aint_t;
typedef volatile int8_t         aint8_t;
typedef volatile int16_t        aint16_t;
typedef volatile int32_t        aint32_t;
typedef volatile int64_t        aint64_t;
typedef volatile unsigned       auint_t;
typedef volatile uint8_t        auint8_t;
typedef volatile uint16_t       auint16_t;
typedef volatile uint32_t       auint32_t;
typedef volatile uint64_t       auint64_t;
typedef volatile void*          aptr_t;

//-----------------------------------------------------------------------------
// 原子操作：优先使用 C11 stdatomic.h，否则使用平台特定实现
// 注意：所有 P_inc/P_and/P_or/P_xor 等操作返回新值（操作后的值）
// P_get_and_xxx 操作返回旧值（操作前的值）
// P_test_and_set 返回 bool（true 表示成功）
//-----------------------------------------------------------------------------

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
// C11 atomics
#   include <stdatomic.h>

#define P_get(pVar) atomic_load_explicit((_Atomic typeof(*pVar)*)pVar, memory_order_relaxed)
#define P_set(pVar, v) atomic_store_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_relaxed)
#define P_get_acq(pVar) atomic_load_explicit((_Atomic typeof(*pVar)*)pVar, memory_order_acquire)
#define P_set_rel(pVar, v) atomic_store_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_release)
#define P_get_ord(pVar) atomic_load_explicit((_Atomic typeof(*pVar)*)pVar, memory_order_seq_cst)
#define P_set_ord(pVar, v) atomic_store_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_seq_cst)

#define P_get_and_set(pVar, v) atomic_exchange_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_relaxed)
#define P_get_and_set_dbl(pVar, v) atomic_exchange_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_acq_rel)
#define P_get_and_set_acq(pVar, v) atomic_exchange_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_acquire)
#define P_get_and_set_rel(pVar, v) atomic_exchange_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_release)
#define P_get_and_set_ord(pVar, v) atomic_exchange_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_seq_cst)

#define P_inc(pVar, v) atomic_fetch_add_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_relaxed) + (v)
#define P_inc_dbl(pVar, v) atomic_fetch_add_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_acq_rel) + (v)
#define P_inc_acq(pVar, v) atomic_fetch_add_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_acquire) + (v)
#define P_inc_rel(pVar, v) atomic_fetch_add_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_release) + (v)
#define P_inc_ord(pVar, v) atomic_fetch_add_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_seq_cst) + (v)
#define P_and(pVar, v) atomic_fetch_and_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_relaxed) & (v)
#define P_and_dbl(pVar, v) atomic_fetch_and_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_acq_rel) & (v)
#define P_and_acq(pVar, v) atomic_fetch_and_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_acquire) & (v)
#define P_and_rel(pVar, v) atomic_fetch_and_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_release) & (v)
#define P_and_ord(pVar, v) atomic_fetch_and_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_seq_cst) & (v)
#define P_or(pVar, v) atomic_fetch_or_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_relaxed) | (v)
#define P_or_dbl(pVar, v) atomic_fetch_or_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_acq_rel) | (v)
#define P_or_acq(pVar, v) atomic_fetch_or_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_acquire) | (v)
#define P_or_rel(pVar, v) atomic_fetch_or_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_release) | (v)
#define P_or_ord(pVar, v) atomic_fetch_or_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_seq_cst) | (v)
#define P_xor(pVar, v) atomic_fetch_xor_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_relaxed) ^ (v)
#define P_xor_dbl(pVar, v) atomic_fetch_xor_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_acq_rel) ^ (v)
#define P_xor_acq(pVar, v) atomic_fetch_xor_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_acquire) ^ (v)
#define P_xor_rel(pVar, v) atomic_fetch_xor_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_release) ^ (v)
#define P_xor_ord(pVar, v) atomic_fetch_xor_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_seq_cst) ^ (v)

#define P_get_and_inc(pVar, v) atomic_fetch_add_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_relaxed)
#define P_get_and_inc_dbl(pVar, v) atomic_fetch_add_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_acq_rel)
#define P_get_and_inc_acq(pVar, v) atomic_fetch_add_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_acquire)
#define P_get_and_inc_rel(pVar, v) atomic_fetch_add_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_release)
#define P_get_and_inc_ord(pVar, v) atomic_fetch_add_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_seq_cst)
#define P_get_and_and(pVar, v) atomic_fetch_and_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_relaxed)
#define P_get_and_and_dbl(pVar, v) atomic_fetch_and_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_acq_rel)
#define P_get_and_and_acq(pVar, v) atomic_fetch_and_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_acquire)
#define P_get_and_and_rel(pVar, v) atomic_fetch_and_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_release)
#define P_get_and_and_ord(pVar, v) atomic_fetch_and_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_seq_cst)
#define P_get_and_or(pVar, v) atomic_fetch_or_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_relaxed)
#define P_get_and_or_dbl(pVar, v) atomic_fetch_or_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_acq_rel)
#define P_get_and_or_acq(pVar, v) atomic_fetch_or_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_acquire)
#define P_get_and_or_rel(pVar, v) atomic_fetch_or_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_release)
#define P_get_and_or_ord(pVar, v) atomic_fetch_or_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_seq_cst)
#define P_get_and_xor(pVar, v) atomic_fetch_xor_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_relaxed)
#define P_get_and_xor_dbl(pVar, v) atomic_fetch_xor_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_acq_rel)
#define P_get_and_xor_acq(pVar, v) atomic_fetch_xor_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_acquire)
#define P_get_and_xor_rel(pVar, v) atomic_fetch_xor_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_release)
#define P_get_and_xor_ord(pVar, v) atomic_fetch_xor_explicit((_Atomic typeof(*pVar)*)pVar, v, memory_order_seq_cst)

#define P_test_and_set(pVar, pTestVar, v) atomic_compare_exchange_strong_explicit((_Atomic typeof(*pVar)*)pVar, pTestVar, v, memory_order_relaxed, memory_order_relaxed)
#define P_test_and_set_acq(pVar, pTestVar, v) atomic_compare_exchange_strong_explicit((_Atomic typeof(*pVar)*)pVar, pTestVar, v, memory_order_acquire, memory_order_relaxed)
#define P_test_and_set_rel(pVar, pTestVar, v) atomic_compare_exchange_strong_explicit((_Atomic typeof(*pVar)*)pVar, pTestVar, v, memory_order_release, memory_order_relaxed)
#define P_test_and_set_dbl(pVar, pTestVar, v) atomic_compare_exchange_strong_explicit((_Atomic typeof(*pVar)*)pVar, pTestVar, v, memory_order_acq_rel, memory_order_relaxed)
#define P_test_and_set_ord(pVar, pTestVar, v) atomic_compare_exchange_strong_explicit((_Atomic typeof(*pVar)*)pVar, pTestVar, v, memory_order_seq_cst, memory_order_relaxed)

#define P_test_and_set_or_acq(pVar, pTestVar, v) atomic_compare_exchange_strong_explicit((_Atomic typeof(*pVar)*)pVar, pTestVar, v, memory_order_relaxed, memory_order_acquire)
#define P_test_and_set_acq_or_acq(pVar, pTestVar, v) atomic_compare_exchange_strong_explicit((_Atomic typeof(*pVar)*)pVar, pTestVar, v, memory_order_acquire, memory_order_acquire)
#define P_test_and_set_rel_or_acq(pVar, pTestVar, v) atomic_compare_exchange_strong_explicit((_Atomic typeof(*pVar)*)pVar, pTestVar, v, memory_order_release, memory_order_acquire)
#define P_test_and_set_dbl_or_acq(pVar, pTestVar, v) atomic_compare_exchange_strong_explicit((_Atomic typeof(*pVar)*)pVar, pTestVar, v, memory_order_acq_rel, memory_order_acquire)
#define P_test_and_set_ord_or_acq(pVar, pTestVar, v) atomic_compare_exchange_strong_explicit((_Atomic typeof(*pVar)*)pVar, pTestVar, v, memory_order_seq_cst, memory_order_acquire)

#elif P_WIN && !defined(__GNUC__)
// Windows MSVC (Interlocked* 操作默认有 full barrier，无法实现弱内存序)

#define P_get(pVar) (*(pVar))
#define P_set(pVar, v) InterlockedExchange((volatile LONG*)(pVar), (LONG)(v))
#define P_get_acq(pVar) (*(pVar))
#define P_set_rel(pVar, v) InterlockedExchange((volatile LONG*)(pVar), (LONG)(v))
#define P_get_ord(pVar) (*(pVar))
#define P_set_ord(pVar, v) InterlockedExchange((volatile LONG*)(pVar), (LONG)(v))

#define P_get_and_set(pVar, v) InterlockedExchange((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_set_dbl(pVar, v) InterlockedExchange((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_set_acq(pVar, v) InterlockedExchange((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_set_rel(pVar, v) InterlockedExchange((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_set_ord(pVar, v) InterlockedExchange((volatile LONG*)(pVar), (LONG)(v))

// 返回新值：InterlockedExchangeAdd 返回旧值，需要 + v
#define P_inc(pVar, v) (InterlockedExchangeAdd((volatile LONG*)(pVar), (LONG)(v)) + (v))
#define P_inc_dbl(pVar, v) (InterlockedExchangeAdd((volatile LONG*)(pVar), (LONG)(v)) + (v))
#define P_inc_acq(pVar, v) (InterlockedExchangeAdd((volatile LONG*)(pVar), (LONG)(v)) + (v))
#define P_inc_rel(pVar, v) (InterlockedExchangeAdd((volatile LONG*)(pVar), (LONG)(v)) + (v))
#define P_inc_ord(pVar, v) (InterlockedExchangeAdd((volatile LONG*)(pVar), (LONG)(v)) + (v))
// 返回新值：InterlockedAnd 返回旧值，需要 & v
#define P_and(pVar, v) (InterlockedAnd((volatile LONG*)(pVar), (LONG)(v)) & (v))
#define P_and_dbl(pVar, v) (InterlockedAnd((volatile LONG*)(pVar), (LONG)(v)) & (v))
#define P_and_acq(pVar, v) (InterlockedAnd((volatile LONG*)(pVar), (LONG)(v)) & (v))
#define P_and_rel(pVar, v) (InterlockedAnd((volatile LONG*)(pVar), (LONG)(v)) & (v))
#define P_and_ord(pVar, v) (InterlockedAnd((volatile LONG*)(pVar), (LONG)(v)) & (v))
#define P_or(pVar, v) (InterlockedOr((volatile LONG*)(pVar), (LONG)(v)) | (v))
#define P_or_dbl(pVar, v) (InterlockedOr((volatile LONG*)(pVar), (LONG)(v)) | (v))
#define P_or_acq(pVar, v) (InterlockedOr((volatile LONG*)(pVar), (LONG)(v)) | (v))
#define P_or_rel(pVar, v) (InterlockedOr((volatile LONG*)(pVar), (LONG)(v)) | (v))
#define P_or_ord(pVar, v) (InterlockedOr((volatile LONG*)(pVar), (LONG)(v)) | (v))
#define P_xor(pVar, v) (InterlockedXor((volatile LONG*)(pVar), (LONG)(v)) ^ (v))
#define P_xor_dbl(pVar, v) (InterlockedXor((volatile LONG*)(pVar), (LONG)(v)) ^ (v))
#define P_xor_acq(pVar, v) (InterlockedXor((volatile LONG*)(pVar), (LONG)(v)) ^ (v))
#define P_xor_rel(pVar, v) (InterlockedXor((volatile LONG*)(pVar), (LONG)(v)) ^ (v))
#define P_xor_ord(pVar, v) (InterlockedXor((volatile LONG*)(pVar), (LONG)(v)) ^ (v))

// 返回旧值
#define P_get_and_inc(pVar, v) InterlockedExchangeAdd((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_inc_dbl(pVar, v) InterlockedExchangeAdd((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_inc_acq(pVar, v) InterlockedExchangeAdd((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_inc_rel(pVar, v) InterlockedExchangeAdd((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_inc_ord(pVar, v) InterlockedExchangeAdd((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_and(pVar, v) InterlockedAnd((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_and_dbl(pVar, v) InterlockedAnd((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_and_acq(pVar, v) InterlockedAnd((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_and_rel(pVar, v) InterlockedAnd((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_and_ord(pVar, v) InterlockedAnd((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_or(pVar, v) InterlockedOr((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_or_dbl(pVar, v) InterlockedOr((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_or_acq(pVar, v) InterlockedOr((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_or_rel(pVar, v) InterlockedOr((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_or_ord(pVar, v) InterlockedOr((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_xor(pVar, v) InterlockedXor((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_xor_dbl(pVar, v) InterlockedXor((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_xor_acq(pVar, v) InterlockedXor((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_xor_rel(pVar, v) InterlockedXor((volatile LONG*)(pVar), (LONG)(v))
#define P_get_and_xor_ord(pVar, v) InterlockedXor((volatile LONG*)(pVar), (LONG)(v))

// 返回 bool：InterlockedCompareExchange 返回旧值，比较是否等于 expected
static inline bool P_test_and_set_impl(volatile LONG* pVar, LONG* pTestVar, LONG v) {
    LONG old = InterlockedCompareExchange(pVar, v, *pTestVar);
    if (old == *pTestVar) return true;
    *pTestVar = old;
    return false;
}
#define P_test_and_set(pVar, pTestVar, v) P_test_and_set_impl((volatile LONG*)(pVar), (LONG*)(pTestVar), (LONG)(v))
#define P_test_and_set_acq(pVar, pTestVar, v) P_test_and_set_impl((volatile LONG*)(pVar), (LONG*)(pTestVar), (LONG)(v))
#define P_test_and_set_rel(pVar, pTestVar, v) P_test_and_set_impl((volatile LONG*)(pVar), (LONG*)(pTestVar), (LONG)(v))
#define P_test_and_set_dbl(pVar, pTestVar, v) P_test_and_set_impl((volatile LONG*)(pVar), (LONG*)(pTestVar), (LONG)(v))
#define P_test_and_set_ord(pVar, pTestVar, v) P_test_and_set_impl((volatile LONG*)(pVar), (LONG*)(pTestVar), (LONG)(v))

#define P_test_and_set_or_acq(pVar, pTestVar, v) P_test_and_set_impl((volatile LONG*)(pVar), (LONG*)(pTestVar), (LONG)(v))
#define P_test_and_set_acq_or_acq(pVar, pTestVar, v) P_test_and_set_impl((volatile LONG*)(pVar), (LONG*)(pTestVar), (LONG)(v))
#define P_test_and_set_rel_or_acq(pVar, pTestVar, v) P_test_and_set_impl((volatile LONG*)(pVar), (LONG*)(pTestVar), (LONG)(v))
#define P_test_and_set_dbl_or_acq(pVar, pTestVar, v) P_test_and_set_impl((volatile LONG*)(pVar), (LONG*)(pTestVar), (LONG)(v))
#define P_test_and_set_ord_or_acq(pVar, pTestVar, v) P_test_and_set_impl((volatile LONG*)(pVar), (LONG*)(pTestVar), (LONG)(v))

#elif defined(__GNUC__)

#define P_get(pVar) __atomic_load_n(pVar, __ATOMIC_RELAXED)
#define P_set(pVar, v) __atomic_store_n(pVar, v, __ATOMIC_RELAXED)
#define P_get_acq(pVar) __atomic_load_n(pVar, __ATOMIC_ACQUIRE)
#define P_set_rel(pVar, v) __atomic_store_n(pVar, v, __ATOMIC_RELEASE)
#define P_get_ord(pVar) __atomic_load_n(pVar, __ATOMIC_SEQ_CST)
#define P_set_ord(pVar, v) __atomic_store_n(pVar, v, __ATOMIC_SEQ_CST)

#define P_get_and_set(pVar, v) __atomic_exchange_n(pVar, v, __ATOMIC_RELAXED)
#define P_get_and_set_dbl(pVar, v) __atomic_exchange_n(pVar, v, __ATOMIC_ACQ_REL)
#define P_get_and_set_acq(pVar, v) __atomic_exchange_n(pVar, v, __ATOMIC_ACQUIRE)
#define P_get_and_set_rel(pVar, v) __atomic_exchange_n(pVar, v, __ATOMIC_RELEASE)
#define P_get_and_set_ord(pVar, v) __atomic_exchange_n(pVar, v, __ATOMIC_SEQ_CST)

#define P_inc(pVar, v) __atomic_add_fetch(pVar, v, __ATOMIC_RELAXED)
#define P_inc_dbl(pVar, v) __atomic_add_fetch(pVar, v, __ATOMIC_ACQ_REL)
#define P_inc_acq(pVar, v) __atomic_add_fetch(pVar, v, __ATOMIC_ACQUIRE)
#define P_inc_rel(pVar, v) __atomic_add_fetch(pVar, v, __ATOMIC_RELEASE)
#define P_inc_ord(pVar, v) __atomic_add_fetch(pVar, v, __ATOMIC_SEQ_CST)
#define P_and(pVar, v) __atomic_and_fetch(pVar, v, __ATOMIC_RELAXED)
#define P_and_dbl(pVar, v) __atomic_and_fetch(pVar, v, __ATOMIC_ACQ_REL)
#define P_and_acq(pVar, v) __atomic_and_fetch(pVar, v, __ATOMIC_ACQUIRE)
#define P_and_rel(pVar, v) __atomic_and_fetch(pVar, v, __ATOMIC_RELEASE)
#define P_and_ord(pVar, v) __atomic_and_fetch(pVar, v, __ATOMIC_SEQ_CST)
#define P_or(pVar, v) __atomic_or_fetch(pVar, v, __ATOMIC_RELAXED)
#define P_or_dbl(pVar, v) __atomic_or_fetch(pVar, v, __ATOMIC_ACQ_REL)
#define P_or_acq(pVar, v) __atomic_or_fetch(pVar, v, __ATOMIC_ACQUIRE)
#define P_or_rel(pVar, v) __atomic_or_fetch(pVar, v, __ATOMIC_RELEASE)
#define P_or_ord(pVar, v) __atomic_or_fetch(pVar, v, __ATOMIC_SEQ_CST)
#define P_xor(pVar, v) __atomic_xor_fetch(pVar, v, __ATOMIC_RELAXED)
#define P_xor_dbl(pVar, v) __atomic_xor_fetch(pVar, v, __ATOMIC_ACQ_REL)
#define P_xor_acq(pVar, v) __atomic_xor_fetch(pVar, v, __ATOMIC_ACQUIRE)
#define P_xor_rel(pVar, v) __atomic_xor_fetch(pVar, v, __ATOMIC_RELEASE)
#define P_xor_ord(pVar, v) __atomic_xor_fetch(pVar, v, __ATOMIC_SEQ_CST)

#define P_get_and_inc(pVar, v) __atomic_fetch_add(pVar, v, __ATOMIC_RELAXED)
#define P_get_and_inc_dbl(pVar, v) __atomic_fetch_add(pVar, v, __ATOMIC_ACQ_REL)
#define P_get_and_inc_acq(pVar, v) __atomic_fetch_add(pVar, v, __ATOMIC_ACQUIRE)
#define P_get_and_inc_rel(pVar, v) __atomic_fetch_add(pVar, v, __ATOMIC_RELEASE)
#define P_get_and_inc_ord(pVar, v) __atomic_fetch_add(pVar, v, __ATOMIC_SEQ_CST)
#define P_get_and_and(pVar, v) __atomic_fetch_and(pVar, v, __ATOMIC_RELAXED)
#define P_get_and_and_dbl(pVar, v) __atomic_fetch_and(pVar, v, __ATOMIC_ACQ_REL)
#define P_get_and_and_acq(pVar, v) __atomic_fetch_and(pVar, v, __ATOMIC_ACQUIRE)
#define P_get_and_and_rel(pVar, v) __atomic_fetch_and(pVar, v, __ATOMIC_RELEASE)
#define P_get_and_and_ord(pVar, v) __atomic_fetch_and(pVar, v, __ATOMIC_SEQ_CST)
#define P_get_and_or(pVar, v) __atomic_fetch_or(pVar, v, __ATOMIC_RELAXED)
#define P_get_and_or_dbl(pVar, v) __atomic_fetch_or(pVar, v, __ATOMIC_ACQ_REL)
#define P_get_and_or_acq(pVar, v) __atomic_fetch_or(pVar, v, __ATOMIC_ACQUIRE)
#define P_get_and_or_rel(pVar, v) __atomic_fetch_or(pVar, v, __ATOMIC_RELEASE)
#define P_get_and_or_ord(pVar, v) __atomic_fetch_or(pVar, v, __ATOMIC_SEQ_CST)
#define P_get_and_xor(pVar, v) __atomic_fetch_xor(pVar, v, __ATOMIC_RELAXED)
#define P_get_and_xor_dbl(pVar, v) __atomic_fetch_xor(pVar, v, __ATOMIC_ACQ_REL)
#define P_get_and_xor_acq(pVar, v) __atomic_fetch_xor(pVar, v, __ATOMIC_ACQUIRE)
#define P_get_and_xor_rel(pVar, v) __atomic_fetch_xor(pVar, v, __ATOMIC_RELEASE)
#define P_get_and_xor_ord(pVar, v) __atomic_fetch_xor(pVar, v, __ATOMIC_SEQ_CST)

#define P_test_and_set(pVar, pTestVar, v) __atomic_compare_exchange_n(pVar, pTestVar, v, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)

#define P_test_and_set_acq(pVar, pTestVar, v) __atomic_compare_exchange_n(pVar, pTestVar, v, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)
#define P_test_and_set_rel(pVar, pTestVar, v) __atomic_compare_exchange_n(pVar, pTestVar, v, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED)
#define P_test_and_set_dbl(pVar, pTestVar, v) __atomic_compare_exchange_n(pVar, pTestVar, v, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)
#define P_test_and_set_ord(pVar, pTestVar, v) __atomic_compare_exchange_n(pVar, pTestVar, v, false, __ATOMIC_SEQ_CST, __ATOMIC_RELAXED)

#define P_test_and_set_or_acq(pVar, pTestVar, v) __atomic_compare_exchange_n(pVar, pTestVar, v, false, __ATOMIC_RELAXED, __ATOMIC_ACQUIRE)
#define P_test_and_set_acq_or_acq(pVar, pTestVar, v) __atomic_compare_exchange_n(pVar, pTestVar, v, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)
#define P_test_and_set_rel_or_acq(pVar, pTestVar, v) __atomic_compare_exchange_n(pVar, pTestVar, v, false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE)
#define P_test_and_set_dbl_or_acq(pVar, pTestVar, v) __atomic_compare_exchange_n(pVar, pTestVar, v, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)
#define P_test_and_set_ord_or_acq(pVar, pTestVar, v) __atomic_compare_exchange_n(pVar, pTestVar, v, false, __ATOMIC_SEQ_CST, __ATOMIC_ACQUIRE)

#else
#error "Unsupported platform"
#endif

//-----------------------------------------------------------------------------

#if P_WIN
#include <Synchapi.h>

#define P_mutex_t                               CRITICAL_SECTION

#define P_mutex_init(pMutex)                    InitializeCriticalSection(pMutex)
#define P_mutex_final(pMutex)                   DeleteCriticalSection(pMutex)
#define P_mutex_lock(pMutex)                    EnterCriticalSection(pMutex)
#define P_mutex_unlock(pMutex)                  LeaveCriticalSection(pMutex)

#define P_cond_t                                CONDITION_VARIABLE
#define P_cond_init(pCond)                      InitializeConditionVariable(pCond)
#define P_cond_final(pCond)                     (void)0
#define P_cond_one(pCond)                       WakeConditionVariable(pCond)
#define P_cond_all(pCond)                       WakeAllConditionVariable(pCond)

#define P_wait(pCond, pMutex)                   SleepConditionVariableCS(pCond, pMutex, INFINITE)
// pTimeout 是相对超时时长，返回 1 表示超时，0 表示被唤醒
static inline int P_wait_timeout(P_cond_t* pCond, P_mutex_t* pMutex, const P_clock* pTimeout) {
    DWORD ms = (DWORD)(pTimeout->tv_sec * 1000 + pTimeout->tv_nsec / 1000000);
    return SleepConditionVariableCS(pCond, pMutex, ms) ? 0 : (GetLastError() == ERROR_TIMEOUT ? 1 : -1);
}

#else
#include <pthread.h>

#define P_mutex_t                               pthread_mutex_t
#define P_mutex_init(pMutex)                    pthread_mutex_init(pMutex, NULL)
#define P_mutex_final(pMutex)                   pthread_mutex_destroy(pMutex)
#define P_mutex_lock(pMutex)                    pthread_mutex_lock(pMutex)
#define P_mutex_unlock(pMutex)                  pthread_mutex_unlock(pMutex)

#define P_cond_t                                pthread_cond_t
#define P_cond_init(pCond)                      pthread_cond_init(pCond, NULL)
#define P_cond_final(pCond)                     pthread_cond_destroy(pCond)
#define P_cond_one(pCond)                       pthread_cond_signal(pCond)
#define P_cond_all(pCond)                       pthread_cond_broadcast(pCond)

#define P_wait(pCond, pMutex)                   pthread_cond_wait(pCond, pMutex)
// pTimeout 是相对超时时长，返回 1 表示超时，0 表示被唤醒，-1 表示错误
static inline int P_wait_timeout(P_cond_t* pCond, P_mutex_t* pMutex, const P_clock* pTimeout) {
#if P_DARWIN
    // macOS 直接使用相对时间接口，无需转换
    int ret = pthread_cond_timedwait_relative_np(pCond, pMutex, pTimeout);
    return ret == ETIMEDOUT ? 1 : (ret == 0 ? 0 : -1);
#else
    // 其他 POSIX 平台：需要转换为绝对时间
    struct timespec abs_time;
    if (clock_gettime(CLOCK_REALTIME, &abs_time) != 0) return -1;
    
    abs_time.tv_sec += pTimeout->tv_sec;
    abs_time.tv_nsec += pTimeout->tv_nsec;
    if (abs_time.tv_nsec >= 1000000000L) {
        abs_time.tv_sec++;
        abs_time.tv_nsec -= 1000000000L;
    }
    
    int ret = pthread_cond_timedwait(pCond, pMutex, &abs_time);
    return ret == ETIMEDOUT ? 1 : (ret == 0 ? 0 : -1);
#endif
}
#endif

///////////////////////////////////////////////////////////////////////////////
// 多线程
///////////////////////////////////////////////////////////////////////////////

#if P_WIN
#   include <processthreadsapi.h>
    typedef HANDLE                          thd_t;
#else
#   include <pthread.h>
#   if defined(__FreeBSD__)
#       include <sys/thr.h>
#   elif P_LINUX
#       include <sys/syscall.h>
#   endif
    typedef pthread_t                       thd_t;
#endif

typedef int32_t (*process_cb)(void *pCtx);

typedef enum P_thd_policy {

    P_THD_IDLE = -2,                   /* 空闲时执行 */
    P_THD_BACKGROUND = -1,             /* （后台）工作线程 */
    P_THD_NORMAL = 0,                  /* 控制线程 */
    P_THD_FOREGROUND,                  /* （前台）交互线程 */
    P_THD_REALTIME,                    /* 实时处理线程 */

} P_thd_policy_e;

typedef struct {
    process_cb                          cb_process;
    void*                               pCtx;
    P_thd_policy_e                      ePolicy;
    int32_t                             i32StackSize;
    thd_t                               pHandle;
    uint64_t                            ID;
} P_thd_self_st;
static TLS P_thd_self_st*       tls_this;

#if P_WIN
static inline DWORD WINAPI proto_thread_proc(LPVOID pParam) {
    P_thd_self_st* pThread = (P_thd_self_st*)pParam;

    pThread->pHandle = GetCurrentThread();
    pThread->ID = GetCurrentThreadId();

#else
static inline void* proto_thd_proc(void* pParam) {
    P_thd_self_st* pThread = (P_thd_self_st*)pParam;

    pThread->pHandle = pthread_self();

#if P_DARWIN
    // 获取 Mach thread ID
    pThread->ID = pthread_mach_thread_np(pThread->pHandle);
#elif defined(__FreeBSD__)
    long tid;
    thr_self(&tid);
    pThread->ID = (uint64_t)tid;
#elif defined(__NetBSD__)
    pThread->ID = (uint64_t)_lwp_self();
#elif defined(__OpenBSD__)
    pThread->ID = (uint64_t)getthrid();
#elif P_LINUX
    // Linux: glibc 2.30+ 有 gettid()，否则用 syscall
    #if defined(__GLIBC__) && defined(__GLIBC_MINOR__) && \
        (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 30))
        pThread->ID = gettid();
    #else
        pThread->ID = (uint64_t)syscall(SYS_gettid);
    #endif
#else
    // 其他平台：使用 pthread_self() 作为 ID（不是真正的 TID）
    pThread->ID = (uint64_t)(uintptr_t)pThread->pHandle;
#endif
#endif

    tls_this = pThread;
    int32_t exit_code = pThread->cb_process(pThread->pCtx);
    tls_this = NULL;
    free(pThread);

#if P_WIN
    return (DWORD)exit_code;
#else
    return (void*)(long)exit_code;
#endif
}

/* 创建线程
 * @param r_hThread 线程句柄
 * 如果 r_hThread 为 NULL，则会以 detach 方式创建线程。
 * 也就是线程退出后会自己负责其自身的资源释放；
 * 否则对于非 detach 模式的线程，需要应用来最终执行 P_thd_free() 来完成对线程资源的释放。
 * @param cb_process 线程执行函数
 * @param pCtx 线程执行函数的上下文
 * @param ePolicy 线程调度策略
 * @param i32StackSize 线程栈大小
 * @return 错误码
 */
static inline ret_t
P_thread(thd_t* r_hThread/* nullable */,
        process_cb cb_process, void* pCtx,
        P_thd_policy_e ePolicy, int32_t i32StackSize) {

    P_check(cb_process, return E_INVALID;)
    P_thd_self_st* pThread = (P_thd_self_st*)malloc(sizeof(P_thd_self_st));
    P_check(pThread, return E_OUT_OF_MEMORY;)
    pThread->cb_process = cb_process;
    pThread->pCtx = pCtx;
    pThread->ePolicy = ePolicy;
    pThread->pHandle = 0;
    pThread->ID = 0;

    thd_t pHandle = 0;

#if P_WIN

    pThread->i32StackSize = !i32StackSize ? 0 : i32StackSize < 0 ? 8192 : i32StackSize + 8192;

    pHandle = CreateThread(
        NULL,                   // default security attributes
        pThread->i32StackSize,  // default stack size
        proto_thread_proc,      // thread function
        pThread,                // argument to thread function
        0,                      // default creation flags
        NULL                    // thread identifier (optional)
    );
    P_check(pHandle,
        free(pThread);
        return E_EXTERNAL(GetLastError());
    )

    if (ePolicy != P_THD_NORMAL) {

        BOOL ret;
        switch (pThread->ePolicy) {
            case P_THD_IDLE:
                ret = SetThreadPriority(pHandle, THREAD_PRIORITY_IDLE);
                break;
            case P_THD_BACKGROUND:
                ret = SetThreadPriority(pHandle, THREAD_PRIORITY_BELOW_NORMAL);
                break;
            case P_THD_FOREGROUND:
                ret = SetThreadPriority(pHandle, THREAD_PRIORITY_ABOVE_NORMAL);
                break;
            case P_THD_REALTIME:
                ret = SetThreadPriority(pHandle, THREAD_PRIORITY_TIME_CRITICAL);
                break;
            default: ret = TRUE;
        }
        P_check(ret,
            CloseHandle(pHandle);
            free(pThread);
            return E_EXTERNAL(GetLastError());
        )
    }

    if (r_hThread) *r_hThread = pHandle;
    else CloseHandle(pHandle);

#else

    pThread->i32StackSize = !i32StackSize ? 0 : i32StackSize < 0 ? PTHREAD_STACK_MIN : i32StackSize + PTHREAD_STACK_MIN;

    ret_t err;
    pthread_attr_t attr;
    pthread_attr_t* pattr = NULL;

    // 只在需要自定义参数时创建 attr
    if (pThread->i32StackSize
#if defined(__ANDROID__)
        || ePolicy != P_THD_NORMAL
#endif
    ) {
        err = pthread_attr_init(&attr);
        if (err) { free(pThread); return E_EXTERNAL(err); }
        pattr = &attr;
        
        do {
            if (pThread->i32StackSize) {
                err = pthread_attr_setstacksize(&attr, pThread->i32StackSize);
                if (err) break;
            }

#if defined(__ANDROID__)
            // Android 平台支持调度策略和优先级设置
            // > SCHED_IDLE 和 SCHED_BATCH 是 Linux 扩展策略
            // > SCHED_OTHER 为默认分时调度策略
            err = pthread_attr_setschedpolicy(&attr, pThread->ePolicy == P_THD_IDLE       ? SCHED_IDLE :
                                                         pThread->ePolicy == P_THD_BACKGROUND ? SCHED_BATCH :
                                                                                                SCHED_OTHER);
            if (err) break;

            struct sched_param param = {0};
            switch (pThread->ePolicy) {
                case P_THD_IDLE: param.sched_priority = 19; break;
                case P_THD_BACKGROUND: param.sched_priority = 10; break;
                case P_THD_FOREGROUND: param.sched_priority = -4; break;
                case P_THD_REALTIME: param.sched_priority = -16; break;
                default:;
            }
            err = pthread_attr_setschedparam(&attr, &param);
            if (err) break;
#endif

        } while (0);

        if (err) {
            pthread_attr_destroy(&attr);
            free(pThread);
            return E_EXTERNAL(err);
        }
    }

    err = pthread_create(&pHandle, pattr, proto_thd_proc, pThread);
    if (pattr) pthread_attr_destroy(&attr);
    if (err) {
        free(pThread);
        return E_EXTERNAL(err);
    }

    if (r_hThread) *r_hThread = pHandle;
    else pthread_detach(pHandle);

#endif
    return E_NONE;
}

static inline ret_t P_join(thd_t hThread, int32_t* r_i32ExitCode/* nullable */) {

    P_check(hThread, return E_INVALID;)

#if P_WIN
    if (WaitForSingleObject(hThread, INFINITE) != WAIT_OBJECT_0) return E_EXTERNAL(GetLastError());
    if (r_i32ExitCode) { DWORD dwExitCode = 0;
        if (GetExitCodeThread(hThread, &dwExitCode)) {
            *r_i32ExitCode = (int32_t)dwExitCode;
        } else {
            *r_i32ExitCode = (int32_t)GetLastError();
        }
    }
    CloseHandle(hThread);
#else
    void* pExitCode = NULL;
    int err = pthread_join(hThread, &pExitCode);
    if (err) return E_EXTERNAL(err);
    if (r_i32ExitCode) *r_i32ExitCode = (int32_t)(long)pExitCode;
#endif
    return E_NONE;
}

///////////////////////////////////////////////////////////////////////////////
// 网络
///////////////////////////////////////////////////////////////////////////////

#if P_WIN
#   include <winsock2.h>
#   include <ws2tcpip.h>
#   include <mswsock.h> // WSASendMsg
    // MSVC 自动链接网络库（MinGW 需要在编译时指定 -lws2_32）
#   if defined(_MSC_VER)
#       pragma comment(lib, "ws2_32.lib")
#   endif
    typedef SOCKET                      sock_t;
    typedef int                         socklen_t;
#   define P_INVALID_SOCKET             INVALID_SOCKET
#   define P_SOCKET_ERROR               SOCKET_ERROR
    // Windows MSVC 没有 ssize_t
#   if defined(_MSC_VER) && !defined(ssize_t)
        typedef intptr_t ssize_t;
#   endif
#else
#   include <sys/socket.h>
#   include <sys/types.h>
#   include <netinet/in.h>
#   include <netinet/tcp.h>
#   include <arpa/inet.h>
#   include <netdb.h>
#   include <fcntl.h>
    typedef int                         sock_t;
    typedef socklen_t                   socklen_t;
#   define P_INVALID_SOCKET             (-1)
#   define P_SOCKET_ERROR               (-1)
#endif

//-----------------------------------------------------------------------------
// 网络初始化/清理（Windows WSA 特性）
//-----------------------------------------------------------------------------
// 注意：inet_pton/inet_ntop 已是标准跨平台 API，无需封装

#if P_WIN
extern LPFN_WSASENDMSG fn_WSASendMsg = NULL;
ret_t P_net_init(void);
static inline void P_net_cleanup(void) { WSACleanup(); }
#else
// 使用 volatile 可避免 "always false" 警告
#define P_net_init()        ((ret_t)((volatile int){E_NONE}))
#define P_net_cleanup()     ((void)0)
#endif

static inline int P_sock_errno(void) {
#if P_WIN
    return WSAGetLastError();
#else
    return errno;
#endif
}

// 检查错误是否为"操作进行中"（非阻塞连接）
static inline bool P_sock_is_inprogress(void) {
#if P_WIN
    int err = WSAGetLastError();
    return err == WSAEWOULDBLOCK || err == WSAEINPROGRESS;
#else
    return errno == EINPROGRESS || errno == EWOULDBLOCK;
#endif
}

// 检查错误是否为"暂无数据/缓冲区满"（非阻塞 I/O）
static inline bool P_sock_is_wouldblock(void) {
#if P_WIN
    return WSAGetLastError() == WSAEWOULDBLOCK;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}

// 检查错误是否为"连接已重置"
static inline bool P_sock_is_connreset(void) {
#if P_WIN
    return WSAGetLastError() == WSAECONNRESET;
#else
    return errno == ECONNRESET;
#endif
}

// 检查错误是否为"被信号中断"（用于 select/recv/send 等调用）
static inline bool P_sock_is_interrupted(void) {
#if P_WIN
    return WSAGetLastError() == WSAEINTR;
#else
    return errno == EINTR;
#endif
}

//-----------------------------------------------------------------------------
// Socket 基础操作
// 注意：bind, listen, accept, connect, send, recv, sendto, recvfrom, shutdown
//       这些函数在所有平台上名字和语义一致，直接使用即可
//       只需确保类型定义统一（sock_t, socklen_t, ssize_t）
//-----------------------------------------------------------------------------

// 关闭 socket（跨平台差异：closesocket vs close）
static inline ret_t P_sock_close(sock_t s) {
    if (s == P_INVALID_SOCKET) return E_INVALID;
#if P_WIN
    int ret = closesocket(s);
#else
    int ret = close(s);
#endif
    return ret == 0 ? E_NONE : E_EXTERNAL(P_sock_errno());
}

//-----------------------------------------------------------------------------
// Socket 选项设置
//-----------------------------------------------------------------------------

// 设置非阻塞模式
static inline ret_t P_sock_nonblock(sock_t s, bool enable) {
    if (s == P_INVALID_SOCKET) return E_INVALID;
#if P_WIN
    u_long mode = enable ? 1 : 0;
    int ret = ioctlsocket(s, FIONBIO, &mode);
    return ret == 0 ? E_NONE : E_EXTERNAL(WSAGetLastError());
#else
    int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0) return E_EXTERNAL(errno);
    if (enable) flags |= O_NONBLOCK;
    else flags &= ~O_NONBLOCK;
    int ret = fcntl(s, F_SETFL, flags);
    return ret == 0 ? E_NONE : E_EXTERNAL(errno);
#endif
}

// 设置地址重用（SO_REUSEADDR）
static inline ret_t P_sock_reuseaddr(sock_t s, bool enable) {
    if (s == P_INVALID_SOCKET) return E_INVALID;
    int opt = enable ? 1 : 0;
    int ret = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    return ret == 0 ? E_NONE : E_EXTERNAL(P_sock_errno());
}

// 设置端口重用（SO_REUSEPORT，仅 POSIX）
#if !P_WIN
static inline ret_t P_sock_reuseport(sock_t s, bool enable) {
    if (s == P_INVALID_SOCKET) return E_INVALID;
    int opt = enable ? 1 : 0;
    int ret = setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (const char*)&opt, sizeof(opt));
    return ret == 0 ? E_NONE : E_EXTERNAL(P_sock_errno());
}
#endif

// 设置 TCP_NODELAY（禁用 Nagle 算法）
static inline ret_t P_sock_nodelay(sock_t s, bool enable) {
    if (s == P_INVALID_SOCKET) return E_INVALID;
    int opt = enable ? 1 : 0;
    int ret = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(opt));
    return ret == 0 ? E_NONE : E_EXTERNAL(P_sock_errno());
}

// 设置 SO_KEEPALIVE
static inline ret_t P_sock_keepalive(sock_t s, bool enable) {
    if (s == P_INVALID_SOCKET) return E_INVALID;
    int opt = enable ? 1 : 0;
    int ret = setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (const char*)&opt, sizeof(opt));
    return ret == 0 ? E_NONE : E_EXTERNAL(P_sock_errno());
}

// 设置发送超时
static inline ret_t P_sock_sndtimeo(sock_t s, int ms) {
    if (s == P_INVALID_SOCKET) return E_INVALID;
#if P_WIN
    DWORD timeout = (DWORD)ms;
    int ret = setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv = { ms / 1000, (ms % 1000) * 1000 };
    int ret = setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
    return ret == 0 ? E_NONE : E_EXTERNAL(P_sock_errno());
}

// 设置接收超时
static inline ret_t P_sock_rcvtimeo(sock_t s, int ms) {
    if (s == P_INVALID_SOCKET) return E_INVALID;
#if P_WIN
    DWORD timeout = (DWORD)ms;
    int ret = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv = { ms / 1000, (ms % 1000) * 1000 };
    int ret = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
    return ret == 0 ? E_NONE : E_EXTERNAL(P_sock_errno());
}

// 设置发送缓冲区大小
static inline ret_t P_sock_sndbuf(sock_t s, int size) {
    if (s == P_INVALID_SOCKET) return E_INVALID;
    int ret = setsockopt(s, SOL_SOCKET, SO_SNDBUF, (const char*)&size, sizeof(size));
    return ret == 0 ? E_NONE : E_EXTERNAL(P_sock_errno());
}

// 设置接收缓冲区大小
static inline ret_t P_sock_rcvbuf(sock_t s, int size) {
    if (s == P_INVALID_SOCKET) return E_INVALID;
    int ret = setsockopt(s, SOL_SOCKET, SO_RCVBUF, (const char*)&size, sizeof(size));
    return ret == 0 ? E_NONE : E_EXTERNAL(P_sock_errno());
}

//-----------------------------------------------------------------------------
// sendmsg
//-----------------------------------------------------------------------------

#if P_WIN
typedef WSAMSG sock_msg_t;
#define P_msg_buf(msg)  ((const void*)(msg)->buf)
#define P_msg_len(msg)  ((int)(msg)->len)
static inline void P_msg_set(sock_msg_t* msg, const void* buf, size_t len) { msg->buf = (char*)buf; msg->len = (ULONG)len; }
static inline ret_t P_msg_send_to(sock_t s, const sock_msg_t* msgs, size_t num, const struct sockaddr_in* addr) {
    if (!fn_WSASendMsg) return E_EXTERNAL(WSAENOSYS);
    WSAMSG mh = {0};
    mh.name = (LPSOCKADDR)addr;
    mh.namelen = sizeof(struct sockaddr_in);
    mh.lpBuffers = (LPWSABUF)msgs;
    mh.dwBufferCount = (DWORD)num;
    DWORD bytesSent = 0;
    int ret = fn_WSASendMsg(s, &mh, 0, &bytesSent, NULL, NULL);
    return ret == 0 ? (ret_t)bytesSent : E_EXTERNAL(WSAGetLastError());
}
#else
typedef struct iovec sock_msg_t;
#define P_msg_buf(msg)  ((const void*)(msg)->iov_base)
#define P_msg_len(msg)  ((int)(msg)->iov_len)
static inline void P_msg_set(sock_msg_t* msg, const void* buf, size_t len) { msg->iov_base = (void*)buf; msg->iov_len = len; }
static inline ret_t P_msg_send_to(sock_t s, const sock_msg_t* msgs, size_t num, const struct sockaddr_in* addr) {
    struct msghdr mh = {0};
    mh.msg_name = (void*)addr;
    mh.msg_namelen = sizeof(struct sockaddr_in);
    mh.msg_iov = (struct iovec*)msgs;
    mh.msg_iovlen = (int)num;
    ret_t sz = (ret_t)sendmsg(s, &mh, 0);
    return sz >= 0 ? sz : E_EXTERNAL(errno);
}
#endif

///////////////////////////////////////////////////////////////////////////////
// 终端/控制
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// 终端检测：检查文件流是否是终端设备
//-----------------------------------------------------------------------------

#if P_WIN
#   include <io.h>
#   include <conio.h>
#   define P_isatty(f) _isatty(_fileno(f))
#else
#   include <sys/ioctl.h>
#   include <termios.h>
#   define P_isatty(f) isatty(fileno(f))
#endif

//-----------------------------------------------------------------------------
// ANSI VT 序列支持
//
//  P_TERM_VT: 编译期标志，控制所有 ANSI 转义序列宏是否生成实际内容
//    POSIX:            始终为 1（所有现代终端均支持）
//    Windows 10+:      为 1（需先调用 P_term_init() 启用 VT 模式）
//    Windows 旧版本:    为 0（所有宏回退为空字符串）
//
//  可在 #include <stdc.h> 前通过 #define P_TERM_VT 0/1 手动覆盖
//  （例如：输出重定向到文件、或目标终端不兼容 VT 序列时强制置 0）
//-----------------------------------------------------------------------------

#ifndef P_TERM_VT
#   define P_TERM_VT 1
#endif

#if P_TERM_VT && P_WIN
#   if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0A00
#       undef P_TERM_VT
#       define P_TERM_VT 0
#   endif
#endif

//-----------------------------------------------------------------------------
// 终端上下文与尺寸
//-----------------------------------------------------------------------------

typedef struct {
#if P_WIN
    HANDLE  hin;                /* STD_INPUT_HANDLE  */
    HANDLE  hout;               /* STD_OUTPUT_HANDLE */
    DWORD   in_mode;            /* 原始输入模式 */
    DWORD   out_mode;           /* 原始输出模式 */
#else
    struct termios  termios;    /* 原始 termios */
    int             fl;         /* 原始 fcntl 标志 */
#endif
    bool            pty;        /* true = ConPTY/管道（Windows），POSIX 始终 false */
} P_term_ctx_t;

/**
 * @param ctx  终端上下文；可为 NULL（退回 GetStdHandle / STDOUT_FILENO）
 */
static inline int P_term_rows(const P_term_ctx_t *ctx) {
#if P_WIN
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE h = (ctx && ctx->hout) ? ctx->hout : GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(h, &csbi)) {
        int rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        if (rows > 4) return rows;
    }
#else
    (void)ctx;
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 4)
        return (int)ws.ws_row;
#endif
    return 24;
}

static inline int P_term_cols(const P_term_ctx_t *ctx) {
#if P_WIN
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE h = (ctx && ctx->hout) ? ctx->hout : GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(h, &csbi)) {
        int cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        if (cols > 10) return cols;
    }
#else
    (void)ctx;
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 10)
        return (int)ws.ws_col;
#endif
    return 80;
}

//-----------------------------------------------------------------------------
// 终端初始化与恢复
//
// P_term_init(ctx)  — 检测 tty、启用 VT 模式(Win)、进入 raw 模式
//                     ctx 非 NULL 时保存原始状态; ctx->pty 标识 ConPTY 管道
//                     返回 false 表示非终端（管道/重定向），不做任何改动
// P_term_final(ctx) — 恢复原始终端设置
// P_term_input(ctx) — 非阻塞读取一个字符
//-----------------------------------------------------------------------------

/**
 * 终端初始化：检测 tty、启用 VT 模式、进入 raw 模式（禁用行缓冲/回显）
 * @param ctx  保存原始状态，供 P_term_final 恢复；可为 NULL（仅检测 + VT 启用）
 * @return true  — 成功（stdout 是终端）
 *         false — 非终端（管道/重定向），不做任何改动
 */
static inline bool P_term_init(P_term_ctx_t *ctx) {
    if (!P_isatty(stdout)) return false;
#if P_WIN
    HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hin  = GetStdHandle(STD_INPUT_HANDLE);
    DWORD out_mode = 0;
    GetConsoleMode(hout, &out_mode);
#if P_TERM_VT
    SetConsoleMode(hout, out_mode | 0x0004u  /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */
                                  | 0x0008u); /* DISABLE_NEWLINE_AUTO_RETURN */
#endif
    SetConsoleOutputCP(65001);  /* UTF-8 */
    SetConsoleCP(65001);        /* UTF-8 */
    if (ctx) {
        ctx->hout = hout;
        ctx->hin  = hin;
        ctx->out_mode = out_mode;
        ctx->pty = (GetFileType(hin) != FILE_TYPE_CHAR);
        if (!ctx->pty) {
            GetConsoleMode(hin, &ctx->in_mode);
            DWORD new_in = ctx->in_mode & ~(0x0002u /* ENABLE_LINE_INPUT */
                                           |0x0004u /* ENABLE_ECHO_INPUT */);
#if P_TERM_VT
            new_in |= 0x0200u; /* ENABLE_VIRTUAL_TERMINAL_INPUT */
#endif
            SetConsoleMode(hin, new_in);
        }
    }
#else
    if (ctx) {
        ctx->pty = false;
        tcgetattr(STDIN_FILENO, &ctx->termios);
        ctx->fl = fcntl(STDIN_FILENO, F_GETFL);
        struct termios t = ctx->termios;
        t.c_lflag &= ~(unsigned)(ICANON | ECHO);
        t.c_cc[VMIN]  = 0;
        t.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
        fcntl(STDIN_FILENO, F_SETFL, ctx->fl | O_NONBLOCK);
    }
#endif
    return true;
}

static inline void P_term_final(const P_term_ctx_t *ctx) {
    if (!ctx) return;
#if P_WIN
    if (!ctx->pty)
        SetConsoleMode(ctx->hin,  ctx->in_mode);
    SetConsoleMode(ctx->hout, ctx->out_mode);
#else
    tcsetattr(STDIN_FILENO, TCSANOW, &ctx->termios);
    fcntl(STDIN_FILENO, F_SETFL, ctx->fl);
#endif
}

/**
 * 非阻塞读取一个字符（需先 P_term_init 进入 raw 模式）
 * @return 0-255 字符值，-1 表示无输入
 */
static inline int P_term_input(const P_term_ctx_t *ctx) {
#if P_WIN
    if (ctx->pty) {
        DWORD avail = 0;
        if (!PeekNamedPipe(ctx->hin, NULL, 0, NULL, &avail, NULL) || avail == 0) return -1;
        DWORD nr = 0; CHAR raw = 0;
        if (!ReadFile(ctx->hin, &raw, 1, &nr, NULL) || nr == 0) return -1;
        return (unsigned char)raw;
    } else {
        if (!_kbhit()) return -1;
        int ch = _getch();
        if (ch == 0 || ch == 0xE0) { _getch(); return -1; }
        return ch;
    }
#else
    (void)ctx;
    unsigned char tmp;
    if (read(STDIN_FILENO, &tmp, 1) != 1) return -1;
    return tmp;
#endif
}

//--- 光标与屏幕控制 ---
//  动态参数版本（如 "\033[%d;1H"）仍需应用层自行 printf
//  以下仅提供常用的固定序列

#if P_TERM_VT
#   define P_CSI                 "\033["        // 控制序列引导符
#   define P_CURSOR_SAVE         "\0337"        // 保存光标位置 (DEC)
#   define P_CURSOR_RESTORE      "\0338"        // 恢复光标位置 (DEC)
#   define P_CURSOR_HOME         "\033[H"       // 光标移到 (1,1)
#   define P_CURSOR_TO           "\033[%d;%dH"  // 光标移到 (row, col)  — 2 参数
#   define P_CURSOR_ROW          "\033[%d;1H"   // 光标移到 (row, 1)   — 1 参数
#   define P_CLEAR_EOL           "\033[K"       // 清除光标到行尾
#   define P_CLEAR_LINE          "\033[2K"      // 清除整行
#   define P_CLEAR_SCREEN        "\033[2J"      // 清除整屏
#   define P_SCROLL_SET          "\033[%d;%dr"  // 设置滚动区域 (top, bottom) — 2 参数
#   define P_SCROLL_RESET        "\033[r"       // 重置滚动区域为全屏
#   define P_BOLD_BEGIN          "\033[1m"      // 加粗
#   define P_DIM_BEGIN           "\033[2m"      // 变暗
#   define P_UNDERLINE_BEGIN     "\033[4m"      // 下划线
#   define P_REVERSE_BEGIN       "\033[7m"      // 反色（前景/背景互换）
#else
#   define P_CSI                 ""
#   define P_CURSOR_SAVE         ""
#   define P_CURSOR_RESTORE      ""
#   define P_CURSOR_HOME         ""
#   define P_CURSOR_TO           "%.0d%.0d"     // 消耗 2 个 int 参数，不输出
#   define P_CURSOR_ROW          "%.0d"         // 消耗 1 个 int 参数，不输出
#   define P_CLEAR_EOL           ""
#   define P_CLEAR_LINE          ""
#   define P_CLEAR_SCREEN        ""
#   define P_SCROLL_SET          "%.0d%.0d"     // 消耗 2 个 int 参数，不输出
#   define P_SCROLL_RESET        ""
#   define P_BOLD_BEGIN          ""
#   define P_DIM_BEGIN           ""
#   define P_UNDERLINE_BEGIN     ""
#   define P_REVERSE_BEGIN       ""
#endif

//--- 颜色 ---

#if P_TERM_VT
#   define P_COL(c, s)           "\033[" #c "m" s "\033[0m"
#   define P_COL_END             "\033[0m"
#   define P_COLOR_RESET         "\033[0m"
#   define P_BLACK_BEGIN         "\033[30m"
#   define P_RED_BEGIN           "\033[31m"
#   define P_GREEN_BEGIN         "\033[32m"
#   define P_YELLOW_BEGIN        "\033[33m"
#   define P_BLUE_BEGIN          "\033[34m"
#   define P_PURPLE_BEGIN        "\033[35m"
#   define P_CYAN_BEGIN          "\033[36m"
#   define P_WHITE_BEGIN         "\033[37m"
#   define P_GRAY_BEGIN          "\033[90m"
#   define P_HL_RED_BEGIN        "\033[91m"
#   define P_HL_GREEN_BEGIN      "\033[92m"
#   define P_HL_YELLOW_BEGIN     "\033[93m"
#   define P_HL_BLUE_BEGIN       "\033[94m"
#   define P_HL_PURPLE_BEGIN     "\033[95m"
#   define P_HL_CYAN_BEGIN       "\033[96m"
#   define P_HL_WHITE_BEGIN      "\033[97m"
#   define P_BLACK(s)            P_COL(30, s)
#   define P_RED(s)              P_COL(31, s)
#   define P_GREEN(s)            P_COL(32, s)
#   define P_YELLOW(s)           P_COL(33, s)
#   define P_BLUE(s)             P_COL(34, s)
#   define P_PURPLE(s)           P_COL(35, s)
#   define P_CYAN(s)             P_COL(36, s)
#   define P_WHITE(s)            P_COL(37, s)
#   define P_GRAY(s)             P_COL(90, s)
#   define P_HL_RED(s)           P_COL(91, s)
#   define P_HL_GREEN(s)         P_COL(92, s)
#   define P_HL_YELLOW(s)        P_COL(93, s)
#   define P_HL_BLUE(s)          P_COL(94, s)
#   define P_HL_PURPLE(s)        P_COL(95, s)
#   define P_HL_CYAN(s)          P_COL(96, s)
#   define P_HL_WHITE(s)         P_COL(97, s)
#   define P_HL_BLACK(s)         P_COL(90, s)
#else
#   define P_COL(c, s)           s
#   define P_COL_END             ""
#   define P_COLOR_RESET         ""
#   define P_BLACK_BEGIN         ""
#   define P_RED_BEGIN           ""
#   define P_GREEN_BEGIN         ""
#   define P_YELLOW_BEGIN        ""
#   define P_BLUE_BEGIN          ""
#   define P_PURPLE_BEGIN        ""
#   define P_CYAN_BEGIN          ""
#   define P_WHITE_BEGIN         ""
#   define P_GRAY_BEGIN          ""
#   define P_HL_RED_BEGIN        ""
#   define P_HL_GREEN_BEGIN      ""
#   define P_HL_YELLOW_BEGIN     ""
#   define P_HL_BLUE_BEGIN       ""
#   define P_HL_PURPLE_BEGIN     ""
#   define P_HL_CYAN_BEGIN       ""
#   define P_HL_WHITE_BEGIN      ""
#   define P_BLACK(s)            s
#   define P_RED(s)              s
#   define P_GREEN(s)            s
#   define P_YELLOW(s)           s
#   define P_BLUE(s)             s
#   define P_PURPLE(s)           s
#   define P_CYAN(s)             s
#   define P_WHITE(s)            s
#   define P_GRAY(s)             s
#   define P_HL_RED(s)           s
#   define P_HL_GREEN(s)         s
#   define P_HL_YELLOW(s)        s
#   define P_HL_BLUE(s)          s
#   define P_HL_PURPLE(s)        s
#   define P_HL_CYAN(s)          s
#   define P_HL_WHITE(s)         s
#   define P_HL_BLACK(s)         s
#endif

///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif
#pragma ide diagnostic pop
#pragma clang diagnostic pop
#endif  // STDC_H_
