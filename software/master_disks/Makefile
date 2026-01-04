zsdos.img: .always_build
	./make_floppy.sh zsdos.img
	cpmcp -f rz80_floppy zsdos.img ../original/ZSDOS2/* 0:

kaymisc.img: .always_build
	./make_floppy.sh kaymisc.img
	cpmcp -f rz80_floppy kaymisc.img ../original/KAYMISC/* 0:

applications: .always_build
	for n in 
