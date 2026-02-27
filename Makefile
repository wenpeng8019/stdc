# stdc 静态库 Makefile
# 支持: Linux, macOS, BSD, Windows (MSYS2/MinGW)

# 库名称
LIB_NAME = stdc
TARGET = lib$(LIB_NAME).a

# 目录设置
BUILD_DIR = build
TEST_DIR = test

# 源文件和对象文件
SRCS = stdc.c
OBJS = $(addprefix $(BUILD_DIR)/, $(SRCS:.c=.o))

# 编译器设置
CC = gcc
AR = ar
ARFLAGS = rcs
CFLAGS = -Wall -Wextra -O2 -fPIC

# 平台检测
UNAME_S := $(shell uname -s)

# macOS 特定设置
ifeq ($(UNAME_S),Darwin)
    CFLAGS += -D__DARWIN__
    # macOS 可能需要特定的框架
    LDFLAGS += -framework CoreFoundation
endif

# Linux 特定设置
ifeq ($(UNAME_S),Linux)
    CFLAGS += -D_GNU_SOURCE
    LDFLAGS += -lpthread
endif

# FreeBSD 特定设置
ifeq ($(UNAME_S),FreeBSD)
    CFLAGS += -D__FreeBSD__
    LDFLAGS += -lpthread
endif

# Windows (MSYS2/MinGW) 检测
ifneq (,$(findstring MINGW,$(UNAME_S)))
    TARGET = $(LIB_NAME).lib
    CFLAGS += -D_WIN32
    LDFLAGS += -lws2_32
endif

# 安装路径
PREFIX ?= /usr/local
INSTALL_LIB_DIR = $(PREFIX)/lib
INSTALL_INC_DIR = $(PREFIX)/include

# 默认目标
.PHONY: all
all: $(TARGET)

# 构建静态库
$(TARGET): $(OBJS)
	@echo "Creating static library: $@"
	$(AR) $(ARFLAGS) $@ $^
	@echo "Build complete: $@"

# 编译对象文件
$(BUILD_DIR)/%.o: %.c stdc.h | $(BUILD_DIR)
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# 创建构建目录
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# 构建测试示例
.PHONY: example
example: $(TARGET) | $(TEST_DIR)
	@echo "Building test example..."
	$(CC) $(CFLAGS) -I. -o $(TEST_DIR)/example $(TEST_DIR)/example.c -L. -l$(LIB_NAME) $(LDFLAGS)
	@echo "Test example built: $(TEST_DIR)/example"

# 创建测试目录
$(TEST_DIR):
	@mkdir -p $(TEST_DIR)

# 清理
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR) $(TARGET) lib$(LIB_NAME).a $(LIB_NAME).lib
	rm -f $(TEST_DIR)/example
	@echo "Clean complete"

# 安装
.PHONY: install
install: $(TARGET)
	@echo "Installing library to $(INSTALL_LIB_DIR)"
	@mkdir -p $(INSTALL_LIB_DIR)
	@mkdir -p $(INSTALL_INC_DIR)
	install -m 644 $(TARGET) $(INSTALL_LIB_DIR)/
	install -m 644 stdc.h $(INSTALL_INC_DIR)/
	@echo "Installation complete"

# 卸载
.PHONY: uninstall
uninstall:
	@echo "Uninstalling library from $(INSTALL_LIB_DIR)"
	rm -f $(INSTALL_LIB_DIR)/$(TARGET)
	rm -f $(INSTALL_INC_DIR)/stdc.h
	@echo "Uninstall complete"

# 帮助信息
.PHONY: help
help:
	@echo "stdc Static Library Build System"
	@echo ""
	@echo "Usage:"
	@echo "  make              - Build the static library"
	@echo "  make example      - Build test example"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make install      - Install library and headers"
	@echo "  make uninstall    - Uninstall library and headers"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  PREFIX=$(PREFIX)  - Installation prefix"
	@echo "  CC=$(CC)          - C compiler"
	@echo ""
	@echo "Platform: $(UNAME_S)"
