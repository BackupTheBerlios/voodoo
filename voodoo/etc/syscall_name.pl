#!/usr/bin/perl -ws
use strict;

# From /usr/include/asm/unistd.h, on parisc-debian30r1
#
# /*
#  *   HP-UX system calls get their native numbers for binary compatibility.
#  */
#
#   #define __NR_HPUX_exit                    1
# 
# and down below we have
#
#   #define __NR_Linux                0
#   #define __NR_syscall              (__NR_Linux + 0)
#   #define __NR_exit                 (__NR_Linux + 1)
#
# Life sucks. Yeah.

my @syscall;
while(<>)
{
  if (m/^\s*#define\s+__NR_(\w+)\s+(\d+)/)
  {
    my ( $name, $number ) = ( $1, $2 );
    $syscall[$number] = $name unless ($name =~ m/^HPUX_/);
  }
  elsif (m/^\s*#define\s+__NR_(\w+)\s+\(\s*__NR_Linux\s*\+\s*(\d+)\)/)
  {
    $syscall[$2] = $1;
  }
}

my $ident = "const NAME syscall_name[]";

open(H, "> $::base.h") || die "$!: $::base.h";
my $guard = '__' . uc($::base) . '_H';
$guard =~ s/[^\w]/_/g;

printf H <<BLOCK_1;
#ifndef $guard
#define $guard

#define DEF_NAME(n)	{ sizeof(n) - 1, n }
#define DEF_NULL	{ 0, 0 }
typedef struct { size_t len; const char* const name; } NAME;

extern $ident;
enum { NR_ELEMS_syscall_name = $#syscall };

#endif /* $guard */
BLOCK_1
close H;

open(C, '>' . $::base . '.inc') || die "$!";
my $sep = $ident . " =\n{\n";
for my $syscall(@syscall)
{
  printf C "%s  ", $sep;
  $sep = ",\n";
  if (defined($syscall))
    { printf C "DEF_NAME(\"%s\")", $syscall; }
  else
    { print C "DEF_NULL"; }
}
print C "\n};\n";
close C;
