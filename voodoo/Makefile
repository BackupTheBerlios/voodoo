ARCH    = $(shell etc/getarch)
PACKAGE	= voodoo
VERSION = 0.1.0

OBJS	= main.o msg.o signal.o trace.o
_CFLAGS = -D PACKAGE=\"$(PACKAGE)\" \
	-D VERSION=\"$(VERSION)\" \
	-pipe -Wall -O2 $(CFLAGS) 

all: default
default: mkdir $(PACKAGE)

mkdir: ; test -d generated || mkdir generated

# On sparc asm/unistd.h includes either asm-sparc/unistd.h or
# asm-sparc64/unistd.h. These files are slightly different.
# Don't know when to choose 64-bit, since my machine is just 32.
generated/syscall_name.inc generated/syscall_name.h: \
	etc/syscall_name.pl
	./$?	-base=generated/syscall_name \
		/usr/include/asm/unistd.h \
		/usr/include/asm-$(ARCH)/unistd.h

generated/ptrace_name.inc: etc/ptrace_name.pl
	./$? /usr/include/sys/ptrace.h > $@

$(ARCH)/%.o: $(ARCH)/%.S
	$(CC) -c -Wall -o $@ $<
$(ARCH)/%.inc: $(ARCH)/%.o
	objdump -d $< \
	| sed -ne '/^[[:space:]]*[[:xdigit:]]\{1,\}:/ p' \
	| etc/disasm.pl -identifier=disasm_$* \
	> $@

%.o: src/%.c
	$(CC) -c $(_CFLAGS) -I . -o $@ $<

$(PACKAGE): \
	generated/syscall_name.inc \
	generated/ptrace_name.inc \
	$(ARCH)/chdir.inc \
	$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

clean:
	rm -f $(ARCH)/*.inc generated/*
	find . -type f -size 0 | xargs -r rm
	find .	-type f -name '*.o' -o \
		-type f -name '*~' -o \
		-type f -name '*.tar.gz' -o \
		-type f -name 'a.out' -o \
		-type f -name 'core' -o \
		-type f -name 'core.*' -o \
		-type f -name 'log' \
	| xargs -r rm

dist: ; etc/mkdist
ps: ; ps auwx | grep vmstat
