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

#if !defined (_JOBS_H_)
#  define _JOBS_H_

typedef int option_value_t;

#define OPTION_INVALID_VALUE ((option_value_t)-1)
#define OPTION_VALUE_UNSET   ((option_value_t)-2)

/* Rather than bald enums, use singleton structs to ensure we cannot
 * accidentally mix these up with ints; on platforms where this unreasonably
 * impacts performance, define FAST_ENUM to use int instead. */

/******************************************************************************/

/* OptionClass: when setting an option, whether it's allowed may depend on how it's
 * attempted to be changed. */
enum option_class_e {
  OC_0,
  OC_any,		/* unlabelled zero default; computation of correct value is still pending */
  OC_short,		/* using set -X (a short single-letter option) */
  OC_set_o,		/* using set -o NAME */
  OC_shopt,		/* using shopt -s NAME */
  OC_argv,		/* parsed from argv when Bash starts up */
  OC_environ,		/* read from the environment when Bash starts up */
  OC_unwind,		/* automatically being restored during unwinding */
};

#define OptionClassV(X)		OC_##X
#define OptionClassVM(X)	(1U << OptionClassV (X))

#ifdef FAST_ENUM

typedef enum option_class_e option_class_t;
# define OptionClass(X)		OptionClassV (X)
# define OptionClassC(E)	(E)

#else

/* Use a small struct to prevent it being conflated with an "int".
 * (Modern compilers generate identical code either way, but older compilers
 * may generate markedly worse code for structs.) */
typedef struct option_class_s {
  enum option_class_e oclass;
} option_class_t;
# define OptionClass(X)		(option_class_t){ .oclass = OptionClassV (X) }
# define OptionClassC(E)	(E).oclass

#endif

#define OptionClassCM(E)	(1U << OptionClassC (E))

#define OptionClassIs(E, X)	(OptionClassC (E) == OptionClassV (X))
#define OptionClassIsNot(E, X)	(! OptionClassIs(E, X))

/******************************************************************************/

enum display_style_e {
  DS_std = 0x10,	/* unlabelled zero default; computation of correct value is still pending */
//DS_standard = 0x10,	/* unlabelled zero default; computation of correct value is still pending */
  DS_short,		/* using set -X (a short single-letter option) */
  DS_set_o,		/* using set -o NAME */
  DS_shopt,		/* using shopt -s NAME */
};

#define DisplayStyleV(X)	DS_##X
#define DisplayStyleVM(X)	(1U << DisplayStyleV (X))

#ifdef FAST_ENUM

typedef enum option_class_e display_style_t;
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
  /* These values are offset by 0x200 to ensure they don't overlap with POSIX
   * exit codes, or the extended codes shell.h */
  Result_OK = 0x200,

  /* Results from attempting to set values */
  Result_Unchanged,	/* New value is same as old value */
  Result_Ignored,	/* Change request ignored, silently */
  Result_NotFound,	/* No entry with the supplied name/letter */
  Result_ReadOnly,	/* Change never possible */
  Result_Forbidden,	/* Not changed because new value is permitted */
  Result_BadValue,	/* Like ReadOnly but doesn't complain if the new value is the same as the old value */

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
extern int res_to_ex(op_result_t res);

/******************************************************************************/

typedef struct opt_def_s opt_def_t;  /* forward ref */

typedef op_result_t opt_set_func_t (opt_def_t const *option_def,
				    option_class_t set_vs_shopt,
				    option_value_t new_value);
typedef option_value_t opt_get_func_t (opt_def_t const *option_def,
				       option_class_t set_vs_shopt);

struct opt_def_s {
  char const *name;
  option_value_t *store;
  option_value_t const *init;
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
	:0;
};

/******************************************************************************/

extern op_result_t register_option(opt_def_t const *def);

extern opt_def_t const * find_option (char const *name);
extern opt_def_t const * find_short_option (char letter);
extern const char *get_short_opt_names(void);

/******************************************************************************/

extern option_value_t  get_opt_value (opt_def_t const *d,
                                      option_class_t w);
extern op_result_t set_opt_value (opt_def_t const *d,
                                      option_class_t w,
                                      option_value_t new_value);

/******************************************************************************/

extern void show_one_option (opt_def_t const *option_definition,
			     option_class_t selection,
			     display_style_t display_style);

extern void show_one_option_unless_value (opt_def_t const *option_definition,
					  option_class_t selection,
					  uintmax_t hide_value_mask,
					  display_style_t display_style);

extern void list_all_options (option_class_t selection,
			      unsigned hide_value_mask,
			      display_style_t display_style);

/******************************************************************************/

/* manage SHELLOPTS and BASHOPTS */

extern void get_shellopts (void);
extern void set_shellopts (void);
extern void initialize_shell_options (int dont_import_environment);

extern void set_bashopts (void);
extern void get_bashopts (void);

/******************************************************************************/

#endif
