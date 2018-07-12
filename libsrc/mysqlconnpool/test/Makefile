REL := ../../..

CCFLAG = -g -Wall -Wno-unused-variable -fPIC -Wfloat-equal -Wsign-compare -Wconversion
CC = g++

IMDB_PATH = $(REL)/libsrc/db
IMDB_INC = $(IMDB_PATH)/output/include
IMDB_LIB = $(IMDB_PATH)/output/lib

IMLOGGER_PATH = $(REL)/libsrc/logger
IMLOGGER_INC = $(IMLOGGER_PATH)/output/include
IMLOGGER_LIB = $(IMLOGGER_PATH)/output/lib

THIRD64_INC = $(REL)/../../../../third-64/gtest/include
THIRD64_LIB = $(REL)/../../../../third-64/gtest/lib

MYSQL_INC    = $(REL)/../../../../third-64/mysql/include
MYSQL_LIB    = $(REL)/../../../../third-64/mysql/lib

PUBLIC_INC = $(REL)/public
BASETYPE_INC = $(REL)/../../../../public/hi/basetype-def

OMNI_INC = $(REL)/omnibase/include
OMNI_LIB = $(REL)/omnibase/libs

ATOMIC_INC = -I$(REL)/libsrc/atomic-op/output/include
ATOMIC_LIB= -L$(REL)/libsrc/atomic-op/output/lib -latomic-op

BOOST_INC = -I$(REL)/../thirdsrc/boost_1_36/include/boost-1_36

INC = -I. -I$(IMDB_INC) -I$(IMLOGGER_INC) -I$(THIRD64_INC) -I$(MYSQL_INC) -I$(PUBLIC_INC) -I$(BASETYPE_INC) $(ATOMIC_INC) -I$(OMNI_INC) $(BOOST_INC)
LIBS = -L$(IMDB_LIB) -L$(IMLOGGER_LIB) -L$(THIRD64_LIB) -L$(MYSQL_LIB) -ldb -lmysqlclient -llogger -lgtest -lgtest_main -lpthread -lz \
	   -L$(OMNI_LIB) -lthread -limutil -lrt $(ATOMIC_LIB) 

TARGETS = unittest

all:$(TARGETS)

.PHONY:all

CASE_BASE_OBJS = unittest.o 
unittest:$(CASE_BASE_OBJS)
	$(CC) $(CCFLAG) -o $@ $^ $(LIBS)

%.o: %.cpp 
	$(CC) $(CCFLAG) $(INC)  -o $@ -c $<

clean:
	-rm -rf $(TARGETS)
	find . -name "*.o" -exec rm -rf {} \;
	find . -name "*.log" -exec rm -rf {} \;
	find . -name "core.*" -exec rm -rf {} \;

