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

typedef bool (*PfnOnEntry)(int sysc_nr);
typedef bool (*PfnOnReturn)(int sysc_nr, bool is_errno);

/* array of system call numbers, terminated by -1 */
static int* synchronize = 0;

static bool
on_entry_print(int sysc_nr)
{
  const char* name = "";
  if (sysc_nr > 0 && sysc_nr < NR_ELEMS_syscall_name)
  {
    name = syscall_name[sysc_nr].name;
    if (name == 0)
      name = "";
  }
  printf("SYSCALL %3d: %-32s", sysc_nr, name);
  return true;
}

static bool
on_return_print(int sysc_rc, bool is_errno)
{
  if (is_errno) 
  {
    printf(" : %3d", sysc_rc);
    printf(" %s\n", strerror(sysc_rc));
  }
  else
    printf(" = %d\n", sysc_rc);
  return true;
}

static int
get_syscall_index(const char* name)
{
  size_t len;
  const NAME* def;

  assert(name != 0);
  len = strlen(name);
  assert(len > 0);
  def = syscall_name;
  while(def < syscall_name + NR_ELEMS_syscall_name - 1)
  {
    if (len == def->len && 0 == memcmp(name, def->name, len))
      return def - syscall_name;
    def++;
  }
  return -1;
}

static bool
init_synchronize(int* syscall, char* copy)
{
  static const char delim[] = ",";
  char* p = strtok(copy, delim);
  while(p != 0)
  {
    int index = get_syscall_index(p);
    if (index == -1)
    {
      printf(PACKAGE ": Can't synchronize to invalid syscall name '%s'\n", p);
      return false;
    }
    *syscall++ = index;
    p = strtok(0, delim);
  }
  *syscall++ = -1;
  return true;
}

bool
set_synchronize(const char* arg)
{
  char* copy;
  char* p;
  int nr;
  int* result;

  assert(arg != 0);
  p = copy = strdup(arg);
  if (copy == 0)
    return 0;
  nr = 2; /* assume at least one entry plus one for termination */
  while(*p != 0)
    if (*p++ == ',')
      nr++;
  result = malloc(sizeof(*result) * nr);
  if (result != 0)
  {
    if (init_synchronize(result, copy))
    {
      free(synchronize);
      synchronize = result;
      return true;
    }
    free(result);
  }
  free(copy);
  return false;
}

static bool
on_entry_sync(int sysc_nr)
{
  int* s = synchronize;
  while(*s != -1)
    if (*s++ == sysc_nr)
      return false;
  return true;
}

static bool
on_return_nothing(int sysc_rc, bool is_errno)
{
  return true;
}

bool
trace_syscall(pid_t pid, bool until_sync)
{
  PfnOnEntry on_entry = on_entry_print;
  PfnOnReturn on_return = on_return_print;
  if (until_sync)
  {
    on_entry = on_entry_sync;
    on_return = on_return_nothing;
  }
  for(;;)
  {
    int sysc_nr;
    int sysc_rc;
    bool is_errno;

    /* trace until we reach next syscall */
    if (-1 == msg_ptrace(pid, PTRACE_SYSCALL, 0, 0))
      return false;
    if (!msg_wait(VOODOO_SIGTRAP, pid))
      return false;
    if (!msg_get_syscall(pid, &sysc_nr))
      return false;
    if (!on_entry(sysc_nr))
      break;

    /* get return value of syscall */
    if (-1 == msg_ptrace(pid, PTRACE_SYSCALL, 0, 0))
      return false;
    if (!msg_wait(VOODOO_SIGTRAP, pid))
      return false;
    is_errno = msg_get_errno(pid, &sysc_rc);
    if (!on_return(sysc_rc, is_errno))
      break;
  }
  return true;
}

