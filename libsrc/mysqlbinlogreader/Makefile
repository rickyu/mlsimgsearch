
CXX=g++
CXXFLAGS=-g -O1 -std=c++0x -fPIC
CFLAGS=-g -O1 -fPIC
LIBS=-lmysqlclient -L./ -lmysqlbinlogreader
CXXFLAGS+=-Iinclude -Isrc -I./

OBJS=src/mysql_binlog_reader.o src/log_event.o src/my_bitmap.o src/my_bit.o src/rpl_utility.o src/mysql_operation.o src/decimal.o
BIN=mysqlbinlogreader
LIB_OUTPUT=libmysqlbinlogreader.a
.PHONY: all
all: $(LIB_OUTPUT)
	[ -d output ] ||  mkdir output
	[ -d output/lib ] ||  mkdir output/lib
	[ -d output/include ] ||  mkdir output/include
	cd include;find . -name "*.h" -print | xargs -n 1 echo | cpio -p -d --quiet ../output/include
	cp $(LIB_OUTPUT) output/lib

.PHONY:lib
lib:$(LIB_OUTPUT)
$(LIB_OUTPUT):$(OBJS)
	$(AR)  $(ARFLAGS) $@ $^

TEST_OBJS=test/main.o
.PHONY:test
test:$(BIN)
$(BIN):$(TEST_OBJS) $(LIB_OUTPUT)
	$(CXX) -g  -o mysqlbinlogreader $(TEST_OBJS) $(LDFLAGS) $(LIBS)

.PHONY:clean
clean:
	-rm -f $(OBJS) 
	-rm -f $(TEST_OBJS)
	-rm -f $(BIN)
	-rm -f $(LIB_OUTPUT)
	-rm -rf output

