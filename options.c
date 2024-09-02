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

#define OPTFMT "%-23s\t%s"

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

int
res_to_ex (op_result_t res)
{
  enum op_result_e r = ResultC (res);
  if (r < Result_OK
   || r >= sizeof result_to_ex_map / sizeof *result_to_ex_map)
    return -1;
  return result_to_ex_map[r];
}

/******************************************************************************/

option_value_t
get_opt_value (opt_def_t const *d, accessor_t why)
{
  if (!d)
    return OPTION_INVALID_VALUE;
  if (d->get_func)
    return d->get_func(d, why);
  if (d->store)
    return d->store[0];
  return OPTION_INVALID_VALUE;
}

op_result_t
set_opt_value (opt_def_t const *d,
	       accessor_t why,
	       option_value_t new_value)
{
  if (!d)
    return Result (NotFound);

  if (d->set_func)
    {
      op_result_t r = d->set_func(d, why, new_value);
      if (ResultC (r) == Result_OK)
	{
	  /* only trigger these for exactly Result_OK; not Result_Unchanged or Result_Ignored */
	  if (d->adjust_shellopts) set_shellopts ();
	  if (d->adjust_bashopts) set_bashopts ();
	}
      return r;
    }

  if (d->readonly && ! AccessorIsPrivileged (why))	/* on behalf of the user; not unwind or unload */
    return Result (ReadOnly);
  if (d->forbid_change && ! AccessorIsStartup (why))	/* not argv or env; also not privileged */
    {
      option_value_t old_value = get_opt_value (d, why);
      if (new_value == old_value)
	return Result (Unchanged);
      else if (d->ignore_change)
	return Result (Ignored);
      else
	return Result (Forbidden);
    }
  if (d->ignore_change)
    return Result (Ignored);

  /* TODO: handle the case where writing is direct but reading is mediated by a
	   function; but it seems unlikely that such a case will ever exist.
  */
  _Bool adjust_shellopts = d->adjust_shellopts && d->store[0] != new_value;
  _Bool adjust_bashopts  = d->adjust_bashopts && d->store[0] != new_value;
  if (d->store)
    d->store[0] = new_value;
  if (adjust_shellopts) set_shellopts ();
  if (adjust_bashopts) set_bashopts ();
  return Result (OK);
}

/******************************************************************************/

typedef struct od_vec_s {
  const opt_def_t **defs;
  size_t cap;
  size_t len;
} od_vec_t;

static void
resize_vector (struct od_vec_s *oo, size_t new_cap, _Bool logscale)
{
  size_t old_cap = oo->cap;
  if (logscale)
    new_cap = old_cap << 1 | new_cap;
  oo->cap = new_cap;
  size_t new_size = new_cap * sizeof *oo->defs;
  if (oo->defs)
    oo->defs = xrealloc (oo->defs, new_size);
  else
    oo->defs = xmalloc (new_size);
}

static void
flush_vector (struct od_vec_s *oo)
{
  if (oo->defs)
    xfree (oo->defs);
  *oo = (struct od_vec_s){0};
}

/******************************************************************************/

static od_vec_t OrderedOpts = {0};
#define for_each_opt(i) for (size_t i = 0; i < OrderedOpts.len; ++i)
#define each_opt(i,d) \
		      opt_def_t const *d = OrderedOpts.defs[i]; \
		      if (!d) continue

typedef struct search_result_s {
  size_t  index;
  _Bool	  match;
} search_result_t;

/* Find the name, if it exists,
   or the point where it should be inserted, if not.
 */
static inline search_result_t
search_options (char const *name)
{
  ssize_t left = 0,
	  right = OrderedOpts.len;
  for (; left < right ;)
  {
    size_t mid = (left+right)>>1;
    int res = strcmp (name, OrderedOpts.defs[mid]->name);
    if (res < 0)
      right = mid;
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
  search_result_t S = search_options (name);
  if (S.match)
    return OrderedOpts.defs[S.index];
  return NULL;
}

/*************************************/

#define MAX_SHORT_NAMES	(1+CHAR_MAX)
static struct {
  size_t count;
  char min;
  char max;
  char const *enumeration;	/* doubles as a "been init" flag */
  const opt_def_t *map[MAX_SHORT_NAMES];
} ShortOpts = {0};

static void
invalidate_short_opt_names (void)
{
    if (ShortOpts.enumeration)
      xfree (ShortOpts.enumeration);	/* avoid leak if we're called more than once */
    ShortOpts.enumeration = NULL;
}

opt_def_t const *
find_short_option (char letter)
{
  return ShortOpts.map[letter];
}

const char *
get_short_opt_names (void)
{
  if (ShortOpts.enumeration)
    return ShortOpts.enumeration;
  for (ShortOpts.max = 0, ShortOpts.min = 1
      ; ShortOpts.min < MAX_SHORT_NAMES && ShortOpts.map[ShortOpts.min] == 0
      ; ++ShortOpts.min) {}
  /* list is empty */
  if (ShortOpts.min == MAX_SHORT_NAMES)
    return "";
  /* make it big enough ... */
  char *ep = xmalloc (MAX_SHORT_NAMES + 1 - ShortOpts.min);
  char const *p = ep;
  for (size_t c = ShortOpts.max; c<MAX_SHORT_NAMES ; ++c)
    {
      if (! ShortOpts.map[c])
	continue;
      ShortOpts.max = c;
      *ep++ = c;
    }
  *ep = 0;
  /* ... and then make is small again */
  ShortOpts.count = ep - p;
  ShortOpts.enumeration = xrealloc (p, ShortOpts.count + 1);
  return ShortOpts.enumeration;
}

/**************************************/

void
initialize_option_framework ()
{
}

op_result_t
register_option (opt_def_t const *def)
{
  /* Check both long and short names are available */
  _Bool reloading_name = false;
  _Bool reloading_letter = false;
  search_result_t S = {0};
  if (def->name)
    {
      S = search_options (def->name);
      if (S.match)
	{
	  opt_def_t const *old = OrderedOpts.defs[S.index];
	  if (old != def)
	    return Result (Duplicate);
	  reloading_name = true;
	}
    }
  char c = def->letter;
  if (c)
    {
      opt_def_t const *old = ShortOpts.map[c];
      if (old)
	{
	  if (old != def)
	    return Result (Duplicate);
	  reloading_letter = true;
	  c = 0;  /* no further action required */
	}
    }

  if (reloading_name && reloading_letter)
    return Result (Unchanged);  /* re-adding the same entry */

  /* Make more room ... approximately double previous size */
  if (OrderedOpts.len >= OrderedOpts.cap)
    {
      /* Make more room ... approximately double previous size */
      resize_vector (&OrderedOpts, OrderedOpts.len + 1, true);
      for (size_t i = OrderedOpts.len; i < OrderedOpts.cap; ++i)
	OrderedOpts.defs[i] = NULL;
    }

  if (def->name)
    {
      if (!S.match)
	{
	  if (OrderedOpts.len > S.index)
	    memmove (OrderedOpts.defs+S.index+1,
		     OrderedOpts.defs+S.index,
		     (OrderedOpts.len-S.index) * sizeof *OrderedOpts.defs);
	  ++OrderedOpts.len;
	  OrderedOpts.defs[S.index] = def;
	  #if 0
	  if (S.index > 0 &&
	      strcmp (OrderedOpts.defs[S.index-1]->name, def->name) >= 0)
	    {
	      fprintf (stderr, "");
	      fprintf (stderr,
		       "Out-of-order insertion, placed \"%s\" at %zu after \"%s\"\n",
		       def->name,
		       S.index,
		       OrderedOpts.defs[S.index-1]->name);
	      fflush (stderr);
	      abort ();
	    }
	  if (S.index < OrderedOpts.len - 1 &&
	      strcmp (def->name, OrderedOpts.defs[S.index+1]->name) >= 0)
	    {
	      fprintf (stderr,
		       "Out-of-order insertion, placed \"%s\" at %zu before \"%s\"\n",
		       def->name,
		       S.index,
		       OrderedOpts.defs[S.index+1]->name);
	      fflush (stderr);
	      abort ();
	    }
	  #endif
	}
    }

  if (c)
    {
      ++ShortOpts.count;
      ShortOpts.map[c] = def;
      if (ShortOpts.enumeration)
	xfree (ShortOpts.enumeration);	/* avoid leak if we're called more than once */
      ShortOpts.enumeration = NULL;
    }
  return Result (OK);
}

static op_result_t
deregister_letter (opt_def_t const *def)
{
 #ifdef QUICK_DEREGISTER
  char c = def->letter;
  if (c)
    {
      opt_def_t const **dd = &ShortOpts.map[c];
      if (*dd == def)
	{
	  --ShortOpts.count;
	  *dd == NULL;
	  return Result (OK);
	}
      return Result (BadValue);
    }
  /* wasn't registered with a letter */
  return Result (Unchanged);
 #else
  int count = 0;
  for (int c = 0 ; c < MAX_SHORT_NAMES ; ++c)
    {
      opt_def_t const **dd = &ShortOpts.map[c];
      if (*dd == def)
	{
	  --ShortOpts.count;
	  *dd == NULL;
	  ++count;
	}
    }
  return count == 1 ? Result (OK)
		    : Result (BadValue);
 #endif
}

static op_result_t
deregister_name (opt_def_t const *def)
{
 #ifdef QUICK_DEREGISTER
  if (def->name)
    {
      search_result_t S = search_options (def->name);
      if (S.match && OrderedOpts.defs[i] == def)
	{
	  --OrderedOpts.len;
	  if (OrderedOpts.len > i)
	    memmove (OrderedOpts.defs+i, OrderedOpts.defs+i+1, (OrderedOpts.len - i) * sizeof *OrderedOpts.defs);
	  return Result (OK);
	}
      return Result (BadValue);
    }
  return Result (Unchanged);
 #else
  int count = 0;
  for (size_t i = 0 ; i < OrderedOpts.len ; ++i)
    if (OrderedOpts.defs[i] == def)
      {
	--OrderedOpts.len;
	if (OrderedOpts.len > i)
	  memmove (OrderedOpts.defs+i, OrderedOpts.defs+i+1, (OrderedOpts.len - i) * sizeof *OrderedOpts.defs);
	++count;
	--i;
      }
  return count > 0 ? Result (OK)
		   : Result (BadValue);
 #endif
}

/* Need to allow deregistration, to support use by loadable modules that can
 * themselves be unloaded */
op_result_t
deregister_option (opt_def_t const *def)
{
  deregister_letter (def);
  deregister_name (def);

  /* If this option was incorporated into BASHOPTS or SHELLOPTS, make sure
   * those variables are regenerated without this option. */
  if ((def->adjust_shellopts ||
       def->adjust_bashopts) &&
      get_opt_value (def, Accessor (unload)))
    {
      if (def->adjust_shellopts) set_shellopts ();
      if (def->adjust_bashopts) set_bashopts ();
    }

  return Result (OK);
}

/******************************************************************************/

_Bool
hide_for_set_o (opt_def_t const *d)
{
  return d->hide_set_o;
}

_Bool
hide_for_short (opt_def_t const *d)
{
  return d->letter == 0;
}

_Bool
hide_for_shopt (opt_def_t const *d)
{
  return d->hide_shopt;
}

_Bool
hide_for_env_shellopts (opt_def_t const *d)
{
  return !d->adjust_shellopts;
}

_Bool
hide_for_env_bashopts (opt_def_t const *d)
{
  return !d->adjust_bashopts;
}

static opt_test_func_t *const hide_for_map[] = {
  [AC_set_o] = hide_for_set_o,
  [AC_shopt] = hide_for_shopt,
  [AC_short] = hide_for_short,
  [AC_env_bashopts] = hide_for_env_bashopts,
  [AC_env_shellopts] = hide_for_env_shellopts,
};

/* not static */
inline opt_test_func_t *
hide_check_for (accessor_t why)
{
  int x = AccessorC (why);
  if (x < 0 || x >= sizeof hide_for_map / sizeof *hide_for_map)
    return NULL;
  return hide_for_map[x];
}

typedef void show_func_t (opt_def_t const *d, accessor_t why);

static show_func_t show_option_on_off;
static show_func_t show_option_shopt;
static show_func_t show_option_short;
static show_func_t show_option_set_o;

static void
show_option_on_off (opt_def_t const *d, accessor_t why)
{
  printf (OPTFMT "\n", d->name, get_opt_value (d, why) ? "on" : "off");
}

static void
show_option_shopt (opt_def_t const *d, accessor_t why)
{
  printf ("shopt -%c %s\n", get_opt_value (d, why) ? 's' : 'u', d->name);
}

static void
show_option_short (opt_def_t const *d, accessor_t why)
{
  printf ("set %c%c\n", bool_to_flag (get_opt_value (d, why)), d->letter);
}

static void
show_option_set_o (opt_def_t const *d, accessor_t why)
{
  printf ("set %co %s\n", bool_to_flag (get_opt_value (d, why)), d->name);
}

static void
show_option_help1 (opt_def_t const *d, accessor_t why)
{
  if (d->name)
    {
      if (d->letter)
	printf (OPTFMT "\t%c%c\n",
		d->name, get_opt_value (d, why) ? "on" : "off",
		bool_to_flag (get_opt_value (d, why)), d->letter
	       );
      else
	printf (OPTFMT "\n",
		d->name, get_opt_value (d, why) ? "on" : "off"
	       );
    }
  else if (d->letter)
    {
      printf ("%c%c\n",
	      bool_to_flag (get_opt_value (d, why)), d->letter
	     );
    }
  else
    printf("(%s)\n", _("This option has no name"));
}

static void
show_option_help2 (opt_def_t const *d, accessor_t why)
{
  printf ("\n");
  show_option_help1 (d, why);

  if (d->readonly)
    printf("\n\t(%s)\n", _("This option is read-only."));

  char const *h = d->help;
  if (h)
    {
      h = _(h);	/* delay localization until we need it */
      /* print the entire help text, indented one tabstop */
      for (char const *n; (n = strchr (h, '\n')) != NULL; h = n)
	printf ("\t%.*s", (int)((++n)-h), h);
      if (*h)
	printf ("\t%s\n", h);
    }
}

static void
show_option_help3 (opt_def_t const *d, accessor_t why)
{
  show_option_help2 (d, why);

  if (d->name)
    {
      printf ("\n\t%s:\n", _("Display"));
      printf("\t\tshopt -P %s\n", d->name);
    }

  printf ("\n\t%s:\n", _("Query"));
  if (d->name)
    {
      printf("\t\tshopt -q %s\n", d->name);
      if (d->adjust_bashopts)
	printf("\t\t[[ :$BASHOPTS: = *:%s:* ]]\n", d->name);
      if (d->adjust_shellopts)
	printf("\t\t[[ :$SHELLOPTS: = *:%s:* ]]\n", d->name);
    }
  if (d->letter)
    printf("\t\t[[ $- = *'%c'* ]]\n", d->letter);

  if (!d->readonly)
    {
      printf ("\n\t%s:\n", _("Turn on"));
      if (d->name)
	{
	  printf ("\t\tshopt -s %s\n", d->name);
	  if (1 || ! d->hide_set_o)
	    printf ("\t\tset -o %s\n", d->name);
	}
      if (d->letter)
	printf ("\t\tset -%c\n", d->letter);

      printf ("\n\t%s:\n", _("Turn off"));
      if (d->name)
	{
	  printf ("\t\tshopt -u %s\n", d->name);
	  if (1 || ! d->hide_set_o)
	    printf ("\t\tset +o %s\n", d->name);
	}
      if (d->letter)
	printf ("\t\tset +%c\n", d->letter);
    }
}

static show_func_t *show_map[] = {
  [DS_on_off] = show_option_on_off,
  [DS_short] = show_option_short,
  [DS_set_o] = show_option_set_o,
  [DS_shopt] = show_option_shopt,
  [DS_help1] = show_option_help1,
  [DS_help2] = show_option_help2,
  [DS_help3] = show_option_help3,
};

static inline show_func_t *
get_show (display_style_t display_style)
{
    int v = DisplayStyleC (display_style);
    if (v < 0 || v >= sizeof show_map / sizeof *show_map)
      return NULL;
    return show_map[v];
}

void
show_one_option (opt_def_t const *d,
		 accessor_t why,
		 display_style_t display_style)
{
  get_show (display_style)(d, why);
}

void
show_one_option_unless_value (opt_def_t const *d,
			      accessor_t why,
			      uintmax_t hide_value_mask,
			      display_style_t display_style)
{
  if (hide_value_mask & ((uintmax_t)1U << get_opt_value (d, why)))
    get_show (display_style) (d, why);
}

void
list_all_options (accessor_t why,
		  unsigned hide_value_mask,
		  display_style_t display_style)
{
  opt_test_func_t *hidden =  hide_check_for (why);
  show_func_t *show_how = get_show (display_style);

  for_each_opt (i)
    {
      each_opt (i, d);

      /* Hide options that aren't in the preferred group; use Accessor (any) when you want ALL of them */
      if (hidden && hidden (d))
	continue;

      /* Hide options that don't have an appropriate kind of name (short or long) */
      if (DisplayStyleC (display_style) == DS_short
	    ? d->letter == 0
	    : d->name == NULL)
	continue;

      if (hide_value_mask
       && hide_value_mask & ((uintmax_t)1U << get_opt_value (d, why)))
	continue;

      show_how (d, why);
    }
}

/* generic iterator */

iter_opt_def_t
begin_iter_opts (opt_test_func_t *hide_if)
{
  return (iter_opt_def_t){
      .position = 0,
      .hide_if = hide_if,
    };
}

extern opt_def_t const * get_iter_opts (iter_opt_def_t *it);	/* returns NULL to signal end of iteration */

opt_def_t const *
get_iter_opts (iter_opt_def_t *it)
{
  _Bool (*hidden)(opt_def_t const *d) = it->hide_if;
  for (; it->position < OrderedOpts.len ;)
    {
      opt_def_t const *d = OrderedOpts.defs[it->position++];
      if (! d)
	continue;
      if (! hidden || ! hidden (d))
	return d;
    }
  return NULL;
}

size_t
count_options (void)
{
  return OrderedOpts.len;
}

size_t
count_options_func (opt_test_func_t *fn)
{
  if (!fn)
    return count_options ();

  size_t result = 0;
  opt_def_t const *d;
  for_each_opt (i)
    {
      each_opt (i, d);
      if (! fn (d))
	++result;
    }
  return result;
}

size_t
count_options_class (accessor_t why)
{
  return count_options_func (hide_check_for (why));
}

/******************************************************************************/

#include "variables.h"

void
get_options_from_env (char const *varname, accessor_t why, opt_test_func_t *filter, _Bool quiet)
{
  /* set up any shell options we may have inherited. */
  SHELL_VAR *var = find_variable (varname);
  if (var == NULL) return;

  if (! imported_p (var)) return;
  if (! array_p (var) && ! assoc_p (var)) return;

  char *shellopts_env = savestring (value_cell (var));
  if (shellopts_env == NULL) return;

  opt_test_func_t *hidden =  hide_check_for (why);

  char const*vname;
  int vptr = 0;

  for (; vname = extract_colon_unit (shellopts_env, &vptr); xfree (vname))
    {
      opt_def_t const *d = find_option (vname);
      if (!d) continue;
      if (filter && filter (d)) continue;
      if (hidden && hidden (d)) continue;
      op_result_t r = set_opt_value (d, why, true);
      if (BadResult (r) && !quiet)
	warn_invalidopt (vname);
    }
  xfree (shellopts_env);
}

void
set_env_from_options (char const *varname, accessor_t why, opt_test_func_t *filter)
{
  char *tflag = xmalloc (count_options ());	/* over-estimate */
  size_t vsize = 0;
  opt_test_func_t *hidden =  hide_check_for (why);

  for_each_opt (i)
    {
      each_opt (i, d);
      tflag[i] = false;
      if (filter && filter (d))
	continue;
      if (hidden && hidden (d))
	continue;
      option_value_t v = get_opt_value (d, Accessor (set_o));
      if (v <= 0)
	continue;
      vsize += strlen (d->name) + 1;
      tflag[i] = true;
    }

  char *value = xmalloc (vsize + 1);
  char *vend = value;

  for_each_opt (i)
    {
      each_opt (i, d);
      if (!tflag[i])
	continue;
      vend = stpcpy (vend, d->name);
      *vend++ = ':';
    }
  xfree (tflag);

  if (vend > value)
    vend--;			/* cut off trailing colon */
  *vend = '\0';

  /* ASS_FORCE so we don't have to temporarily turn off readonly;
   * ASS_NOMARK so we don't tickle `set -a`. */
  SHELL_VAR *var = bind_variable (varname, value, ASS_FORCE | ASS_NOMARK);
  xfree (value);

  /* Turn the read-only attribute back on. */
  VSETATTR (var, att_readonly);
}

/******************************************************************************/

#if 0

void
initialize_shell_options (int dont_import_environment)
{
  if (! dont_import_environment)
    {
      get_options_from_env ("SHELLOPTS", Accessor (set_o), true);
      get_options_from_env ("BASHOPTS", Accessor (shopt), true);
    }

  /* Set up the $SHELLOPTS variable. */
  set_env_from_options ("SHELLOPTS", Accessor (set_o), NULL);
  set_env_from_options ("BASHOPTS", Accessor (shopt), NULL);
}

#endif

/******************************************************************************/
