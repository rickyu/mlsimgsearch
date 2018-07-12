#
# Makefile
# 1. 通用定义
# 2. 包含模块公共定义文件
# * 定义主要产出:lib或者可执行程序
# 4. 包含测试Makefile
#    3.1. 测试Makefile需要输出: TEST_SOURCES TEST_OBJS TEST_PROGRAMS TEST_OUTPUT
#         SMOKE_PROGRAMS
#      2. 定义所有的测试target生成rule.
# 5. smoke_test : all : 执行在SMOKE_PROGRAMS中的程序.
# 6. slow_test : 执行所有在TEST_PROGRAMS中的程序.
# 7. lint : 对所有代码（TEST_SOURCES)进行lint检查.
# 8. TAGS : 对所有代码和头文件目录建立tags
#
SHELL=/bin/sh
.SUFFIXES:
.SUFFIXES: .cc .cpp .o

CXX = g++ 
CC=gcc
MAKE=make
AR  = ar
ARFLAGS = ruv
CPPFLAGS ?=
LIBS ?=
CXXFLAGS += -g -std=c++0x -Wall -Wextra
CFLAGS += -g -Wall -Wextra


CPPFLAGS += $(PROJ_INCLUDE)
CPPFLAGS += -I./include
LDFLAGS += -L./lib



include Makefile.proj

# 自动生成依赖, 放到.deps目录下.
deps_dir=.deps
deps_cc=$(patsubst %.cc, $(deps_dir)/%.d, $(filter %.cc,$(SOURCES)))
$(deps_cc) : $(deps_dir)/%.d: %.cc
	@[ -d $(dir $@) ] || mkdir -p $(dir $@) ;  $(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $<  \
	| sed "s,\($(basename $(patsubst %.cc,%.o,$(notdir $<)))\)\.o[ :]*,$(dir $<)\1.o $@ : ,g" > $@ ; \
	[ -s $@ ] || rm -f $@
#-----------------------------------------------------------------#
# make clean
#-----------------------------------------------------------------#
all_objs = $(patsubst %.cc, %.o, $(SOURCES))
clean:
	-rm -rf $(OUTPUT_PATH)
	-rm -rf $(all_objs) $(OUTPUT)
	find . -name \*.o | xargs -I {}  rm -f {}


