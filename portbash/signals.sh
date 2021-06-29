#! /bin/sh
#
CC=cc
export CC

cat > x.c <<EOF
#include <signal.h>

int main(int argc, char **argv)
{
	return 0;
}
EOF

${CC} -E x.c > x.i || { rm -f x.c x.i ; exit 1; }

if egrep 'void.*signal' x.i >/dev/null 2>&1
then
	echo '#define VOID_SIGHANDLER'
fi
rm -f x.c x.i

cat > x.c << EOF
#include <signal.h>
sigset_t set, oset;
int main(int argc, char **argv)
{
	sigemptyset(&set);
	sigemptyset(&oset);
	sigaddset(&set, 2);
	sigprocmask(SIG_BLOCK, &set, &oset);
	return 0;
}
EOF
if ${CC} x.c >/dev/null 2>&1; then
	echo '#define HAVE_POSIX_SIGNALS'
else
	cat > x.c << EOF
#include <signal.h>
int main(int argc, char **argv)
{
	long omask = sigblock(sigmask(2));
	sigsetmask(omask);
	return 0;
}
EOF
	if ${CC} x.c >/dev/null 2>&1; then
		echo '#define HAVE_BSD_SIGNALS'
	else
		cat > x.c << EOF
#include <signal.h>
int main(int argc, char **argv)
{
	int n;
	n = sighold(2);
	sigrelse(2);
	return 0;
}
EOF
		if ${CC} x.c >/dev/null 2>&1; then
			echo '#define HAVE_USG_SIGHOLD'
		fi
	fi
fi

rm -f x.c x.o a.out

exit 0
