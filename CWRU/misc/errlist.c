/*
 * If necessary, link with lib/sh/libsh.a
 */

/* Copyright (C) 1998-2009 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

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

#include <stdio.h>
#include <errno.h>

extern char *strerror();

extern int sys_nerr;

int
main(int argc, char **argv)
{
	int	i, n;

	if (argc == 1) {
		for (i = 1; i < sys_nerr; i++)
			printf("%d --> %s\n", i, strerror(i));
	} else {
		for (i = 1; i < argc; i++) {
			n = atoi(argv[i]);
			printf("%d --> %s\n", n, strerror(n));
		}
	}
	return 0;
}

int
programming_error(char *a, int b)
{
}

void
fatal_error(void)
{
}
