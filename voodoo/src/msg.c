/*
 *      Copyright (c) 2003 Alexander Bartolich
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "main.h"
#include <sys/wait.h>

# include "generated/ptrace_name.inc"

#if defined(__alpha__)

# define REG_R0		0
# define REG_A0		16
# define REG_A3		19
# define REG_FP		30
# define REG_PC		64
# define VOODOO_PEEK_SYSCALL	REG_R0

#elif defined(__i386__)

# include <asm/ptrace.h>
# define VOODOO_PEEK_SYSCALL	(4 * ORIG_EAX)

#elif defined(__sparc__)

# include <asm/psr.h>

# define fpq kernel_fpq
# define fq kernel_fq
# define fpu kernel_fpu
#include <asm/reg.h>
# undef fpq
# undef fq
# undef fpu

#endif

bool
msg_wait(int signo, pid_t pid_target)
{ 
  int status;
  pid_t pid = wait(&status);
  if (pid == -1)
  {
    printf(PACKAGE ": Target %d, wait failed, %s\n",
      pid_target, strerror(errno)
    );
    return false;
  }

  if (WIFEXITED(status))
    printf("child %d exited%s with status %d\n",
      pid,
      WCOREDUMP(status) ? " and dumped core" : "",
      WEXITSTATUS(status)
    );
  if (WIFSIGNALED(status))
    printf("child %d signalled by signal %d\n",
      pid, WTERMSIG(status)
    );
  if (WIFSTOPPED(status))
  {
    if (WSTOPSIG(status) != signo)
    {
      /* a different signal - send it on and wait */
      printf("Waited for signal %d, got %d\n",
	signo, WSTOPSIG(status)
      );
      msg_ptrace(pid, PTRACE_SYSCALL, 0, (void*)(long)WSTOPSIG(status));
      return msg_wait(signo, pid);
    }
    else if (flags & FLG_DEBUG)
      printf("child %d stopped by signal %d\n",
	pid, WSTOPSIG(status)
      );
  }
  return true;
}

long
msg_ptrace(pid_t pid, int request, void *addr, void *data)
{
  long rc;
  errno = 0;
  
  rc = ptrace(request, pid, addr, data);
  if (rc >= 0 || errno == 0)
    return rc;

  printf(PACKAGE ": Target %d, ", pid);
  if (request >= 0 && request < NR_ELEMS(ptrace_name))
    printf("PTRACE_%s, ", ptrace_name[request]);
  printf("%s\n", strerror(errno));
  return -1;
}

/*
 * A child stopped at a syscall has status as if it received SIGTRAP.
 * In order to distinguish between SIGTRAP and syscall, some kernel
 * versions have the PTRACE_O_TRACESYSGOOD option, that sets an extra
 * bit 0x80 in the syscall case.
 */
#ifdef PTRACE_O_TRACESYSGOOD

int VOODOO_SIGTRAP = SIGTRAP;

void
msg_set_sysgood(pid_t pid)
{
  int rc = msg_ptrace(pid, PTRACE_SETOPTIONS, 0,
    (void*)PTRACE_O_TRACESYSGOOD);
  if (rc == 0)
    VOODOO_SIGTRAP |= 0x80;
}
#endif

bool
msg_get_syscall(pid_t pid, int* nr)
{
# if defined(VOODOO_PEEK_SYSCALL)
  int rc = *nr = msg_ptrace(
    pid, PTRACE_PEEKUSER, (void*)VOODOO_PEEK_SYSCALL, 0
  );
  return rc >= 0;
# elif defined(__sparc__)
  {
    struct regs regs;
    if (msg_ptrace(pid, PTRACE_GETREGS, (void*)&regs, 0) < 0)
      return false;
    return (*nr = regs.r_g1) >= 0;
  }
# else
#   error Unkown platform.
# endif
}

bool
msg_get_errno(pid_t pid, int* rc)
{
#if defined(__alpha__)
  *rc = msg_ptrace(pid, PTRACE_PEEKUSER, (void*)REG_R0, 0);
  return 0 != msg_ptrace(pid, PTRACE_PEEKUSER, (void*)REG_A3, 0);
#elif defined(__i386__)
  int eax = msg_ptrace(pid, PTRACE_PEEKUSER, (void*)(4 * EAX), 0);
  if (eax >= 0)
    { *rc = eax; return false; }
  else
    { *rc = -eax; return true; }
# elif defined(__sparc__)
  {
    struct regs regs;
    if (msg_ptrace(pid, PTRACE_GETREGS, (void*)&regs, 0) < 0)
      { *rc = -1; return true; }
    *rc = regs.r_o0;
    return regs.r_psr & PSR_C;
  }
#else
# error Unkown platform.
#endif
}

int
msg_ptrace_cpy(pid_t pid, int request, const long* src, long* dst, size_t size)
{
  for(; size > 0; size -= sizeof(*src))
    if (ptrace(request, pid, dst++, *src++) == -1)
    {
      printf(PACKAGE ": Target %d, PEEK/POKE failed, %s\n",
	pid, strerror(errno)
      );
      return -1;
    }
  return 0;
}
