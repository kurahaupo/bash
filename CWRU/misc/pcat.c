/* $Header:cat.c 12.0$ */
/* $ACIS:cat.c 12.0$ */
/* $Source: /ibm/acis/usr/src/bin/RCS/cat.c,v $ */

#ifndef lint
static char *rcsid = "$Header:cat.c 12.0$";
#endif

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)cat.c	5.2 (Berkeley) 12/6/85";
#endif /*not lint*/

/*
 * Concatenate files.
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <malloc.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#if 0
#define OPTSIZE BUFSIZ	/* define this only if not 4.2 BSD or beyond */
#endif

int	bflg, eflg, show_line_numbers, sflg, tflg, use_stdio, vflg;
int	lno, ibsize, obsize;

static void
sigpipe(int signum)
{
	write(2, "pcat: caught SIGPIPE\n", 21);
	exit(1);
}

void copyopt(FILE *f);
int fastcat(int fd);

int
main(int argc, char **argv)
{
	int fflg = 0;
	register FILE *fi;
	register int c;
	int dev, ino = -1;
	struct stat statb;
	int retval = 0;

	signal(SIGPIPE, sigpipe);
	lno = 1;
	for( ; argc>1 && argv[1][0]=='-'; argc--,argv++) {
		switch(argv[1][1]) {
		case 0:
			break;
		case 'u':
			setbuf(stdout, (char *)NULL);
			use_stdio++;
			continue;
		case 'n':
			show_line_numbers++;
			continue;
		case 'b':
			bflg++;
			show_line_numbers++;
			continue;
		case 'v':
			vflg++;
			continue;
		case 's':
			sflg++;
			continue;
		case 'e':
			eflg++;
			vflg++;
			continue;
		case 't':
			tflg++;
			vflg++;
			continue;
		}
		break;
	}
	if (fstat(fileno(stdout), &statb) == 0) {
		statb.st_mode &= S_IFMT;
		if (statb.st_mode!=S_IFCHR && statb.st_mode!=S_IFBLK) {
			dev = statb.st_dev;
			ino = statb.st_ino;
		}
#ifndef	OPTSIZE
		obsize = statb.st_blksize;
#endif
	}
	else
		obsize = 0;
	if (argc < 2) {
		argc = 2;
		fflg++;
	}
	while (--argc > 0) {
		if (fflg || ((*++argv)[0]=='-' && (*argv)[1]=='\0'))
			fi = stdin;
		else {
			if ((fi = fopen(*argv, "r")) == NULL) {
				perror(*argv);
				retval = 1;
				continue;
			}
		}
		if (fstat(fileno(fi), &statb) == 0) {
			if ((statb.st_mode & S_IFMT) == S_IFREG &&
			    statb.st_dev==dev && statb.st_ino==ino) {
				fprintf(stderr, "cat: input %s is output\n",
				   fflg?"-": *argv);
				fclose(fi);
				retval = 1;
				continue;
			}
#ifndef	OPTSIZE
			ibsize = statb.st_blksize;
#endif
		}
		else
			ibsize = 0;
		if (show_line_numbers||sflg||vflg)
			copyopt(fi);
		else if (use_stdio) {
			while ((c = getc(fi)) != EOF)
				putchar(c);
		} else
			retval |= fastcat(fileno(fi));	/* no flags specified */
		if (fi!=stdin)
			fclose(fi);
		else
			clearerr(fi);		/* reset sticky eof */
		if (ferror(stdout)) {
			fprintf(stderr, "cat: output write error\n");
			retval = 1;
			break;
		}
	}
	exit(retval);
}

void
copyopt(FILE *f)
{
	register int c;
        static int inl, spaced;

	for (c = getc(f) ; c != EOF ; c = getc(f)) {
		if (c == '\n') {
			if (! inl) {
				if (sflg && spaced)
					continue;
				spaced = 1;
			}
			if (show_line_numbers && ! bflg && ! inl)
				printf("%6d\t", lno++);
			if (eflg)
				putchar('$');
			putchar('\n');
			inl = 0;
			continue;
		}
		if (show_line_numbers && ! inl)
			printf("%6d\t", lno++);
		inl = 1;
		if (vflg) {
			if (! tflg && c == '\t')
				putchar(c);
			else {
				if (c > 0177) {
					printf("M-");
					c &= 0177;
				}
				if (c < ' ')
					printf("^%c", c+'@');
				else if (c == 0177)
					printf("^?");
				else
					putchar(c);
			}
		} else
			putchar(c);
		spaced = 0;
	}
}

int
fastcat(int fd)
{
	register int	buffsize, n, nwritten, offset;
	register char	*buff;
	struct stat	statbuff;

#ifndef	OPTSIZE
	if (obsize)
		buffsize = obsize;	/* common case, use output blksize */
	else if (ibsize)
		buffsize = ibsize;
	else
		buffsize = BUFSIZ;
#else
	buffsize = OPTSIZE;
#endif

	if ((buff = malloc(buffsize)) == NULL) {
		perror("cat: no memory");
		return (1);
	}

	/*
	 * Note that on some systems (V7), very large writes to a pipe
	 * return less than the requested size of the write.
	 * In this case, multiple writes are required.
	 */
	while ((n = read(fd, buff, buffsize)) > 0) {
		offset = 0;
		do {
			nwritten = write(fileno(stdout), &buff[offset], n);
			if (nwritten <= 0) {
				perror("cat: write error");
				exit(2);
			}
			offset += nwritten;
		} while ((n -= nwritten) > 0);
	}

	free(buff);
	if (n < 0) {
		perror("cat: read error");
		return (1);
	}
	return (0);
}
