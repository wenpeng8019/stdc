# stdc - 跨平台 C 标准库扩展

跨平台 C 语言标准库扩展，提供统一的文件系统、线程、网络、日志等功能抽象层。

## 支持的平台

- **Linux** (Ubuntu, Debian, CentOS, etc.)
- **macOS** (10.12+)
- **FreeBSD / OpenBSD / NetBSD**
- **Windows** (Windows 7+, MSVC/MinGW)
- **QNX**
- **Android**

## 构建方式

### 方式 1: 使用 Makefile（推荐用于 Unix-like 系统）

```bash
# 构建
make

# 清理
make clean

# 安装到系统（需要 root 权限）
sudo make install

# 自定义安装路径
make install PREFIX=/opt/local

# 卸载
sudo make uninstall
```

### 方式 2: 使用 CMake（跨平台推荐）

```bash
# 配置构建（Release 模式）
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 构建
cmake --build build

# 安装
sudo cmake --install build

# 自定义安装路径
cmake -B build -DCMAKE_INSTALL_PREFIX=/opt/local
cmake --build build
cmake --install build
```

#### Windows 使用 CMake

```cmd
# 使用 MSVC
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release

# 使用 MinGW
cmake -B build -G "MinGW Makefiles"
cmake --build build
```

### 构建产物

两种构建系统产生相同的输出结构：

```
stdc/
├── libstdc.a          # 静态库（项目根目录）
├── build/             # 中间文件目录
│   ├── *.o            # Makefile: build/stdc.o
│   └── CMakeFiles/    # CMake: build/CMakeFiles/stdc.dir/stdc.c.o
└── test/
    └── example        # 测试示例（执行 make example 后生成）
```

**说明**：
- **静态库**：统一输出到项目根目录 `libstdc.a`，方便直接引用
- **中间文件**：`.o` 文件放在 `build/` 目录，保持根目录整洁
- **测试程序**：编译后放在 `test/` 目录

## 使用方法

### 链接依赖

在不同平台上，`libstdc.a` 需要链接以下系统库：

| 平台 | 必需的链接库 | 说明 |
|------|-------------|------|
| **Linux** | `-lstdc -lpthread -lrt` | pthread: POSIX 线程<br>rt: 实时扩展（时钟、定时器等） |
| **macOS** | `-lstdc -lpthread -framework CoreFoundation` | pthread: POSIX 线程<br>CoreFoundation: 系统日志 (os_log) |
| **FreeBSD** | `-lstdc -lpthread` | pthread: POSIX 线程 |
| **OpenBSD** | `-lstdc -lpthread` | pthread: POSIX 线程 |
| **NetBSD** | `-lstdc -lpthread` | pthread: POSIX 线程 |
| **Windows (MinGW)** | `-lstdc -lws2_32` | ws2_32: Winsock 2 网络库 |
| **Windows (MSVC)** | `stdc.lib` | ws2_32.lib 通过 `#pragma comment(lib)` 自动链接 |
| **QNX** | `-lstdc -lsocket` | socket: QNX 网络库 |
| **Android** | `-lstdc -llog` | log: Android 日志系统 (__android_log) |

**注意事项**：
- 上述链接库顺序通常不敏感，但建议 `-lstdc` 放在最前面
- 如果使用 CMake 的 `find_package(stdc)`，依赖会自动处理
- 部分平台（如 musl libc）可能不需要 `-lrt`，会自动包含在 C 库中
- **Windows MSVC**：`stdc.h` 中使用了 `#pragma comment(lib, "ws2_32.lib")` 自动链接依赖库，只需链接 `stdc.lib` 即可

### 在项目中使用（Makefile）

```makefile
# 编译选项
CFLAGS += -I/usr/local/include

# Linux
LDFLAGS += -L/usr/local/lib -lstdc -lpthread -lrt

# macOS
LDFLAGS += -L/usr/local/lib -lstdc -lpthread -framework CoreFoundation

# FreeBSD/OpenBSD/NetBSD
LDFLAGS += -L/usr/local/lib -lstdc -lpthread

# Windows (MinGW)
LDFLAGS += -L/usr/local/lib -lstdc -lws2_32

# Windows (MSVC) - ws2_32.lib 会通过 #pragma comment 自动链接
LDFLAGS += /LIBPATH:C:\lib stdc.lib
```

### 在项目中使用（CMake）

```cmake
# 查找库
find_package(stdc REQUIRED)

# 链接库
target_link_libraries(your_target PRIVATE stdc::stdc)
```

### 代码示例

```c
#include <stdc.h>

int main(int argc, char** argv) {
    // 使用命令行参数解析
    ARGS_B(false, verbose, 'v', "verbose", "Enable verbose output");
    ARGS_parse(argc, argv, &ARGS_DEF_verbose, NULL);
    
    // 使用日志系统（自动使用可执行文件名）
    log_output((log_cb)-2, false);  // 输出到系统日志
    printf("I: Application started\n");
    
    // 文件系统操作
    if (P_file_exists("config.ini")) {
        printf("I: Config file found\n");
    }
    
    // 目录遍历
    dir_t dir;
    if (P_open_dir(&dir, ".") == E_NONE) {
        while (P_dir_next(&dir) == E_NONE) {
            printf("  - %s\n", P_dir_name(&dir));
        }
        P_close_dir(&dir);
    }
    
    // 网络初始化
    P_net_init();
    
    // ... 其他操作
    
    // 清理
    log_output(NULL, false);
    P_net_cleanup();
    
    return 0;
}
```

**完整示例**：参见 [test/example.c](test/example.c)

### 构建和运行测试示例

```bash
# 使用 Makefile
make clean
make
make example
./test/example --help

# 或者手动编译（Linux）
gcc -o test/example test/example.c -L. -lstdc -lpthread -lrt

# macOS
gcc -o test/example test/example.c -L. -lstdc -lpthread -framework CoreFoundation

# 运行
./test/example arg1 arg2
```

## API 速查表

完整 API 文档请查看 [API.md](API.md)，这里列出常用功能快速参考。

### 平台检测
```c
#if P_WIN / P_DARWIN / P_LINUX / P_BSD / P_UNIX / P_POSIX
```

### 错误码
```c
E_NONE (0)           // 成功
E_INVALID (-2)       // 无效参数
E_OUT_OF_MEMORY (-5) // 内存不足
E_NONE_EXISTS (-8)   // 不存在
E_NO_PERMISSION (-12)// 无权限
```

### 日志系统
```c
log_output((log_cb)-1, false);  // stdout
log_output((log_cb)-2, false);  // 系统日志 (syslog/os_log)
printf("I: 信息 %d\n", value);  // 自动识别级别 (V/D/I/W/E/F)
```

### 命令行参数（全局作用域定义）
```c
ARGS_B(false, verbose, 'v', "verbose", "详细输出");
ARGS_I(true, port, 'p', "port", "端口");
ARGS_S(false, host, 'h', "host", "主机名");
ARGS_parse(argc, argv, &ARGS_DEF_verbose, &ARGS_DEF_port, NULL);
```

### 时间与时钟
```c
P_clock clock;
P_time_now(&clock);              // 墙钟时间 (UTC)
P_clock_now(&clock);             // 单调时钟 (测量时间间隔)
P_cost_now(&clock, true);        // CPU 时间
P_usleep(1000);                  // 睡眠 1000 微秒
```

### 文件系统
```c
P_access(path, true, false);     // 检查可读
P_size(path);                    // 文件大小
P_exe_file(buf, size);           // 可执行文件路径
P_work_dir(buf, size);           // 当前工作目录
P_is_dir(path, false);           // 是否为目录
P_mkdir(path);                   // 创建目录
P_remo(path);                    // 删除文件/空目录
```

### 目录遍历
```c
dir_t dir;
P_open_dir(&dir, ".");
while (P_dir_next(&dir)) {
    P_dir_name(&dir);            // 文件名
    P_dir_fullname(&dir);        // 完整路径
    P_dir_size(&dir);            // 大小
    P_dir_is_dir(&dir);          // 是否为目录
}
P_close_dir(&dir);
```

### 原子操作
```c
aint32_t counter = 0;
P_inc(&counter, 1);              // 原子增加（返回新值）
P_get_and_inc(&counter, 1);      // 原子增加（返回旧值）
P_set(&counter, 10);             // 原子存储
P_get(&counter);                 // 原子加载
P_and/or/xor(&counter, mask);    // 原子位运算

// 比较并交换
int expected = 5;
P_test_and_set(&counter, &expected, 10);
```

### 线程与同步
```c
// 互斥锁
P_mutex_t mutex;
P_mutex_init(&mutex);
P_mutex_lock(&mutex);
P_mutex_unlock(&mutex);
P_mutex_destroy(&mutex);

// 条件变量
P_cond_t cond;
P_cond_init(&cond);
P_cond_wait(&cond, &mutex);
P_cond_signal(&cond);           // 唤醒一个
P_cond_broadcast(&cond);        // 唤醒全部
P_cond_destroy(&cond);

// 线程
thd_t thread;
P_thread(&thread, worker_fn, param);
P_join(thread, &exit_code);
P_thd_id();                     // 当前线程 ID
```

### 网络编程
```c
P_net_init();                   // 初始化（Windows WSAStartup）

sock_t sock = socket(...);
P_set_nonblock(sock, true);     // 非阻塞模式
P_set_reuseaddr(sock, true);    // SO_REUSEADDR
P_set_nodelay(sock, true);      // TCP_NODELAY
P_set_keepalive(sock, true);    // SO_KEEPALIVE
P_set_sndtimeo(sock, 1000);     // 发送超时（毫秒）
P_set_rcvtimeo(sock, 1000);     // 接收超时（毫秒）

P_inet_pton(AF_INET, "127.0.0.1", &addr);
P_inet_ntop(AF_INET, &addr, buf, sizeof(buf));

P_net_errno();                  // 获取网络错误码
P_net_errno_is_inprogress();    // 是否为 EINPROGRESS
P_net_close(sock);
P_net_cleanup();
```

### 终端操作
```c
P_isatty(stdout);               // 是否为终端
P_term_rows();                  // 终端行数
P_term_cols();                  // 终端列数
printf(C_RED "错误" C_RESET);   // ANSI 颜色
```

### 内存与工具
```c
void* ptr = P_alloc(size);      // malloc
P_realloc(ptr, new_size);
P_free(ptr);
is_little_endian();             // 字节序检测
P_check(expr, action);          // 运行时断言
```

## 主要功能模块

### 命令行参数解析
- `ARGS_parse()` - 解析命令行参数
- 支持短选项、长选项、必选/可选参数
- 自动生成帮助信息

### 日志系统
- `log_output()` - 设置日志输出目标
- `printf()` - 格式化日志（支持级别：V/D/I/W/E/F）
- 自动使用可执行文件名作为日志标识
- 平台适配：os_log(macOS)，syslog(Linux)，OutputDebugString(Windows)

### 文件系统
- `P_file_exists()` / `P_is_dir()` / `P_is_file()`
- `P_open_dir()` / `P_dir_next()` / `P_close_dir()` - 目录遍历
- `P_mkdir()` / `P_rmdir()` / `P_remove()`
- `P_read_link()` - 读取符号链接

### 原子操作
- `P_inc()` / `P_dec()` - 原子增减
- `P_add()` / `P_sub()` - 原子加减
- `P_and()` / `P_or()` / `P_xor()` - 原子位运算
- 使用 C11 stdatomic.h（优先），fallback 到平台特定实现

### 多线程
- `P_mutex_t` / `P_mutex_init()` / `P_mutex_lock()` / `P_mutex_unlock()`
- `P_cond_t` / `P_cond_wait()` / `P_wait_timeout()`
- `P_thread()` - 跨平台线程创建
- `P_thd_id()` - 获取线程 ID

### 网络
- `P_net_init()` / `P_net_cleanup()`
- `P_set_nonblock()` - 设置非阻塞
- `P_set_reuseaddr()` / `P_set_nodelay()` - Socket 选项
- `P_inet_pton()` / `P_inet_ntop()` - 地址转换

### 终端
- `P_isatty()` - 检测是否为终端
- `P_term_rows()` / `P_term_cols()` - 获取终端尺寸
- ANSI 颜色支持（17 种颜色变体）

## 构建要求

- **C 编译器**: GCC 4.8+, Clang 3.5+, MSVC 2015+
- **CMake**: 3.10+ (如果使用 CMake 构建)
- **Make**: GNU Make 或兼容版本 (如果使用 Makefile 构建)

## 许可证

MIT License - 详见 [LICENSE](LICENSE) 文件。

## 文档

- **完整 API 参考**: 查看 [API.md](API.md) 获取所有函数、宏和类型的详细文档

## 贡献

欢迎提交 issue 和 pull request。
