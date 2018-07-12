BASE:=$(CURDIR)/../../../..

HIREDIS=$(BASE)/thirdlib/hiredis
HIREDIS_INC = -I$(HIREDIS)/include
HIREDIS_LIB = -L$(HIREDIS)/lib -lhiredis

UTILS=$(BASE)/libsrc/utils
UTILS_INC=-I $(UTILS)/include
UTILS_LIB=-L$(UTILS)/lib -lutils

BLITZ=$(BASE)/libsrc/blitz
BLITZ_LIB=-L$(BLITZ)/lib -lblitz
BLITZ_INC=-I$(BLITZ)/include

THRIFT=$(BASE)/thirdlib/thrift
THRIFT_LIB=-L$(THRIFT)/lib -lthrift
THRIFT_INC=-I $(THRIFT)/include/thrift

BOOST=$(BASE)/thirdlib/boost
BOOST_INC=-I$(BOOST)/include
BOOST_LIB=-L$(BOOST)/lib -lboost_program_options

LIBEVENT=$(BASE)/thirdlib/libevent
LIBEVENT_INC=-I$(LIBEVENT)/include
LIBEVENT_LIB=-L$(LIBEVENT)/lib -levent

CPP=g++
CPPFLAG=-g -std=c++0x -Wall
BIN=thrift_server
all : $(BIN)

THRIFT_GEN_CPP=thrift/gen-cpp

INCS=-Isrc -I$(THRIFT_GEN_CPP) $(THRIFT_INC) $(LIBEVENT_INC) $(BLITZ_INC)  $(HIREDIS_INC) $(UTILS_INC) $(BOOST_INC)
LIB=$(BLITZ_LIB) $(UTILS_LIB) $(HIREDIS_LIB) $(THRIFT_LIB) $(BOOST_LIB) $(LIBEVENT_LIB) 
SOURCES = $(wildcard src/*.cc)
SRC_OBJS = $(patsubst %.cc,%.o,$(SOURCES))
OBJS=$(SRC_OBJS)  $(THRIFT_GEN_CPP)/test_constants.o $(THRIFT_GEN_CPP)/test_types.o $(THRIFT_GEN_CPP)/Test.o
	
$(BIN):$(THRIFT_GEN_CPP) $(OBJS)
	$(CPP) $(CPPFLAG) -o $(BIN)  $(OBJS) $(LIB)

$(THRIFT_GEN_CPP) :thrift/test.thrift
	cd thrift; $(THRIFT)/bin/thrift --gen cpp test.thrift
	touch $(THRIFT_GEN_CPP)


%.o:%.cc
	$(CPP) -c $(CPPFLAG) $(INCS) -o $@  $<
%.o:%.cpp
	$(CPP) -c $(CPPFLAG) $(INCS) -o $@  $<

.PHONY:clean
clean:
	-rm -f $(OBJS) 
	-rm -f $(BIN)
	-rm -rf $(THRIFT_GEN_CPP)

