/* ufuncs - sleep and alarm functions that understand fractional values */

/* Copyright (C) 2008,2009-2020,2022 Free Software Foundation, Inc.

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

#include <errno.h>
#if !defined (errno)
extern int errno;
#endif /* !errno */

#include "quit.h"

#if defined (HAVE_SELECT)
#  include "posixselect.h"
#  include "trap.h"
#  include "stat-time.h"
#endif

#include "error.h"

/* A version of `alarm' using setitimer if it's available. */

#if defined (HAVE_SETITIMER)
unsigned int
falarm (unsigned int secs, unsigned int usecs)
{
  struct itimerval it, oit;

  it.it_interval.tv_sec = 0;
  it.it_interval.tv_usec = 0;

  it.it_value.tv_sec = secs;
  it.it_value.tv_usec = usecs;

  if (setitimer(ITIMER_REAL, &it, &oit) < 0)
    return (-1);		/* XXX will be converted to unsigned */

  /* Backwards compatibility with alarm(3) */
  if (oit.it_value.tv_usec)
    oit.it_value.tv_sec++;
  return (oit.it_value.tv_sec);
}
#else
int
falarm (unsigned int secs, unsigned int usecs)
{
  if (secs == 0 && usecs == 0)
    return (alarm (0));

  if (secs == 0 || usecs >= 500000)
    {
      secs++;
      usecs = 0;
    }
  return (alarm (secs));
}
#endif /* !HAVE_SETITIMER */

/* A version of sleep using fractional seconds and select.  I'd like to use
   `usleep', but it's already taken */

#if defined (HAVE_NANOSLEEP)
static int
nsleep (unsigned int sec, unsigned int usec)
{
  int r;
  struct timespec req, rem;

  req.tv_sec = sec;
  req.tv_nsec = usec * 1000;

  for (;;)
    {
      QUIT;
      r = nanosleep (&req, &rem);
      if (r == 0 || errno != EINTR)
	return r;
      req = rem;
    }
  return r;
}
#endif

#if defined (HAVE_TIMEVAL) && (defined (HAVE_SELECT) || defined (HAVE_PSELECT))
static int
ssleep (unsigned int sec, unsigned int usec)
{
  int e, r;
  sigset_t blocked_sigs;
#  if defined (HAVE_PSELECT)
  struct timespec ts;
#  else
  sigset_t prevmask;
  struct timeval tv;
#  endif

  sigemptyset (&blocked_sigs);
#  if defined (SIGCHLD)
  sigaddset (&blocked_sigs, SIGCHLD);
#  endif

#  if defined (HAVE_PSELECT)
  ts.tv_sec = sec;
  ts.tv_nsec = usec * 1000;
#  else
  sigemptyset (&prevmask);
  tv.tv_sec = sec;
  tv.tv_usec = usec;
#  endif /* !HAVE_PSELECT */

  do
    {
#  if defined (HAVE_PSELECT)
      r = pselect(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &ts, &blocked_sigs);
#  else
      sigprocmask (SIG_SETMASK, &blocked_sigs, &prevmask);
      r = select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &tv);
      sigprocmask (SIG_SETMASK, &prevmask, NULL);
#  endif
      e = errno;
      if (r < 0 && errno == EINTR)
	return -1;		/* caller will handle XXX - QUIT;? */
      errno = e;
    }
  while (r < 0 && errno == EINTR);

  return r;
}
#endif

#if !defined (HAVE_SELECT)
static int
ancientsleep(unsigned int sec, unsigned int usec)
{
  if (usec >= 500000)	/* round */
   sec++;
  return (sleep(sec));
}
#endif

int
fsleep(unsigned int sec, unsigned int usec)
{
#if defined (HAVE_NANOSLEEP)
  return (nsleep (sec, usec));
#elif defined (HAVE_TIMEVAL) && (defined (HAVE_SELECT) || defined (HAVE_PSELECT))
  return (ssleep (sec, usec));
#else /* !HAVE_TIMEVAL || !HAVE_SELECT */
  return (ancientsleep (sec, usec));
#endif /* !HAVE_TIMEVAL || !HAVE_SELECT */
}
