LIBNAME = utils
SRC_DIR = src
LIB_DIR = lib
OUTPUT_PATH=./output

CCFLAG = -Wall  -Wshadow -Wno-unused-variable -fPIC -Wfloat-equal -Wsign-compare -Wconversion -pipe -W -Wpointer-arith -Wformat -Wwrite-strings -Wswitch -Wredundant-decls
DFLAG = -O3
CXXCFLAG = -Wreorder -Wsign-promo
INCLUDE = -I./include/utils -I./src -I/usr/include/libxml2

LIB     =  

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst %.cpp,%.o,$(SOURCES))


CXX = g++44
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
	$(CXX) $(DFLAG) -c $< -o $@ $(INCLUDE) $(LIB) $(CCFLAG) $(CXXCFLAG)

.PHONY: debug
debug: DFLAG = -g -D __DEBUG
debug: all
	@echo "Debug Version."
	
deps :
	$(CXX) -MM -MG $(SOURCES) >depends

-include depends

#-----------------------------------------------------------------#
# make clean
#-----------------------------------------------------------------#
clean:
	rm -f $(OBJS) $(LIB_DIR)/*.a
	if [ -d $(OUTPUT_PATH) ]; then rm  -r $(OUTPUT_PATH); fi

.PHONY : clean
