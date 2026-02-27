#ifndef STDC_H_
#define STDC_H_

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma ide diagnostic ignored "OCUnusedMacroInspection"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

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
#if !P_WIN || defined(__CYGWIN__) || defined(__MINGW32__)
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

#ifndef __FILE_NAME__
#define __FILE_NAME__ __FILE__
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
#define P_check(expr, ...) { if (!(expr)) { printf("E: %s@%d(%d)", __FUNCTION__, __LINE__, (int)(long)(expr)); __VA_ARGS__ ;} }
#else
#define P_check(expr, ...) assert(expr);
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
#define E_NO_PERMISSION         (-12)               /* 无权限访问 */
#define E_NO_SUPPORT            (-13)               /* 程序不支持（未设计）该功能 */
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
#define P_alloc(size)           malloc(size)
#define P_realloc(ptr, size)    realloc(ptr, size)
#define P_free(ptr)             free(ptr)

///////////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TAG
#define ROOT_TAG            ""                      /* 必须是预定义常量字符串 */
#endif
#ifndef MOD_TAG
#define MOD_TAG             ""                      /* 可以是一个字符串函数 */
#endif

#ifndef LOG_DEF
#define LOG_DEF             LOG_SLOT_NONE
#endif

#ifndef LOG_TAG_MAX
#define LOG_TAG_MAX         (-24)                   /* tag 格式化布局，abs 大小为 tag 宽度
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

#define LOG_LINE_MAX        2048

#ifndef LOG_OUTPUT_FILE
#define LOG_FILE_DISABLED
#endif

typedef enum {
    LOG_SLOT_VERBOSE = 0,                            /* 任何信息（常用于输出说明文档） */
    LOG_SLOT_DEBUG,                                  /* 用于程序调试 */
    LOG_SLOT_INFO,                                   /* 运行状态 */
    LOG_SLOT_WARN,                                   /* 警告信息。既可能会产生问题，或无法达到预期 */
    LOG_SLOT_ERROR,                                  /* 不应该发生的错误，但不影响程序继续运行 */
    LOG_SLOT_FATAL,                                  /* 导致程序无法再继续运行的错误 */
    LOG_SLOT_NONE,
} log_level_e;

typedef void(*log_cb)(log_level_e level, const char* tag, char *txt, int len);

/**
 * @brief 设置日志输出目标
 * @param cb_log 回调函数指针或特殊值：
 *   - (log_cb)-1: 输出到 stdout（默认）
 *   - (log_cb)-2: 输出到系统日志（os_log/syslog/OutputDebugString等）
 *   - NULL: 清理日志资源
 *   - 其他: 自定义回调函数
 * @param tag_separate 是否分离tag（传递给回调时tag是否独立）
 */
void
log_output(log_cb cb_log, bool tag_separate);

void
log_slot(log_level_e level, const char* tag, const char* fmt, va_list params);

static inline void printf_slot(const char* fmt, ...) {

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

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantConditionsOC"
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
#pragma clang diagnostic pop
    }

    assert(fmt); va_list args = {0};
    static TLS bool s_begin = false;

    // 如果 printf(""), 开启缓存模式。此外，如果之前已经开启缓存，则清空缓存，从头开始
    if (!*fmt) { s_begin = true;
        log_slot(LOG_SLOT_NONE, NULL, NULL, args);
        return;
    }
    if (*fmt == ':') {
        // 如果以双 ':' 开头，则在调整状态下，直接输出到标准输出
        if (*++fmt == ':') {
#ifndef NDEBUG
            fmt += fmt[1] == ' ' ? 2 : 1;
            va_start(args, fmt);
            vprintf(fmt, args);
            va_end(args);
#endif
            return;
        }
        // 否则如果以单 ':' 开头，则同 printf("") 一样开启缓存模式，并直接输出到缓存
        s_begin = true;
        va_start(args, fmt);
        log_slot(LOG_SLOT_NONE, NULL, fmt, args);
        va_end(args);
        return;
    }

    log_level_e level = LOG_DEF;
    if (fmt[1] == ':') {
        const char *q="VDIWEF", *p = strchr(q, *fmt);
        if (p) { s_begin = false;                           // 只要指定 level 标识，就会关闭缓存模式
            level = (log_level_e)(p - q); fmt+=2;
            if (*fmt == ' ') ++fmt;                         // 忽略 1 个且只忽略 1 个空格（即允许多个空格作为缩进）
        }
    }

    if (s_begin) {
        va_start(args, fmt);
        log_slot(LOG_SLOT_NONE, NULL, fmt, args);
        va_end (args);
        return;
    }

    if (level >= LOG_SLOT_NONE) return;                     // 如果默认级别为 NONE，则默认不输出任何内容

    va_start(args, fmt);
    log_slot(level, s_tag, fmt, args);
    va_end (args);
}

#define printf(fmt, ...)    printf_slot(fmt, ##__VA_ARGS__)

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
    ARG_DIR,                    ///< 目录路径，自动规范化：展开~、移除末尾/、合并//、处理/./
    ARG_LS,
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
    const char*  str;           ///< ARG_STR, ARG_DIR
    int64_t      i64;           ///< ARG_BOOL, ARG_INT
    double       f64;           ///< ARG_FLOAT
    const char** ls;            ///< ARG_LS，用 ARGS_ls_count() 获取数量
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
#define ARGS_B(req, name, s_cmd, l_cmd, desc)    ARGS_DEF(req, name, BOOL, s_cmd, l_cmd, desc)
#define ARGS_I(req, name, s_cmd, l_cmd, desc)    ARGS_DEF(req, name, INT, s_cmd, l_cmd, desc)
#define ARGS_F(req, name, s_cmd, l_cmd, desc)    ARGS_DEF(req, name, FLOAT, s_cmd, l_cmd, desc)
#define ARGS_S(req, name, s_cmd, l_cmd, desc)    ARGS_DEF(req, name, STR, s_cmd, l_cmd, desc)
#define ARGS_D(req, name, s_cmd, l_cmd, desc)    ARGS_DEF(req, name, DIR, s_cmd, l_cmd, desc)
#define ARGS_L(req, name, s_cmd, l_cmd, desc)    ARGS_DEF(req, name, LS, s_cmd, l_cmd, desc)

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

static inline bool is_little_endian(void) { int i = 1; return *(char*)&i; }

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
#define P_usleep(us) Sleep((DWORD)(((uint64_t)us + 999ULL) / 1000ULL));
#else
#define P_usleep(us) usleep(us);
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

///////////////////////////////////////////////////////////////////////////////
// 文件系统访问
///////////////////////////////////////////////////////////////////////////////

#if P_WIN
#   include <direct.h>              // _mkdir, _getcwd, _get_pgmptr
#   include <io.h>                  // _access, _stat64
    typedef struct _stat64 stat_t;
#elif P_DARWIN
#   include <mach-o/dyld.h>         // _NSGetExecutablePath
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
#endif

#ifndef PATH_MAX
#   if defined(FILENAME_MAX)
#       define PATH_MAX FILENAME_MAX
#   else
#       define PATH_MAX 4096
#   endif
#endif

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

static inline ret_t P_remo(cstr_t csFilePathName) {
#if P_WIN
    if (DeleteFile(csFilePathName)) return E_NONE;
   return E_EXTERNAL(GetLastError());
#else
    if (remove(csFilePathName) == 0) return E_NONE;
    return E_EXTERNAL(errno);
#endif
}

static inline bool P_exe_file(char buffer[], uint32_t size) {
#if P_WIN
    char *path;
    if (_get_pgmptr(&path) != 0) {
        buffer[0] = 0;
        printf("error: get exe path error(%d)\n", errno);  // todo 应该用 log 接口
        return false;
    }
    strncpy(buffer, path, size);
    buffer[size-1] = 0;
#elif P_DARWIN
    if (_NSGetExecutablePath(buffer, &size) != 0) {
        buffer[0] = 0;
        printf("error: get exe path error(%d)\n", errno);
        return false;
    }
    char *canonicalPath = realpath(buffer, NULL);
    if (!canonicalPath) {
        buffer[0] = 0;
        printf("error: get exe path error(%d)\n", errno);
        return false;
    }
    strncpy(buffer, canonicalPath, size);
    buffer[size-1] = 0;
    free(canonicalPath);
#elif P_BSD
    int mib[4];  mib[0] = CTL_KERN;  mib[1] = KERN_PROC;  mib[2] = KERN_PROC_PATHNAME;  mib[3] = -1;
    if (sysctl(mib, 4, buffer, &size, NULL, 0) != 0)
        *buffer = '\0';
#elif P_LINUX
    ssize_t len = readlink("/proc/self/exe", buffer, size);
    if (len == -1 || len == size) {
        buffer[0] = '\0';
        printf("error: get exe path error(%d)\n", errno);
        return false;
    }
    buffer[len] = '\0';
#elif defined(__QNX__)
    FILE* fp = fopen("/proc/self/exefile", "r");
    if (!fp) {
        *buffer = '\0';
        printf("E: read exe path failed: %d\n", errno);
        return false;
    }
    size = fread(buffer, 1, size, fp);
    fclose(fp);
    if (buffer[--size]) buffer[size] = 0;
#else
#error "Unsupported platform"
#endif
    return true;
}

//-----------------------------------------------------------------------------

static inline ret_t P_work_dir(char buffer[], uint32_t size) {
#if P_WIN
    if (!GetCurrentDirectory(size, buffer)) return E_EXTERNAL(GetLastError());
#else
    if (!getcwd(buffer, size)) return E_EXTERNAL(errno);
#endif
    return E_NONE;
}

static inline ret_t P_is_dir(const char* path, bool writeable) {
#if P_WIN
    DWORD dwAttr = GetFileAttributes(path);
   if (dwAttr == INVALID_FILE_ATTRIBUTES) {
       printf("debug: dir \"%s\" access error(%d)\n", path, GetLastError());
       return E_EXTERNAL(GetLastError());
   }
   if (!(dwAttr & FILE_ATTRIBUTE_DIRECTORY)) {
       printf("debug: dir \"%s\" not a directory\n", path);
       return E_NONE_CONTEXT;
   }
#else
    // 注: access 对路径末尾的 '/' 会自适应
    if (access(path, writeable ? W_OK : F_OK) != 0) {
        if (errno == ENOENT) { printf("debug: dir \"%s\" no exits\n", path); return E_NONE_EXISTS; }
        if (errno == EACCES || errno == EPERM) { printf("debug: dir \"%s\" no permission\n", path); return E_NO_PERMISSION; }
        printf("debug: dir \"%s\" access error(%d)\n", path, errno);
        return E_EXTERNAL(errno);
    }

    DIR* dp = opendir(path);
    if (!dp) {
        if (errno == ENOTDIR) {
            printf("debug: dir \"%s\" not a directory\n", path);
            return E_NONE_CONTEXT;
        }
        printf("debug: dir \"%s\" access error(%d)\n", path, errno);
        return E_EXTERNAL(errno);
    }
    closedir(dp);
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

    int err;
    pthread_attr_t attr;
    pthread_attr_t* pattr = NULL;

    // 只在需要自定义参数时创建 attr
    if (pThread->i32StackSize
#if defined(__ANDROID__)
        || ePolicy != P_THD_NORMAL
#endif
    ) {
        err = pthread_attr_init(&attr);
        P_check(!err, free(pThread); return E_EXTERNAL(err);)
        pattr = &attr;
        
        do {
            if (pThread->i32StackSize) {
                err = pthread_attr_setstacksize(&attr, pThread->i32StackSize);
                P_check(!err, break;)
            }

#if defined(__ANDROID__)
            // Android 平台支持调度策略和优先级设置
            // > SCHED_IDLE 和 SCHED_BATCH 是 Linux 扩展策略
            // > SCHED_OTHER 为默认分时调度策略
            err = pthread_attr_setschedpolicy(&attr, pThread->ePolicy == P_THD_IDLE       ? SCHED_IDLE :
                                                         pThread->ePolicy == P_THD_BACKGROUND ? SCHED_BATCH :
                                                                                                SCHED_OTHER);
            P_check(!err, break;)

            struct sched_param param = {0};
            switch (pThread->ePolicy) {
                case P_THD_IDLE: param.sched_priority = 19; break;
                case P_THD_BACKGROUND: param.sched_priority = 10; break;
                case P_THD_FOREGROUND: param.sched_priority = -4; break;
                case P_THD_REALTIME: param.sched_priority = -16; break;
                default:;
            }
            err = pthread_attr_setschedparam(&attr, &param);
            P_check(!err, break;)
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
    P_check(!err,
        free(pThread);
        return E_EXTERNAL(err);
    )

    if (r_hThread) *r_hThread = pHandle;
    else pthread_detach(pHandle);

#endif
    return E_NONE;
}

static inline ret_t P_join(thd_t hThread, int32_t* r_i32ExitCode/* nullable */) {

    P_check(hThread, return E_INVALID;)

#if P_WIN
    P_check(WaitForSingleObject(hThread, INFINITE) == WAIT_OBJECT_0,
        return E_EXTERNAL(GetLastError());
    )
    if (r_i32ExitCode) {
        DWORD dwExitCode = 0;
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
    P_check(!err,
        return E_EXTERNAL(err);
    )
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

#if P_WIN
static inline ret_t P_net_init(void) {
    WSADATA wsa;
    int err = WSAStartup(MAKEWORD(2, 2), &wsa);
    return err == 0 ? E_NONE : E_EXTERNAL(err);
}
static inline void P_net_cleanup(void) {
    WSACleanup();
}
#else
#define P_net_init()        E_NONE
#define P_net_cleanup()     ((void)0)
#endif

static inline int P_net_errno(void) {
#if P_WIN
    return WSAGetLastError();
#else
    return errno;
#endif
}

// 检查错误是否为"操作进行中"（非阻塞连接）
static inline bool P_net_errno_is_inprogress(void) {
#if P_WIN
    int err = WSAGetLastError();
    return err == WSAEWOULDBLOCK || err == WSAEINPROGRESS;
#else
    return errno == EINPROGRESS || errno == EWOULDBLOCK;
#endif
}

// 检查错误是否为"连接已重置"
static inline bool P_net_errno_is_connreset(void) {
#if P_WIN
    return WSAGetLastError() == WSAECONNRESET;
#else
    return errno == ECONNRESET;
#endif
}

//-----------------------------------------------------------------------------
// Socket 基础操作
// 注意：bind, listen, accept, connect, send, recv, sendto, recvfrom, shutdown
//       这些函数在所有平台上名字和语义一致，直接使用即可
//       只需确保类型定义统一（sock_t, socklen_t, ssize_t）
//-----------------------------------------------------------------------------

// 关闭 socket（跨平台差异：closesocket vs close）
static inline ret_t P_net_close(sock_t s) {
    if (s == P_INVALID_SOCKET) return E_INVALID;
#if P_WIN
    int ret = closesocket(s);
#else
    int ret = close(s);
#endif
    return ret == 0 ? E_NONE : E_EXTERNAL(P_net_errno());
}

//-----------------------------------------------------------------------------
// Socket 选项设置
//-----------------------------------------------------------------------------

// 设置非阻塞模式
static inline ret_t P_set_nonblock(sock_t s, bool enable) {
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
static inline ret_t P_set_reuseaddr(sock_t s, bool enable) {
    if (s == P_INVALID_SOCKET) return E_INVALID;
    int opt = enable ? 1 : 0;
    int ret = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    return ret == 0 ? E_NONE : E_EXTERNAL(P_net_errno());
}

// 设置端口重用（SO_REUSEPORT，仅 POSIX）
#if !P_WIN
static inline ret_t P_set_reuseport(sock_t s, bool enable) {
    if (s == P_INVALID_SOCKET) return E_INVALID;
    int opt = enable ? 1 : 0;
    int ret = setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (const char*)&opt, sizeof(opt));
    return ret == 0 ? E_NONE : E_EXTERNAL(P_net_errno());
}
#endif

// 设置 TCP_NODELAY（禁用 Nagle 算法）
static inline ret_t P_set_nodelay(sock_t s, bool enable) {
    if (s == P_INVALID_SOCKET) return E_INVALID;
    int opt = enable ? 1 : 0;
    int ret = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(opt));
    return ret == 0 ? E_NONE : E_EXTERNAL(P_net_errno());
}

// 设置 SO_KEEPALIVE
static inline ret_t P_set_keepalive(sock_t s, bool enable) {
    if (s == P_INVALID_SOCKET) return E_INVALID;
    int opt = enable ? 1 : 0;
    int ret = setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (const char*)&opt, sizeof(opt));
    return ret == 0 ? E_NONE : E_EXTERNAL(P_net_errno());
}

// 设置发送超时
static inline ret_t P_set_sndtimeo(sock_t s, int ms) {
    if (s == P_INVALID_SOCKET) return E_INVALID;
#if P_WIN
    DWORD timeout = (DWORD)ms;
    int ret = setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv = { ms / 1000, (ms % 1000) * 1000 };
    int ret = setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
    return ret == 0 ? E_NONE : E_EXTERNAL(P_net_errno());
}

// 设置接收超时
static inline ret_t P_set_rcvtimeo(sock_t s, int ms) {
    if (s == P_INVALID_SOCKET) return E_INVALID;
#if P_WIN
    DWORD timeout = (DWORD)ms;
    int ret = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv = { ms / 1000, (ms % 1000) * 1000 };
    int ret = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
    return ret == 0 ? E_NONE : E_EXTERNAL(P_net_errno());
}

// 设置发送缓冲区大小
static inline ret_t P_set_sndbuf(sock_t s, int size) {
    if (s == P_INVALID_SOCKET) return E_INVALID;
    int ret = setsockopt(s, SOL_SOCKET, SO_SNDBUF, (const char*)&size, sizeof(size));
    return ret == 0 ? E_NONE : E_EXTERNAL(P_net_errno());
}

// 设置接收缓冲区大小
static inline ret_t P_set_rcvbuf(sock_t s, int size) {
    if (s == P_INVALID_SOCKET) return E_INVALID;
    int ret = setsockopt(s, SOL_SOCKET, SO_RCVBUF, (const char*)&size, sizeof(size));
    return ret == 0 ? E_NONE : E_EXTERNAL(P_net_errno());
}

//-----------------------------------------------------------------------------
// 地址转换辅助函数
//-----------------------------------------------------------------------------

// 将字符串 IP 转换为二进制格式
static inline ret_t P_inet_pton(int af, const char* src, void* dst) {
    if (!src || !dst) return E_INVALID;
#if P_WIN
    // Windows 提供 inet_pton (Vista+)
    int ret = inet_pton(af, src, dst);
    if (ret == 1) return E_NONE;
    if (ret == 0) return E_INVALID;  // 无效格式
    return E_EXTERNAL(WSAGetLastError());
#else
    int ret = inet_pton(af, src, dst);
    if (ret == 1) return E_NONE;
    if (ret == 0) return E_INVALID;  // 无效格式
    return E_EXTERNAL(errno);
#endif
}

// 将二进制格式转换为字符串 IP
static inline const char* P_inet_ntop(int af, const void* src, char* dst, socklen_t size) {
    if (!src || !dst) return NULL;
#if P_WIN
    return inet_ntop(af, src, dst, size);
#else
    return inet_ntop(af, src, dst, size);
#endif
}

///////////////////////////////////////////////////////////////////////////////
// 终端
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// 终端检测：检查文件流是否是终端设备
//-----------------------------------------------------------------------------

#if P_WIN
#   include <io.h>
#   define P_isatty(f) _isatty(_fileno(f))
#else
#   define P_isatty(f) isatty(fileno(f))
#endif

//-----------------------------------------------------------------------------
// 终端尺寸：获取终端行数和列数
//-----------------------------------------------------------------------------

#if !P_WIN
#   include <sys/ioctl.h>
#endif

static inline int P_term_rows(void) {
#if P_WIN
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        int rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        if (rows > 4) return rows;
    }
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 4)
        return (int)ws.ws_row;
#endif
    return 24;  // 默认值
}

static inline int P_term_cols(void) {
#if P_WIN
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        int cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        if (cols > 10) return cols;
    }
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 10)
        return (int)ws.ws_col;
#endif
    return 80;  // 默认值
}

/*
 * ============================================================================
 * 注意：以下终端操作不适合跨平台统一封装（应由应用层按需实现）
 * ============================================================================
 *
 * 1. 终端模式控制（raw 模式 / 行缓冲 / 回显设置）：
 *    【Windows】GetConsoleMode() + SetConsoleMode()
 *               - 使用 DWORD 标志位组合（ENABLE_LINE_INPUT, ENABLE_ECHO_INPUT 等）
 *               - 需区分输入/输出句柄，支持 VT 模式（ENABLE_VIRTUAL_TERMINAL_PROCESSING）
 *    【POSIX】 tcgetattr() + tcsetattr()
 *               - 使用 termios 结构体，包含 c_iflag / c_lflag / c_cc 等多个字段
 *               - 典型 raw 模式设置：t.c_lflag &= ~(ICANON | ECHO)
 *    → 两者语义和 API 完全不同，强行统一会失去灵活性
 *
 * 2. 非阻塞键盘输入检测与读取：
 *    【Windows 真实控制台】_kbhit() + _getch()           （需 <conio.h>）
 *    【Windows ConPTY/管道】PeekNamedPipe() + ReadFile()  （VS Code 等模拟终端）
 *    【POSIX】read(STDIN_FILENO, ...) 配合 fcntl(O_NONBLOCK) + termios raw 模式
 *    → 实现方式差异巨大，依赖终端模式预配置，不适合通用封装
 *
 * 3. 终端专有 API（高度应用相关）：
 *    - POSIX: 窗口大小变化信号 SIGWINCH
 *    - Windows: Console Buffer / Screen Buffer 操作
 *    - 光标控制、滚动区域设置（ANSI 转义序列）
 */

//-----------------------------------------------------------------------------
// ANSI 转义序列（完整颜色集合）
// Windows 10+ 支持 VT 模式，可通过 P_FORCE_COLOR 强制开启
//-----------------------------------------------------------------------------

#if P_WIN && !defined(P_FORCE_COLOR)
// Windows 旧版本不支持 ANSI，禁用所有颜色输出
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
#else
// 支持 ANSI 转义序列（POSIX 或 Windows 10+ with VT mode）
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
#endif

///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif
#pragma clang diagnostic pop
#endif  // STDC_H_
