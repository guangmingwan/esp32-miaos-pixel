# LavaX 项目 Makefile
# 默认编译器为 lc

# 编译器和虚拟机
LC = lc
LVM = lvm2
SDL2_DLL = D:/msys64/ucrt64/bin/SDL2.dll
PACKAGE_EXE_DIR = .package_exe

# 主文件
MAIN = main.c
MAIN_OLD = 中国象棋.c
MAIN_CCHESS = 中国象棋.c

# AI 模块文件
AI_MODULES = check.c ai.c

# 默认目标
.PHONY: all clean test install old test_old cchess test_cchess package_lav package_exe help

# 默认编译目标
all: $(MAIN:.c=.lav)

# 编译规则 .c -> .lav
%.lav: %.c
	@$(LC) $<

# 运行测试
test: $(MAIN:.c=.lav)
	@$(LVM) -v $(MAIN:.c=.lav)

# 编译原始中国象棋
old: $(MAIN_OLD:.c=.lav)

# 测试原始中国象棋
test_old: $(MAIN_OLD:.c=.lav)
	@$(LVM) -v $(MAIN_OLD:.c=.lav)

# 编译 cchess-fc AI 版本
cchess: $(MAIN_CCHESS:.c=.lav)

# 测试 cchess-fc AI 版本
test_cchess: $(MAIN_CCHESS:.c=.lav)
	@echo "运行 cchess-fc AI 版本测试..."
	@$(LVM) -v $(MAIN_CCHESS:.c=.lav)

# 安装到 Lvc 存储卡
install: $(MAIN:.c=.lav)
	@./copy_to_lvc.sh

# 打包 lav 版本
package_lav: $(MAIN:.c=.lav)
	@zip -r "中国象棋(lava).zip" *.lav LavaData/ icon.bmp

# 打包 exe 版本
package_exe:
	@rm -rf "$(PACKAGE_EXE_DIR)"
	@rm -f "中国象棋(win32-x64).zip"
	@mkdir -p "$(PACKAGE_EXE_DIR)/LavaData"
	@cp "cchess.exe" "$(PACKAGE_EXE_DIR)/"
	@cp "$(SDL2_DLL)" "$(PACKAGE_EXE_DIR)/SDL2.dll"
	@cp -r LavaData/* "$(PACKAGE_EXE_DIR)/LavaData/"
	@cd "$(PACKAGE_EXE_DIR)" && zip -r "../中国象棋(win32-x64).zip" cchess.exe SDL2.dll LavaData/
	@rm -rf "$(PACKAGE_EXE_DIR)"

# 清理生成的文件
clean:
	@rm -f *.lav *.lvc *.lav~ *.bak

# 帮助信息
help:
	@echo "LavaX 中国象棋 Makefile"
	@echo "===================="
	@echo ""
	@echo "AI 模块文件：$(AI_MODULES)"
	@echo ""
	@echo "make        - 编译 $(MAIN)"
	@echo "make test   - 运行 $(MAIN)"
	@echo "make cchess - 编译 cchess-fc AI 版本"
	@echo "make test_cchess - 运行 cchess-fc AI 版本测试"
	@echo "make old    - 编译原始中国象棋"
	@echo "make test_old - 运行原始中国象棋"
	@echo "make install - 安装到 Lvc 存储卡"
	@echo "make package_lav - 打包 lav 到 中国象棋(lava).zip"
	@echo "make package_exe - 打包 exe 到 中国象棋(win32-x64).zip"
	@echo "make clean  - 清理生成的文件"
	@echo "make help   - 显示帮助信息"

# 这里依赖main_cchess.lav 其实就是 main.c 它还依赖 include 的文件
main_cchess.lav: main.c gui.c gui_difficulty_menu.c dialog.c \
                  check.c \
                   test_unit.c
	@$(LC) $<
