/*
 * cat replacement
 *
 * no options - the way cat was intended
 */

/*
   Copyright (C) 1999-2009,2022 Free Software Foundation, Inc.

   This file is part of GNU Bash.
   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include "builtins.h"
#include "shell.h"

#ifndef errno
extern int errno;
#endif

extern char *strerror (int);
extern char **make_builtin_argv (WORD_LIST *, int *);

static int
fcopy(int fd, char *fn)
{
	char	buf[4096];
	ssize_t	n, w;

	while (n = read(fd, buf, sizeof (buf))) {
		if (n < 0) {
			fprintf(stderr, "cat: read error: %s: %s\n", fn, strerror(errno));
			return 1;
		}
		QUIT;
		w = write(1, buf, n);
		if (w < 0) {
			fprintf(stderr, "cat: write error: %s: %s\n", fn, strerror(errno));
			return 1;
		}
		if (w != n) {
                        /* errno is not set in this case */
			fprintf(stderr,
                        	"cat: write shortfall: %s: only %zd of %zd bytes written\n",
                                fn, w, n);
			return 1;
		}
		QUIT;
	}
	return 0;
}

int
cat_main (int argc, char **argv)
{
	int	i, fd, r;
	char	*s;

	if (argc == 1)
		return (fcopy(0, "standard input"));

	for (i = r = 1; i < argc; i++) {
		QUIT;
		if (argv[i][0] == '-' && argv[i][1] == '\0')
			fd = 0;
		else {
			fd = open(argv[i], O_RDONLY, 0666);
			if (fd < 0) {
				s = s;
				fprintf(stderr, "cat: cannot open %s: %s\n", argv[i], strerror(errno));
				continue;
			}
		}
		r = fcopy(fd, argv[i]);
		if (fd != 0)
			close(fd);
	}
	QUIT;
	return (r);
}

int
cat_builtin(WORD_LIST *list)
{
	char	**v;
	int	c, r;

	v = make_builtin_argv(list, &c);
	QUIT;
	r = cat_main(c, v);
	free(v);

	return r;
}

static char const *cat_doc[] = {
	"Display files.",
	"",
	"Read each FILE and display it on the standard output.   If any",
	"FILE is `-' or if no FILE argument is given, the standard input",
	"is read.",
	NULL
};

struct builtin cat_struct = {
	"cat",
	cat_builtin,
	BUILTIN_ENABLED,
	cat_doc,
	"cat [-] [file ...]",
	0
};
