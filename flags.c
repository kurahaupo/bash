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

#include "builtins/common.h"
#include "execute_cmd.h"
#include "flags.h"
#include "options.h"

#if defined (BANG_HISTORY)
#  include "bashhist.h"
#endif

#if defined (JOB_CONTROL)
#include "jobs.h"
extern int set_job_control (int);
#endif

/* **************************************************************** */
/*								    */
/*			The Standard sh Flags.			    */
/*								    */
/* **************************************************************** */


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

/* Non-zero means that shell functions inherit the DEBUG trap. */
int function_trace_mode = 0;

/* Non-zero means that shell functions inherit the ERR trap. */
int error_trace_mode = 0;

/* Non-zero means that the rightmost non-zero exit status in a pipeline
   is the exit status of the entire pipeline.  If each processes exits
   with a 0 status, the status of the pipeline is 0. */
int pipefail_opt = 0;

/* **************************************************************** */
/*								    */
/*			The Flags ALIST.			    */
/*								    */
/* **************************************************************** */

const struct flags_alist shell_flags[] = {
  /* Standard sh flags. */
  { 'e', &errexit_flag },
  { 'f', &disallow_filename_globbing },
  { 'h', &hashing_enabled },
  { 'i', &forced_interactive },
  { 'k', &place_keywords_in_env },
#if defined (JOB_CONTROL)
  { 'm', &jobs_m_flag },
#endif /* JOB_CONTROL */
  { 'n', &read_but_dont_execute },
  { 'p', &privileged_mode },
#if defined (RESTRICTED_SHELL)
  { 'r', &restricted },
#endif /* RESTRICTED_SHELL */
  { 't', &just_one_command },
  { 'u', &unbound_vars_is_error },
  { 'v', &verbose_flag },
  { 'x', &echo_command_at_execute },

  /* New flags that control non-standard things. */
#if 0
  { 'l', &lexical_scoping },
#endif
#if defined (BRACE_EXPANSION)
  { 'B', &brace_expansion },
#endif
  { 'C', &noclobber },
  { 'E', &error_trace_mode },
#if defined (BANG_HISTORY)
  { 'H', &histexp_flag },
#endif /* BANG_HISTORY */
  { 'P', &no_symbolic_links },
  { 'T', &function_trace_mode },
  {0}
};

#define NUM_SHELL_FLAGS (sizeof (shell_flags) / sizeof (struct flags_alist) - 1)

static const char opt_letters[] = "knptuvxCEPT"
#if defined (JOB_CONTROL)
                           "m"
#endif
#if defined (RESTRICTED_SHELL)
                           "r"
#endif
#if 0
                           "l"
#endif
#if defined (BRACE_EXPANSION)
                           "B"
#endif
#if defined (BANG_HISTORY)
                           "H"
#endif
			   ;

char const *
get_short_flag_names (void)
{
  return opt_letters;
}

int *
find_flag (char name)
{
  int i;
  for (i = 0; shell_flags[i].name; i++)
    {
      if (shell_flags[i].name == name)
	return (shell_flags[i].value);
    }
  return (NULL);
}

/* Change the state of a flag, and return it's original value, or return
   FLAG_ERROR if there is no flag FLAG.  ON_OR_OFF must be either
   FLAG_ON or FLAG_OFF. */
int
change_flag (char flag, char on_or_off)
{
  int *value, old_value;

  opt_def_t const *d = find_short_option (flag);
  if (d)
    {
      old_value = get_opt_value (d, Accessor (short));
      op_result_t r = set_opt_value (d, Accessor (short), flag_to_bool (on_or_off));
      if (GoodResult (r))
	return old_value;
      else
	return FLAG_ERROR;
    }

  value = find_flag (flag);

  if ((value == NULL) || (on_or_off != FLAG_ON && on_or_off != FLAG_OFF))
    return (FLAG_ERROR);

  old_value = *value;

  /* Special cases for a few flags. */
  switch (flag)
    {
#if defined (BANG_HISTORY)
    case 'H':
      /* *value = ... */
      histexp_flag = flag_to_bool (on_or_off);
      history_expansion = histexp_flag;
      if (flag_to_bool (on_or_off))
	bash_initialize_history ();
      break;
#endif

#if defined (JOB_CONTROL)
    case 'm':
      /* *value = ... */
      jobs_m_flag = flag_to_bool (on_or_off);
      set_job_control (flag_to_bool (on_or_off));
      break;
#endif /* JOB_CONTROL */

    case 'e':
      /* *value = ... */
      errexit_flag = flag_to_bool (on_or_off);
      if (builtin_ignoring_errexit == 0)
	exit_immediately_on_error = errexit_flag;
      break;

    case 'n':
      /* *value = ... */
      read_but_dont_execute = flag_to_bool (on_or_off);
      if (interactive_shell)
	read_but_dont_execute = 0;
      break;

    case 'p':
      /* *value = ... */
      privileged_mode = flag_to_bool (on_or_off);
      if (! flag_to_bool (on_or_off))
	disable_priv_mode ();
      break;

#if defined (RESTRICTED_SHELL)
    case 'r':
      /* Don't allow "set +r" in a shell which is `restricted'. */
      if (restricted && on_or_off == FLAG_OFF)
	return (FLAG_ERROR);
      /* *value = ... */
      restricted = flag_to_bool (on_or_off);
      if (on_or_off == FLAG_ON && shell_initialized)
	maybe_make_restricted (shell_name);
      break;
#endif

    case 'v':
      /* *value = ... */
      verbose_flag = flag_to_bool (on_or_off);
      echo_input_at_read = verbose_flag;
      break;

    default:
      *value = flag_to_bool (on_or_off);
      break;
    }

  return (old_value);
}

/* Return a string which is the names of all the currently
   set shell flags; this value is used for `$-`. */
char *
which_set_flags (void)
{
  char const * option_letters = get_short_opt_names ();
  size_t limit = strlen (option_letters)
	       + strlen (opt_letters);

  char *result = xmalloc (1 + limit + read_from_stdin + want_pending_command);
  size_t j = 0;

  for (size_t i = 0; option_letters[i]; i++)
    {
      char letter = option_letters[i];
      if (get_opt_value (find_short_option (letter),
			 Accessor (short)))
	result[j++] = letter;
    }
  for (const struct flags_alist *sf = shell_flags; sf->name; ++sf)
    {
      char letter = sf->name;
      if (! find_short_option (letter))
	if (sf->value[0])
	  result[j++] = letter;
    }

  if (want_pending_command)
    result[j++] = 'c';
  if (read_from_stdin)
    result[j++] = 's';

  result[j] = '\0';
  return result;
}

char *
get_current_flags (void)
{
  char const * option_letters = get_short_opt_names ();
  size_t limit = strlen (option_letters)
	       + strlen (opt_letters);

  char *bitmap = xmalloc (1 + limit);
  size_t j = 0;

  for (size_t i = 0; option_letters[i]; i++)
    bitmap[j++] = get_opt_value (find_short_option (option_letters[i]),
				 Accessor (unwind));
  for (const struct flags_alist *sf = shell_flags; sf->name; ++sf)
    if (! find_short_option (sf->name))
      bitmap[j++] = sf->value[0];
  bitmap[j] = '\0';	// XXX probably unnecessary
  return bitmap;
}

void
set_current_flags (const char *bitmap)
{
  if (bitmap == 0)
    return;

  char const * option_letters = get_short_opt_names ();
  int j = 0;

  for (size_t i = 0; option_letters[i]; i++)
    set_opt_value (find_short_option (option_letters[i]),
		   Accessor (unwind), bitmap[j++]);
  for (const struct flags_alist *sf = shell_flags; sf->name; ++sf)
    if (! find_short_option (sf->name))
      sf->value[0] = bitmap[j++];
}

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

void
register_flags_opts (void)
{
}
