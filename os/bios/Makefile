all: bios.rel boot.rel

boot.rel: boot.z80
	../../bin/zmac --nmnv --zmac -m  --dri -z --rel7 boot.z80  -o boot.rel
bios.rel: bios.z80
	../../bin/zmac --nmnv --zmac -m  --dri -z --rel7 bios.z80  -o bios.rel


# use --rel7 and .rel for relative files.
clean:
	rm boot.rel bios.rel
