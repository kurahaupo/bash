/* test.c - GNU test program (ksb and mjb) */

/* Modified to run with the GNU shell Apr 25, 1988 by bfox. */

/* Copyright (C) 1987-2024 Free Software Foundation, Inc.

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

/* Define PATTERN_MATCHING to get the csh-like =~ and !~ pattern-matching
   binary operators. */
/* #define PATTERN_MATCHING */

#if defined (HAVE_CONFIG_H)
#  include <config.h>
#endif

#include <stdio.h>

#include "bashtypes.h"

#if !defined (HAVE_LIMITS_H) && defined (HAVE_SYS_PARAM_H)
#  include <sys/param.h>
#endif

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include <errno.h>
#if !defined (errno)
extern int errno;
#endif /* !errno */

#if !defined (_POSIX_VERSION) && defined (HAVE_SYS_FILE_H)
#  include <sys/file.h>
#endif /* !_POSIX_VERSION */
#include "posixstat.h"
#include "filecntl.h"
#include "stat-time.h"

#include "bashintl.h"

#include "shell.h"

#include "options.h"
#include "pathexp.h"
#include "test.h"
#include "builtins/common.h"

#include <glob/strmatch.h>

#if !defined (STRLEN)
#  define STRLEN(s) ((s)[0] ? ((s)[1] ? ((s)[2] ? strlen(s) : 2) : 1) : 0)
#endif

#if !defined (STREQ)
#  define STREQ(a, b)		(strcmp (a, b) == 0)
#endif /* !STREQ */
#define STRCOLLEQ(a, b) 	(strcoll (a, b) == 0)

/* single-character tokens like `!`, `(`, and `)` */
#define ISTOKEN(s, c)		((s)[0] == (c) && (s)[1] == '\0')

/* two-character tokens like `-z` and `!=` */
#define ISTOKEN2(s, c1, c2)	((s)[0] == (c1) && (s)[1] == (c2) && (s)[2] == '\0')

#ifndef ISOPTION	/* also in builtins/common.h */
/* two-character tokens starting with '-' are very common ... */
#  define ISOPTION(s, c)	ISTOKEN2 (s, '-', c)
#endif /*ISOPTION*/

#define ISANDOR(s)  		(ISOPTION(s, 'a') || ISOPTION(s, 'o'))

#define ISEMPTY(s)		((s)[0] == 0)

#if !defined (R_OK)
/* These should be in fcntl.h and/or unistd.h.
   On a POSIX system, these bits match `.st_mode & S_IRWXO` in `struct stat`.
 */
#  define R_OK 4		/* S_IROTH */
#  define W_OK 2		/* S_IWOTH */
#  define X_OK 1		/* S_IXOTH */
#  define F_OK 0		/* (empty set) */
#endif /* R_OK */

#define EQ	0
#define NE	1
#define LT	2
#define GT	3
#define LE	4
#define GE	5

#define NT	0
#define OT	1
#define EF	2

/* Internally all logic follows normal C rules (non-zero is true), which is
   then converted to a shell exit status (non-zero is failure or false), using
   bool_to_status as the last step.
*/

static inline int
bool_to_status(_Bool value)
{
  return ! value;
}

#define TEST_ERREXIT_STATUS	2

static procenv_t test_exit_buf;
static int test_error_return;
#define test_exit(val) \
	do { test_error_return = val; sh_longjmp (test_exit_buf, 1); } while (0)

extern int sh_stat (const char *, struct stat *);

static int pos;			/* The offset of the current argument in ARGV. */
static int argc;		/* The number of arguments present in ARGV. */
static char const * const *argv;	/* The argument list. */

static _Bool unary_test (char const *, char const *, int);
static _Bool binary_test (char const *, char const *, char const *, int);

static _Bool unary_operator (void);
static _Bool binary_operator (void);
static _Bool two_arguments (void);
static _Bool three_arguments (void);
static _Bool posixtest (int nargs, _Bool top);
static _Bool disjunction (void);

static void
test_syntax_error (char const *format, char const *arg)
{
  builtin_error (format, arg);
  test_exit (TEST_ERREXIT_STATUS);
}

/*
 * beyond - call when we're beyond the end of the argument list (an
 *	error condition)
 */
static void
beyond (void)
{
  test_syntax_error (_("argument expected"), (char *)NULL);
}

/* Syntax error for when an integer argument was expected, but
   something else was found. */
static void
integer_expected_error (char const *pch)
{
  test_syntax_error (_("%s: integer expected"), pch);
}

/* Increment our position in the argument list.  Check that we're not
   past the end of the argument list.  This check is suppressed if the
   argument is false. */

static inline _Bool
want_args (int by)
{
  return pos + by <= argc;
}

static inline void
need_args (int by)
{
  if (! want_args (by))
    beyond ();
}

/* advance_and_need_1_more advances past the current token, and then also
 * checks that there's at least one more arg after that. */
static inline void
advance_and_need_1_more (void)
{
  ++pos;
  need_args (1);
}

static inline void
advance_by (int by)
{
  if (pos + by > argc)
    beyond ();
  pos += by;
}

static inline _Bool
advance_after_by (int by, _Bool exp_value)
{
  advance_by (by);
  return exp_value;
}

/*
 * expr:
 *	disjunction
 */
static _Bool
expr (void)
{
  return disjunction ();
}

/*
 * disjunction:
 *	conjunction [ '-o' conjunction ]...
 */
static _Bool
disjunction (void)
{
  if (pos >= argc)
    beyond ();

  _Bool value = conjunction ();
  while (want_args (2) && ISOPTION (argv[pos], 'o'))
    {
      ++pos;
      value |= conjunction ();
    }

  return value;
}

/*
 * conjunction:
 *	neg_term [ '-a' neg_term ] ...
 */
static _Bool
conjunction (void)
{
  _Bool value = neg_term ();
  while (want_args (2) && ISOPTION (argv[pos], 'a'))
    {
      ++pos;
      value &= neg_term ();
    }
  return value;
}

/*
 * neg_term - parse and evaluate a term preceded by any number of `!`; the
 * return status is that of the term, inverted if the the number of `!` is odd.
 */
static _Bool
neg_term (void)
{
  _Bool neg = 0;
  /* Deal with leading `not's. */
  while (want_args (2) && ISTOKEN (argv[pos], '!'))
    {
      ++pos;
      --neg;			/* logical inversion */
    }
  return neg ^ term();
}

/*
 * term - parse and evaluate a term
 *
 * term ::=
 *	'-'('a'|'b'|'c'|'d'|'e'|'f'|'g'|'h'|'k'|'p'|'r'|'s'|'u'|'w'|'x'|'G'|'L'|'N'|'O'|'S') filename
 * 	'-t' [int]
 *	'-'('n'|'z') string
 *	'-'('v'|'R') varname
 *	'-o' option
 *	string
 *	string ('!='|'='|'=='|'<'|'>') string
 *	<int> '-'(eq|ne|le|lt|ge|gt) <int>
 *	file '-'(nt|ot|ef) file
 *	'(' <posix_test> ')'  (only if there are fewer than 5 remaining args)
 *	'(' <disjunction> ')'
 * int ::=
 *	positive and negative integers
 */
static _Bool
term (void)
{
  need_args (1);

  /* A paren-bracketed argument. */
  if (ISTOKEN (argv[pos], '('))	/* ) */
    {
      int nargs, count;

      ++pos;
      need_args (1);
      /* Steal an idea from coreutils and scan forward to check where the right
	 paren appears to prevent some ambiguity. If we find a valid sub-
	 expression that has 1-4 arguments, call posixtest on it. Handle
	 nested subexpressions. ( */
      for (nargs = count = 1; pos + nargs < argc; nargs++)
	{
	  if (ISTOKEN (argv[pos + nargs], ')'))
	    count--;
	  else if (ISTOKEN (argv[pos + nargs], '('))	/*) */
	    count++;
	  if (count == 0)
	    break;
	}
      /* only use posixtest if we have a valid parenthesized expression */
      _Bool value = 0;
	value = posixtest (nargs, false);
      if (argv[pos] == 0)	/* ( */
	test_syntax_error (_("`)' expected"), (char *)NULL);
      else if (! ISTOKEN (argv[pos], ')'))	/* ( */
	test_syntax_error (_("`)' expected, found %s"), argv[pos]);
      ++pos;
      return (value);
    }

  /* are there enough arguments left that this could be dyadic? */
  if (pos + 2 < argc && test_binop (argv[pos + 1]))
    return binary_operator ();

  /* Might be a switch type argument -- make sure we have enough arguments for
     the unary operator and argument */
  if (pos + 1 < argc && test_unop (argv[pos]))
    return unary_operator ();

  /* test whether argument is non-empty */
  _Bool value = ! ISEMPTY (argv[pos]);
  ++pos;
  return value;
}

static int
stat_mtime (const char *fn, struct stat *st, struct timespec *ts)
{
  int r;

  r = sh_stat (fn, st);
  if (r < 0)
    return r;
  *ts = get_stat_mtime (st);
  return 0;
}

static int
filecomp (const char *s, const char *t, int op)
{
  struct stat st1, st2;
  struct timespec ts1, ts2;
  int r1, r2;

  if ((r1 = stat_mtime (s, &st1, &ts1)) < 0)
    {
      if (op == EF)
	return (false);
    }
  if ((r2 = stat_mtime (t, &st2, &ts2)) < 0)
    {
      if (op == EF)
	return (false);
    }

  switch (op)
    {
    case OT: return (r1 < r2 || (r2 == 0 && timespec_cmp (ts1, ts2) < 0));
    case NT: return (r1 > r2 || (r1 == 0 && timespec_cmp (ts1, ts2) > 0));
    case EF: return (same_file (s, t, &st1, &st2));
    }
  return false;
}

static int
arithcomp (char const *s, char const *t, int op, int flags)
{
  intmax_t l, r;
  int expok;

  if (flags & TEST_ARITHEXP)	/* conditional command */
    {
      int eflag;

      eflag = (shell_compatibility_level > 51) ? 0 : EXP_EXPANDED;
      l = evalexp (s, eflag, &expok);
      if (expok == 0)
	return false;		/* should probably longjmp here */
      r = evalexp (t, eflag, &expok);
      if (expok == 0)
	return false;		/* ditto */
    }
  else
    {
      if (valid_number (s, &l) == 0)
	integer_expected_error (s);
      if (valid_number (t, &r) == 0)
	integer_expected_error (t);
    }

  switch (op)
    {
    case EQ: return l == r;
    case NE: return l != r;
    case LT: return l < r;
    case GT: return l > r;
    case LE: return l <= r;
    case GE: return l >= r;
    }

  return false;
}

static int
patcomp (char const *string, char const *pat, int op)
{
  _Bool m = strmatch (pat, string, FNMATCH_EXTFLAG | FNMATCH_IGNCASE);
  return (op == EQ) != m;
}

static _Bool
binary_test (char const *op, char const *arg1, char const *arg2, int flags)
{
  _Bool patmatch = flags & TEST_PATMATCH;

  if (op[0] == '=' && (op[1] == '\0' || (op[1] == '=' && op[2] == '\0')))
    return patmatch ? patcomp (arg1, arg2, EQ)
		    : STREQ (arg1, arg2);
  else if ((op[0] == '>' || op[0] == '<') && op[1] == '\0')
    {
#if defined (HAVE_STRCOLL)
      /* POSIX interp 375 */
      if (posixly_correct && (flags & TEST_LOCALE))
	return ((op[0] == '>') ? (strcoll (arg1, arg2) > 0) : (strcoll (arg1, arg2) < 0));
      else if (shell_compatibility_level > 40 && (flags & TEST_LOCALE))
	return ((op[0] == '>') ? (strcoll (arg1, arg2) > 0) : (strcoll (arg1, arg2) < 0));
      else
#endif
	return ((op[0] == '>') ? (strcmp (arg1, arg2) > 0) : (strcmp (arg1, arg2) < 0));
    }
  else if (op[0] == '!' && op[1] == '=' && op[2] == '\0')
    return patmatch ? patcomp (arg1, arg2, NE)
		    : ! STREQ (arg1, arg2);


  else if (op[2] == 't')
    {
      switch (op[1])
	{
	case 'n': return filecomp (arg1, arg2, NT);		/* -nt */
	case 'o': return filecomp (arg1, arg2, OT);		/* -ot */
	case 'l': return arithcomp (arg1, arg2, LT, flags);	/* -lt */
	case 'g': return arithcomp (arg1, arg2, GT, flags);	/* -gt */
	}
    }
  else if (op[1] == 'e')
    {
      switch (op[2])
	{
	case 'f': return filecomp (arg1, arg2, EF);		/* -ef */
	case 'q': return arithcomp (arg1, arg2, EQ, flags);	/* -eq */
	}
    }
  else if (op[2] == 'e')
    {
      switch (op[1])
	{
	case 'n': return arithcomp (arg1, arg2, NE, flags);	/* -ne */
	case 'g': return arithcomp (arg1, arg2, GE, flags);	/* -ge */
	case 'l': return arithcomp (arg1, arg2, LE, flags);	/* -le */
	}
    }

  return false;			/* should never get here */
}

static _Bool
binary_operator (void)
{
  char const *w = argv[pos + 1];
  if (  ISTOKEN2 (w, '!', '=')
     || ISTOKEN  (w, '=')
     || ISTOKEN2 (w, '=', '=')
     || ISTOKEN  (w, '<')
     || ISTOKEN  (w, '>'))
    {
      /* POSIX interp 375 11/9/2022 */
      return advance_after_by (3, binary_test (w, argv[pos], argv[pos + 2], (posixly_correct ? TEST_LOCALE : 0)));
    }

#if defined (PATTERN_MATCHING)
  if (ISTOKEN2 (w, '=', '~') || ISTOKEN2 (w, '!', '~'))
    return advance_after_by (3, patcomp (argv[pos], argv[pos + 2], w[0] == '=' ? EQ : NE));
#endif

  if ((w[0] != '-' || w[3] != '\0') || test_binop (w) == 0)
    test_syntax_error (_("%s: binary operator expected"), w); /* NORETURN */

  return advance_after_by (3, binary_test (w, argv[pos], argv[pos + 2], 0));
}

static _Bool
unary_operator (void)
{
  char const *op;
  intmax_t r;

  op = argv[pos];
  if (test_unop (op) == 0)
    return (false);

  /* the only tricky case is `-t', which may or may not take an argument. */
  if (posixly_correct == 0 && op[1] == 't')
    {
      ++pos;
      if (want_args (1))
	{
	  if (valid_number (argv[pos], &r))
	    {
	      ++pos;
	      return (unary_test (op, argv[pos - 1], 0));
	    }
	  else if (argc >= 5 && ISANDOR (argv[pos]))
	    return (unary_test (op, "1", 0));
	  else
	    integer_expected_error (argv[pos]);
	}
      else
	/* this is not called when pos == argc; the one-argument code is used */
	return (unary_test (op, "1", 0));
    }

  /* All of the unary operators take an argument, so we first call
     advance_by (2), which checks to make sure that there is an
     argument, and then advances pos right past it.  This means that
     pos - 1 is the location of the argument. */
  advance_by (2);
  return (unary_test (op, argv[pos - 1], 0));
}

static _Bool
unary_test (char const *op, char const *arg, int flags)
{
  intmax_t r;
  struct stat stat_buf;
  struct timespec mtime, atime;
  SHELL_VAR *v;
  int aflags;

  switch (op[1])
    {
    case 'a':			/* file exists in the file system? */
    case 'e':
      return (sh_stat (arg, &stat_buf) == 0);

    case 'r':			/* file is readable? */
      return (sh_eaccess (arg, R_OK) == 0);

    case 'w':			/* File is writeable? */
      return (sh_eaccess (arg, W_OK) == 0);

    case 'x':			/* File is executable? */
      return (sh_eaccess (arg, X_OK) == 0);

    case 'O':			/* File is owned by you? */
      return (sh_stat (arg, &stat_buf) == 0 &&
	      (uid_t)current_user.euid == (uid_t)stat_buf.st_uid);

    case 'G':			/* File is owned by your group? */
      return (sh_stat (arg, &stat_buf) == 0 &&
	      (gid_t)current_user.egid == (gid_t)stat_buf.st_gid);

    case 'N':
      if (sh_stat (arg, &stat_buf) < 0)
	return (false);
      atime = get_stat_atime (&stat_buf);
      mtime = get_stat_mtime (&stat_buf);
      return (timespec_cmp (mtime, atime) > 0);

    case 'f':			/* File is a file? */
      if (sh_stat (arg, &stat_buf) < 0)
	return (false);

      /* -f is true if the given file exists and is a regular file. */
#if defined (S_IFMT)
      return (S_ISREG (stat_buf.st_mode) || (stat_buf.st_mode & S_IFMT) == 0);
#else
      return (S_ISREG (stat_buf.st_mode));
#endif /* !S_IFMT */

    case 'd':			/* File is a directory? */
      return (sh_stat (arg, &stat_buf) == 0 && (S_ISDIR (stat_buf.st_mode)));

    case 's':			/* File has something in it? */
      return (sh_stat (arg, &stat_buf) == 0 && stat_buf.st_size > (off_t)0);

    case 'S':			/* File is a socket? */
#if !defined (S_ISSOCK)
      return (false);
#else
      return (sh_stat (arg, &stat_buf) == 0 && S_ISSOCK (stat_buf.st_mode));
#endif /* S_ISSOCK */

    case 'c':			/* File is character special? */
      return (sh_stat (arg, &stat_buf) == 0 && S_ISCHR (stat_buf.st_mode));

    case 'b':			/* File is block special? */
      return (sh_stat (arg, &stat_buf) == 0 && S_ISBLK (stat_buf.st_mode));

    case 'p':			/* File is a named pipe? */
#ifndef S_ISFIFO
      return (false);
#else
      return (sh_stat (arg, &stat_buf) == 0 && S_ISFIFO (stat_buf.st_mode));
#endif /* S_ISFIFO */

    case 'L':			/* Same as -h  */
    case 'h':			/* File is a symbolic link? */
#if !defined (S_ISLNK) || !defined (HAVE_LSTAT)
      return (false);
#else
      return ((arg[0] != '\0') &&
	      (lstat (arg, &stat_buf) == 0) && S_ISLNK (stat_buf.st_mode));
#endif /* S_IFLNK && HAVE_LSTAT */

    case 'u':			/* File is setuid? */
      return (sh_stat (arg, &stat_buf) == 0 && (stat_buf.st_mode & S_ISUID) != 0);

    case 'g':			/* File is setgid? */
      return (sh_stat (arg, &stat_buf) == 0 && (stat_buf.st_mode & S_ISGID) != 0);

    case 'k':			/* File has sticky bit set? */
#if !defined (S_ISVTX)
      /* This is not Posix, and is not defined on some Posix systems. */
      return (false);
#else
      return (sh_stat (arg, &stat_buf) == 0 && (stat_buf.st_mode & S_ISVTX) != 0);
#endif

    case 't':			/* File fd is a terminal? */
      if (valid_number (arg, &r) == 0)
	integer_expected_error (arg);
      return ((r == (int)r) && isatty ((int)r));

    case 'n':			/* True if arg has some length. */
      return (arg[0] != '\0');

    case 'z':			/* True if arg has no length. */
      return (arg[0] == '\0');

    case 'o':			/* True if option `arg' is set. */
      {
	opt_def_t const *d = find_option (arg);
	return d && get_opt_value (d, Accessor (set_o));
      }

    case 'v':
#if defined (ARRAY_VARS)
      aflags = array_expand_once ? AV_NOEXPAND : 0;
      if (valid_array_reference (arg, aflags))
	{
	  char *t;
	  int ret;
	  array_eltstate_t es;

	  /* Let's assume that this has already been expanded once. */
	  /* bash-5.2 fix with corresponding fix to execute_cmd.c:
	     execute_cond_node() that passes TEST_ARRAYEXP in FLAGS */

	  if (shell_compatibility_level > 51)
	    /* Allow associative arrays to use `test -v array[@]' to look for
	       a key named `@'. */
	    aflags |= AV_ATSTARKEYS;	/* XXX */
	  init_eltstate (&es);
	  t = get_array_value (arg, aflags | AV_ALLOWALL, &es);
	  ret = t != NULL;
	  if (es.subtype > 0)	/* subscript is * or @ */
	    free (t);
	  flush_eltstate (&es);
	  return ret;
	}
      else if (valid_number (arg, &r))	/* -v n == is $n set? */
	return r >= 0 && r <= number_of_args ();
#endif
      v = find_variable (arg);
      if (v && ! invisible_p (v))
	{
#if defined (ARRAY_VARS)
	  /* [[ -v foo ]] == [[ -v foo[0] ]] */
	  if (array_p (v))
	    return array_reference (array_cell (v), 0) != NULL;
	  if (assoc_p (v))
	    return assoc_reference (assoc_cell (v), "0") != NULL;
#endif
	  return var_isset (v);
	}
      return false;

    case 'R':
      v = find_variable_noref (arg);
      return (v && invisible_p (v) == 0 && var_isset (v) && nameref_p (v));
    }

  /* We can't actually get here, but this shuts up gcc. */
  return (false);
}

/* Return true if OP is one of the test command's binary operators. */
int
test_binop (char const *op)
{
  if (op[0] == '=' && op[1] == '\0')
    return (1);			/* '=' */
  else if ((op[0] == '<' || op[0] == '>') && op[1] == '\0')	/* string <, > */
    return (1);
  else if ((op[0] == '=' || op[0] == '!') && op[1] == '=' && op[2] == '\0')
    return (1);			/* `==' and `!=' */
#if defined (PATTERN_MATCHING)
  else if (op[2] == '\0' && op[1] == '~' && (op[0] == '=' || op[0] == '!'))
    return (1);
#endif
  else if (op[0] != '-' || op[1] == '\0' || op[2] == '\0' || op[3] != '\0')
    return (0);
  else
    {
      if (op[2] == 't')
	switch (op[1])
	  {
	  case 'n':		/* -nt */
	  case 'o':		/* -ot */
	  case 'l':		/* -lt */
	  case 'g':		/* -gt */
	    return (1);
	  default:
	    return (0);
	  }
      else if (op[1] == 'e')
	switch (op[2])
	  {
	  case 'q':		/* -eq */
	  case 'f':		/* -ef */
	    return (1);
	  default:
	    return (0);
	  }
      else if (op[2] == 'e')
	switch (op[1])
	  {
	  case 'n':		/* -ne */
	  case 'g':		/* -ge */
	  case 'l':		/* -le */
	    return (1);
	  default:
	    return (0);
	  }
      else
	return (0);
    }
}

/* Return non-zero if OP is one of the test command's unary operators. */
int
test_unop (char const *op)
{
  if (op[0] != '-' || (op[1] && op[2] != 0))
    return (0);

  switch (op[1])
    {
    case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'k': case 'n':
    case 'o': case 'p': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'z':
    case 'G': case 'L': case 'O': case 'S': case 'N':
    case 'R':
      return (1);
    }

  return (0);
}

static _Bool
two_arguments (void)
{
  if (ISTOKEN (argv[pos], '!'))
    {
      ++pos;
      return ISEMPTY (argv[pos++]);
    }
  else if (argv[pos][0] == '-' && argv[pos][1] && argv[pos][2] == '\0')
    {
      if (test_unop (argv[pos]))
	return (unary_operator ());
      else
	test_syntax_error (_("%s: unary operator expected"), argv[pos]);
    }
  else
    test_syntax_error (_("%s: unary operator expected"), argv[pos]);

  return (0);
}

static _Bool
three_arguments (void)
{
  need_args (3);

  char const *mid = argv[pos + 1];

  if (test_binop (mid))
    return binary_operator ();

  if (ISOPTION (mid, 'a'))
    return advance_after_by (3, ! ISEMPTY (argv[pos]) && ! ISEMPTY (argv[pos + 2]));

  if (ISOPTION (mid, 'o'))
    return advance_after_by (3, ! ISEMPTY (argv[pos]) || ! ISEMPTY (argv[pos + 2]));

  if (ISTOKEN (argv[pos], '!'))
    {
      ++pos;
      return advance_after_by (2, !two_arguments ());
    }

  if (ISTOKEN (argv[pos], '(') && ISTOKEN (argv[pos + 2], ')'))
    {
      return advance_after_by (2, ! ISEMPTY (argv[++pos]));
    }

  test_syntax_error (_("%s: binary operator expected"), argv[pos + 1]);
}

/* This is an implementation of a Posix.2 proposal by David Korn. */
static _Bool
posixtest (int nargs, _Bool top)
{
  if (top || shell_compatibility_level > 52 && pos + nargs < argc && nargs <= 4)
    switch (nargs)
      {
      case 0:
	return false;

      case 1:
	return advance_after_by (1, ! ISEMPTY (argv[pos]));

      case 2:
	return two_arguments ();

      case 3:
	return three_arguments ();

      case 4:
	if (ISTOKEN (argv[pos], '!'))
	  {
	    ++pos;
	    return ! three_arguments ();
	  }
	else if (ISTOKEN (argv[pos], '(') && ISTOKEN (argv[pos + 3], ')'))
	  {
	    ++pos;
	    return advance_after_by (1, two_arguments ());
	  }
      }
  return disjunction ();
}

#if defined (COND_COMMAND)
int
cond_test (char const *op, char const *arg1, char const *arg2, int flags)
{
  int code, ret;

  code = setjmp_nosigs (test_exit_buf);

  if (code)
    return (test_error_return);

  ret = arg2 ? binary_test (op, arg1, arg2, flags) : unary_test (op, arg1, flags);

  return (ret ? EXECUTION_SUCCESS : EXECUTION_FAILURE);
}
#endif

/*
 * [:
 *	'[' [ disjunction ] ']'
 * test:
 *	test [ disjunction ]
 */
int
test_command (int margc, char **margv)
{
  int code = setjmp_nosigs (test_exit_buf);

  if (code)
    return (test_error_return);

  argc = margc;
  argv = margv;
  pos = 1;

  if (margv[0] && margv[0][0] == '[' && margv[0][1] == '\0')
    {
      --argc;

      if (margv[argc] && (margv[argc][0] != ']' || margv[argc][1]))
	test_syntax_error (_("missing `]'"), (char *)NULL);
    }

  if (pos >= argc)
    test_exit (bool_to_status (false));

  _Bool value = posixtest (argc - 1, true);

  if (pos != argc)
    {
      if (want_args (1) && argv[pos][0] == '-')
	test_syntax_error (_("syntax error: `%s' unexpected"), argv[pos]);
      else
	test_syntax_error (_("too many arguments"), (char *)NULL);
    }

  test_exit (bool_to_status (value));
}
