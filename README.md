ipsbehead
=========

ipsbehead is an utility to adjust IPS patches that require a header in the
target SNES ROM in order to use these patches with a headerless SNES ROM. It
does so by discarding any alterations before `0x200` and recalculating offsets
of the other records.

Author notes
------------

Although the program was intended for SNES ROMs, output patches should work
with any ROM file that has a fixed size 512 byte wide header.

**It is not recomended to directly patch your ROMs because broken patches may
damage your ROM. Please use an emulator or ROM loading tool that has built-in
IPS patch loading support.**

Compiling
---------

To compile ipsbehead just run `make` in the source directory. ipsbehead does
not depend on any third party libraries and is known to compile under gcc and
clang. Microsoft compilers have not been tested.

Usage
-----

	./ipsbehead input.ips output.ips

