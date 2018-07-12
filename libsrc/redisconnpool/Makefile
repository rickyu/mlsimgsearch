
BASE:=../..
COMPONENT ?= ./components
CPPFLAGS += -I$(COMPONENT)/include
LDFLAGS += -L$(COMPONENT)/lib

LIBNAME = redisconnpool

SRC_DIR = src
LIB_DIR = lib
OUTPUT_PATH=./output

CXXFLAGS = -g -Wall -Wshadow -Wno-unused-variable -fPIC -Wfloat-equal -Wsign-compare -Wconversion


INCLUDE = -I./include  -I/usr/local/include/hiredis -I../utils/include

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst %.cpp,%.o,$(SOURCES))

CPPFLAGS += $(INCLUDE)

CXX = g++ 
AR  = ar
#=========================================================================
OUTPUT_LIB=lib$(LIBNAME).a


#-----------------------------------------------------------------#
# make all
#-----------------------------------------------------------------#
all: clean output
	if [ ! -d $(OUTPUT_PATH) ]; then mkdir $(OUTPUT_PATH); fi
	if [ ! -d $(OUTPUT_PATH)/lib ]; then mkdir $(OUTPUT_PATH)/lib; fi
	if [ ! -d $(OUTPUT_PATH)/include ]; then mkdir $(OUTPUT_PATH)/include; fi
	cd include;find . -name "*.h" -print | xargs -n 1 echo | cpio -p -d --quiet ../$(OUTPUT_PATH)/include
	cd $(LIB_DIR);find . -name "*.a" -print | xargs -n 1 echo | cpio -p -d --quiet ../$(OUTPUT_PATH)/lib 

#-----------------------------------------------------------------#
# make library
#-----------------------------------------------------------------#
output: $(OBJS)
	if [ ! -d $(LIB_DIR) ]; then mkdir $(LIB_DIR); fi
	ar -ruv $(LIB_DIR)/$(OUTPUT_LIB) $(OBJS)

.PHONY : output
$(OBJS): %.o: %.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS) $(CPPFLAGS)
	

#-----------------------------------------------------------------#
# make clean
#-----------------------------------------------------------------#
clean:
	rm -f $(OBJS) $(LIB_DIR)/*.a
	if [ -d $(OUTPUT_PATH) ]; then rm  -r $(OUTPUT_PATH); fi


