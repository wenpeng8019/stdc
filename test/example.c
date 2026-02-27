/**
 * stdc 库使用示例
 * 编译: gcc -o example example.c -L. -lstdc -lpthread -framework CoreFoundation
 */

#include "stdc.h"
#include <stdio.h>

// 命令行参数定义必须在全局作用域
ARGS_B(false, verbose, 'v', "verbose", "Enable verbose output");
ARGS_S(false, name, 'n', "name", "Your name");

int main(int argc, char** argv) {
    printf("=== stdc Library Test ===\n\n");
    
    // 1. 命令行参数解析测试
    printf("1. Command Line Arguments:\n");
    
    int pos_count = ARGS_parse(argc, argv, &ARGS_DEF_verbose, &ARGS_DEF_name, NULL);
    
    if (ARGS_verbose.i64) {
        printf("   Verbose mode: ON\n");
    }
    if (ARGS_name.str) {
        printf("   Name: %s\n", ARGS_name.str);
    }
    printf("   Positional args: %d\n", pos_count);
    
    // 2. 日志系统测试
    printf("\n2. Logging System:\n");
    log_output((log_cb)-2, false);  // 输出到系统日志
    printf("I: This is an info message\n");
    printf("W: This is a warning\n");
    printf("E: This is an error\n");
    log_output((log_cb)-1, false);  // 恢复到 stdout
    printf("   Logging test complete\n");
    
    // 3. 文件系统测试
    printf("\n3. Filesystem Operations:\n");
    if (P_access("stdc.h", true, false)) {
        printf("   ✓ stdc.h is accessible\n");
    }
    
    if (P_is_dir(".", false) == E_NONE) {
        printf("   ✓ Current directory is valid\n");
    }
    
    // 4. 目录遍历测试
    printf("\n4. Directory Listing (current dir):\n");
    dir_t dir;
    if (P_open_dir(&dir, ".") == E_NONE) {
        int count = 0;
        while (P_dir_next(&dir) == E_NONE && count < 5) {
            printf("   - %s%s\n", P_dir_name(&dir), P_dir_is_dir(&dir) ? "/" : "");
            count++;
        }
        if (count == 5) printf("   ... (more files)\n");
        P_close_dir(&dir);
    }
    
    // 5. 原子操作测试
    printf("\n5. Atomic Operations:\n");
    _Atomic int atomic_val = 0;
    P_inc(&atomic_val, 1);
    P_inc(&atomic_val, 5);
    printf("   Atomic value: %d (expected: 6)\n", atomic_val);
    
    // 6. 网络初始化测试
    printf("\n6. Network Initialization:\n");
    ret_t ret = P_net_init();
    if (ret == E_NONE) {
        printf("   ✓ Network initialized\n");
        P_net_cleanup();
        printf("   ✓ Network cleaned up\n");
    } else {
        printf("   ✗ Network init failed: %d\n", ret);
    }
    
    // 7. 终端测试
    printf("\n7. Terminal Info:\n");
    if (P_isatty(stdout)) {
        printf("   Terminal size: %d rows x %d cols\n", P_term_rows(), P_term_cols());
        printf("   Color test: ");
        printf(P_RED("Red") " ");
        printf(P_GREEN("Green") " ");
        printf(P_BLUE("Blue") "\n");
    } else {
        printf("   Not a terminal\n");
    }
    
    printf("\n=== All Tests Complete ===\n");
    return 0;
}
