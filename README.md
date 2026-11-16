# Page Fault Decoder for the Virtual Memory Project

This repository contains a page fault decoder for CS 1670. Copy `fault.h` and
`fault.c` into your kernel's `kernel/vm/` directory.

`fault_decode()` turns the `ESR_EL1` value of an instruction or data abort
into a `struct fault`, and `fault_print()` prints it on one line. `fault.h`
describes both functions.

The decoder uses the page fault constants in `kernel/aarch64.h`, which come
with the stencil code for the virtual memory project's Quests 3 and 4, and your
own `printf()` (only `%s`, `%d`, `%x`, and `%p`).
