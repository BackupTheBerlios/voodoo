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
#include <sys/times.h>

#include "generated/syscall_name.inc"

#if defined(__alpha__)
# include "alpha/chdir.inc"
#elif defined(__i386__)
  #include "i386/chdir.inc"
#elif defined(__sparc__)
  #include "sparc/chdir.inc"
#else
  #error Undefined hardware.
#endif

#if defined(__hppa__)
# define DEF_SELECT		"_newselect"
#else
# define DEF_SELECT		"select"
#endif
 
#define DEF_SYNCHRONIZE		"nanosleep," DEF_SELECT ",waitpid,wait4"

static const char sz_opt[] = "dhp:s:tv";
static const char sz_version[] = PACKAGE " " VERSION "\n";
static const char sz_help[] =
  "Usage: " PACKAGE " [OPTION]... -p PID -t\n"
  "       " PACKAGE " [OPTION]... -p PID SYSCALL [ARGUMENT]...\n"
  "\n"
  "OPTION\n"
  "  -d        enable diagnostic messages\n"
  "  -h        write this message, then exit\n"
  "  -p pid    work on process specified by pid\n"
  "  -s list   set syscalls to synchronize with\n"
  "            default: " DEF_SYNCHRONIZE "\n"
  "  -t        do a poor man's strace\n"
  "  -v        write version number, then exit\n"
  "\n"
  ;

static int
do_trace(pid_t pid, bool until_sync)
{
  set_signals(pid);
  if (-1 == msg_ptrace(pid, PTRACE_ATTACH, 0, 0))
    return EXIT_ATTACH;
  if (msg_wait(SIGSTOP, pid))
  {
    msg_set_sysgood(pid);
    trace_syscall(pid, until_sync);
  }
  msg_ptrace(pid, PTRACE_DETACH, 0, 0);
  return 0;
}


static int
one_process(pid_t pid, bool sync)
{
  set_signals(pid);
  if (-1 == msg_ptrace(pid, PTRACE_ATTACH, 0, 0))
    return EXIT_ATTACH;
  if (msg_wait(SIGSTOP, pid))
  {
    msg_set_sysgood(pid);
    /*
     * we stopped the program in the middle of what it was doing
     * continue it, and make it stop at the next syscall
     */
    #if 0
    {
      voodoo_regs regs;
printf("%s:%d\n", __FILE__, __LINE__);
      if (-1 == msg_ptrace(pid, PTRACE_SYSCALL, 0, 0))
        return;
printf("%s:%d\n", __FILE__, __LINE__);
      if (!msg_wait(SIGSTOP))
        return;
printf("%s:%d\n", __FILE__, __LINE__);
      if (-1 == msg_ptrace(pid, PTRACE_GETREGS, 0, &regs))
        return;
printf("%s:%d\n", __FILE__, __LINE__);
      dump_regs(&regs);
printf("%s:%d\n", __FILE__, __LINE__);
fflush(stdout);
      ((void(*)())disasm_chdir)();
printf("%s:%d\n", __FILE__, __LINE__);

      msg_ptrace_cpy(pid, PTRACE_POKETEXT, (long*)disasm_chdir,
        (long*)voodoo_regs_ip(&regs), sizeof(disasm_chdir)
      );
      if (-1 != msg_ptrace(pid, PTRACE_SYSCALL, 0, 0))
      ;
    }
  #else
    trace_syscall(pid, sync);
  #endif
  }
  msg_ptrace(pid, PTRACE_DETACH, 0, 0);
  return 0;
}

static bool
num_arg(const char* p, long* result)
{
  char* endp;
  *result = strtol(p, &endp, 0);
  if (*endp == 0)
    return true;
  printf(PACKAGE ": Invalid numerical argument %s\n", p);  
  return false;
}

static pid_t
read_target(const char* arg)
{
  pid_t pid;
#if defined(__alpha__)
  long l;
  if (!num_arg(arg, &l))
    return -1;
  pid = l;
#else
  assert(sizeof(pid) == sizeof(long));
  if (!num_arg(arg, (long*)&pid))
    return -1;
#endif
  if (pid == getpid())
  {
    fputs(PACKAGE ": I'm sorry, I can't let you do that, Dave.\n",
      stderr);
    return -1;
  }
  return pid;
}

int
main(int argc, char**argv)
{
  pid_t pid = -1;
  const char* synchronize_arg = DEF_SYNCHRONIZE;
  if (argc < 2)
  {
    fputs(sz_help, stdout);
    return -1;
  }
  for(;;)
  {
    switch(getopt(argc, argv, sz_opt))
    {
      default: return -1;
      case 'h': fputs(sz_help, stdout); break;
      case 'v': fputs(sz_version, stdout); break;
      case 'p':
        if (-1 == (pid = read_target(optarg)))
	  return -1;
        continue;
      case 's': synchronize_arg = optarg; continue;
      case 't': flags |= FLG_TRACE; continue;
      case 'd': flags |= FLG_DEBUG; continue;
      case -1:
        if (pid == -1)
	{
	  fputs(PACKAGE ": No process specified, nothing to do.\n", stderr);
	  return -1;
	}
	if (flags & FLG_TRACE)
	  return do_trace(pid, false);
	
	else if (!set_synchronize(synchronize_arg))
	  return -1;
	return one_process(pid, true);
    }
    break;
  }
  return 0;
}
