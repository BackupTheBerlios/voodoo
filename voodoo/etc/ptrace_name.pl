#!/usr/bin/perl -w
use strict;
my @request;
while(<>)
{
  next unless (m/^\s+PTRACE_(\w+)\s*=\s*(\d+)/);
  $request[$2] = $1;
}

my $sep = "const char* const ptrace_name[] =\n{\n";
for my $request(@request)
{
  printf "%s  ", $sep;
  $sep = ",\n";
  if (defined($request))
    { printf "\"%s\"", $request; }
  else
    { print "0"; }
}
print "\n};\n"
