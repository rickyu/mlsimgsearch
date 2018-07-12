#
# Makefile.proj定义如下：
# SOURCES:变量,所有源码
# HEADERS:变量，所有头文件.
# OUTPUT_EXES: 变量，生成的所有可执行文件, 比如EXES=a1 a2
# OUTPUT_LIBS: 变量, 所有的lib库.
# OUTPUT_FILES: 变量，要拷贝到output下的文件或者目录.
# INCLUDES: 变量，包含的所有头文件目录.
# UT_PROGRAMS: 变量，所有的unit test程序.
# SMOKE_PROGRAMS: 变量,所有的smoke测试程序.
# TEST_PROGRAMS: 变量,所有的test程序.
# TMPFILES: 变量，其它所有生成的临时文件.
#
# Makefile
# target: all, lint, compile, ut, smoke, test, clean, tags
# * smoke : all : 执行在SMOKE_PROGRAMS中的程序.
# * test : 执行所有在TEST_PROGRAMS中的程序.
# * lint : 对所有代码（TEST_SOURCES)进行lint检查.
# * tags : 对所有代码和头文件目录建立tags
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


CPPFLAGS += $(INCLUDES)
.PHONY: all
all: compile ut smoke test tags
	[ -d output/bin ] || mkdir -p output/bin
	@for exe in $(OUTPUT_EXES); do \
		cp $$exe output/bin/;\
		done;
	[ -d output/lib ] || mkdir -p output/lib
	@for lib in $(OUTPUT_LIBS); do \
		cp $$lib output/lib/;\
		done;
	@for d in $(OUTPUT_FILES); do \
	        if [ -e $$d ] ;then find $$d  -print | grep -v ".svn" | xargs -n 1 echo | cpio -p -d --quiet output/; fi;\
		done

include Makefile.proj
#-----------------------------------------------------------------#
# make clean
#-----------------------------------------------------------------#
objects = $(foreach file, $(SOURCES), $(shell echo $(file) | sed "s/\.c[pc]*[p]*/.o/"))
clean:
	-rm -rf .deps
	-rm -f $(objects) $(TMPFILES) $(OUTPUT_EXES) $(OUTPUT_LIBS) 
	-rm -rf $(TMPDIRS)
	-rm -f $(UT_PROGRAMS)
	-rm -f $(TEST_PROGRAMS)
	-rm -f $(SMOKE_PROGRAMS)
	find . -name \*.gcno -o -name \*.gcda | xargs -I {}  rm -f {}
	-rm -rf $(test_report)
	-rm -rf output

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

-include $(deps_cc)
-include $(deps_cpp)
-include $(deps_c)




.PHONY:compile
compile:$(OUTPUT_EXES) $(OUTPUT_LIBS)
test_report = test_report
.PHONY:ut
ut:$(UT_PROGRAMS)
	@[ -d $(test_report) ] || mkdir $(test_report)
	@for prog in $(UT_PROGRAMS); do   \
		./$$prog --gtest_output=xml:$(test_report)/$$prog.gtest.xml; \
		done; 

.PHONY:smoke
smoke:$(SMOKE_PROGRAMS)

.PHONY:test
test:$(TEST_PROGRAMS)

tags:$(HEADERS) $(SOURCES)
	ctags $(HEADERS) $(SOURCES)
.PHONY : lint
lint:
	cpplint.py $(SOURCES) $(HEADERS)


