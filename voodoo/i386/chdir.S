#include <asm/unistd.h>

	mov	$__NR_exit,%eax
	mov	$42,%ebx
	int	$0x80
