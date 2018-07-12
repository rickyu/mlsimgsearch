SVN_BASE := ../../..
UTILS=$(SVN_BASE)/libsrc/utils
UTILS_INC=-I$(UTILS)/include
UTILS_LIB=-L$(UTILS)/lib -lutils

JSON_LIB=-L/lib64 -ljson
JSON_INC=-I/usr/include

CCFLAG = -g -Wall -Wshadow -Wno-unused-variable -fPIC -Wfloat-equal -Wsign-compare -Wconversion
CC = g++


INC = -I. $(UTILS_INC) $(JSON_INC)
LIBS = $(UTILS_LIB)
LIBS += $(JSON_LIB)


TARGETS = basetest

all:$(TARGETS)

.PHONY:all

CASE_BASE_OBJS = baseTest.o
basetest:$(CASE_BASE_OBJS)
	$(CC) $(CCFLAG) -o $@ $^ $(LIBS)



%.o: %.cpp 
	$(CC) $(CCFLAG) $(INC)  -o $@ -c $<

clean:
	-rm -rf $(TARGETS)
	find . -name "*.o" -exec rm -rf {} \;

