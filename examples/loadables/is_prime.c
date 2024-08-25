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

static _Bool
num_from_str(intmax_t *result, char const*word)
{
  char *ep;
  errno = 0;
  intmax_t x = strtoimax (word, &ep, 0);
  if (*ep == '.' && !ep[1+strspn (ep+1, "0123456789")])
    {
      /* looks like a decimal fraction */
      printf (_("%s is not an integer\n"), word);
      return false;
    }
  if (*ep)
    {
      /* starts with digits but followed by non-digits */
      printf (_("%s is not a number\n"), word);
      return false;
    }
  if (errno == ERANGE)
    {
      if (x == INTMAX_MAX)
	printf (_("%s is too big\n"), word);
      else
	printf (_("%s is too small\n"), word);
      return false;
    }
  *result = x;
  return true;
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

option_value_t verbose_factorize = false;

static opt_def_t OPTDEF_verbose_factorize = {
  .store = &verbose_factorize,	/* not needed because have both .get_func & .set_func */
  .name = "verbose_factorize",
  .adjust_bashopts = true,
  .hide_set_o = true,
  .help = "Expand the output of \"is_prime\"",
};

option_value_t auto_factorize = true;

static opt_def_t OPTDEF_auto_factorize = {
  .store = &auto_factorize,	/* not needed because have both .get_func & .set_func */
  .name = "auto_factorize",
  .letter = 'Z',
  .adjust_bashopts = true,
  .hide_set_o = true,
  .help = "Automatically divide PRIME_DIVISOR into PRIME_CANDIDATE whenever the former is read",
};

  /* find_factor returns:
     0 if prime;
     -1 if negative;
     the smallest divisor if composite;
   */
static intmax_t
find_factor (intmax_t candidate)
{
  if (candidate < 2)
    {
      /* numbers below 2 are not prime, but in various different ways */
      if (candidate < 0)
	{
	  if (verbose_factorize)
	    fprintf (stderr, "%jd is quickly divisible by -1\n", candidate);
	  return -1;
	}
      if (candidate < 2)
	{
	  if (verbose_factorize)
	    fprintf (stderr, "%jd is quickly divisible by 1 (anything)\n", candidate);
	  return 1;
	}
    }
  if (candidate > 2 && 1 & ~candidate)
    {
      /* all even numbers, except 2, are composite */
      if (verbose_factorize)
	fprintf (stderr, "%jd is quickly divisible by 2\n", candidate);
      return 2;
    }
  if (candidate < 9)
    {
      /* There are no odd composite numbers less than 9 */
      if (verbose_factorize)
	fprintf (stderr, "%jd is quickly prime\n", candidate);
      return 0;
    }

  /* skip 2, since we've already check it */
  static uint8_t const small_primes[] = { /*2,*/ 3, 5, 7, 0 };
  uint8_t const *pf = small_primes;

  for (;*pf;++pf)
    if (candidate % *pf == 0)
      {
	if (verbose_factorize)
	  fprintf (stderr, "%jd is divisible by %d\n", candidate, *pf);
	return *pf;
      }

  /*
   * At this point we know that the candidate is not divisible by any prime up
   * to 7, so now we only need to test for divisibility by numbers which are
   * themselves co-prime with 210=2×3×5×7; that is, numbers of the form
   * x=210n+k, with k∈{ 1, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59,
   *  61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113 123, 129, 131,
   *  137, 139, 143, 149 151, 157, 163, 167, 169, 173, 179 181, 187, 191, 193,
   *  197, 199, 209 }
   *
   * Other cycles are possible:
   *  x=2n+1 = 2×n+k,         k∈{ 1 }
   *  x=6n±1 = 2×3×n+k,       k∈{ 1, 5 }
   *  x=2×3×5×n+k,            k∈{ 1, 7, 11, 13, 17, 19, 23, 29 }
   *  x=2×3×5×7×n+k,	      k∈{ the list of 48 terms above }
   *  x=2×3×5×7×11×n+k,       k∈{ a list of 480 terms }
   *  x=2×3×5×7×11×13×n+k,    k∈{ a list of 5760 terms }
   *  x=2×3×5×7×11×13×17×n+k, k∈{ a list of 92160 terms }
   *  ...etc.
   */

  static uint8_t const rotor_steps[  (2-1)*(3-1)*(5-1)*(7-1) +1 ] = {
    /* start at 1, then add the following on each step: */
    10, 2, 4, 2, 4, 6, 2, 6, 4, 2, 4, 6, 6, 2, 6, 4, 2, 6, 4, 6, 8, 4, 2, 4,
    2, 4, 8, 6, 4, 6, 2, 4, 6, 2, 6, 6, 4, 2, 4, 6, 2, 6, 4, 2, 4, 2, 10, 2,
    0 /* ... and start over at 211, 421, 631, 841, etc */
  };

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
	    fprintf (stderr, "%jd is divisible by %jd\n", candidate, trial_divisor);
	  return trial_divisor;
	}
      if (verbose_factorize)
	fprintf (stderr, "%jd is not divisible by %jd\n", candidate, trial_divisor);
      if (trial_divisor*trial_divisor > candidate)
	{
	  if (verbose_factorize)
	    fprintf (stderr, "%jd is prime\n", candidate);
	  return 0;
	}
    }
}

typedef struct prime_probe_s {
  intmax_t candidate;
  intmax_t factor;
    /*
     * factor == 0 → calculation needed
     * factor == 1 → candidate is prime
     * otherwise factor is a divisor of candidate
     */
} prime_probe_t;

static void
check_prime (prime_probe_t *probe)
{
  if (probe->factor)
    return;

  if (probe->candidate == INTMAX_MIN)
    {
      probe->factor = -2;
      if (verbose_factorize)
	fprintf (stderr, "for INTMAX_MIN (%jd) use fixed factor %jd\n", probe->candidate, probe->factor);
      /* Avoid producing an infinite sequence of '-1' factors */
    }
  else
    {
      probe->factor = find_factor (probe->candidate);
      if (verbose_factorize)
	fprintf (stderr, "find_factor(%jd) returned %jd\n", probe->candidate, probe->factor);
      if (probe->factor == 0)
	probe->factor = 1;
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

static prime_probe_t default_candidate = {42};

static option_value_t
get_is_prime (opt_def_t const *d, accessor_t why)
{
  check_prime (&default_candidate);
  return default_candidate.factor == 1;
}

static SHELL_VAR *
assign_prime_candidate (SHELL_VAR *var, char *value, arrayind_t unused, char *key)
{
  if (! num_from_str(&default_candidate.candidate, value))
    return NULL;	/* value error, not an integer */
  default_candidate.factor = 0;
  return var;
}

static SHELL_VAR *
get_prime_candidate (SHELL_VAR *var)
{
  char *p = itos (default_candidate.candidate);
  if (! p)
    return NULL;

  xfree (value_cell (var));
  VSETATTR (var, att_integer);
  var_setvalue (var, p);
  return var;
}

static SHELL_VAR *
assign_prime_divisor (SHELL_VAR *var, char *value, arrayind_t unused, char *key)
{
  /* assignment only allowed if a factor */

  intmax_t factor;
  if (! num_from_str(&factor, value))
    return NULL;

  if (factor <= 1)
    return NULL;	/* non-positive not allowed */

  if (default_candidate.candidate % factor != 0)
    return NULL;

  default_candidate.factor = factor;

  xfree (value_cell (var));
  VSETATTR (var, att_integer);
  var_setvalue (var, value);
  return var;
}

static SHELL_VAR *
get_prime_divisor (SHELL_VAR *var)
{
  check_prime (&default_candidate);

  char *p = itos (default_candidate.factor);
  xfree (value_cell (var));
  VSETATTR (var, att_integer);
  var_setvalue (var, p);

  if (auto_factorize && default_candidate.candidate != 1)
    {
      if (default_candidate.factor == 1 || default_candidate.candidate == 0)
	/* prime */
	default_candidate.candidate = 1;
      else
	/* composite */
	default_candidate.candidate /= default_candidate.factor;
    }
  default_candidate.factor = 0;
  return var;
}

int
is_prime_builtin_load (char *s)
{
  init_dynamic_var ("PRIME_CANDIDATE", "0", get_prime_candidate, assign_prime_candidate);
  init_dynamic_var ("PRIME_DIVISOR",   "1", get_prime_divisor,   assign_prime_divisor);
  op_result_t r;
  r = register_option (&OPTDEF_auto_factorize);		if (BadResult (r)) fprintf (stderr, "Cannot register shopt %s (%#x)\n", "auto_factorize", ResultC (r));
  r = register_option (&OPTDEF_is_prime);		if (BadResult (r)) fprintf (stderr, "Cannot register shopt %s (%#x)\n", "is_prime", ResultC (r));
  r = register_option (&OPTDEF_verbose_factorize);	if (BadResult (r)) fprintf (stderr, "Cannot register shopt %s (%#x)\n", "verbose_factorize", ResultC (r));

  return 1;	/* non-zero for success */
}

static int
classify_number (prime_probe_t *p, _Bool verbose, _Bool all_factors)
{
  if (p->candidate == 0)
    {
      if (verbose)
	printf (_("%jd is universally divisible\n"), p->candidate);
      if (all_factors)
	printf ("*\n");	/* infinite set of divisors */
      return 2;
    }
  if (p->candidate == 1)
    {
      if (verbose)
	printf (_("%jd is the multiplicative identity\n"), p->candidate);
      if (all_factors)
	printf ("\n");
      return 0;
    }
  check_prime (p);
  if (p->factor == 1)
    {
      if (verbose)
	printf (_("%jd is prime\n"), p->candidate);
      if (all_factors)
	printf ("%jd\n", p->candidate);
      return EXECUTION_SUCCESS;
    }
  if (all_factors)
    {
      if (verbose)
	if (p->candidate < 0)
	  printf (_("%jd is negative\n"), p->candidate);
	else
	  printf (_("%jd is composite\n"), p->candidate);
      if (p->candidate != -1)
	while (p->factor != 1)
	  {
	    printf ("%jd ", p->factor);
	    p->candidate /= p->factor;
	    p->factor = 0;
	    check_prime (p);
	  }
      printf ("%jd\n", p->candidate);
    }
  else
    {
      if (verbose)
	printf (_("%jd is divisible by %jd giving %jd\n"),
		p->candidate, p->factor, p->candidate/p->factor);
    }
  return EXECUTION_FAILURE;	/* false */
}

/* function invoked by the "is_prim_resulte" command */
static int
is_prime_builtin (WORD_LIST *list)
{
  _Bool quiet = false;
  _Bool all_factors = false;

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
    }
  list = loptend;

  if (!(list && list->word && list->word->word))
    return classify_number (&default_candidate, !quiet, all_factors);

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
      intmax_t x;
      if (! num_from_str(&x, word))
	{
	  ++errors;
	  continue;
	}
      prime_probe_t cx = {x};
      int cl = classify_number (&cx, !quiet, all_factors);
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

static char const *is_prime_doc[] = {
  "is_prime [-a] [-q] [NUMBER...]",
  "",
  "Test each NUMBER for primality.",
  "If no NUMBER is given, report on $PRIME_CANDIDATE instead.",
  "",
  "\tWith -a, show all prime factors of each NUMBER",
  "\tWithout -q, explain why each NUMBER is or isn't prime",
  "",
  "Exit status is 0 if all tested numbers are prime, non-zero otherwise",
  NULL
};

struct builtin is_prime_struct = {
  .name	    = "is_prime",
  .function = is_prime_builtin,
  .flags    = BUILTIN_ENABLED,
  .long_doc = is_prime_doc,
  .short_doc= "is_prime [-a] [-q] [NUMBER...]",
};
