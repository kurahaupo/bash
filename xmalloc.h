/* xmalloc.h -- defines for the `x' memory allocation functions */

/* Copyright (C) 2001-2024 Free Software Foundation, Inc.

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

#if !defined (_XMALLOC_H_)
#  define _XMALLOC_H_

#  include "stdc.h"
#  include "bashansi.h"

/* Generic pointer type. */
#  ifndef PTR_T
#    define PTR_T	void *
#  endif	/* PTR_T */

/* Allocation functions in xmalloc.c */
#  ifdef xmalloc
#    undef xmalloc
#  endif
extern void *xmalloc (size_t);
#  ifdef xrealloc
#    undef xrealloc
#  endif
extern void *xrealloc (void const *, size_t);
#  ifdef xreallocarray
#    undef xreallocarray
#  endif
extern void *xreallocarray (void const *, size_t, size_t);
#  ifdef xfree
#    undef xfree
#  endif
extern void xfree (void const *);
#  ifdef xxfree
#    undef xxfree
#  endif
extern void xxfree (void *);

#  if defined(USING_BASH_MALLOC)
extern void *sh_xmalloc (size_t, const char *, int);
extern void *sh_xrealloc (void const *, size_t, const char *, int);
extern void *sh_xreallocarray (void const *, size_t, size_t, const char *, int);
extern void sh_xfree (void const *, const char *, int);

#    if !defined (DISABLE_MALLOC_WRAPPERS)

#      define xm__void_pointer(x)	((void *)&*(_Generic(x, void * : (char *)(x), void const * : (char *)(x), default : (x))))

#      define xmalloc(x)		sh_xmalloc((x), __FILE__, __LINE__)
#      define xrealloc(x, n)		sh_xrealloc((x), (n), __FILE__, __LINE__)
#      define xreallocarray(x, n, s)	sh_xreallocarray((x), (n), (s), __FILE__, __LINE__)
#      define xfree(x)			sh_xfree(xm__void_pointer(x), __FILE__, __LINE__)
#      define xxfree(x)			sh_xfree(xm__void_pointer(x), __FILE__, __LINE__)
#    else /* DISABLE_MALLOC_WRAPPERS */

#      define xm__void_pointer(x)	((void*) (x))

#    endif /* DISABLE_MALLOC_WRAPPERS */

#    ifdef free
#      undef free
#    endif
#    define free(x)			sh_xfree(xm__void_pointer(x), __FILE__, __LINE__)

extern void *sh_malloc (size_t, const char *, int);

#    ifdef malloc
#      undef malloc
#    endif
#    define malloc(x)			sh_malloc((x), __FILE__, __LINE__)

#  endif /* USING_BASH_MALLOC */

#endif /* _XMALLOC_H_ */
