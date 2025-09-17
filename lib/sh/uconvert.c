/* uconvert - convert string representations of decimal numbers into whole
	      number/fractional value pairs. */

/* Copyright (C) 2008,2009,2020,2022 Free Software Foundation, Inc.

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

#include "config.h"

#include "bashtypes.h"

#include "posixtime.h"

#if defined (HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <stdio.h>
#include "chartypes.h"

#include "shell.h"
#include "builtins.h"

enum { DECIMAL = '.' };		/* XXX - should use locale */

/*
 * An incredibly simplistic floating point converter.
 */

/* Take a decimal number int-part[.[micro-part]] and convert it to the whole
   and fractional portions.  The fractional portion is returned in
   millionths (micro); callers are responsible for multiplying appropriately.
   EP, if non-null, gets the address of the character where conversion stops.
   Return 1 if value converted; 0 if invalid integer for either whole or
   fractional parts. */
static bool
xconvert (const char *p, size_t frac_scale, long *ip, long *up, const char **ep)
{
  bool negative = 0;
  long ipart = 0, upart = 0;

  if (p == 0)	/* callers ensure p can never be 0; this is to shut up clang */
    goto Good;	/* Arguably Bad? */

  if (*p == '-' || *p == '+')
    negative = *p++ == '-';

  for (; DIGIT(*p) ; ipart *= 10, ipart += *p++ - '0') {}

  if (negative)
    ipart = -ipart;

  if (*p == DECIMAL)
    p++;

  if (*p == 0)
    goto Good;

  if (! DIGIT(*p))
    goto Bad;

  /* Look for up to six digits past a decimal point. */
  char const *q = p;

  static const long multiplier[] = {
				    1, 10, 100, 1000, 10000, 100000,
				    1000000, 10000000, 100000000,
				    1000000000, 10000000000, 100000000000
				  };
  const long *ms = multiplier + frac_scale - 1;

  for (; p-q < frac_scale && DIGIT(*p) ; p++)
    upart += (*p - '0') * ms[q-p];

  /* round 0.5 up to 1; TODO: fix this for negatives */
  if (p-q == frac_scale && *p >= '5' && *p <= '9')
    upart++;

  if (negative && upart > 0)
    {
      upart = *ms - upart;
      ipart -= 1;
    }

  while (DIGIT(*p))
    p++;

  if (*p == 0)
    goto Good;

Bad:
  if (ip) *ip = ipart;
  if (up) *up = upart;
  if (ep) *ep = p;
  return 0;

Good:
  if (ip) *ip = ipart;
  if (up) *up = upart;
  if (ep) *ep = (char *)p;
  return 1;
}

bool
uconvert (const char *p, long *ip, long *up, char **ep)
{
  return xconvert (p, 6, ip, up, (char const **)ep);
}

bool
nconvert (const char *p, long *ip, long *up, char const **ep)
{
  return xconvert (p, 9, ip, up, ep);
}

bool
tvconvert (const char *p, struct timeval *t, char const **ep)
{
  return xconvert (p, 6, &t->tv_sec, &t->tv_usec, ep);
}

bool
tsconvert (const char *p, struct timespec *t, char const **ep)
{
  return xconvert (p, 9, &t->tv_sec, &t->tv_nsec, ep);
}
