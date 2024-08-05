/* flags.c -- Everything about flags except the `set' command.  That
   is in builtins.c */

/* Copyright (C) 1987-2022 Free Software Foundation, Inc.

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
#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#ifndef NDEBUG
#  include <stdio.h>
#endif

#include "shell.h"
#include "execute_cmd.h"
#include "flags.h"

#if defined (BANG_HISTORY)
#  include "bashhist.h"
#endif

/* **************************************************************** */
/*								    */
/*			The Standard sh Flags.			    */
/*								    */
/* **************************************************************** */

/* Non-zero means automatically mark variables which are modified or created
   as auto export variables. */
int mark_modified_vars = 0;

/* Non-zero causes asynchronous job notification.  Otherwise, job state
   notification only takes place just before a primary prompt is printed. */
int asynchronous_notification = 0;

/* Non-zero means exit immediately if a command exits with a non-zero
   exit status.  The first is what controls set -e; the second is what
   bash uses internally. */
int errexit_flag = 0;
int exit_immediately_on_error = 0;

/* Non-zero means disable filename globbing. */
int disallow_filename_globbing = 0;

/* Non-zero means that all keyword arguments are placed into the environment
   for a command, not just those that appear on the line before the command
   name. */
int place_keywords_in_env = 0;

/* Non-zero means read commands, but don't execute them.  This is useful
   for debugging shell scripts that should do something hairy and possibly
   destructive. */
int read_but_dont_execute = 0;

/* Non-zero means end of file is after one command. */
int just_one_command = 0;

/* Non-zero means don't overwrite existing files while doing redirections. */
int noclobber = 0;

/* Non-zero means trying to get the value of $i where $i is undefined
   causes an error, instead of a null substitution. */
int unbound_vars_is_error = 0;

/* Non-zero means type out input lines after you read them. */
int echo_input_at_read = 0;
int verbose_flag = 0;

/* Non-zero means type out the command definition after reading, but
   before executing. */
int echo_command_at_execute = 0;

/* Non-zero means turn on the job control features. */
int jobs_m_flag = 0;

/* Non-zero means this shell is interactive, even if running under a
   pipe. */
int forced_interactive = 0;

/* By default, follow the symbolic links as if they were real directories
   while hacking the `cd' command.  This means that `cd ..' moves up in
   the string of symbolic links that make up the current directory, instead
   of the absolute directory.  The shell variable `nolinks' also controls
   this flag. */
int no_symbolic_links = 0;

/* **************************************************************** */
/*								    */
/*		     Non-Standard Flags Follow Here.		    */
/*								    */
/* **************************************************************** */

#if 0
/* Non-zero means do lexical scoping in the body of a FOR command. */
int lexical_scoping = 0;
#endif

/* Non-zero means look up and remember command names in a hash table, */
int hashing_enabled = 1;

#if defined (BANG_HISTORY)
/* Non-zero means that we are doing history expansion.  The default.
   This means !22 gets the 22nd line of history. */
int history_expansion = HISTEXPAND_DEFAULT;
int histexp_flag = 0;
#endif /* BANG_HISTORY */

/* Non-zero means that we allow comments to appear in interactive commands. */
int interactive_comments = 1;

#if defined (RESTRICTED_SHELL)
/* Non-zero means that this shell is `restricted'.  A restricted shell
   disallows: changing directories, command or path names containing `/',
   unsetting or resetting the values of $PATH and $SHELL, and any type of
   output redirection. */
int restricted = 0;		/* currently restricted */
int restricted_shell = 0;	/* shell was started in restricted mode. */
#endif /* RESTRICTED_SHELL */

/* Non-zero means that this shell is running in `privileged' mode.  This
   is required if the shell is to run setuid.  If the `-p' option is
   not supplied at startup, and the real and effective uids or gids
   differ, disable_priv_mode is called to relinquish setuid status. */
int privileged_mode = 0;

#if defined (BRACE_EXPANSION)
/* Zero means to disable brace expansion: foo{a,b} -> fooa foob */
int brace_expansion = 1;
#endif

/* Are we currently ignoring the -e option for the duration of a builtin's
   execution? */
int builtin_ignoring_errexit = 0;

/* Non-zero means that shell functions inherit the DEBUG trap. */
int function_trace_mode = 0;

/* Non-zero means that shell functions inherit the ERR trap. */
int error_trace_mode = 0;

/* Non-zero means that the rightmost non-zero exit status in a pipeline
   is the exit status of the entire pipeline.  If each processes exits
   with a 0 status, the status of the pipeline is 0. */
int pipefail_opt = 0;

void
reset_shell_flags (void)
{
  mark_modified_vars = disallow_filename_globbing = 0;
  place_keywords_in_env = read_but_dont_execute = just_one_command = 0;
  noclobber = unbound_vars_is_error = 0;
  echo_command_at_execute = jobs_m_flag = forced_interactive = 0;
  no_symbolic_links = 0;
  privileged_mode = pipefail_opt = 0;

  error_trace_mode = function_trace_mode = 0;

  exit_immediately_on_error = errexit_flag = 0;
  echo_input_at_read = verbose_flag = 0;

  hashing_enabled = interactive_comments = 1;

#if defined (JOB_CONTROL)
  asynchronous_notification = 0;
#endif

#if defined (BANG_HISTORY)
  histexp_flag = 0;
#endif

#if defined (BRACE_EXPANSION)
  brace_expansion = 1;
#endif

#if defined (RESTRICTED_SHELL)
  restricted = 0;
#endif
}

#ifndef NDEBUG
void
FailedValidation (char const *file, unsigned int line,
		  int got_value, char const *expected_description,
		  char const *expr_str)
{
  fprintf (stderr,
	   "\n\n"
	   "FATAL ERROR at line %u in %s\n"
	   "Attempt to pass value %d where %s was expected\n"
	   "using expression \"%s\"\n"
	   "\n", line, file, got_value, expected_description, expr_str);
  fflush (stderr);
  abort ();
}
#endif

void
initialize_flags (void)
{
}
