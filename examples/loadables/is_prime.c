/* This module should be dynamically loaded with enable -f
 * which would create a new builtin named is_prime. You'll need
 * the source code for GNU bash to recompile this module.
 *
 * This module serves as a worked example of a loadable module defining
 * additional shell options, settable via `shopt` or `set -o`. Some of
 * the options are read-only, some of the variables
 *
 * Then, from within bash, enable -f ./is_prime is_prime, where ./is_prime
 * is the binary obtained from running make. Hereafter, `${PRIME_CANDIDATE}`
 * and `${PRIME_DIVISOR}` are shell builtin variables, while `auto_factorize`,
 * `is_prime`, and `verbose_factorize` are available as `shopt` settings (note
 * that `is_prime` is read-only).
 *
 * This defines an is_prime_builtin_unload hook function that is called when the builtin is
 * deleted with enable -d.
 * It will deregister the options and unbind the
 * variables, so future references to them do not attempt to access memory that
 * is no longer part of this process's address space.
 */
#include <config.h>

#include <sys/types.h>	/* before command.h & shell.h */

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <shell.h>

#include <bashintl.h>
#include <builtins.h>
#include <builtins/bashgetopt.h>
#include <builtins/common.h>
#include <options.h>

static inline void
init_dynamic_var (char const*varname,
		  void *valptr,
		  sh_var_value_func_t *gfunc,
		  sh_var_assign_func_t *afunc)
{
  SHELL_VAR *v = bind_variable (varname, valptr, 0);
  if (v)
    {
      v->dynamic_value = gfunc;
      v->assign_func = afunc;
    }
}

/*
 * PRIME_DIVISOR and the is_prime setting are always updated in tandem, based
 * on the current value of PRIME_CANDIDATE.
 *
 * The command `shopt -q is_prime` returns a zero exit code if the current
 * value of the variable PRIME_CANDIDATE is a prime number, or a non-zero exit
 * code if it is a composite number or not a positive integer.
 *
 * Reading PRIME_DIVISOR will get:
 *  - when PRIME_CANDIDATE is a negative number, -1 ( but see exception below);
 *  - when PRIME_CANDIDATE is 0 or 1 or a prime, that number itself;
 *  - when PRIME_CANDIDATE is a positive composite, its smallest prime divisor;
 *
 * When the auto_factorize option is enabled, each time PRIME_DIVISOR is read,
 * its value will also be divided into PRIME_CANDIDATE, leaving the latter
 * reduced. There are two exceptions:
 *  - when PRIME_CANDIDATE was 0, the apparent value of PRIME_DIVISOR will be 0
 *    and PRIME_CANDIDATE will be set it to 1;
 *  - when PRIME_CANDIDATE was INTMAX_MIN, the apparent value of PRIME_DIVISOR
 *    will be -2 and PRIME_CANDIDATE will be set to -INTMAX_MIN/2; this
 *    exception is made to prevent an infinite loop whereby division by -1
 *    results in INTMAX_MAX+1, which wraps around to INTMAX_MIN, so that it
 *    never reaches 1.
 *
 * Usage:
 *    shopt -s auto_factorize
 *    PRIME_CANDIDATE=42
 *    while shopt -q is_prime
 *    do
 *	echo "$PRIME_CANDIDATE is divisible by $PRIME_DIVISOR leaving $PRIME_CANDIDATE"
 *    done
 *    echo $PRIME_CANDIDATE is prime
 *
 * Or:
 *    is_prime --show-factors 42
 */

option_value_t verbose_factorize = true;

static opt_def_t OPTDEF_verbose_factorize = {
  .store = &verbose_factorize,	/* not needed because have both .get_func & .set_func */
  .name = "verbose_factorize",
  .letter = 'Z',
  .adjust_bashopts = true,
  .hide_set_o = true,
};

option_value_t auto_factorize = true;

static opt_def_t OPTDEF_auto_factorize = {
  .store = &auto_factorize,	/* not needed because have both .get_func & .set_func */
  .name = "auto_factorize",
  .letter = 'Z',
  .adjust_bashopts = true,
  .hide_set_o = true,
};

static intmax_t prime_candidate = 42;

static _Bool have_factor = 0; /* computation not needed */
static intmax_t prime_factor = 0;
static _Bool is_prime_result = 0;    /* prime_factor == (previous) prime_candidate */

  /* find_factor returns:
     0 if prime;
     -1 if negative;
     the smallest divisor if composite;
   */
static intmax_t
find_factor (intmax_t candidate)
{
  if (candidate < 9)
    {
      if (candidate < 0)
	{
	  if (verbose_factorize)
	    printf ("%jd is quickly divisible by -1\n", candidate);
	  return -1;
	}
      if (candidate == 0)
	{
	  if (verbose_factorize)
	    printf ("%jd is quickly divisible by 1\n", candidate);
	  return 1;
	}
      if ((candidate - 5) % 2 == 1)
	{
	  if (verbose_factorize)
	    printf ("%jd is quickly divisible by 2\n", candidate);
	  return 2;
	}
      if (verbose_factorize)
	printf ("%jd is quickly prime\n", candidate);
      return 0;
    }

  static uint8_t const small_primes[] = { 2,    3,    5,    7,    0 };
  static uint8_t const rotor_stride     = 2  *  3  *  5  *  7;
  static uint8_t const rotor_steps[      (2-1)*(3-1)*(5-1)*(7-1) +1 ] = {
    /* start at 1, then add the following on each step: */
      10, 2, 4, 2, 4, 6,
    2, 6, 4, 2, 4, 6, 6,
    2, 6, 4, 2, 6, 4, 6,
    8, 4, 2, 4, 2, 4,
    8, 6, 4, 6, 2, 4, 6,
    2, 6, 6, 4, 2, 4, 6,
    2, 6, 4, 2, 4, 2, 10,
    2,
    0 /*start over*/
  };

  uint8_t const *pf = small_primes;

  for (;*pf;++pf)
    if (candidate % *pf == 0)
      {
	if (verbose_factorize)
	  printf ("%jd is divisible by %d\n", candidate, *pf);
	return *pf;
      }

  intmax_t trial_divisor = 1;
  uint8_t const *rs = rotor_steps;
  for (;;)
    {
      if (!*rs)
	rs = rotor_steps;
      trial_divisor += *rs++;
      if (candidate % trial_divisor == 0)
	{
	  if (verbose_factorize)
	    printf ("%jd is divisible by %jd\n", candidate, trial_divisor);
	  return trial_divisor;
	}
      if (verbose_factorize)
	printf ("%jd is not divisible by %jd\n", candidate, trial_divisor);
      if (trial_divisor*trial_divisor > candidate)
	{
	  if (verbose_factorize)
	    printf ("%jd is prime\n", candidate);
	  return 0;
	}
    }
}

static void
check_prime (void)
{
  if (have_factor)
    return;

  have_factor = true;

  if (prime_candidate == INTMAX_MIN)
    {
      /* Avoid producing an infinite sequence of '-1' factors */
      prime_factor = -2;
    }
  else
    prime_factor = find_factor (prime_candidate);
  if (prime_factor == 0)
    {
      is_prime_result = false;
      prime_factor = prime_candidate;
    }
  else
    {
      is_prime_result = true;
    }
}

static opt_get_func_t get_is_prime;

static opt_def_t OPTDEF_is_prime = {
  .name = "is_prime",
  .get_func = get_is_prime,
  .readonly = true,
  .hide_set_o = true,
  .hide_shopt = true,
};

static option_value_t
get_is_prime (opt_def_t const *d, accessor_t why)
{
  check_prime ();
  return is_prime_result;
}

static SHELL_VAR *
assign_prime_candidate (SHELL_VAR *self, char *value, arrayind_t unused, char *key)
{
  char *p;
  intmax_t x = strtoimax(value, &p, 10);
  if (*p)
    {
      /* value error, not an integer */
      return NULL;
    }
  prime_candidate = x;
  have_factor = false;
  return self;
}

static SHELL_VAR *
get_prime_candidate (SHELL_VAR *var)
{
  char *p = itos (prime_candidate);
  xfree (value_cell (var));
  VSETATTR (var, att_integer);
  var_setvalue (var, p);
  return (var);
}

static SHELL_VAR *
assign_prime_divisor (SHELL_VAR *self, char *value, arrayind_t unused, char *key)
{
  /* assignment not allowed */
  return NULL;
}

static SHELL_VAR *
get_prime_divisor (SHELL_VAR *var)
{
  check_prime ();
  char *p = itos (prime_factor);
  xfree (value_cell (var));
  VSETATTR (var, att_integer);
  var_setvalue (var, p);

  if (auto_factorize && prime_candidate != 1)
    {
      if (is_prime_result || prime_candidate == 0)
	/* prime */
	prime_candidate = 1;
      else
	/* composite */
	prime_candidate /= prime_factor;
      have_factor = 0;
    }
  return (var);
}

int
is_prime_builtin_load (char *s)
{
  init_dynamic_var ("PRIME_CANDIDATE", "0", get_prime_candidate, assign_prime_candidate);
  init_dynamic_var ("PRIME_DIVISOR",   "1", get_prime_divisor,   assign_prime_divisor);
  register_option (&OPTDEF_auto_factorize);
  register_option (&OPTDEF_is_prime);
  register_option (&OPTDEF_verbose_factorize);
  return 1;	/* non-zero for success */
}

static int
classify_number (intmax_t c)
{
  if (c < 1)
    {
      printf (_("%jd is not positive\n"), c);
      return 2;
    }
  if (c == 1)
    {
      printf (_("1 is neither prime nor composite\n"));
      return 1;
    }
  intmax_t f = find_factor (c);
  if (f == 0)
    printf (_("%jd is prime\n"), c);
  else
    printf (_("%jd is divisible by %jd, giving %jd\n"), c, f, c/f);
  return f == 0 ? EXECUTION_SUCCESS : EXECUTION_FAILURE;
}

/* function invoked by the "is_prim_resulte" command */
static int
is_prime_builtin(WORD_LIST *list)
{
  _Bool quiet;
  _Bool all_factors;

  reset_internal_getopt ();
  for (int opt;;)
    {
      if (lcurrent && lcurrent->word)
	{
	  /* Stop option processing when we see something that looks like a
	   * negative number */
	  char *w = lcurrent->word->word;
	  if (w[0] == '-' && isdigit (w[1]))
	    {
	      loptend = lcurrent;
	      break;
	    }
	}
      int opt = internal_getopt (list, "aq");
      if (opt == -1)
	break;
      switch (opt)
	{
	  case 'a': all_factors = true; break;
	  case 'q': quiet = true; break;
	  CASE_HELPOPT;
	  default:
	    builtin_usage ();
	    return EX_USAGE;
	}
      list = loptend;
    }

  if (!(list && list->word && list->word->word))
    return classify_number (prime_candidate);

  int errors = 0;
  int composites = 0;
  for (;list; list = list->next)
    {
      char *word = list->word->word;
      if (*word != '-' && !isdigit (*word))
	{
	  printf (_("%s is not a number\n"), word);
	  ++errors;
	  continue;
	}
      char *ep;
      errno = 0;
      intmax_t x = strtoimax(word, &ep, 10);
      if (*ep == '.' && !ep[1+strspn(ep+1, "0123456789")])
	{
	  /* looks like a decimal fraction */
	  printf (_("%s is not an integer\n"), word);
	  continue;
	}
      if (*ep)
	{
	  /* starts with digits but followed by non-digits */
	  printf (_("%s is not a number\n"), word);
	  ++errors;
	  continue;
	}
      if (errno == ERANGE)
	{
	  if (x == INTMAX_MAX)
	    printf (_("%s is too big\n"), word);
	  else
	    printf (_("%s is too small\n"), word);
	  continue;
	}
      int cl = classify_number (x);
      if (cl == EXECUTION_FAILURE)
	++composites;
      else if (cl != EXECUTION_SUCCESS)
	++errors;
    }
  if (errors > 0)
    return 2;
  if (composites > 0)
    return EXECUTION_FAILURE; /* really just false */
  return EXECUTION_SUCCESS;
}

void
is_prime_builtin_unload (char *s)
{
  deregister_option (&OPTDEF_auto_factorize);
  deregister_option (&OPTDEF_is_prime);
  deregister_option (&OPTDEF_verbose_factorize);
  unbind_variable ("PRIME_CANDIDATE");
  unbind_variable ("PRIME_DIVISOR");
}

char const *is_prime_doc[] = {
  "Enable $BASHCONFIG.",
  "",
  "Enables use of the ${BASHCONFIG} dynamic variable.  ",
  "It will yield the current pid of a subshell.",
  (char *)0
};

struct builtin is_prime_struct = {
  "is_prime",
  is_prime_builtin,
  BUILTIN_ENABLED,
  (char**)(void*)is_prime_doc,
  "is_prime N",
  0
};
