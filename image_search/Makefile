MODULES += bof_core \
	   vocabulary_creator \
	   indexer \
	   searchd \
	   parallel_compute

.PHONY: all clean $(MODULES)

all: $(MODULES)

$(MODULES):
	@$(MAKE) -C $@;

clean:
	@for dir in $(MODULES); do\
		$(MAKE) -C $$dir clean; \
	done

install_depends:
	@for dir in $(MODULES); do\
		echo ==================== begin install $$dir depends; \
		cd $$dir; \
		octopus install_depends; \
		cd -; \
	done

vocabulary_creator: bof_core

indexer: bof_core

searchd: bof_core

parallel_compute: bof_core
