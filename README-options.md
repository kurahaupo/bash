This file explains the changes introduced by this git branch.

Firstly, it's built on top of the
[depends](https://github.com/kurahaupo/bash/tree/depends) branch, because it
makes so many changes to the dependencies.

Secondly, it interoduces a new framework for option handlers, such that they
are registered during program startup. This also supports registration (and
deregistration) of additional options by loadable builtins.

Thirdly, all the existing C<flags>, C<set>, and  C<shopt> options have been
rewritten to use this new framework. In the process of doing this I discovered
numerous opportunities for other simplifications.

Options can:
  * have ‘get’ and ‘set’ methods, to replace simply reading and writing through
    an C<int *> pointer;
  * be marked as C<readonly> (for options that simply report a status);
  * be marked C<forbid\_change> (for options that may only be set at startup);
  * specify a value to be used when performing a general reset or reinit.

Taken together, this means that options can be functionally private to a single
module, even declared static. This significantly reducs the number of C<extern>
declarations that are needed. (Tidying this up is still a work in progress.)

Custom setters can be used to enforce less common policies; they are provided
with a ‘whence’ context to indicate whether it's a direct result of a command
like C<set> or C<shopt>, or from the command line, or from the environment, or
when unwinding C<local ->.

The existing code used a mixture of command ‘exit status’ (0 is success, non-0
is failure), ‘C int’ (0 is false, non-0 is true), and ‘flag’ ('-' is true, '+'
is false), which made correctly propagating values rather tricky.

I decided to replace most of these with ‘strict enums’, by which I mean,
various enums each wrapped in a struct, with accessor macros, so that mixing up
their types would result compiler errors. (This accounts for roughly half of
“options.h”.) Modern compilers with optimisation enabled usually put such
structs into registers, so there's nil or negligible performance penalty, but
for compilers that cannot put small structs into registers, compiling with
C<-DFAST\_ENUM> will change these types back to bare ints.

In particular, custom setter functions can return a more detailed status than
simply "success" or "failure", so that the calling code can decide whether
and how that should be presented as an exit status code.
