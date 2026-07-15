# Makefile for Lava Runtime Library
# 编译生成 ../liblava-rt.a 静态库

# 检测操作系统
ifeq ($(OS),Windows_NT)
    IS_WINDOWS = 1
else
    UNAME_S := $(shell uname -s 2>/dev/null)
    ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
        IS_WINDOWS = 1
    else ifeq ($(findstring MSYS,$(UNAME_S)),MSYS)
        IS_WINDOWS = 1
    else
        IS_WINDOWS = 0
    endif
endif

# 编译器设置
CC ?= gcc
ifeq ($(notdir $(CC)),cc)
    RESOLVED_CC := $(shell command -v $(CC) 2>/dev/null)
    ifneq ($(RESOLVED_CC),)
        CC := $(patsubst %/cc,%/gcc,$(RESOLVED_CC))
    else
        CC := gcc
    endif
endif
AR ?= ar

ifeq ($(IS_WINDOWS),1)
    #CFLAGS ?= -Wall -O2 -D_WIN32
    CFLAGS ?= -Wall -g -D_WIN32
else
    #CFLAGS ?= -Wall -O2 -fPIC -D_LINUX
    CFLAGS ?= -Wall -g -fPIC -D_LINUX
endif

# 目标库（输出到上级目录）
LIB = ../liblava-rt.a

# 源文件
SRCS = lava_rt.c \
       font.c \
       pinyin.c \
       py2gb.c \
       graph/vdc.c \
       utf8gb.c \
       myctype.c

# 对象文件
OBJS = $(SRCS:.c=.o)

# 默认目标
all: $(LIB)

# 创建静态库
$(LIB): $(OBJS)
	$(AR) rcs $@ $^

# 编译规则
%.o: %.c
	$(CC) $(CFLAGS) -I. -c $< -o $@

# 清理
clean:
	rm -f $(OBJS) $(LIB) test/test_runtime$(EXESUF)

# 安装（可选）
install: $(LIB)
ifeq ($(IS_WINDOWS),1)
	@echo "Windows: copy $(LIB) to your lib directory"
else
	install -d $(DESTDIR)/usr/local/lib
	install -m 644 $(LIB) $(DESTDIR)/usr/local/lib/
	install -d $(DESTDIR)/usr/local/include/lava
	install -m 644 lava_rt.h $(DESTDIR)/usr/local/include/lava/
	install -m 644 graph/vdc.h $(DESTDIR)/usr/local/include/lava/
endif

# ============== 测试 ==============

ifeq ($(IS_WINDOWS),1)
    EXESUF = .exe
else
    EXESUF =
endif

# 测试程序
TEST_BIN = test/test_runtime$(EXESUF)

# 编译并运行测试
test: $(LIB) $(TEST_BIN)
	@echo ""
	@echo "=== Running Runtime Test ==="
	@cd test && ./test_runtime$(EXESUF)

# 编译测试程序
$(TEST_BIN): test/test_runtime.c
	$(CC) $(CFLAGS) -I. $< -o $@ -L.. -llava-rt

.PHONY: all clean install test
