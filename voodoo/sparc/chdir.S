#include <asm-sparc/unistd.h>

		.section .text
		set	__NR_exit, %g1		! exit
		mov	%g0, %o0		! status=0
		ta	0x10
