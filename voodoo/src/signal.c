/*
 *      Copyright (c) 2003-2004 Alexander Bartolich
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

volatile pid_t pid_target = -1;
volatile unsigned nr_sigchld = 0;
volatile unsigned flags = 0;

void
sig_int(int signo, siginfo_t* siginfo, void* p)
{
  if (flags & FLG_DEBUG)
    fprintf(stderr, "sig_int=%d pid=%d\n", signo, siginfo->si_pid);
  msg_ptrace(pid_target, PTRACE_DETACH, 0, 0);
  exit(EXIT_SIGINT);
}

void
sig_chld(int signo, siginfo_t* siginfo, void* p)
{
  if (flags & FLG_DEBUG)
    fprintf(stderr, "sig_chld=%d pid=%d status=%08x addr=%p\n",
      signo, siginfo->si_pid, siginfo->si_status, siginfo->si_addr
    );
  if (siginfo->si_pid == pid_target)
    nr_sigchld++;
}

void
set_signals(pid_t pid)
{
  struct sigaction sa;

  pid_target = pid;
  nr_sigchld = 0;

  sa.sa_sigaction = sig_int;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;
  sigaction(SIGHUP, &sa, 0);
  sigaction(SIGINT, &sa, 0);
  sigaction(SIGQUIT, &sa, 0);
  sigaction(SIGPIPE, &sa, 0);
  sigaction(SIGTERM, &sa, 0);

  sa.sa_sigaction = sig_chld;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_NOCLDWAIT | SA_SIGINFO;
  sigaction(SIGCHLD, &sa, 0);
}
