LIBNAME = sqlparse
SRC_DIR = .
LIB_DIR = lib
OUTPUT_PATH=./output

CCFLAG_RELEASE = -O3 -Wall  -Wshadow -Wno-unused-variable -fPIC -Wfloat-equal -Wsign-compare -Wconversion -pipe -W -Wpointer-arith -Wformat -Wwrite-strings -Wswitch -Wredundant-decls
CCFLAG_DEBUG = -g -Wall  -Wshadow -Wno-unused-variable -fPIC -Wfloat-equal -Wsign-compare -Wconversion -pipe -W -Wpointer-arith -Wformat -Wwrite-strings -Wswitch -Wredundant-decls
CCFLAG=$(CCFLAG_RELEASE)
CXXCFLAG = -Wreorder -Wsign-promo
INCLUDE = -I./include/ -I.

LIB     =  -ltcmalloc
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst %.c,%.o,$(SOURCES))


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

flexbison:
	flex --header=flex.h flex.y
	bison -d bison.y
	mv *.h include/sqlparse
$(OBJS): %.o: %.c
	$(CXX) -c $< -o $@ $(INCLUDE) $(LIB) $(CCFLAG) $(CXXCFLAG)
	
deps :
	$(CXX) -MM -MG $(SOURCES) >depends

-include depends

#-----------------------------------------------------------------#
# make clean
#-----------------------------------------------------------------#
clean:
	rm -f $(OBJS) $(LIB_DIR)/*.a
	if [ -d $(OUTPUT_PATH) ]; then rm  -r $(OUTPUT_PATH); fi


