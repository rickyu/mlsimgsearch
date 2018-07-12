MIDDLEWARE_BASE:=../../..
IMAGE_SERVICE_BASE:=../..
IMAGE_PROCESS_BASE:=..


CXX = g++ 
CCFLAG = -g -std=c++0x -Wall
OUTPUT=.
BINFILE=common_tester
SRC_DIR=.

MIDDLEWARE_UTIL=$(MIDDLEWARE_BASE)/libsrc/utils
MIDDLEWARE_UTIL_INC=$(MIDDLEWARE_UTIL)/include
MIDDLEWARE_UTIL_LIB=$(MIDDLEWARE_UTIL)/lib

GEARMAND=$(MIDDLEWARE_BASE)/thirdlib/gearmand
GEARMAND_INC=$(GEARMAND)/include
GEARMAND_LIB=$(GEARMAND)/lib

MYSQL=$(MIDDLEWARE_BASE)/thirdlib/mysql
MYSQL_INC=$(MYSQL)/include
MYSQL_LIB=$(MYSQL)/lib

BOOST=$(MIDDLEWARE_BASE)/thirdlib/boost
BOOST_INC=$(BOOST)/include
BOOST_LIB=$(BOOST)/lib

IMAGE_SERVICE_COMMON=..
IMAGE_SERVICE_COMMON_INC=$(IMAGE_SERVICE_COMMON)/output/include
IMAGE_SERVICE_COMMON_LIB=$(IMAGE_SERVICE_COMMON)/output/lib

JSONC=$(MIDDLEWARE_BASE)/thirdlib/json-c
JSONC_INC=$(JSONC)/include
JSONC_LIB=$(JSONC)/lib

#=========================================================================
SOURCES = $(wildcard $(SRC_DIR)/*.cc)
OBJS = $(patsubst %.cc,%.o,$(SOURCES))
LIB     =  -L$(IMAGE_SERVICE_COMMON_LIB) -limgcom \
	-L$(MIDDLEWARE_UTIL_LIB) -lutils \
	-L$(JSONC_LIB) -ljson \
	-L$(GEARMAND_LIB) -lgearman \
	-L$(MYSQL_LIB) -lmysqlclient \
	-lpthread -lcurl -lrt -lxml2
INCLUDE = -I./include -I./src -I$(MIDDLEWARE_UTIL_INC) -I$(IMAGE_SERVICE_COMMON_INC) -I$(GEARMAND_INC) -I$(MYSQL_INC) -I$(BOOST_INC) -I$(JSONC_INC) -I/usr/include/libxml2 


#-----------------------------------------------------------------#
# make all
#-----------------------------------------------------------------#
all: $(BINFILE)


$(BINFILE): $(OBJS)
	$(CXX) -o $@ $^  $(CCFLAG) $(INCLUDE) $(LIB)
	if [ ! -d $(OUTPUT) ]; then mkdir $(OUTPUT); fi;


%.o	: %.cc
	$(CXX) $(CCFLAG) -c $< -o $@ $(INCLUDE)

#-----------------------------------------------------------------#
# make clean
#-----------------------------------------------------------------#
clean:
	-rm -rf $(OUTPUT)/$(BINFILE)
	-rm -rf $(OBJS)

