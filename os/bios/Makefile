all: bios.rel 

bios.rel: bios.z80
	../../bin/zmac --nmnv --zmac -m  --dri -z --rel7 bios.z80  -o bios.rel -l > bios.lst


# use --rel7 and .rel for relative files.
clean:
	rm -f bios.rel bios.lst
