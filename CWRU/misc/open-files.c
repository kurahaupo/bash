#include <sys/types.h>
#include <fcntl.h>
#include <sys/file.h>
#include <stdio.h>

int main(int argc, char**argv)
{
	register int	i;

	for (i = 0; i < getdtablesize(); i++) {
		if (fcntl(i, F_GETFD, 0) != -1)
			fprintf(stderr, "fd %d: open\n", i);
	}
	return 0;
}

