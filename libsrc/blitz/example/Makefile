BASE:=../../..


CXX=g++
CXXFLAG=-g -std=c++0x -static -Wall -O1

THRIFT=$(BASE)/thirdlib/thrift
THRIFT_INC=-I$(THRIFT)/include/thrift 
THRIFT_LIB=-L$(THRIFT)/lib  -lthrift

LIBEVENT=$(BASE)/thirdlib/libevent
LIBEVENT_INC=-I$(LIBEVENT)/include
LIBEVENT_LIB=-L$(LIBEVENT)/lib -levent

BOOST=$(BASE)/thirdlib/boost
BOOST_INC=-I$(BOOST)/include
BOOST_LIB=-L$(BOOST)/lib
HIREDIS=$(BASE)/thirdlib/hiredis
HIREDIS_INC=-I$(HIREDIS)/include
HIREDIS_LIB=-L$(HIREDIS)/lib -lhiredis

UTILS=$(BASE)/libsrc/utils
UTILS_INC=-I$(UTILS)/include
UTILS_LIB=-L$(UTILS)/lib -lutils

BLITZ=$(BASE)/libsrc/blitz
BLITZ_INC=-I$(BLITZ)/include
BLITZ_LIB=-L$(BLITZ)/lib -lblitz

LIBS=$(BLITZ_LIB) $(UTILS_LIB) $(LIBEVENT_LIB) $(HIREDIS_LIB)  $(THRIFT_LIB) -lpthread
INCS=$(BLITZ_INC) $(UTILS_INC) $(LIBEVENT_INC) $(HIREDIS_INC) $(THRIFT_INC) $(BOOST_INC)

EXAMPLES = echo_server hello_thrift evil_client hello_http
all : $(EXAMPLES)

ECHO_SERVER_OBJS=echo_server.o 
echo_server:$(ECHO_SERVER_OBJS)
	$(CXX) -o $@ $< $(LIBS)

THRIFT_SERVER_OBJS=thrift_server.o 
hello_thrift:$(THRIFT_SERVER_OBJS)
	$(CXX) -o $@ $< $(LIBS)
EVIL_CLIENT_OBJS=evil_client.o 
evil_client:$(EVIL_CLIENT_OBJS)
	$(CXX) -o $@ $< $(LIBS)

TEST_REDIS_POOL_OBJS=test_redis_pool.o
TEST_REDIS_POOL_LIBS=$(LIBS)
test_redis_pool:$(TEST_REDIS_POOL_OBJS)
	$(CXX) -o $@ $< $(TEST_REDIS_POOL_LIBS)


HELLO_HTTP_OBJS=hello_http.o
HELLO_HTTP_LIBS=$(LIBS)
hello_http:$(HELLO_HTTP_OBJS)
	$(CXX) -o $@ $< $(HELLO_HTTP_LIBS)




CLIENT_OBJS=client.o 
client:$(CLIENT_OBJS)
	$(CXX) -o $@ $(CLIENT_OBJS) $(LIBS)

TEST_POOL_OBJS=test_pool.o 
test_pool:$(TEST_POOL_OBJS)
	$(CXX) -o $@ $(TEST_POOL_OBJS) $(LIBS)
TEST_MYSQL_OBJS=test_mysql.o 
test_mysql:$(TEST_MYSQL_OBJS)
	$(CXX) -o $@ $(TEST_MYSQL_OBJS) $(LIBS)

TEST_POOL_MEMCACHED_OBJS=test_pool_memcached.o 
test_pool_memcached:$(TEST_POOL_MEMCACHED_OBJS)
	$(CXX) -o $@ $(TEST_POOL_MEMCACHED_OBJS) $(LIBS)

clean:
	-rm -rf $(EXAMPLES) *.o
%.o: %.cc
	$(CXX) $(CXXFLAG) -c $< -o $@ $(INCS) 


# test_mysql.o:test_mysql.cc 
# 	$(CXX) $(CXXFLAG) $(INCS) -c -o $@ $<
# test_pool.o:test_pool.cc 
# 	$(CXX) $(CXXFLAG) $(INCS) -c -o $@ $<
# server.o:server.cc 
# 	$(CXX) $(CXXFLAG) $(INCS) -c -o $@ $<
# client.o:client.cc 
# 	$(CXX) $(CXXFLAG) $(INCS) -c -o $@ $<
# test_pool_memcached.o:test_pool_memcached.cc 
# 	$(CXX) $(CXXFLAG) $(INCS) -c -o $@ $<
