/*
 *  When a shell without job control starts a command in the background, it
 *  will block SIGINT and SIGQUIT, and because the inheritance of ignored
 *  signals is based on the signal disposition when the shell itself started,
 *  this cannot be undone within the shell, even by using an explicit
 *      «trap - INT QUIT».
 *  This messes up reporting from some tests when parallel testing is enabled.
 *
 *  This program sets SIGINT and SIGQUIT back to their default dispositions,
 *  and then invokes the requested command.
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

int
main (int argc, char **argv)
{
  if (argc < 2)
    {
      fputs ("Command required\n", stderr);
      return 2;
    }

#define unignore(sig) \
    if (signal (SIG##sig, SIG_DFL) == SIG_ERR) \
      { \
        perror("signal(SIG" #sig ", SIG_DFL)"); \
        return 2; \
      } else 0
  unignore (INT);
  unignore (QUIT);
  unignore (TTIN);
  unignore (TTOU);
  unignore (TSTP);
#undef unignore

  execvp (argv[1], argv + 1);
  int e = errno == ENOEXEC ? 126 : 127;
  perror (argv[1]);
  return e;
}
