/* bashgetopt.h -- extern declarations for stuff defined in bashgetopt.c. */

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

/* See getopt.h for the explanation of these variables. */

#if !defined (__BASH_GETOPT_H)
#  define __BASH_GETOPT_H

#include <stdc.h>

#define GETOPT_EOF	-1
#define GETOPT_HELP	-99

extern char *list_optarg;
extern int list_optflags;

extern int list_optopt;
extern int list_opttype;

extern WORD_LIST *lcurrent;
extern WORD_LIST *loptend;

extern int internal_getopt (WORD_LIST *, char *);
extern void reset_internal_getopt (void);

typedef enum getopt_arg_state_e {
  GO_INVALID,
  GO_VALID,
  GO_NEED_ARG,
  GO_OPT_ARG,
} getopt_arg_state_t;

#ifdef NOT_ASCII
#define MIN_GRAPH 0
#define MAX_GRAPH UCHAR_MAX
#else
#define MIN_GRAPH '!'
#define MAX_GRAPH '~'
#endif

typedef struct getopt_parser_s {
  _Bool lead_minus_minus :1;
  _Bool lead_plus :1;
  _Bool stop_on_solo_minus :1;
  _Bool stop_on_solo_minus_minus :1;
  unsigned char opts[MAX_GRAPH+1-MIN_GRAPH];
} getopt_parser_t;

static inline getopt_parser_t
getopt_parser_start(char const *opts)
{
  getopt_parser_t P = {
    .lead_minus = true,
  };
  for (; *opts;)
    {
      unsigned char c = *opts++;
      if (c == '+')
	P.lead_plus = true; 
      else if (c == '-')
	P.lead_minus_minus = false; 
      else
	{
	  c -= MIN_GRAPH;
	  if (c >= sizeof P.opts)
	    abort();
	  unsigned char *p = &P.opts[c];
	  if (*opts == ':')
	    ++opts, *p = GO_OPT_NEEDED;
	  else if (*opts == ';')
	    ++opts, *p = GO_OPT_ARG;
	  else
	    *p = GO_VALID;
	}
    }
  return P;
}

typedef struct getopt_wl_iterator_s {
  WORD_LIST const **curr;
  size_t next_in_word;
  getopt_parser_t *parser;
  unsigned char opt;
  const char * arg;
  _Bool error :1;
  _Bool force_stop :1;
} getopt_wl_iterator_t;

static inline getopt_wl_iterator_t
getopt_wl_start(getopt_parser_t const *parser, WORD_LIST const **list)
{
  return (getopt_wl_iterator_t) {
    .curr = list,
    .next_in_word = 0,
    .parser = parser,
  };
}

static inline getopt_wl_iterator_t
getopt_wlnc_start(char const *opts, WORD_LIST **list)
{
  return getopt_wl_start(opts, (WORD_LIST const **)list);
}

static inline _Bool
getopt_wl_check(getopt_wl_iterator_t *I)
{
  if (!( I->curr && I->curr[0] && !I->force_stop ))
    return false;

  if (I->next_in_word == 0)
    {
      ;
    }
  unsigned char c = I->curr[0]->word->word[I->next_in_word];
  c -= MIN_GRAPH;
    {
    }
  return true;
}

#define for_each_wlist_getopt(opts, list, I) \
	for (getopt_parser_t P = getopt_parser_start (opts), \
	     getopt_wl_iterator_t I = getopt_wl_start(&P, &(list)) \
	    ; getopt_wl_check(I) \
	    ;)

/* Use the following when the list is WORD_LIST *w rather than WORD_LIST const*w */
#define for_each_wlist_getopt_nc(opts, list, I) \
	for (getopt_parser_t P = getopt_parser_start (opts), \
	     getopt_wl_iterator_t I = getopt_wlnc_start(&P, &(list)) \
	    ; getopt_wl_check(I) \
	    ;)

#endif /* !__BASH_GETOPT_H */
