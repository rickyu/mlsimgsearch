DIRS = utils mysqlconnpool sphinxconnpool redisconnpool blitz

release: WHAT=
release: $(DIRS)

debug: WHAT=debug
debug: $(DIRS)

clean: WHAT=clean
clean: $(DIRS)

.PHONY:$(DIRS)
$(DIRS):
	make $(WHAT) -C $@
