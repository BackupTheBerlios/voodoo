#ifndef __MAIN_H
#define __MAIN_H

#ifndef __linux__
# error Get yourself a real operating system, dude.
#endif

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/procfs.h>
#include <sys/ptrace.h>

#if defined(__i386__)
# include <asm/ptrace.h> /* for PTRACE_O_TRACESYSGOOD */
#endif

#if defined(__alpha__)
# define	SIZEOF_LONG_GT_PID
#endif

#include "generated/syscall_name.h"

#define NR_ELEMS(a)	(sizeof(a) / sizeof(a[0]))

typedef enum { false, true } bool;

enum
{
  EXIT_ARGV   = -1,
  EXIT_SIGINT = -2,
  EXIT_ATTACH =	-3
};

enum
{
  FLG_DEBUG = 0x0001,
  FLG_TRACE = 0x0002
};

/* extern volatile pid_t pid_target; */
extern volatile unsigned nr_sigchld;
extern volatile unsigned flags;
void set_signals(pid_t pid);

void sig_int(int signo, siginfo_t* siginfo, void* p);
void sig_chld(int signo, siginfo_t* siginfo, void* p);

bool msg_wait(int signo, pid_t pid_target);
long msg_ptrace(pid_t pid, int request, void *addr, void *data);
bool msg_get_syscall(pid_t pid, int* nr);
bool msg_get_errno(pid_t pid, int* rc);

#ifdef PTRACE_O_TRACESYSGOOD
  void msg_set_sysgood(pid_t pid);
  int VOODOO_SIGTRAP;
#else
# define msg_set_sysgood(pid)
# define VOODOO_SIGTRAP         SIGTRAP
#endif

bool trace_syscall(pid_t pid, bool until_sync);
bool set_synchronize(const char* arg);

#endif
