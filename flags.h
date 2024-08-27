/* flags.h -- a list of all the flags that the shell knows about.  You add
   a flag to this program by adding the name here, and in flags.c. */

/* Copyright (C) 1993-2022 Free Software Foundation, Inc.

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

#if !defined (_FLAGS_H_)
#define _FLAGS_H_

#include "stdc.h"

extern void register_flags (void);

/* Welcome to the world of Un*x, where everything is slightly backwards. */
#define FLAG_ON '-'
#define FLAG_OFF '+'

#define FLAG_ERROR -1

extern char const *get_opt_letters (void);

static inline char
bool_to_flag (_Bool b)
{
  return b ? FLAG_ON : FLAG_OFF;
}

static inline _Bool
flag_to_bool (char f)
{
  return f == FLAG_ON;
}

static inline _Bool
valid_flag (char f)
{
  return f == FLAG_ON || f == FLAG_OFF;
}

static inline _Bool
valid_flag_or_error (char f)
{
  return f == FLAG_ON || f == FLAG_OFF || f == FLAG_ERROR;
}

/*
 * Validate that bools and flags are present when expected
 *
 * Usage:
 *
 *      #define your_func(     boolarg,     flagarg,  otherarg) \
 *              your_func_ (VB(boolarg), VF(flagarg), otherarg)
 *      extern void your_func_ (_Bool b, char f, int o);
 *
 * If you don't need to forward-declare your function, you can avoid having
 * separate name for for your_func and your_func_, by placing the #define
 * after the end of the function definition:
 *
 *      static void
 *      your_func (_Bool b, char f, int o)
 *      {
 *        [... function body here ...]
 *      }
 *      #define your_func(    flagarg,     boolarg,  otherarg) \
 *              your_func (VF(flagarg), VB(boolarg), otherarg)
 *
 * The drawback of using a single name is that you won't be alerted if the
 * #define is not in scope, so it's really only suitable for static functions.
 *
 * If you intend to accept FLAG_ERROR, use VFE rather than VF, and declare the
 * parameter type as `int` rather than `char`.
 */
#ifdef NDEBUG

# define VB(X) (X)
# define VF(X) (X)
# define VFE(X) (X)

#else

extern void FailedValidation (char const *file, unsigned int line,
			      int got_value, char const *expected_description,
			      char const *expr_str);

static inline int
ValidateBool (char const *file, unsigned int line, char const *expr_str, int X)
{
  if (X == 0 || X == 1)
    return X;
  FailedValidation (file, line, X, "a bool (0 or 1)", expr_str);
}
# define VB(X) ValidateBool (__FILE__, __LINE__, #X, (X))

static inline int
ValidateFlag (char const *file, unsigned int line, char const *expr_str, int X)
{
  if (valid_flag (X))
    return X;
  FailedValidation (file, line, X, "a flag ('+' or '-')", expr_str);
}
# define VF(X) ValidateFlag (__FILE__, __LINE__, #X, (X))

static inline int
ValidateFlagOrError (char const *file, unsigned int line, char const *expr_str, int X)
{
  if (valid_flag_or_error (X))
    return X;
  FailedValidation (file, line, X, "a flag ('+' or '-') or error (-1)", expr_str);
}
# define VFE(X) ValidateFlagOrError (__FILE__, __LINE__, #X, (X))

#define bool_to_flag(b) bool_to_flag (VB (b))
#define flag_to_bool(f) flag_to_bool (VF (f))

#endif


extern int
  just_one_command, unbound_vars_is_error, echo_input_at_read, verbose_flag,
  echo_command_at_execute, noclobber,
  privileged_mode,
  interactive_comments, no_symbolic_links,
  function_trace_mode, error_trace_mode, pipefail_opt;

/* -c, -s invocation options -- not really flags, but they show up in $- */
extern int want_pending_command, read_from_stdin;

#if 0
extern int lexical_scoping;
#endif

#if defined (BRACE_EXPANSION)
extern int brace_expansion;
#endif

#if defined (RESTRICTED_SHELL)
extern int restricted;
extern int restricted_shell;
#endif /* RESTRICTED_SHELL */

extern int change_flag (char, char);
extern char *which_set_flags (void);

extern char *get_current_flags (void);
extern void set_current_flags (const char *);

extern void initialize_flags (void);
extern void register_flags_opts (void);

/* A macro for efficiency. */
#define change_flag_char(flag, on_or_off)  change_flag (flag, on_or_off)

#endif /* _FLAGS_H_ */
