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
ifeq ($(GCOV), 1)
    CXXFLAGS += -fprofile-arcs -ftest-coverage
    CFLAGS += -fprofile-arcs -ftest-coverage
    LDFLAGS += -fprofile-arcs -ftest-coverage
endif

# 文件相关定义.
# 测试代码源文件.
TEST_SOURCES ?=
# 生成的测试程序.
TEST_PROGRAMS ?=
# 冒烟测试需要使用的测试程序，应该是TEST_PROGRAMS的一个子集.
SMOKE_PROGRAMS ?=
# 测试需要的所有产出, 比如还需要.a，应该包含TEST_PROGRAMS.
TEST_OUTPUT ?=



CPPFLAGS += $(PROJ_INCLUDE)
CPPFLAGS += -I./components/include
LDFLAGS += -L./components/lib



include Makefile.proj

# 自动生成依赖, 放到.deps目录下.
deps_dir=.deps
deps_cc=$(patsubst %.cc, $(deps_dir)/%.d, $(filter %.cc,$(SOURCES)))
deps_cpp=$(patsubst %.cpp, $(deps_dir)/%.d, $(filter %.cpp,$(SOURCES)))
deps_c=$(patsubst %.c, $(deps_dir)/%.d, $(filter %.c,$(SOURCES)))
$(deps_cc) : $(deps_dir)/%.d: %.cc
	@[ -d $(dir $@) ] || mkdir -p $(dir $@) ;  $(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $<  \
	| sed "s,\($(basename $(patsubst %.cc,%.o,$(notdir $<)))\)\.o[ :]*,$(dir $<)\1.o $@ : ,g" > $@ ; \
	[ -s $@ ] || rm -f $@
$(deps_cpp) : $(deps_dir)/%.d: %.cpp
	[ -d $(dir $@) ] || mkdir -p $(dir $@) ;  $(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $<  \
 	| sed "s,\($(basename $(patsubst %.cpp,%.o,$(notdir $<)))\)\.o[ :]*,$(dir $<)\1.o $@ : ,g" > $@ ; \
 	[ -s $@ ] || rm -f $@
$(deps_c) : $(deps_dir)/%.d: %.c
	[ -d $(dir $@) ] || mkdir -p $(dir $@) ;  $(CC) -MM $(CPPFLAGS) $(CXXFLAGS) $<  \
 	| sed "s,\($(basename $(patsubst %.c,%.o,$(notdir $<)))\)\.o[ :]*,$(dir $<)\1.o $@ : ,g" > $@ ; \
 	[ -s $@ ] || rm -f $@

include $(deps_cc)
include $(deps_cpp)
include $(deps_c)



test_report = test_report
smoke_test:all
	@source scan_app.sh $(BASE)/thirdlib; 
	@[ -d $(test_report) ] || mkdir $(test_report)
	@for prog in $(SMOKE_PROGRAMS); do   \
		./$$prog --gtest_output=xml:$(test_report)/$$prog.gtest.xml; \
		done; 
all_test:all
	@source scan_app.sh $(BASE)/thirdlib; 
	@[ -d $(test_report) ] || mkdir $(test_report)
	@for prog in $(TEST_PROGRAMS); do   \
		./$$prog --gtest_output=xml:$(test_report)/$$prog.gtest.xml; \
		done; 

#-----------------------------------------------------------------#
# make clean
#-----------------------------------------------------------------#
all_objs = $(patsubst %.cc, %.o, $(SOURCES))
.PHONY: clean
clean:
	-rm -rf $(OUTPUT_PATH)
	-rm -rf $(all_objs) $(OUTPUT)
	find . -name \*.gcno -o -name \*.gcda | xargs -I {}  rm -f {}
	-rm -rf $(test_report)


