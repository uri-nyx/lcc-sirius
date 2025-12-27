# A compiler toolchain (c compiler, assembler and linker) for the Sirius CPU

forked from [lcc_riscv32](https://github.com/michg/riscv32_lcc)
based on [lcc](https://github.com/drh/lcc) 
and binutils from [co32](ttps://github.com/hgeisse/eco32).

It currently supports all the instruction set, but testing and tidying is needed.
The linker and archiver use a non-standard `a.out` format for object and executable files.

## Building:

In a unix machine (or WSL):

`chmod +x mkbinaries.sh`

If you want to build the programs separately, use their respective makefiles

The binaries shall be under `./bin`
