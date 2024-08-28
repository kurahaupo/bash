/* signames.c -- Create an array of signal names. */

/* Copyright (C) 2006-2021 Free Software Foundation, Inc.

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

#include <config.h>

#include <stdio.h>

#include <sys/types.h>
#include <signal.h>

#if defined (HAVE_STDLIB_H)
#  include <stdlib.h>
#else
#  include "ansi_stdlib.h"
#endif /* HAVE_STDLIB_H */

#if !defined (NSIG)
#  define NSIG 64
#endif

/*
 * Special traps:
 *	EXIT == 0
 *	DEBUG == NSIG
 *	ERR == NSIG+1
 *	RETURN == NSIG+2
 */
#define LASTSIG NSIG+2

#define signal_names_size (sizeof(signal_names)/sizeof(signal_names[0]))

/* AIX 4.3 defines SIGRTMIN and SIGRTMAX as 888 and 999 respectively.
   I don't want to allocate so much unused space for the intervening signal
   numbers, so we just punt if SIGRTMAX is past the bounds of the
   signal_names array (handled in configure). */
#if defined (SIGRTMAX) && defined (UNUSABLE_RT_SIGNALS)
#  undef SIGRTMAX
#  undef SIGRTMIN
#endif

#if defined (SIGRTMAX) || defined (SIGRTMIN)
#  define RTLEN 14
#  define RTLIM 256
#endif

char *signal_names[2 * (LASTSIG)] = {
  "EXIT"

#if defined (SIGLOST)	/* resource lost (eg, record-lock lost) */
  , [SIGLOST] = "SIGLOST"
#endif

  /* AIX */
#if defined (SIGMSG)	/* HFT input data pending */
  , [SIGMSG] = "SIGMSG"
#endif

#if defined (SIGDANGER)	/* system crash imminent */
  , [SIGDANGER] = "SIGDANGER"
#endif

#if defined (SIGMIGRATE) /* migrate process to another CPU */
  , [SIGMIGRATE] = "SIGMIGRATE"
#endif

#if defined (SIGPRE)	/* programming error */
  , [SIGPRE] = "SIGPRE"
#endif

#if defined (SIGPHONE)	/* Phone interrupt */
  , [SIGPHONE] = "SIGPHONE"
#endif

#if defined (SIGVIRT)	/* AIX virtual time alarm */
  , [SIGVIRT] = "SIGVIRT"
#endif

#if defined (SIGTINT)	/* Interrupt */
  , [SIGTINT] = "SIGTINT"
#endif

#if defined (SIGALRM1)	/* m:n condition variables */
  , [SIGALRM1] = "SIGALRM1"
#endif

#if defined (SIGWAITING)	/* m:n scheduling */
  , [SIGWAITING] = "SIGWAITING"
#endif

#if defined (SIGGRANT)	/* HFT monitor mode granted */
  , [SIGGRANT] = "SIGGRANT"
#endif

#if defined (SIGKAP)	/* keep alive poll from native keyboard */
  , [SIGKAP] = "SIGKAP"
#endif

#if defined (SIGRETRACT) /* HFT monitor mode retracted */
  , [SIGRETRACT] = "SIGRETRACT"
#endif

#if defined (SIGSOUND)	/* HFT sound sequence has completed */
  , [SIGSOUND] = "SIGSOUND"
#endif

#if defined (SIGSAK)	/* Secure Attention Key */
  , [SIGSAK] = "SIGSAK"
#endif

#if defined (SIGCPUFAIL)	/* Predictive processor deconfiguration */
  , [SIGCPUFAIL] = "SIGCPUFAIL"
#endif

#if defined (SIGAIO)	/* Asynchronous I/O */
  , [SIGAIO] = "SIGAIO"
#endif

#if defined (SIGLAB)	/* Security label changed */
  , [SIGLAB] = "SIGLAB"
#endif

  /* SunOS5 */
#if defined (SIGLWP)	/* Solaris: special signal used by thread library */
  , [SIGLWP] = "SIGLWP"
#endif

#if defined (SIGFREEZE)	/* Solaris: special signal used by CPR */
  , [SIGFREEZE] = "SIGFREEZE"
#endif

#if defined (SIGTHAW)	/* Solaris: special signal used by CPR */
  , [SIGTHAW] = "SIGTHAW"
#endif

#if defined (SIGCANCEL)	/* Solaris: thread cancellation signal used by libthread */
  , [SIGCANCEL] = "SIGCANCEL"
#endif

#if defined (SIGXRES)	/* Solaris: resource control exceeded */
  , [SIGXRES] = "SIGXRES"
#endif

#if defined (SIGJVM1)	/* Solaris: Java Virtual Machine 1 */
  , [SIGJVM1] = "SIGJVM1"
#endif

#if defined (SIGJVM2)	/* Solaris: Java Virtual Machine 2 */
  , [SIGJVM2] = "SIGJVM2"
#endif

#if defined (SIGDGTIMER1)
  , [SIGDGTIMER1] = "SIGDGTIMER1"
#endif

#if defined (SIGDGTIMER2)
  , [SIGDGTIMER2] = "SIGDGTIMER2"
#endif

#if defined (SIGDGTIMER3)
  , [SIGDGTIMER3] = "SIGDGTIMER3"
#endif

#if defined (SIGDGTIMER4)
  , [SIGDGTIMER4] = "SIGDGTIMER4"
#endif

#if defined (SIGDGNOTIFY)
  , [SIGDGNOTIFY] = "SIGDGNOTIFY"
#endif

  /* Apollo */
#if defined (SIGAPOLLO)
  , [SIGAPOLLO] = "SIGAPOLLO"
#endif

  /* HP-UX */
#if defined (SIGDIL)	/* DIL signal (?) */
  , [SIGDIL] = "SIGDIL"
#endif

  /* System V */
#if defined (SIGCLD)	/* Like SIGCHLD.  */
  , [SIGCLD] = "SIGCLD"
#endif

#if defined (SIGPWR)	/* power state indication */
  , [SIGPWR] = "SIGPWR"
#endif

#if defined (SIGPOLL)	/* Pollable event (for streams)  */
  , [SIGPOLL] = "SIGPOLL"
#endif

  /* Unknown */
#if defined (SIGWINDOW)
  , [SIGWINDOW] = "SIGWINDOW"
#endif

  /* Linux */
#if defined (SIGSTKFLT)
  , [SIGSTKFLT] = "SIGSTKFLT"
#endif

  /* FreeBSD */
#if defined (SIGTHR)	/* thread interrupt */
  , [SIGTHR] = "SIGTHR"
#endif

  /* z/OS */
#if defined (SIGABND)
  , [SIGABND] = "SIGABND"
#endif
#if defined (SIGIOERR)
  , [SIGIOERR] = "SIGIOERR"
#endif
#if defined (SIGTHSTOP)
  , [SIGTHSTOP] = "SIGTHSTOP"
#endif
#if defined (SIGTHCONT)
  , [SIGTHCONT] = "SIGTHCONT"
#endif
#if defined (SIGTRACE)
  , [SIGTRACE] = "SIGTRACE"
#endif
#if defined (SIGDCE)
  , [SIGDCE] = "SIGDCE"
#endif
#if defined (SIGDUMP)
  , [SIGDUMP] = "SIGDUMP"
#endif

  /* Common */
#if defined (SIGHUP)	/* hangup */
  , [SIGHUP] = "SIGHUP"
#endif

#if defined (SIGINT)	/* interrupt */
  , [SIGINT] = "SIGINT"
#endif

#if defined (SIGQUIT)	/* quit */
  , [SIGQUIT] = "SIGQUIT"
#endif

#if defined (SIGILL)	/* illegal instruction (not reset when caught) */
  , [SIGILL] = "SIGILL"
#endif

#if defined (SIGTRAP)	/* trace trap (not reset when caught) */
  , [SIGTRAP] = "SIGTRAP"
#endif

#if defined (SIGIOT)	/* IOT instruction */
  , [SIGIOT] = "SIGIOT"
#endif

#if defined (SIGABRT)	/* Cause current process to dump core. */
  , [SIGABRT] = "SIGABRT"
#endif

#if defined (SIGEMT)	/* EMT instruction */
  , [SIGEMT] = "SIGEMT"
#endif

#if defined (SIGFPE)	/* floating point exception */
  , [SIGFPE] = "SIGFPE"
#endif

#if defined (SIGKILL)	/* kill (cannot be caught or ignored) */
  , [SIGKILL] = "SIGKILL"
#endif

#if defined (SIGBUS)	/* bus error */
  , [SIGBUS] = "SIGBUS"
#endif

#if defined (SIGSEGV)	/* segmentation violation */
  , [SIGSEGV] = "SIGSEGV"
#endif

#if defined (SIGSYS)	/* bad argument to system call */
  , [SIGSYS] = "SIGSYS"
#endif

#if defined (SIGPIPE)	/* write on a pipe with no one to read it */
  , [SIGPIPE] = "SIGPIPE"
#endif

#if defined (SIGALRM)	/* alarm clock */
  , [SIGALRM] = "SIGALRM"
#endif

#if defined (SIGTERM)	/* software termination signal from kill */
  , [SIGTERM] = "SIGTERM"
#endif

#if defined (SIGURG)	/* urgent condition on IO channel */
  , [SIGURG] = "SIGURG"
#endif

#if defined (SIGSTOP)	/* sendable stop signal not from tty */
  , [SIGSTOP] = "SIGSTOP"
#endif

#if defined (SIGTSTP)	/* stop signal from tty */
  , [SIGTSTP] = "SIGTSTP"
#endif

#if defined (SIGCONT)	/* continue a stopped process */
  , [SIGCONT] = "SIGCONT"
#endif

#if defined (SIGCHLD)	/* to parent on child stop or exit */
  , [SIGCHLD] = "SIGCHLD"
#endif

#if defined (SIGTTIN)	/* to readers pgrp upon background tty read */
  , [SIGTTIN] = "SIGTTIN"
#endif

#if defined (SIGTTOU)	/* like TTIN for output if (tp->t_local&LTOSTOP) */
  , [SIGTTOU] = "SIGTTOU"
#endif

#if defined (SIGIO)	/* input/output possible signal */
  , [SIGIO] = "SIGIO"
#endif

#if defined (SIGXCPU)	/* exceeded CPU time limit */
  , [SIGXCPU] = "SIGXCPU"
#endif

#if defined (SIGXFSZ)	/* exceeded file size limit */
  , [SIGXFSZ] = "SIGXFSZ"
#endif

#if defined (SIGVTALRM)	/* virtual time alarm */
  , [SIGVTALRM] = "SIGVTALRM"
#endif

#if defined (SIGPROF)	/* profiling time alarm */
  , [SIGPROF] = "SIGPROF"
#endif

#if defined (SIGWINCH)	/* window changed */
  , [SIGWINCH] = "SIGWINCH"
#endif

  /* 4.4 BSD */
#if defined (SIGINFO) && !defined _SEQUENT_	/* information request */
  , [SIGINFO] = "SIGINFO"
#endif

#if defined (SIGUSR1)	/* user defined signal 1 */
  , [SIGUSR1] = "SIGUSR1"
#endif

#if defined (SIGUSR2)	/* user defined signal 2 */
  , [SIGUSR2] = "SIGUSR2"
#endif

#if defined (SIGKILLTHR)	/* BeOS: Kill Thread */
  , [SIGKILLTHR] = "SIGKILLTHR"
#endif

  , [NSIG] = "DEBUG"
  , [NSIG+1] = "ERR"
  , [NSIG+2] = "RETURN"
};

#if defined (BUILDTOOL)
extern char *progname;
#endif

void
initialize_signames (void)
{
  /* Place signal names which can be aliases for more common signal
     names first.  This allows (for example) SIGABRT to overwrite SIGLOST. */

  /* POSIX 1003.1b-1993 real time signals, but take care of incomplete
     implementations. According to the standard, both SIGRTMIN and
     SIGRTMAX must be defined, SIGRTMIN must be strictly less than
     SIGRTMAX, and the difference must be at least 7; that is, there
     must be at least eight distinct real time signals. */

  /* The generated signal names are SIGRTMIN, SIGRTMIN+1, ...,
     SIGRTMIN+x, SIGRTMAX-x, ..., SIGRTMAX-1, SIGRTMAX. If the number
     of RT signals is odd, there is an extra SIGRTMIN+(x+1).
     These names are the ones used by ksh and /usr/xpg4/bin/sh on SunOS5. */

#if defined (SIGRTMAX)
  signal_names[SIGRTMAX] = "SIGRTMAX";
# if defined (SIGRTMIN)
  signal_names[SIGRTMIN] = "SIGRTMIN";

  const int rtmax = SIGRTMAX;
  const int rtmin = SIGRTMIN;

  if (rtmax > rtmin)
    {
      const int rt_raw_count = rtmax - rtmin + 1;
#  if defined RTLIM
#   if defined BUILDTOOL
    if (RTLIM < SIGRTMAX - SIGRTMIN + 1)
      /* complain if there are too many RT signals */
      fprintf(stderr, "%s: error: more than %d real time signals, fix `%s'\n",
	      progname, RTLIM, progname);
#   endif
      const int rt_count = RTLIM < rt_raw_count
                         ? RTLIM : rt_raw_count;
#  else
      const int rt_count = rt_raw_count;
#  endif
      /* Count half up from RTMIN and half down from RTMAX.
         Round up in case the total count is odd.
      */
      const int rt_half_count = (rt_count+1) / 2;

      char **minp = signal_names + rtmin;
      char **p = minp + 1;

      for (char **midp = signal_names + rtmin + rt_half_count; p < midp ; ++p)
	if (*p == NULL)
	  if (asprintf(p, "SIGRTMIN%+zd", p - minp) < 0)
	    *p = NULL;

      p += rt_raw_count - rt_count; /* Leave a gap if there's too many */

      for (char **maxp = signal_names + rtmax; p < maxp ; ++p)
	if (*p == NULL)
	  if (asprintf(p, "SIGRTMAX%+zd", p - maxp) < 0)
	    *p = NULL;
    }
# endif
#elif defined (SIGRTMIN)
  signal_names[SIGRTMIN] = "SIGRTMIN";
#endif

  for (char **p = signal_names ; p<signal_names+NSIG; ++p)
    if (*p == NULL)
      if (asprintf (p, "SIGJUNK(%zd)", p - signal_names) < 0)
	*p = "SIGJUNK";

}
