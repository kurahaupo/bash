/* See Makefile for compilation details. */

/*
   Copyright (C) 2026 Free Software Foundation, Inc.

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

#include "bashtypes.h"
#include <signal.h>

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include <stdio.h>

#include "loadables.h"
#include "jobs.h"
#include "execute_cmd.h"

static void
printprocs (int job)
{
  PROCESS *p;

  p = jobs[job]->pipe;
  do
    {
      printf ("%ld", (long)p->pid);
      p = p->next;
      putchar (p == jobs[job]->pipe ? '\n' : ' ');
    }
  while (p != jobs[job]->pipe);
}

static int
printinfo (int job, int pgrp, int jobid, int lproc)
{
  if (pgrp)
    {
      printf ("%ld\n", (long)jobs[job]->pgrp);
      return (jobs[job]->pgrp == shell_pgrp);
    }
  else if (jobid)		/* caller validates job */
    {
      printf ("%%%d\n", job + 1);
      return 0;
    }
  else if (lproc)
    {
      PROCESS *p;

      for (p = jobs[job]->pipe; p->next != jobs[job]->pipe; p = p->next)
        ;
      printf ("%ld\n", (long)p->pid);
      return 0;
    }
  else
    printprocs (job);
  return 0;
}

int
jobid_builtin (WORD_LIST *list)
{
  WORD_LIST *l;
  int any_failed, opt, job, alljobs;
  int pgrp, jobid, lproc;
  sigset_t set, oset;

  pgrp = jobid = lproc = alljobs = 0;
  reset_internal_getopt ();
  while ((opt = internal_getopt (list, "agjp")) != -1)
    {
      switch (opt)
	{
	  case 'a': alljobs = 1; break;
	  case 'g': pgrp = 1; break;
	  case 'j': jobid = 1 ; break;
	  case 'p': lproc = 1; break;
	  CASE_HELPOPT;
	  default:
	    builtin_usage ();
	    return (EX_USAGE);
	}
    }

  list = loptend;
  if ((pgrp + jobid + lproc) > 1)
    {
      builtin_usage ();
      return (EX_USAGE);
    }

  any_failed = 0;

  /* The -a option means all jobs; JOBSPEC arguments ignored */
  if (alljobs)
    {
      if (jobs == 0)
	return 0;
      BLOCK_CHILD (set, oset);
      for (job = 0; job < js.j_jobslots; job++)
	{
	  if (INVALID_JOB (job))
	    continue;
	  any_failed += printinfo (job, pgrp, jobid, lproc);
	}
      UNBLOCK_CHILD (oset);
      return (sh_chkwrite (any_failed ? EXECUTION_FAILURE : EXECUTION_SUCCESS));
    }

  /* No JOBSPECs means current job */
  if (list == 0)
    {
      BLOCK_CHILD (set, oset);
      job = get_job_spec (list);	/* current job */
      if ((job == NO_JOB) || jobs == 0 || INVALID_JOB (job))
	{
	  sh_badjob ("%%");
	  any_failed++;
	}
      else
	any_failed = printinfo (job, pgrp, jobid, lproc);
      UNBLOCK_CHILD (oset);
      return (sh_chkwrite (any_failed ? EXECUTION_FAILURE : EXECUTION_SUCCESS));
    }

  /* Otherwise we print info about each JOBSPEC argument */
  BLOCK_CHILD (set, oset);
  for (l = list; l; l = l->next)
    {
      job = get_job_spec (l);
      if ((job == NO_JOB) || jobs == 0 || INVALID_JOB (job))
	{
	  sh_badjob (l->word->word);
	  any_failed++;
	}
      else
        any_failed += printinfo (job, pgrp, jobid, lproc);
    }
  UNBLOCK_CHILD (oset);

  return (sh_chkwrite (any_failed ? EXECUTION_FAILURE : EXECUTION_SUCCESS));
}

char *jobid_doc[] = {
	"Print information about each JOBSPEC.",
	"",
	"JOBSPEC is any string that can be used to refer to a job. If JOBSPEC",
	"is omitted, use the current job.",
	"",
	"With no options, print the process IDs of the processes in each",
	"JOBSPEC on a single line.",
	"",
	"The '-a' option prints information about each job, and any JOBSPEC",
	"arguments are ignored.",
	"",
	"The '-g' option prints the process group for each JOBSPEC. The 'j' option",
	"prints the job identifier for each JOBSPEC using \"%N\" notation, where",
	"N is the job number. The 'p' option prints the process ID of the job's",
	"process group leader (often the same as 'g'). Only one of these three",
	"options may be used at a time.",
	"",
	"The return value is 2 if an invalid option was supplied, or more than",
	"one valid option was supplied; 1 if the 'g' option is supplied and one of",
	"the jobs is not in a separate process group; and 0 otherwise.",
	(char *)NULL
};

/* The standard structure describing a builtin command.  bash keeps an array
   of these structures.  The flags must include BUILTIN_ENABLED so the
   builtin can be used. */
struct builtin jobid_struct = {
	"jobid",		/* builtin name */
	jobid_builtin,		/* function implementing the builtin */
	BUILTIN_ENABLED,	/* initial flags for builtin */
	jobid_doc,		/* array of long documentation strings. */
	"jobid [-a] [-g|-j|-p] [jobspec...]",	/* usage synopsis; becomes short_doc */
	0			/* reserved for internal use */
};
