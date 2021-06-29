#! /bin/sh
#
# test certain aspects of stdio
CC=cc
export CC

cat > x.c << EOF
#include <stdio.h>
#include <stdarg.h>

xp(char const*fmt, ...)
{
	va_list args;
	va_start (args, fmt);
	vfprintf(stdout, fmt, args);
}

int main(int argc, char **argv)
{
	xp("abcde");
	return 0;
}
EOF

if ${CC} x.c >/dev/null 2>&1
then
	echo '#define HAVE_VFPRINTF'
	rm -f x.c x.o a.out
else

	cat > x.c << EOF
#include <stdio.h>

int main(int argc, char **argv)
{
	_doprnt();
	return 0;
}
EOF

	if ${CC} x.c >/dev/null 2>&1
	then
		echo '#define USE_VFPRINTF_EMULATION'
		rm -f x.c x.o a.out
	fi
fi

cat > x.c << EOF
#include <stdio.h>
int main(int argc, char **argv)
{
	setlinebuf(stdout);
	return 0;
}
EOF

if ${CC} x.c > /dev/null 2>&1
then
	rm -f x.c x.o a.out
	echo '#define HAVE_SETLINEBUF'
else
	# check for setvbuf
	# If this compiles, the system has setvbuf.  If this segfaults while
	# running, non-reversed systems get a seg violation

	cat > x.c << EOF
#include <stdio.h>

int main(int argc, char **argv)
{
	if (setvbuf(stdout, _IOLBF, (char *)0, BUFSIZ)==0)
		return 0;	/* reversed systems OK */
	return 1;		/* non-reversed systems error or SEGV */
}
EOF

	if ${CC} x.c >/dev/null 2>&1 ; then
		echo '#define HAVE_SETVBUF'
		if a.out; then
			:
		else
			rm -f core
			echo '#define REVERSED_SETVBUF_ARGS'
		fi
	fi
fi

rm -f x.c x.o a.out
exit 0
