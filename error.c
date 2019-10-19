/* error.c -- Functions for handling errors. */
/* Copyright (C) 1993 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2, or (at your option) any later
   version.

   Bash is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License along
   with Bash; see the file COPYING.  If not, write to the Free Software
   Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. */

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>

#if defined (HAVE_VFPRINTF)
#include <stdarg.h>
#endif

#include <errno.h>
#if !defined (errno)
extern int errno;
#endif /* !errno */

#include "bashansi.h"
#include "flags.h"
#include "error.h"
#include "command.h"
#include "general.h"

extern int interactive_shell;
extern char *dollar_vars[];
extern char *shell_name;
extern char *the_current_maintainer;
#if defined (JOB_CONTROL)
extern pid_t shell_pgrp;
#endif /* JOB_CONTROL */

/* Return the name of the shell or the shell script for error reporting. */
char *
get_name_for_error ()
{
  char *name = (char *) NULL;

  if (!interactive_shell)
    name = dollar_vars[0];
  if (!name && shell_name && *shell_name)
    name = base_pathname (shell_name);
  if (!name)
    name = "bash";

  return (name);
}

/* Report an error having to do with FILENAME. */
void
file_error (filename)
     char *filename;
{
  report_error ("%s: %s", filename, strerror (errno));
}

#if !defined (HAVE_VFPRINTF)
void
programming_error (reason, arg1, arg2, arg3, arg4, arg5)
     char *reason;
{
#if defined (JOB_CONTROL)
  give_terminal_to (shell_pgrp);
#endif /* JOB_CONTROL */

  report_error (reason, arg1, arg2);
  fprintf (stderr, "Report this to %s\n", the_current_maintainer);
  fprintf (stderr, "Stopping myself...");
  fflush (stderr);
  abort ();
}

void
report_error (format, arg1, arg2, arg3, arg4, arg5)
     char *format;
{
  fprintf (stderr, "%s: ", get_name_for_error ());

  fprintf (stderr, format, arg1, arg2, arg3, arg4, arg5);
  fprintf (stderr, "\n");
  if (exit_immediately_on_error)
    exit (1);
}  

void
fatal_error (format, arg1, arg2, arg3, arg4, arg5)
     char *format;
{
  fprintf (stderr, "%s: ", get_name_for_error ());

  fprintf (stderr, format, arg1, arg2, arg3, arg4, arg5);
  fprintf (stderr, "\n");

  exit (2);
}

void
internal_error (format, arg1, arg2, arg3, arg4, arg5)
     char *format;
{
  fprintf (stderr, "%s: ", get_name_for_error ());

  fprintf (stderr, format, arg1, arg2, arg3, arg4, arg5);
  fprintf (stderr, "\n");
}

#else /* We have VARARGS support, so use it. */

#ifdef AVOID_STDIO_ON_ERROR
# define fprintf  bash_fprintf
# define vfprintf bash_vfprintf
# define fflush(x) (void)0

void bash_vfprintf (FILE *ignore, char const *format, va_list args)
{
    char nbuf[40]; /* enough space to convert any kind of integer */
    char *ptr;
    int num;
    int o=0;
    while (*format) {
        size_t l = strcspn (format+o, "%")+o;
        if (l>0)
        {
            write (2, format, l);
            format += l;
        }
        o=0;
            if (*format != '%') return;
            if (format[1] == '%')
            {
                ++format;
                l = strcspn (format+1, "%")+1;
            }
        ++format; /* skip '%' */
        switch (*format++)
        {
            case 0: return;
            case '%': o=1; break;
            case 'd':
                num = va_arg(args, int);
            {
                int sgn = 0;
                if (num < 0)
                {
                    num = -num;
                    sgn = -1;
                }
                ptr = &nbuf[sizeof nbuf];
                do {
                    *--ptr = '0' + num % 10;
                    num /= 10;
                } while (num != 0);
                if (sgn)
                    *--ptr = '-';
                write(2, ptr, &nbuf[sizeof nbuf] - ptr);
                break;
            } break;
            case 'c':
                nbuf[0] = va_arg(args, int); 
                write(2, nbuf, 1);
                break;
            case 's':
                ptr = va_arg(args, char*);
                write(2, ptr, strlen(ptr));
                break;
        }
    }
}

void bash_fprintf(FILE *ignore, char const *format, ...)
{
  va_list args;
  va_start (args, format);
  bash_vfprintf (stderr, format, args);
  va_end (args);
}
#endif


void
programming_error (char const *format, ...)
{
  va_list args;

#if defined (JOB_CONTROL)
  give_terminal_to (shell_pgrp);
#endif /* JOB_CONTROL */

  va_start (args, format);
  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");
  va_end (args);

  fprintf (stderr, "Tell %s to fix this someday.\n", the_current_maintainer);
  fprintf (stderr, "Stopping myself...");
  fflush (stderr);
  abort ();
}

void
report_error (char const *format, ...)
{
  va_list args;

  fprintf (stderr, "%s: ", get_name_for_error ());
  va_start (args, format);
  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");

  va_end (args);
  if (exit_immediately_on_error)
    exit (1);
}

void
fatal_error (char const *format, ...)
{
  va_list args;

  fprintf (stderr, "%s: ", get_name_for_error ());
  va_start (args, format);
  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");

  va_end (args);
  exit (2);
}

void
internal_error (char const *format, ...)
{
  va_list args;

  fprintf (stderr, "%s: ", get_name_for_error ());
  va_start (args, format);
  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");

  va_end (args);
}

void
itrace (char const *format, ...)
{
  va_list args;

  fprintf(stderr, "TRACE: pid %d: ", getpid());
  va_start (args, format);
  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");

  va_end (args);

  fflush(stderr);
}

#if 0
/* A trace function for silent debugging -- doesn't require a control
   terminal. */
trace (char const *format, ...)
{
  va_list args;
  
  static FILE *tracefp = (FILE *)NULL;

  if (tracefp == NULL)
    tracefp = fopen("/usr/tmp/bash-trace.log", "a+");

  if (tracefp == NULL)
    tracefp = stderr;
  else
    fcntl (fileno (tracefp), F_SETFD, 1);     /* close-on-exec */

  fprintf(tracefp, "TRACE: pid %d: ", getpid());

  va_start (args, format);
  vfprintf (tracefp, format, args);
  fprintf (tracefp, "\n");

  va_end (args);

  fflush(tracefp);
}
#endif
#endif /* HAVE_VFPRINTF */
