#include <asm/unistd.h>
#include <alpha/regdef.h>

	.section .text
	addq    zero, zero, a0
	ldi     v0, __NR_exit
	callsys
