from build.ab import simplerule
from build.cpm import cpm_addresses, diskimage
from utils.build import unix2cpm, objectify
from third_party.zmac.build import zmac
from third_party.ld80.build import ld80
from glob import glob

# Memory layout configuration -----------------------------------------------

# Configure the BIOS size here; this will then emit an addresses.lib file
# which contains the position of the BDOS and CCP.

# original bios_size was 0x0300.  Was 0x100 before adding the disk tables...
(cbase, fbase, bbase) = cpm_addresses(name="addresses", bios_size=0x03a0)

# BIOS ----------------------------------------------------------------------

# The CP/M BIOS itself.

zmac(
    name="bios",
    src="./bios.z80",
    deps=[
        "include/cpm.lib",
        "include/cpmish.lib",
        ".+addresses",
    ],
)

# The boot bits at bottom of memory
zmac(
    name="boot",
    src="./boot.z80",
    deps=[
        ".+addresses"
    ]
)


# Builds the memory image. This is a 64kB file containing the entire CP/M
# memory image, including the supervisor at the bottom.
ld80(
    name="memory_img",
    address=0,
    objs={
        0x0000: [".+boot"],
        cbase: ["third_party/zcpr1"],
        fbase: ["third_party/zsdos"],
        bbase: [".+bios"],
    },
)

simplerule(
    name="memoryfile",
    ins=[".+memory_img"],
    outs=["=memoryfile.img"],
    commands=[
        "dd if={ins[0]} of={outs[0]} ibs=65536 obs=65536 conv=sync status=none"
    ],
    label="MEMIMG"
)


# populate the floppy disk image
unix2cpm(name="readme", src="README.md")

diskimage(
    name="diskimage",
    format="retro_z80",
    bootfile=".+memoryfile",
    map={
        "-readme.txt": ".+readme",
        "asm.com": "cpmtools+asm",
        "asm80.com": "third_party/dr/asm80",
        "bbcbasic.com": "third_party/bbcbasic+bbcbasic_ADM3A",
        "camel80.com": "third_party/camelforth",
        "copy.com": "cpmtools+copy",
        "dump.com": "cpmtools+dump",
        "flash.com": "arch/nc200/tools+flash",
        "flipdisk.com": "arch/nc200/tools+flipdisk",
        "mkfs.com": "cpmtools+mkfs",
        "qe.com": "cpmtools+qe_NC200",
        "rawdisk.com": "cpmtools+rawdisk",
        "stat.com": "cpmtools+stat",
        "submit.com": "cpmtools+submit",
        "ted.com": "third_party/ted+ted_NC200",
        "z8e.com": "third_party/z8e+z8e_NC200",
    },
)

