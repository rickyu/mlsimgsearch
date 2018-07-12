.PHONY:all
all:
	make -C test_thrift_sync_server
	pybot -d output .
clean:
	make -C test_thrift_sync_server clean
	find . -name "*.pyc" -delete
	rm -rf output

