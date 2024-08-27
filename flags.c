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
#include "hashcmd.h"
#include "options.h"

#if defined (BANG_HISTORY)
#  include "bashhist.h"
#endif

#if defined (JOB_CONTROL)
#include "jobs.h"
#endif

/* **************************************************************** */
/*								    */
/*			The Standard sh Flags.			    */
/*								    */
/* **************************************************************** */

/* Non-zero means don't overwrite existing files while doing redirections. */
int noclobber = 0;

/* Non-zero means type out the command definition after reading, but
   before executing. */
int echo_command_at_execute = 0;

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

/* Non-zero means that we allow comments to appear in interactive commands. */
int interactive_comments = 1;

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
  { 'P', &no_symbolic_links },
  { 'T', &function_trace_mode },
  {0}
};

#define NUM_SHELL_FLAGS (sizeof (shell_flags) / sizeof (struct flags_alist) - 1)

static const char opt_letters[] = "xCEPT"
#if 0
                           "l"
#endif
#if defined (BRACE_EXPANSION)
                           "B"
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
