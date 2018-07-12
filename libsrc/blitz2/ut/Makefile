BASE=../../..
GTEST_DIR = $(BASE)/thirdsrc/gtest-1.6.0

# Flags passed to the preprocessor.
CPPFLAGS += -I$(GTEST_DIR)/include  -I/usr/local/include/thrift -I../components/include -I../include
LDFLAGS+=-L../components/lib -L/usr/local/lib 
LIBS= -lutils -lhiredis -lthrift -levent -lboost_program_options -lboost_system

# Flags passed to the C++ compiler.
ifeq ($(GCOV), 1)
    CXXFLAGS += -g -Wall -Wextra -std=c++0x -fprofile-arcs -ftest-coverage
else
    CXXFLAGS += -g -Wall -Wextra -std=c++0x 
endif

# All tests produced by this Makefile.  Remember to add new tests you
# created to the list.
TESTS = buffer_ut line_msg_ut state_machine_ut framework_ut ref_object_ut array_list_ut memcached_req_ut memcached_resp_ut

# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

# House-keeping build targets.

all : $(TESTS)

check : all
	source scan_app.sh $(BASE)/thirdlib; \
	for prog in $(TESTS); do   \
		./$$prog --gtest_output=xml:$$prog.gtest.xml; \
		done; 
	


# Builds gtest.a and gtest_main.a.

# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc

gtest.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

gtest_main.a : gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

# Builds a sample test.  A test should link with either gtest.a or
# gtest_main.a, depending on whether it defines its own main()
# function.

framework.o:../src/framework.cc ../include/blitz/framework.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

framework_thread.o:../src/framework_thread.cc ../include/blitz/framework_thread.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
tcp_connect.o:../src/tcp_connect.cc ../include/blitz/tcp_connect.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
tcp_server.o:../src/tcp_server.cc ../include/blitz/tcp_server.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
network_util.o:../src/network_util.cc ../include/blitz/network_util.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
msg.o:../src/msg.cc ../include/blitz/msg.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
fd_manager.o:../src/fd_manager.cc ../include/blitz/fd_manager.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

buffer_ut.o : buffer_ut.cc ../include/blitz/buffer.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
buffer.o:../src/buffer.cc ../include/blitz/buffer.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
buffer_ut : buffer.o buffer_ut.o gtest_main.a 
	$(CXX) $(LDFLAGS) $(CXXFLAGS)  $^ -o $@ $(LIBS)

line_msg_ut:line_msg_ut.o line_msg.o gtest_main.a
	$(CXX) $(LDFLAGS) $(CXXFLAGS)  $^ -o $@ $(LIBS)
line_msg_ut.o : line_msg_ut.cc ../include/blitz/line_msg.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
line_msg.o : ../src/line_msg.cc ../include/blitz/line_msg.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
	

memcached_req_ut.o : memcached_req_ut.cc ../include/blitz2/memcached/memcached_req.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
memcached_req.o : ../src/memcached_req.cc ../include/blitz2/memcached/memcached_req.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
memcached_req_ut:memcached_req.o memcached_req_ut.o gtest_main.a
	$(CXX) $(LDFLAGS) $(CXXFLAGS)  $^ -o $@ $(LIBS)

memcached_resp_ut.o : memcached_resp_ut.cc ../include/blitz2/memcached/memcached_resp.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
memcached_resp_ut:memcached_resp_ut.o gtest_main.a
	$(CXX) $(LDFLAGS) $(CXXFLAGS)  $^ -o $@ $(LIBS)

state_machine_ut:state_machine_ut.o gtest_main.a
	$(CXX) $(LDFLAGS) $(CXXFLAGS) $^ -o $@ $(LIBS)
state_machine_ut.o : state_machine_ut.cc ../include/blitz/state_machine.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

framework_ut:framework_ut.o framework.o  framework_thread.o tcp_connect.o tcp_server.o \
	network_util.o  msg.o fd_manager.o gtest_main.a
	$(CXX) $(LDFLAGS) $(CXXFLAGS)  $^ -o $@ $(LIBS)
framework_ut.o : framework_ut.cc ../include/blitz/framework.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
ref_object_ut.o : ref_object_ut.cc ../include/blitz/ref_object.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
ref_object_ut : ref_object_ut.o msg.o gtest_main.a 
	$(CXX) $(LDFLAGS) $(CXXFLAGS)  $^ -o $@ $(LIBS)

array_list_ut.o : array_list_ut.cc ../include/blitz/array_list.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
array_list_ut : array_list_ut.o gtest_main.a 
	$(CXX) $(LDFLAGS) $(CXXFLAGS)  $^ -o $@  $(LIBS)

service_selector.o : ../src/service_selector.cc ../include/blitz/service_selector.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
service_selector_ut.o : service_selector_ut.cc ../include/blitz/service_selector.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
service_selector_ut : service_selector_ut.o service_selector.o gtest_main.a 
	$(CXX) $(LDFLAGS) $(CXXFLAGS)  $^ -o $@  $(LIBS)

tokenless_service.o : ../src/tokenless_service.cc ../include/blitz/tokenless_service.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
tokenless_service_pool.o : ../src/tokenless_service_pool.cc ../include/blitz/tokenless_service_pool.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
tokenless_service_ut.o : tokenless_service_ut.cc ../include/blitz/tokenless_service_pool.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
tokenless_service_ut : tokenless_service_ut.o \
	tokenless_service_pool.o \
	line_msg.o \
	framework_thread.o \
	tcp_server.o \
	tcp_connect.o \
	network_util.o \
	fd_manager.o \
	msg.o \
	service_selector.o \
	tokenless_service.o \
	framework.o gtest_main.a 
	$(CXX) $(LDFLAGS) $(CXXFLAGS)  $^ -o $@ $(LIBS)

clean :
	rm -f $(TESTS) gtest.a gtest_main.a *.o  
