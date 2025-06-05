.PHONY: host_tools

host_tools:
	make -C tools/zmac install
	make -C tools/ld80 install

