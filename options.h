/* options.h -- structures and definitions used by the options.c file. */

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

#if !defined (_OPTIONS_H_)
#  define _OPTIONS_H_

/******************************************************************************/

/* NOTE: This module uses singleton structs rather than bare enums to ensure
 * clients cannot accidentally mix these up with ints; on platforms where this
 * unreasonably impacts performance, define FAST_ENUM to bare enums instead. */

extern void initialize_option_framework (void);

/******************************************************************************/

typedef int option_value_t;

#define OPTION_INVALID_VALUE ((option_value_t)-1)
#define OPTION_VALUE_UNSET   ((option_value_t)-2)

/******************************************************************************/

/* Several functions have an Accessor parameter that is used to implement access
 * controls.
 *   1. For "ordinary" accessors, they merely limits enumerations to relevant
 *      options; the "any" accessor does not limit enumerations.
 *   2. an "unwind" accessor will bypass the "forbid_changes" option; this is
 *      intended to support restoring settings to the states they had before
 *      "local" was used.
 *  3. an "unload" accessor will bypass all restrictions; this is intended to
 *      support unloading a dynamic shared library, including forceably
 *      deallocating any resources.
 */
enum accessor_e {
  AC_any		/* enumerate everything */
    #ifndef FAST_ENUM
	    = 0x08	/* offset to avoid overlap with other enums */
    #endif
	    ,
  AC_short,		/* enumerate only set -X (a short single-letter option) */
  AC_set_o,		/* enumerate only set -o NAME */
  AC_shopt,		/* enumerate only shopt -s NAME */
  AC_argv,		/* parsed from argv when Bash starts up */
  AC_env_shellopts,	/* read from the environment when Bash starts up */
  AC_env_bashopts,	/* read from the environment when Bash starts up */
  AC_unwind,		/* automatically being restored during unwinding */
  AC_reinit,		/* restoring to default value */
  AC_unload,		/* option being removed */
};

#define AccessorV(X)		AC_##X
#define AccessorVM(X)	(1U << AccessorV (X))

#ifdef FAST_ENUM

typedef enum accessor_e accessor_t;
# define Accessor(X)		AccessorV (X)
# define AccessorC(E)	(E)

#else

/* Use a small struct to prevent it being conflated with an "int".
 * (Modern compilers generate identical code either way, but older compilers
 * may generate markedly worse code for structs.) */
typedef struct accessor_s {
  enum accessor_e oclass;
} accessor_t;
# define Accessor(X)		(accessor_t){ .oclass = AccessorV (X) }
# define AccessorC(E)	(E).oclass

#endif

#define AccessorCM(E)	(1U << AccessorC (E))

#define AccessorIs(E, X)	(AccessorC (E) == AccessorV (X))
#define AccessorIsNot(E, X)	(! AccessorIs (E, X))

#define AccessorIsStartup(E)	(AccessorC (E) > AccessorV (argv))
#define AccessorIsPrivileged(E)	(AccessorC (E) > AccessorV (unwind))

/******************************************************************************/

enum display_style_e {
  DS_on_off		/* show name with "on" or "off" */
    #ifndef FAST_ENUM
	    = 0x10	/* offset to avoid overlap with other enums */
    #endif
	    ,
  DS_short,		/* using set -X (a short single-letter option) */
  DS_set_o,		/* using set -o NAME */
  DS_shopt,		/* using shopt -s NAME */
  DS_help1,		/* show names with option letters */
  DS_help2,		/* as for help1 plus brief explanation */
  DS_help3,		/* as for help2 plus usage instructions */
};

#define DisplayStyleV(X)	DS_##X
#define DisplayStyleVM(X)	(1U << DisplayStyleV (X))

#ifdef FAST_ENUM

typedef enum accessor_e display_style_t;
# define DisplayStyle(X)	DisplayStyleV (X)
# define DisplayStyleC(E)	(E)

#else

/* Use a small struct to prevent it being conflated with an "int".
 * (Modern compilers generate identical code either way, but older compilers
 * may generate markedly worse code for structs.) */
typedef struct display_style_s {
  enum display_style_e dstyle;
} display_style_t;
# define DisplayStyle(X)	(display_style_t){ .dstyle = DisplayStyleV (X) }
# define DisplayStyleC(E)	(E).dstyle

#endif

#define DisplayStyleCM(E)	(1U << DisplayStyleC (E))

/******************************************************************************/

enum op_result_e {
  Result_OK             /* The value was updated or unchanged */
    #ifndef FAST_ENUM
	    = 0x200	/* Values are offset to ensure they don't overlap with
			 * POSIX exit codes, or the extended codes in shell.h,
			 * or with the other enums here. */
    #endif
  ,

  /* Results from attempting to set values */
  Result_Unchanged,	/* New value is same as old value (when change would be forbidden) */
  Result_Ignored,	/* Change request ignored, silently */
  Result_NotFound,	/* No entry with the supplied name/letter */
  Result_ReadOnly,	/* Change never possible */
  Result_Forbidden,	/* Not changed because new value is permitted */
  Result_BadValue,	/* New value is not valid (usually only for non-bool values) */

  /* Results from adding or removing entries */
  Result_Duplicate,	/* Cannot add entry that conflicts with existing entry */
};

#define ResultV(X)	Result_##X
#define ResultVM(X)	(1U << ResultV (X))

#ifdef FAST_ENUM

typedef enum op_result_e op_result_t;
# define Result(X)	ResultV (X)
# define ResultC(R)	(R)

#else

/* Use a small struct to prevent it being conflated with an "int".
 * (Modern compilers generate identical code either way, but older compilers
 * may generate markedly worse code for structs.) */
typedef struct op_result_s {
  enum op_result_e result;
} op_result_t;
# define Result(X)	(op_result_t){ .result = ResultV (X) }
# define ResultC(R)	(R).result

#endif

#define ResultCM(R)	(1U << ResultC (R))

#define GoodResult(R)	(ResultC (R) <= Result_Ignored)
#define BadResult(R)	(! GoodResult (R))

/******************************************************************************/

/* Convert a result to a shell exit code */
extern int res_to_ex (op_result_t res);

/******************************************************************************/

typedef struct opt_def_s opt_def_t;  /* forward ref */

typedef op_result_t opt_set_func_t (opt_def_t const *option_def,
				    accessor_t set_vs_shopt,
				    option_value_t new_value);
typedef option_value_t opt_get_func_t (opt_def_t const *option_def,
				       accessor_t set_vs_shopt);

struct opt_def_s {
  char const *name;
  option_value_t *store;
  option_value_t const *init;	/* used to implement reset_all_options and reinit_all_options */
  char const *help;
  opt_set_func_t *set_func;
  opt_get_func_t *get_func;
  int   reference_value;
  char  letter;
  _Bool hide_set_o:1,
	hide_shopt:1,
	adjust_bashopts:1,
	adjust_shellopts:1,
	readonly:1,		/* when attempting to set an option: error (Readonly) unconditionally */
	forbid_change:1,	/*				     error (Forbidden) if attempting to change its value */
	ignore_change:1,	/*				     succeed (Ignored) without actually changing the value */
	skip_reinit:1,		/* only apply .init for "reset" and not "reinit" */
	direct_reset:1,		/* bypass .set_func for "reset" and "reinit" */
	:0;
};

extern const option_value_t OPTINIT_0[], OPTINIT_1[];
#define OPTINIT_false	OPTINIT_0
#define OPTINIT_true 	OPTINIT_1

#define OPTR1_(NoReinit, Ref)	skip_reinit = NoReinit,	.init = Ref
#define OPTR2_(NoReinit, Val)	OPTR1_(NoReinit, (option_value_t const[]){ Val })

/* Options marked with these will be updated only by reset_all_options() */
#define OPTRESET_0		OPTR1_(true, OPTINIT_0)
#define OPTRESET_1		OPTR1_(true, OPTINIT_1)
#define OPTRESET_false		OPTR1_(true, OPTINIT_0)
#define OPTRESET_true		OPTR1_(true, OPTINIT_1)
#define OPTRESET(Val)		OPTR2_(true, Val)	/* Use this when Val isn't literally true/false/0/1 */

/* Options marked with these will be updated by both reset_all_options() and reinit_all_options() */
#define OPTRESET_REINIT_0	OPTR1_(false, OPTINIT_0)
#define OPTRESET_REINIT_1	OPTR1_(false, OPTINIT_1)
#define OPTRESET_REINIT_false	OPTR1_(false, OPTINIT_0)
#define OPTRESET_REINIT_true	OPTR1_(false, OPTINIT_1)
#define OPTRESET_REINIT(Val)	OPTR2_(false, Val)	/* Use this when V isn't literally true/false/0/1 */

/******************************************************************************/

extern op_result_t register_option (opt_def_t const *def);
extern op_result_t deregister_option (opt_def_t const *def);

extern opt_def_t const * find_option (char const *name);
extern opt_def_t const * find_short_option (char letter);
extern const char *get_short_opt_names (void);

/******************************************************************************/

extern option_value_t  get_opt_value (opt_def_t const *d,
				      accessor_t why);
extern op_result_t set_opt_value (opt_def_t const *d,
				  accessor_t why,
				  option_value_t new_value);

/******************************************************************************/

/* Get a true-or-false answer about an option definition */

typedef _Bool opt_test_func_t(opt_def_t const *d);

extern opt_test_func_t *hide_check_for (accessor_t why);

extern opt_test_func_t hide_for_env_bashopts;
extern opt_test_func_t hide_for_env_shellopts;
extern opt_test_func_t hide_for_set_o;
extern opt_test_func_t hide_for_shopt;
extern opt_test_func_t hide_for_short;

/******************************************************************************/

/* Provide iterator to enumerate all (or some subset) of options */

/* Please don't touch these fields; use the accessors provided */
typedef struct iter_opt_def_s {
  ssize_t position;
  opt_test_func_t *hide_if;
} iter_opt_def_t;

extern iter_opt_def_t begin_iter_opts (opt_test_func_t *comb);
static inline iter_opt_def_t
begin_iter_opts_class (accessor_t why)
{
  return begin_iter_opts (hide_check_for (why));
}

extern opt_def_t const * get_iter_opts (iter_opt_def_t *it);

/*
 *  opt_def_t const *d;
 *  for_each_option_class(d, Accessor (short))
 *    {
 *	do_something_with (d);
 *    }
 */

# define for_each_option_class(dp, why) \
    for ( iter_opt_def_t local_option_iterator = begin_iter_opts_class (why) \
      ; dp = get_iter_opts (&local_option_iterator) \
      ; )

# define for_each_option_func(dp, comb) \
    for ( iter_opt_def_t local_option_iterator = begin_iter_opts (comb) \
      ; dp = get_iter_opts (&local_option_iterator) \
      ; )

# define for_each_option(dp) \
    for ( iter_opt_def_t local_option_iterator = begin_iter_opts (NULL) \
      ; dp = get_iter_opts (&local_option_iterator) \
      ; )


extern size_t count_options (void);
extern size_t count_options_func (opt_test_func_t *comb);
extern size_t count_options_class (accessor_t why);

/******************************************************************************/

extern void show_one_option (opt_def_t const *option_definition,
			     accessor_t why,
			     display_style_t display_style);

extern void show_one_option_unless_value (opt_def_t const *option_definition,
					  accessor_t why,
					  uintmax_t hide_value_mask,
					  display_style_t display_style);

extern void list_all_options (accessor_t why,
			      unsigned hide_value_mask,
			      display_style_t display_style);

/******************************************************************************/

/* manage SHELLOPTS and BASHOPTS */

extern void get_shellopts (void);
extern void set_shellopts (void);
extern void initialize_shell_options (_Bool dont_import_environment);

extern void set_bashopts (void);
extern void get_bashopts (void);

/* set all options to their default values */

extern void reinit_all_options (void);
extern void reset_all_options (void);

/******************************************************************************/

#endif
