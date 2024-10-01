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

#include "bashintl.h"
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

/* Non-zero means disable filename globbing. */
int disallow_filename_globbing = 0;
static opt_def_t const OPTDEF_disallow_filename_globbing = {
  .store = &disallow_filename_globbing,
  .OPTRESET_false,
  .letter = 'f',
  .name = "noglob",
  .adjust_shellopts = true,
  .hide_shopt = true,
  .help = N_(
    "Disable file name generation (globbing)."),
};

/* Non-zero means that all keyword arguments are placed into the environment
   for a command, not just those that appear on the line before the command
   name. */
int place_keywords_in_env = 0;
static opt_def_t const OPTDEF_place_keywords_in_env = {
  .store = &place_keywords_in_env,
  .OPTRESET_false,
  .letter = 'k',
  .name = "keyword",
  .adjust_shellopts = true,
  .hide_shopt = true,
  .help = N_(
    "All assignment arguments are placed in the environment for a\n"
    "command, not just those that precede the command name."),
};

/* Non-zero means read commands, but don't execute them.  This is useful
   for debugging shell scripts that should do something hairy and possibly
   destructive. */
int read_but_dont_execute = 0;
static op_result_t
set_read_but_dont_execute (opt_def_t const *d,
			   accessor_t why,
			   option_value_t new_value)
{
  /* The `noexec` option is a trapdoor; once it's on, there's no way to invoke
     a command to turn it off. So this code ignores attempts to turn `noexec`
     on when in interactive mode, as it would result in the user being unable
     to invoke `exit` or `logout`.

     Note that it can be turned off by returning from a function when `local -`
     is in effect.
  */
  #if 0
  /* TODO decide whether the following is desirable.
     The original code would always turn noexec off if the shell was
     interactive, even when attempting to turn it on, and even if it was
     already on.
     That pre-supposes that this code could be reached when noexec was already
     on, which was not possible when the code was written.
     */
  read_but_dont_execute = new_value;
  if (interactive_shell)
    read_but_dont_execute = 0;
  #endif
  if (interactive_shell && new_value)
    return Result (Ignored);
  read_but_dont_execute = new_value;
  return Result (OK);
}
static opt_def_t const OPTDEF_read_but_dont_execute = {
  .store = &read_but_dont_execute,
  .OPTRESET_false,
  .set_func = set_read_but_dont_execute,
  .letter = 'n',
  .name = "noexec",
  .adjust_shellopts = true,
  .hide_shopt = true,
  .help = N_(
    "Read commands but do not execute them."),
};


/* Non-zero means end of file is after one command. */
int just_one_command = 0;
static opt_def_t const OPTDEF_just_one_command = {
  .store = &just_one_command,
  .OPTRESET_false,
  .letter = 't',
  .name = "onecmd",
  .adjust_shellopts = true,
  .hide_shopt = true,
  .help = N_(
    "Exit after reading and executing one command."),
};

/* Non-zero means don't overwrite existing files while doing redirections. */
int noclobber = 0;
static opt_def_t const OPTDEF_noclobber = {
  .store = &noclobber,
  .OPTRESET_false,
  .letter = 'C',
  .name = "noclobber",
  .adjust_shellopts = true,
  .hide_shopt = true,
  .help = N_(
    "If set, prevent existing regular files from being truncated or\n"
    "overwritten by redirected output."),
};

/* Non-zero means trying to get the value of $i where $i is undefined
   causes an error, instead of a null substitution. */
int unbound_vars_is_error = 0;
static opt_def_t const OPTDEF_unbound_vars_is_error = {
  .store = &unbound_vars_is_error,
  .OPTRESET_false,
  .letter = 'u',
  .name = "nounset",
  .adjust_shellopts = true,
  .hide_shopt = true,
  .help = N_(
    "Treat unset variables as an error when substituting."),
};

/* Non-zero means type out input lines after you read them. */
int echo_input_at_read = 0;
int verbose_flag = 0;
static op_result_t
set_verbose_flag (struct opt_def_s const *d, accessor_t why, option_value_t new_value)
{
  echo_input_at_read = verbose_flag = new_value;
  return Result (OK);
}
static opt_def_t const OPTDEF_verbose_flag = {
  .store = &verbose_flag,
  .OPTRESET_false,
  .set_func = set_verbose_flag,
  .letter = 'v',
  .name = "verbose",
  .adjust_shellopts = true,
  .hide_shopt = true,
  .help = N_(
    "Print shell input lines as they are read."),
};

/* Non-zero means type out the command definition after reading, but
   before executing. */
int echo_command_at_execute = 0;
static opt_def_t const OPTDEF_echo_command_at_execute = {
  .store = &echo_command_at_execute,
  .OPTRESET_false,
  .letter = 'x',
  .name = "xtrace",
  .adjust_shellopts = true,
  .hide_shopt = true,
  .help = N_(
    "Print commands and their arguments as they are executed."),
};

#if defined (JOB_CONTROL)
/* Non-zero means turn on the job control features. */
int jobs_m_flag = 0;
static op_result_t
set_jobs_m_flag (opt_def_t const *d, accessor_t why, option_value_t new_value)
{
  jobs_m_flag = new_value;
  set_job_control (new_value);
  return Result (OK);
}
static opt_def_t const OPTDEF_jobs_m_flag = {
  .store = &jobs_m_flag,
  .OPTRESET_false,
  .direct_reset = true,	/* avoid set_job_control */
  .set_func = set_jobs_m_flag,
  .letter = 'm',
  .name = "monitor",
  .adjust_shellopts = true,
  .hide_shopt = true,
  .help = N_(
    "Job control is enabled."),
};
#endif

/* Non-zero means this shell is interactive, even if running under a
   pipe. */
int forced_interactive = 0;
static opt_def_t const OPTDEF_forced_interactive = {
  .store = &forced_interactive,
  .OPTRESET_REINIT_false,
  .letter = 'i',
  .name = "interactive",
  .adjust_shellopts = true,
  .hide_shopt = true,
  .forbid_change = true,
  .help = N_(
    "(This option is read-only)\n"
    "Bash automatically enters interactive mode if it is started without\n"
    "a script to read or can only be enabled or disabled at start-up.\n"),
};

/* By default, follow the symbolic links as if they were real directories
   while hacking the `cd' command.  This means that `cd ..' moves up in
   the string of symbolic links that make up the current directory, instead
   of the absolute directory.  The shell variable `nolinks' also controls
   this flag. */
int no_symbolic_links = 0;
static opt_def_t const OPTDEF_no_symbolic_links = {
  .store = &no_symbolic_links,
  .OPTRESET_false,
  .letter = 'P',
  .name = "physical",
  .adjust_shellopts = true,
  .hide_shopt = true,
  .help = N_(
    "If set, do not resolve symbolic links when executing commands\n"
    "such as cd which change the current directory."),
};

/* **************************************************************** */
/*								    */
/*		     Non-Standard Flags Follow Here.		    */
/*								    */
/* **************************************************************** */

#if 0
/* Non-zero means do lexical scoping in the body of a FOR command. */
int lexical_scoping = 0;
static opt_def_t const OPTDEF_lexical_scoping = {
  .store = &lexical_scoping,
  .letter = 'l',
  .name = "lexical",
  .adjust_shellopts = true,
  .hide_shopt = true,
  .help = N_(
    "(This option is an illusion; it does not actually exist yet.\n"
    "Some day we hope to have actual lexical scoping in the shell.)"),
};
#endif

/* Non-zero means look up and remember command names in a hash table, */
int hashing_enabled = 1;
static opt_def_t const OPTDEF_hashing_enabled = {
  .store = &hashing_enabled,
  .OPTRESET_true,
  .letter = 'h',
  .name = "hashall",
  .adjust_shellopts = true,
  .hide_shopt = true,
  .help = N_(
    "Remember the location of commands as they are looked up."),
};

#if defined (BANG_HISTORY)
/* Non-zero means that we are doing history expansion.  The default.
   This means !22 gets the 22nd line of history. */
int history_expansion = HISTEXPAND_DEFAULT;
int histexp_flag = 0;
static opt_set_func_t set_histexp_flag;
static op_result_t
set_histexp_flag (opt_def_t const *d, accessor_t why, int new_value)
{
  history_expansion = histexp_flag = new_value;
  if (new_value)
    bash_initialize_history ();
  return Result (OK);
}
static opt_def_t const OPTDEF_histexp_flag = {
  .store = &histexp_flag,
  .OPTRESET_false,
  .set_func = set_histexp_flag,
  .letter = 'H',
  .name = "histexpand",
  .adjust_shellopts = true,
  .hide_shopt = true,
  .help = N_(
    "Enable ! style history substitution.  This flag is on\n"
    "by default when the shell is interactive."),
};
#endif

/* Non-zero means that we allow comments to appear in interactive commands. */
int interactive_comments = 1;

#if defined (RESTRICTED_SHELL)
/* Non-zero means that this shell is `restricted'.  A restricted shell
   disallows: changing directories, command or path names containing `/',
   unsetting or resetting the values of $PATH and $SHELL, and any type of
   output redirection. */
int restricted = 0;		/* currently restricted */
int restricted_shell = 0;	/* shell was started in restricted mode. */
static op_result_t
set_restricted (struct opt_def_s const *d, accessor_t why, option_value_t new_value)
{
  /* Don't allow `set +r` or `set +o restrict` in a shell which is
   * "restricted", but do allow `local -` to unwind `set -r`. */
  if (restricted && !new_value && ! AccessorIsPrivileged (why))
    return Result (Forbidden);
  restricted = new_value;
  if (new_value && shell_initialized)
    maybe_make_restricted (shell_name);
  return Result (OK);
}
static opt_def_t const OPTDEF_restricted = {
  .store = &restricted,
  .OPTRESET_REINIT_false,
  .set_func = set_restricted,
  .letter = 'r',
  .name = "restricted",
  .adjust_shellopts = true,
  .hide_shopt = true,
  .help = N_(
    "If bash is started with the name rbash, or the -r option is supplied at\n"
    "invocation, the shell becomes restricted. This cannot be undone."),
};
#endif

/* Non-zero means that this shell is running in `privileged' mode.  This
   is required if the shell is to run setuid.  If the `-p' option is
   not supplied at startup, and the real and effective uids or gids
   differ, disable_priv_mode is called to relinquish setuid status. */
int privileged_mode = 0;
static op_result_t
set_privileged_mode (struct opt_def_s const *d, accessor_t why, option_value_t new_value)
{
  privileged_mode = new_value;
  if (! new_value)
    disable_priv_mode ();
  return Result (OK);
}
static opt_def_t const OPTDEF_privileged_mode = {
  .store = &privileged_mode,
  .OPTRESET_false,
  .direct_reset = true,	/* avoid disable_priv_mode */
  .set_func = set_privileged_mode,
  .letter = 'p',
  .name = "privileged",
  .adjust_shellopts = true,
  .hide_shopt = true,
  .help = N_(
    "Turned on whenever the real and effective user ids do not match.\n"
    "Disables processing of the $ENV file and importing of shell\n"
    "functions.  Turning this option off causes the effective uid and\n"
    "gid to be set to the real uid and gid."),
};

#if defined (BRACE_EXPANSION)
/* Zero means to disable brace expansion: foo{a,b} -> fooa foob */
int brace_expansion = 1;
static opt_def_t const OPTDEF_brace_expansion = {
  .store = &brace_expansion,
  .OPTRESET_true,
  .letter = 'B',
  .name = "braceexpand",
  .adjust_shellopts = true,
  .hide_shopt = true,
  .help = N_(
    "Perform brace expansion prior to expanding variables."),
};
#endif

/* Non-zero means that shell functions inherit the DEBUG trap. */
int function_trace_mode = 0;
static opt_def_t const OPTDEF_function_trace_mode = {
  .store = &function_trace_mode,
  .OPTRESET_false,
  .letter = 'T',
  .name = "functrace",
  .adjust_shellopts = true,
  .hide_shopt = true,
  .help = N_(
    "Shell functions inherit the DEBUG and RETURN traps."),
};

/* Non-zero means that shell functions inherit the ERR trap. */
int error_trace_mode = 0;
static opt_def_t const OPTDEF_error_trace_mode = {
  .store = &error_trace_mode,
  .OPTRESET_false,
  .letter = 'E',
  .name = "errtrace",
  .adjust_shellopts = true,
  .hide_shopt = true,
  .help = N_(
    "Shell functions inherit the ERR trap."),
};

/* Non-zero means that the rightmost non-zero exit status in a pipeline
   is the exit status of the entire pipeline.  If each processes exits
   with a 0 status, the status of the pipeline is 0. */
int pipefail_opt = 0;

/* **************************************************************** */
/*								    */
/*			The Flags ALIST.			    */
/*								    */
/* **************************************************************** */

/* Change the state of a flag, and return it's original value, or return
   FLAG_ERROR if there is no flag FLAG.  ON_OR_OFF must be either
   FLAG_ON or FLAG_OFF. */
int
change_flag (char flag, char on_or_off)
{
  opt_def_t const *d = find_short_option (flag);
  if (! d)
    return FLAG_ERROR;

  int old_value = get_opt_value (d, Accessor (short));
  op_result_t r = set_opt_value (d, Accessor (short), flag_to_bool (on_or_off));
  if (GoodResult (r))
    return old_value;
  else
    return FLAG_ERROR;
}

/* Return a string which is the names of all the currently
   set shell flags; this value is used for `$-`. */
char *
which_set_flags (void)
{
  char const * option_letters = get_short_opt_names ();
  size_t limit = strlen (option_letters);

  char *result = xmalloc (1 + limit + read_from_stdin + want_pending_command);
  size_t j = 0;

  for (size_t i = 0; option_letters[i]; i++)
    {
      char letter = option_letters[i];
      if (get_opt_value (find_short_option (letter),
			 Accessor (short)))
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
  size_t limit = strlen (option_letters);

  char *bitmap = xmalloc (1 + limit);
  size_t j = 0;

  for (size_t i = 0; option_letters[i]; i++)
    bitmap[j++] = get_opt_value (find_short_option (option_letters[i]),
				 Accessor (unwind));
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
  register_option (&OPTDEF_error_trace_mode);		/* ±E, ±o errtrace     */
  register_option (&OPTDEF_function_trace_mode);	/* ±T, ±o functrace    */
  register_option (&OPTDEF_hashing_enabled);		/* ±h, ±o hashall      */
  register_option (&OPTDEF_forced_interactive);		/* ±i, ±o interactive  */
  register_option (&OPTDEF_place_keywords_in_env);	/* ±k, ±o keyword      */
  register_option (&OPTDEF_jobs_m_flag);		/* ±m, ±o monitor      */
  register_option (&OPTDEF_noclobber);			/* ±C, ±o noclobber    */
  register_option (&OPTDEF_read_but_dont_execute);	/* ±n, ±o noexec       */
  register_option (&OPTDEF_disallow_filename_globbing);	/* ±f, ±o noglob       */
  register_option (&OPTDEF_unbound_vars_is_error);	/* ±u, ±o nounset      */
  register_option (&OPTDEF_just_one_command);		/* ±t, ±o onecmd       */
  register_option (&OPTDEF_no_symbolic_links);		/* ±P, ±o physical     */
  register_option (&OPTDEF_privileged_mode);		/* ±p, ±o privileged"  */
  register_option (&OPTDEF_restricted);			/* ±r, ±o restricted   */
  register_option (&OPTDEF_verbose_flag);		/* ±v, ±o verbose      */
  register_option (&OPTDEF_echo_command_at_execute);	/* ±x, ±o xtrace       */

#if defined (BRACE_EXPANSION)
  register_option (&OPTDEF_brace_expansion);		/* ±B, ±o braceexpand  */
#endif
#if defined (BANG_HISTORY)
  register_option (&OPTDEF_histexp_flag);		/* ±H, ±o histexpand   */
#endif
#if 0
  register_option (&OPTDEF_lexical_scoping);		/* ±l, ±o lexical      */
#endif
}
