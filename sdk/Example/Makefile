# Build the example plugin.  Pick ONE toolchain:
#   make vbcc      (vbcc +aos68k, NDK 3.2 configured in vbcc)
#   make gcc       (m68k-amigaos-gcc / bebbo amiga-gcc)
# Then package it:  make package
NAME = HelloWorld
SDK = ..
SRC = $(NAME)/$(NAME).c $(SDK)/src/beam68plugin.c
OUT = $(NAME)/$(NAME)

vbcc:
	vc +aos68k -O2 -I$(SDK)/include -o $(OUT) $(SRC)

gcc:
	m68k-amigaos-gcc -O2 -m68000 -noixemul -Wall -I$(SDK)/include -o $(OUT) $(SRC)

package:
	python3 $(SDK)/tools/package-plugin.py $(NAME)

clean:
	rm -f $(OUT) *.lha *.zip

.PHONY: vbcc gcc package clean
