/* options.c -- Functions for hacking shell options. */

/* Copyright (C) 2024-2024 Free Software Foundation, Inc.

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

#include <sys/types.h>

#include "bashintl.h"
#include "error.h"
#include "flags.h"
#include "shell.h"

#include "options.h"

/******************************************************************************/

/* Function called when one of the builtin commands detects an invalid
   option. */

static void
warn_invalidopt (const char *s)
{
  if (*s == '-' || *s == '+')
    report_error (_("%s: invalid option"), s);
  else
    report_error (_("%s: invalid option name"), s);
}

/******************************************************************************/

static const int result_to_ex_map[] = {
  [Result_OK]		= EXECUTION_SUCCESS,
  [Result_Unchanged]	= EXECUTION_SUCCESS,	/* New value is same as old value */
  [Result_Ignored]	= EXECUTION_SUCCESS,	/* Change request ignored, silently */
  [Result_NotFound]	= EX_BADUSAGE,		/* No setting with the supplied letter or name */
  [Result_ReadOnly]	= EX_BADUSAGE,		/* Change never possible */
  [Result_Forbidden]	= EX_BADASSIGN,		/* Not changed because new value is permitted */
  [Result_BadValue]	= EX_BADASSIGN,		/* Not changed because new value not valid */
};

int res_to_ex(op_result_t res)
{
  enum op_result_e r = ResultC (res);
  if (r < Result_OK
   || r >= sizeof result_to_ex_map / sizeof *result_to_ex_map)
    return -1;
  return result_to_ex_map[r];
}

/******************************************************************************/

void set_shellopts (void);
void set_bashopts (void);

option_value_t
get_opt_value (opt_def_t const *d, option_class_t w)
{
  if (!d)
    return OPTION_INVALID_VALUE;
  if (d->get_func)
    return d->get_func(d, w);
  if (d->store)
    return d->store[0];
  return OPTION_INVALID_VALUE;
}

op_result_t
set_opt_value (opt_def_t const *d,
	       option_class_t w,
	       option_value_t new_value)
{
  if (!d)
    return Result (NotFound);

  if (d->set_func)
    {
      op_result_t r = d->set_func(d, w, new_value);
      if (ResultC (r) == Result_OK)
	{
	  /* only for exactly Result_OK; "not changed" or "ignored" should not trigger this */
	  if (d->adjust_shellopts) set_shellopts ();
	  if (d->adjust_bashopts) set_bashopts ();
	}
      return r;
    }

  if (d->readonly)
    return Result (ReadOnly);
  if (d->forbid_change && OptionClassC (w) != OC_argv		/* set up from command line, but don't change thereafter */
		       && OptionClassC (w) != OC_environ)	/* read from initial environment, but don't change thereafter */
    {
      option_value_t old_value = get_opt_value (d, w);
      if (new_value == old_value)
	return Result (Unchanged);
      else if (d->ignore_change)
	return Result (Ignored);
      else
	return Result (Forbidden);
    }
  if (d->ignore_change)
    return Result (Ignored);

  if (d->store)
    d->store[0] = new_value;
  if (d->adjust_shellopts) set_shellopts ();
  if (d->adjust_bashopts) set_bashopts ();
  return Result (OK);
}

/******************************************************************************/

static struct od_vec_s {
  const opt_def_t **defs;
  size_t cap;
  size_t len;
  size_t unnamed_count;
} Opts = {0};

#define for_each_opt(i) for (size_t i = 0; i < Opts.len; ++i)
#define each_opt(i,d) \
		      opt_def_t const *d = Opts.defs[i]; \
		      if (!d) continue

typedef struct search_result {
  size_t  index;
  _Bool	  match;
} search_result_t;

static search_result_t
search_options(char const *name)
{
  ssize_t left = 0,
	  right = Opts.len - Opts.unnamed_count - 1;
  for (; left < right ;)
  {
    size_t mid = (left+right+1)>>1;
    int res = strcmp (name, Opts.defs[mid]->name);
    if (res < 0)
      right = mid-1;
    else if (res > 0)
      left = mid+1;
    else
      return (search_result_t){ .match = 1, .index = mid };
  }
  return (search_result_t){ .match = 0, .index = left };
}

opt_def_t const *
find_option (char const *name)
{
#if 0
  for_each_opt(i)
    {
      each_opt(i,d);
      if (d->name != NULL && !strcmp (name, d->name))
	return d;
    }
#else
  search_result_t S = search_options(name);
  if (S.match)
    return Opts.defs[S.index];
#endif
  return NULL;
}

/*************************************/

#define MAX_SHORT_NAMES	(1+CHAR_MAX)
static const opt_def_t *letter_map[MAX_SHORT_NAMES];

static size_t letter_map_min;
static size_t letter_map_max;	/* doubles as a "has been init" flag */

static char const *short_opt_names = NULL;
static size_t N_short_names = 0;

static void invalidate_short_opt_names (void)
{
    if (short_opt_names) xfree (short_opt_names);	/* avoid leak if we're called more than once */
    short_opt_names = NULL;
}

static void
init_letter_map (void)
{
  /* Set up only once; this will have to change if we allow loadables to register new options */
  if (letter_map_max == 0) {
    memset(letter_map, 0, sizeof letter_map);
    letter_map_min = ~0ULL;
    letter_map_max = 0U;

    N_short_names = 0;

    for_each_opt(i)
      {
	each_opt(i, d);

	char c = d->letter;
	if (c <= 0 || c >= MAX_SHORT_NAMES)
	  continue;

	if (!letter_map[c])
	{
	  ++N_short_names;
	  invalidate_short_opt_names ();
	}

	letter_map[c] = d;

	if (c < letter_map_min) letter_map_min = c;
	if (c > letter_map_max) letter_map_max = c;
      }
  }
}

opt_def_t const *
find_short_option (char letter)
{
  init_letter_map ();
  return letter_map[letter];
}

const char *get_short_opt_names(void)
{
  init_letter_map ();
  if (!short_opt_names)
    {
      char *ep = xmalloc (N_short_names+1);
      short_opt_names = ep;
      for (size_t c = letter_map_min; c <= letter_map_max; ++c)
	if (letter_map[c])
	  *ep++ = c;
      *ep = 0;
    }
  return short_opt_names;
}

/**************************************/

op_result_t
register_option(opt_def_t const *def)
{
  if (Opts.len >= Opts.cap)
    {
      /* Make more room ... */
      Opts.cap |= Opts.cap << 1
		| Opts.len + 1;
      size_t new_size = Opts.cap * sizeof *Opts.defs;
      if (Opts.defs)
	Opts.defs = xrealloc(Opts.defs, new_size);
      else
	Opts.defs = xmalloc(new_size);
    }
  if (!def->name)
    {
      ++Opts.unnamed_count;
      Opts.defs[Opts.len++] = def;
    }
  else
    {
      search_result_t S = search_options(def->name);
      if (S.match)
	{
	  opt_def_t const *old = Opts.defs[S.index];
	  if (old == def)
	    return Result (Unchanged);  /* re-adding the same entry */
	  if (old->letter && letter_map[old->letter] == old)
	    {
	      --N_short_names;
	      letter_map[old->letter] = NULL;
	      invalidate_short_opt_names ();
	    }
	}
      else
	{
	  if (Opts.len > S.index)
	    memmove (Opts.defs+S.index+1,
		     Opts.defs+S.index,
		     (Opts.len-S.index) * sizeof *Opts.defs);
	  ++Opts.len;
	}
      Opts.defs[S.index] = def;
    }
  char c = def->letter;
  if (c)
    {
      if (!letter_map[c])
      {
	++N_short_names;
	invalidate_short_opt_names ();
      }
      letter_map[c] = def;
      invalidate_short_opt_names ();
    }
  return Result (OK);
}

/******************************************************************************/

static _Bool
hide_when_shopt (opt_def_t const *d)
{
  return d->hide_shopt;
}

static _Bool
hide_when_short (opt_def_t const *d)
{
  return d->letter == 0;
}

static _Bool
hide_when_set_o (opt_def_t const *d)
{
  return d->hide_set_o;
}

static _Bool (*hide_when_map[])(opt_def_t const *d) = {
  [OC_short] = hide_when_short,
  [OC_set_o] = hide_when_set_o,
  [OC_shopt] = hide_when_shopt,
};

typedef void show_func_t (opt_def_t const *d, option_class_t whence_selection);

static show_func_t show_option_default;
static show_func_t show_option_shopt;
static show_func_t show_option_short;
static show_func_t show_option_set_o;

static void
show_option_default (opt_def_t const *d, option_class_t whence_selection)
{
  printf ("%-15s\t%s\n", d->name, get_opt_value (d, whence_selection) ? "on" : "off");
}

static void
show_option_shopt (opt_def_t const *d, option_class_t whence_selection)
{
  printf ("shopt -%c %s\n", get_opt_value (d, whence_selection) ? 's' : 'u', d->name);
}

static void
show_option_short (opt_def_t const *d, option_class_t whence_selection)
{
  printf ("set %c%c\n", bool_to_flag (get_opt_value (d, whence_selection)), d->letter);
}

static void
show_option_set_o (opt_def_t const *d, option_class_t whence_selection)
{
  printf ("set %co %s\n", bool_to_flag (get_opt_value (d, whence_selection)), d->name);
}

static show_func_t *show_map[] = {
  [DS_std]   = show_option_default,
  [DS_short] = show_option_short,
  [DS_set_o] = show_option_set_o,
  [DS_shopt] = show_option_shopt,
};

static inline show_func_t *
get_show(display_style_t display_style)
{
    int v = DisplayStyleC (display_style);
    if (v < 0 || v >= sizeof show_map / sizeof *show_map)
      return NULL;
    return show_map[v];
}

void
show_one_option (opt_def_t const *d,
		 option_class_t oc,
		 display_style_t display_style)
{
  get_show(display_style)(d, oc);
}

void
show_one_option_unless_value (opt_def_t const *d,
			      option_class_t oc,
			      uintmax_t hide_value_mask,
			      display_style_t display_style)
{
  if (hide_value_mask & ((uintmax_t)1U << get_opt_value (d, oc)))
    get_show (display_style)(d, oc);
}

void
list_all_options (option_class_t whence_selection,
		  unsigned hide_value_mask,
		  display_style_t display_style)
{
  _Bool (*hide_when)(opt_def_t const *d) =  hide_when_map[OptionClassC (whence_selection)];
  show_func_t *show_how = get_show (display_style);

  for_each_opt(i)
    {
      each_opt(i, d);

      /* Hide options that aren't in the preferred group; use OptionClass (any) when you want ALL of them */
      if (hide_when && hide_when (d))
	continue;

      /* Hide options that don't have an appropriate kind of name (short or long) */
      if (DisplayStyleC (display_style) == DS_short
	    ? d->letter == 0
	    : d->name == NULL)
	continue;

      if (hide_value_mask
       && hide_value_mask & ((uintmax_t)1U << get_opt_value (d, whence_selection)))
	continue;

      show_how (d, whence_selection);
    }
}

/******************************************************************************/

#include "variables.h"

void
get_shellopts (void)
{
  /* set up any shell options we may have inherited. */
  SHELL_VAR *var = find_variable ("SHELLOPTS");
  if (var == NULL) return;

  if (! imported_p (var)) return;
  if (! array_p (var) && ! assoc_p (var)) return;

  char *shellopts_env = savestring (value_cell (var));
  if (shellopts_env == NULL) return;

  char *vname;
  int vptr = 0;

  for (; vname = extract_colon_unit (shellopts_env, &vptr); xfree (vname))
    {
      opt_def_t const *d = find_option (vname);
      if (!d) continue;

      op_result_t r = set_opt_value (d, OptionClass (environ), true);
      if (BadResult (r))
	warn_invalidopt (vname);
    }
  xfree (shellopts_env);
}

void
set_shellopts (void)
{
  char tflag[Opts.len];
  size_t ti = 0;
  size_t vsize = 0;

  for_each_opt(i)
    {
      each_opt(i, d);
      if (d->hide_set_o)
	continue;
      option_value_t v = get_opt_value (d, OptionClass (set_o));
      if (v > 0)
	vsize += strlen (d->name) + 1;
      tflag[ti++] = v;
    }

  char *value = xmalloc (vsize + 1);
  char *vend = value;

  ti = 0;
  for_each_opt(i)
    {
      each_opt(i, d);
      if (d->hide_set_o)
	continue;
      if (tflag[ti++])
	{
	  vend = stpcpy (vend, d->name);
	  *vend++ = ':';
	}
    }

  if (vend > value)
    vend--;			/* cut off trailing colon */
  *vend = '\0';

  /* ASS_FORCE so we don't have to temporarily turn off readonly;
   * ASS_NOMARK so we don't tickle `set -a`. */
  SHELL_VAR *var = bind_variable ("SHELLOPTS", value, ASS_FORCE | ASS_NOMARK);

  /* Turn the read-only attribute back on. */
  VSETATTR (var, att_readonly);

  xfree (value);
}

void
initialize_shell_options (int dont_import_environment)
{
  init_letter_map ();

  if (! dont_import_environment)
    get_shellopts ();

  /* Set up the $SHELLOPTS variable. */
  set_shellopts ();

  init_letter_map ();

  if (!dont_import_environment)
    get_bashopts ();

  /* Set up the $BASHOPTS variable. */
  set_bashopts ();
}

/******************************************************************************/

void
set_bashopts (void)
{
  char tflag[Opts.len];
  size_t ti = 0;
  size_t vsize = 0;

  for_each_opt(i)
    {
      each_opt(i, d);
      if (d->hide_shopt)
	continue;
      option_value_t v = get_opt_value (d, OptionClass (shopt));
      if (v > 0)
	vsize += strlen (d->name) + 1;
      tflag[ti++] = v;
    }

  char *value = xmalloc (vsize + 1);
  char *vend = value;

  ti = 0;
  for_each_opt(i)
    {
      each_opt(i, d);
      if (d->hide_shopt)
	continue;
      if (tflag[ti++])
	{
	  vend = stpcpy (vend, d->name);
	  *vend++ = ':';
	}
    }

  if (vend > value)
    vend--;			/* cut off trailing colon */
  *vend = '\0';

  /* ASS_FORCE so we don't have to temporarily turn off readonly;
   * ASS_NOMARK so we don't tickle `set -a`. */
  SHELL_VAR *var = bind_variable ("BASHOPTS", value, ASS_FORCE | ASS_NOMARK);
  xfree (value);

  /* Turn the read-only attribute back on. */
  VSETATTR (var, att_readonly);
}

void
get_bashopts (void)
{
  SHELL_VAR *var = find_variable ("BASHOPTS");
  /* set up any shell options we may have inherited. */

  if (! var
      || ! imported_p (var)
      || array_p (var)
      || assoc_p (var))
    return;

  char *value = savestring (value_cell (var));
  if (!value) return;

  char *vname;
  int vptr = 0;
  while (vname = extract_colon_unit (value, &vptr))
    {
      (void) set_opt_value (find_option (vname), OptionClass (environ), true);
      /* Ignore return status; don't care about unknown shopt names */
      xfree (vname);
    }
  xfree (value);
}

/******************************************************************************/
