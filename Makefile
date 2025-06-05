.PHONY: host_tools

host_tools:
	make -C tools/zmac install
	make -C tools/ld80 install
	cd tools/cpmtools-2.23 && \
		./configure && \
		 make && \
		 cp *.cpm cpm{cp,ls,rm,chmod,chattr} ../../bin

