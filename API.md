# stdc API 参考手册

stdc 跨平台 C 库完整 API 文档。

## 目录

- [平台检测](#平台检测)
- [错误码](#错误码)
- [内存管理](#内存管理)
- [日志系统](#日志系统)
- [命令行参数](#命令行参数)
- [时间与时钟](#时间与时钟)
- [文件系统](#文件系统)
- [目录遍历](#目录遍历)
- [原子操作](#原子操作)
- [线程与同步](#线程与同步)
- [网络编程](#网络编程)
- [终端操作](#终端操作)

---

## 平台检测

跨平台识别宏。

### 平台宏

| 宏 | 说明 |
|-------|-------------|
| `P_WIN` | Windows (包括 Win64) |
| `P_DARWIN` | macOS/Darwin (Apple + Mach 内核) |
| `P_LINUX` | Linux |
| `P_BSD` | FreeBSD/OpenBSD/NetBSD |
| `P_UNIX` | 类 Unix 系统 |
| `P_POSIX` | POSIX 兼容系统 |
| `P_POSIX_LIKE` | POSIX 或 MinGW |

### 使用示例

```c
#if P_WIN
    // Windows 专用代码
#elif P_DARWIN
    // macOS 专用代码
#elif P_LINUX
    // Linux 专用代码
#endif
```

---

## 错误码

标准错误码定义 (`ret_t` = `int32_t`)。

| 错误码 | 值 | 说明 |
|------------|-------|-------------|
| `E_NONE` | 0 | 成功 |
| `E_UNKNOWN` | -1 | 未知错误 |
| `E_INVALID` | -2 | 无效参数或操作 |
| `E_CONFLICT` | -3 | 资源冲突或排他性冲突 |
| `E_OUT_OF_RANGE` | -4 | 请求超出限制 |
| `E_OUT_OF_MEMORY` | -5 | 内存不足 |
| `E_OUT_OF_SUPPLY` | -6 | 供给不足或下溢 |
| `E_OUT_OF_CAPACITY` | -7 | 容量超限或溢出 |
| `E_NONE_EXISTS` | -8 | 项不存在 |
| `E_NONE_CONTEXT` | -9 | 未初始化或未启动 |
| `E_NONE_RELEASED` | -10 | 资源未释放 |
| `E_BUSY` | -11 | 忙碌，请稍后重试 |
| `E_NO_PERMISSION` | -12 | 权限不足 |
| `E_NO_SUPPORT` | -13 | 功能不支持 |
| `E_CUSTOM(e)` | -100-e | 自定义错误码 |
| `E_EXTERNAL(e)` | -(e<<8) | 外部系统错误码 |

### 函数

```c
int E_EXT_CODE(ret_t e)  // 从 E_EXTERNAL() 中提取外部错误码
```

---

## 内存管理

### 函数

```c
void* P_alloc(size_t size)                  // 分配内存 (malloc)
void* P_realloc(void* ptr, size_t size)     // 重新分配内存
void  P_free(void* ptr)                     // 释放内存
```

### 宏

```c
P_check(expr, ...)  // 运行时断言，可自定义失败动作
```

---

## 日志系统

灵活的日志系统，支持自动日志名检测和多种后端。

### 类型

```c
typedef int (*log_cb)(int slot, const char* msg, size_t len);
typedef struct { const char* tag; log_cb cb; bool bLine; } log_slot_st;
```

### 配置

```c
// 配置日志输出
ret_t log_output(log_cb cb, bool bLine);

// cb 的特殊值：
//   (log_cb)-1  : stdout (默认)
//   (log_cb)-2  : 系统日志 (os_log/syslog/OutputDebugString)
//   NULL        : 清理/禁用
```

### 日志函数

```c
// 自动识别日志级别的日志输出 (V/D/I/W/E/F 前缀)
int printf(const char* fmt, ...);

// 定义自定义日志槽
ret_t log_slot(int slot, const char* tag, log_cb cb, bool bLine);
```

### 日志级别

消息通过首字符自动识别级别：
- `V:` - Verbose (详细)
- `D:` - Debug (调试)
- `I:` - Info (信息)
- `W:` - Warning (警告)
- `E:` - Error (错误)
- `F:` - Fatal (致命)

### 配置宏

| 宏 | 默认值 | 说明 |
|-------|---------|-------------|
| `LOG_TAG_MAX` | -24 | 标签字段宽度（负数表示左对齐） |
| `LOG_TAG_L` | `"["` | 标签左括号 |
| `LOG_TAG_R` | `"]"` | 标签右括号 |
| `LOG_LINE_MAX` | 2048 | 最大日志行长度 |

### 示例

```c
log_output((log_cb)-2, false);  // 输出到系统日志
printf("I: 服务器启动于端口 %d\n", 8080);
printf("E: 连接失败: %s\n", error_msg);
```

---

## 命令行参数

声明式命令行参数解析，自动生成帮助信息。

### 参数定义宏

```c
// 定义参数变量（必须在全局作用域）
ARGS_B(required, name, short_opt, long_opt, description)  // 布尔型
ARGS_I(required, name, short_opt, long_opt, description)  // 整数型
ARGS_F(required, name, short_opt, long_opt, description)  // 浮点型
ARGS_S(required, name, short_opt, long_opt, description)  // 字符串型
ARGS_D(required, name, short_opt, long_opt, description)  // 目录型
ARGS_L(required, name, short_opt, long_opt, description)  // 列表型（多值）
```

### 函数

```c
// 解析参数（以 NULL 结束）
int ARGS_parse(int argc, char** argv, .../* arg_vars, NULL */);

// 设置自定义用法文本
void ARGS_usage(const char* pos_desc, const char* usage_ex);

// 打印参数值
int ARGS_print(const char* arg0);

// 获取列表参数数量
int ARGS_ls_count(arg_var_st* var);
```

### 示例

```c
// 全局作用域
ARGS_B(false, verbose, 'v', "verbose", "启用详细输出");
ARGS_I(true, port, 'p', "port", "服务器端口");
ARGS_S(false, host, 'h', "host", "服务器主机名");

int main(int argc, char** argv) {
    ARGS_usage("<输入> <输出>", "myapp --verbose -p 8080 in.txt out.txt");
    
    if (ARGS_parse(argc, argv, &ARGS_DEF_verbose, &ARGS_DEF_port, 
                   &ARGS_DEF_host, NULL) != E_NONE) {
        return 1;
    }
    
    if (verbose) printf("I: 详细模式已启用\n");
    printf("I: 端口: %d\n", port);
    if (host) printf("I: 主机: %s\n", host);
    
    return 0;
}
```

---

## 时间与时钟

高精度时间和时钟操作。

### 类型

```c
typedef struct {
    int64_t sec;   // 秒（Unix 时间戳或持续时间）
    int32_t nsec;  // 纳秒 (0-999,999,999)
} P_clock;
```

### 函数

```c
// 获取系统墙钟时间（UTC）
ret_t P_time_now(P_clock* clock);

// 获取单调时钟（用于可靠的时间间隔）
ret_t P_clock_now(P_clock* clock);

// 获取进程/线程 CPU 时间
ret_t P_cost_now(P_clock* clock, bool bProcessOrThread);

// 睡眠（微秒）
void P_usleep(uint64_t us);
```

### 示例

```c
P_clock start, end;
P_clock_now(&start);

// ... 执行工作 ...

P_clock_now(&end);
int64_t elapsed_ms = (end.sec - start.sec) * 1000 + 
                     (end.nsec - start.nsec) / 1000000;
printf("I: 耗时: %lld 毫秒\n", elapsed_ms);
```

---

## 文件系统

跨平台文件系统操作。

### 函数

```c
// 检查文件访问权限
bool P_access(const char* path, bool bReadable, bool bWritable);

// 获取文件信息
ret_t P_stat(const char* path, stat_t* st);

// 获取文件大小
int64_t P_size(const char* path);

// 删除文件或空目录
ret_t P_remo(const char* path);

// 获取可执行文件路径
bool P_exe_file(char buffer[], uint32_t size);

// 获取当前工作目录
ret_t P_work_dir(char buffer[], uint32_t size);

// 检查路径是否为目录
ret_t P_is_dir(const char* path, bool writeable);

// 创建目录
ret_t P_mkdir(const char* path);

// 删除空目录
ret_t P_rmdir(const char* path);

// 读取符号链接
ret_t P_read_link(const char* path, char* buffer, size_t size);
```

### 示例

```c
if (P_access("config.ini", true, false)) {
    int64_t size = P_size("config.ini");
    printf("I: 配置文件大小: %lld 字节\n", size);
}

char exe_path[256];
if (P_exe_file(exe_path, sizeof(exe_path))) {
    printf("I: 可执行文件: %s\n", exe_path);
}
```

---

## 目录遍历

遍历目录内容。

### 类型

```c
typedef struct {
    // 平台特定成员（不透明）
} dir_t;
```

### 函数

```c
// 打开目录进行遍历
ret_t P_open_dir(dir_t* dir, const char* path);

// 关闭目录句柄
void P_close_dir(dir_t* dir);

// 获取下一个条目（完成时返回 NULL）
const char* P_dir_next(dir_t* dir);

// 获取当前条目名称（仅文件名）
const char* P_dir_name(const dir_t* dir);

// 获取当前条目完整路径
const char* P_dir_fullname(dir_t* dir);

// 获取当前条目大小
int64_t P_dir_size(dir_t* dir);

// 检查当前条目是否为目录
bool P_dir_is_dir(dir_t* dir);
```

### 示例

```c
dir_t dir;
if (P_open_dir(&dir, ".") == E_NONE) {
    while (P_dir_next(&dir)) {
        printf("%s %10lld %s\n",
               P_dir_is_dir(&dir) ? "目录" : "文件",
               P_dir_size(&dir),
               P_dir_name(&dir));
    }
    P_close_dir(&dir);
}
```

---

## 原子操作

无锁原子操作，支持内存顺序控制。

### 类型

```c
typedef volatile int      aint_t;
typedef volatile int32_t  aint32_t;
typedef volatile int64_t  aint64_t;
typedef volatile uint32_t auint32_t;
typedef volatile void*    aptr_t;
// ... 以及更多整数类型
```

### 基本操作

```c
// 加载/存储（宽松顺序）
T P_get(T* pVar)
void P_set(T* pVar, T value)

// 加载/存储（获取/释放顺序）
T P_get_acq(T* pVar)
void P_set_rel(T* pVar, T value)

// 加载/存储（顺序一致性）
T P_get_ord(T* pVar)
void P_set_ord(T* pVar, T value)
```

### 原子算术

```c
// 增加（返回新值）
T P_inc(T* pVar, T value)        // relaxed
T P_inc_acq(T* pVar, T value)    // acquire
T P_inc_rel(T* pVar, T value)    // release
T P_inc_ord(T* pVar, T value)    // seq_cst

// 获取旧值，然后增加
T P_get_and_inc(T* pVar, T value)
```

### 原子位运算

```c
// 位运算（返回新值）
T P_and(T* pVar, T value)
T P_or(T* pVar, T value)
T P_xor(T* pVar, T value)

// 获取旧值，然后操作
T P_get_and_and(T* pVar, T value)
T P_get_and_or(T* pVar, T value)
T P_get_and_xor(T* pVar, T value)
```

### 比较并交换

```c
// 测试并设置（比较交换）
bool P_test_and_set(T* pVar, T* pExpected, T desired)
bool P_test_and_set_acq(T* pVar, T* pExpected, T desired)
bool P_test_and_set_rel(T* pVar, T* pExpected, T desired)
bool P_test_and_set_ord(T* pVar, T* pExpected, T desired)
```

### 示例

```c
aint32_t counter = 0;

// 多线程可以安全地增加
P_inc(&counter, 1);

// 原子比较并交换
int expected = 5;
if (P_test_and_set(&counter, &expected, 10)) {
    printf("I: 从 5 改为 10\n");
} else {
    printf("I: 值是 %d，不是 5\n", expected);
}
```

---

## 线程与同步

跨平台线程原语（Unix 上使用 POSIX 线程，Win32 上使用 Windows 线程）。

### 类型

```c
typedef struct P_mutex_t P_mutex_t;
typedef struct P_cond_t P_cond_t;
typedef uintptr_t thd_t;  // 线程句柄
```

### 互斥锁操作

```c
// 初始化互斥锁
ret_t P_mutex_init(P_mutex_t* mutex);

// 销毁互斥锁
void P_mutex_destroy(P_mutex_t* mutex);

// 锁定互斥锁
ret_t P_mutex_lock(P_mutex_t* mutex);

// 尝试锁定（非阻塞）
ret_t P_mutex_trylock(P_mutex_t* mutex);

// 解锁互斥锁
ret_t P_mutex_unlock(P_mutex_t* mutex);
```

### 条件变量

```c
// 初始化条件变量
ret_t P_cond_init(P_cond_t* cond);

// 销毁条件变量
void P_cond_destroy(P_cond_t* cond);

// 等待条件
ret_t P_cond_wait(P_cond_t* cond, P_mutex_t* mutex);

// 带超时等待
int P_wait_timeout(P_cond_t* cond, P_mutex_t* mutex, const P_clock* timeout);

// 唤醒一个等待者
ret_t P_cond_signal(P_cond_t* cond);

// 广播唤醒所有等待者
ret_t P_cond_broadcast(P_cond_t* cond);
```

### 线程管理

```c
// 线程入口点签名
typedef int (*thd_fn)(void* param);

// 创建线程
ret_t P_thread(thd_t* pHandle, thd_fn fn, void* param);

// 等待线程结束
ret_t P_join(thd_t handle, int32_t* exit_code);

// 获取当前线程 ID
uint64_t P_thd_id(void);

// 设置线程优先级
ret_t P_thd_priority(thd_t handle, int priority);  // 0: 普通, 1: 高, -1: 低
```

### 示例

```c
P_mutex_t mutex;
P_cond_t cond;
int ready = 0;

int worker_thread(void* param) {
    P_mutex_lock(&mutex);
    // ... 执行工作 ...
    ready = 1;
    P_cond_signal(&cond);
    P_mutex_unlock(&mutex);
    return 0;
}

int main() {
    P_mutex_init(&mutex);
    P_cond_init(&cond);
    
    thd_t thread;
    P_thread(&thread, worker_thread, NULL);
    
    P_mutex_lock(&mutex);
    while (!ready) {
        P_cond_wait(&cond, &mutex);
    }
    P_mutex_unlock(&mutex);
    
    P_join(thread, NULL);
    
    P_cond_destroy(&cond);
    P_mutex_destroy(&mutex);
    return 0;
}
```

---

## 网络编程

跨平台 BSD socket 封装（Windows 上使用 Winsock2）。

### 初始化

```c
// 初始化网络子系统（Windows 上执行 WSAStartup）
ret_t P_net_init(void);

// 清理网络子系统
void P_net_cleanup(void);
```

### Socket 类型

```c
typedef sock_t;        // 平台特定的 socket 类型
typedef socklen_t;     // Socket 地址长度类型

#define P_INVALID_SOCKET   // 无效 socket 值
#define P_SOCKET_ERROR     // Socket 错误返回值
```

### Socket 操作

```c
// 关闭 socket
ret_t P_net_close(sock_t s);

// 获取最后的网络错误
int P_net_errno(void);

// 检查错误是否为 EINPROGRESS/EWOULDBLOCK
bool P_net_errno_is_inprogress(void);

// 检查错误是否为连接重置
bool P_net_errno_is_connreset(void);
```

### Socket 选项

```c
// 设置非阻塞模式
ret_t P_set_nonblock(sock_t s, bool enable);

// 设置 SO_REUSEADDR
ret_t P_set_reuseaddr(sock_t s, bool enable);

// 设置 SO_REUSEPORT（并非所有平台都支持）
ret_t P_set_reuseport(sock_t s, bool enable);

// 设置 TCP_NODELAY（禁用 Nagle 算法）
ret_t P_set_nodelay(sock_t s, bool enable);

// 设置 SO_KEEPALIVE
ret_t P_set_keepalive(sock_t s, bool enable);

// 设置发送超时（毫秒）
ret_t P_set_sndtimeo(sock_t s, int ms);

// 设置接收超时（毫秒）
ret_t P_set_rcvtimeo(sock_t s, int ms);

// 设置发送缓冲区大小
ret_t P_set_sndbuf(sock_t s, int size);

// 设置接收缓冲区大小
ret_t P_set_rcvbuf(sock_t s, int size);
```

### 地址转换

```c
// 将文本地址转换为二进制（支持 IPv4/IPv6）
ret_t P_inet_pton(int af, const char* src, void* dst);

// 将二进制地址转换为文本
const char* P_inet_ntop(int af, const void* src, char* dst, socklen_t size);
```

### 示例

```c
P_net_init();

sock_t sock = socket(AF_INET, SOCK_STREAM, 0);
P_set_nonblock(sock, true);
P_set_reuseaddr(sock, true);
P_set_nodelay(sock, true);

struct sockaddr_in addr;
addr.sin_family = AF_INET;
addr.sin_port = htons(8080);
P_inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

// ... 使用 socket ...

P_net_close(sock);
P_net_cleanup();
```

---

## 终端操作

终端检测和功能。

### 函数

```c
// 检查文件句柄是否为终端
bool P_isatty(FILE* stream);

// 获取终端高度（行数）
int P_term_rows(void);

// 获取终端宽度（列数）
int P_term_cols(void);
```

### ANSI 颜色支持

库提供 ANSI 颜色宏用于终端输出（如果支持）：

```c
// 文本颜色: C_BLACK, C_RED, C_GREEN, C_YELLOW, C_BLUE, C_MAGENTA, C_CYAN, C_WHITE
// 背景颜色: C_BG_RED, C_BG_GREEN 等
// 样式: C_BOLD, C_UNDERLINE, C_RESET

printf(C_RED "错误: " C_RESET "出错了\n");
```

### 示例

```c
if (P_isatty(stdout)) {
    int rows = P_term_rows();
    int cols = P_term_cols();
    printf("I: 终端大小: %dx%d\n", cols, rows);
    
    printf(C_GREEN "成功!" C_RESET "\n");
}
```

---

## 工具函数

### 字节序

```c
// 检查系统字节序
bool is_little_endian(void);  // 小端系统上返回 true
```

### 示例

```c
if (is_little_endian()) {
    printf("I: 系统是小端字节序\n");
} else {
    printf("I: 系统是大端字节序\n");
}
```

---

## 构建与链接要求

### 包含

```c
#include <stdc.h>
```

### 链接库

| 平台 | 必需的库 |
|----------|-------------------|
| **Linux** | `-lstdc -lpthread -lrt` |
| **macOS** | `-lstdc -lpthread -framework CoreFoundation` |
| **FreeBSD/OpenBSD/NetBSD** | `-lstdc -lpthread` |
| **Windows (MinGW)** | `-lstdc -lws2_32` |
| **Windows (MSVC)** | `stdc.lib`（ws2_32.lib 自动链接） |
| **QNX** | `-lstdc -lsocket` |
| **Android** | `-lstdc -llog` |

### CMake 集成

```cmake
find_package(stdc REQUIRED)
target_link_libraries(your_target PRIVATE stdc::stdc)
```

---

## 许可证

MIT License - 详见 [LICENSE](LICENSE) 文件。
