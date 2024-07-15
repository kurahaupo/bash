/* $Header:cat.c 12.0$ */
/* $ACIS:cat.c 12.0$ */
/* $Source: /ibm/acis/usr/src/bin/RCS/cat.c,v $ */

#ifndef lint
static char *rcsid __attribute((__unused__)) = "$Header:cat.c 12.0$";
#endif

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] __attribute((__unused__)) = "@(#)cat.c\t5.2 (Berkeley) 12/6/85";
#endif /*not lint*/

/*
 * Concatenate files.
 */

#define _POSIX_C_SOURCE 200809L

#include <sys/types.h>
#include <sys/stat.h>

#include <assert.h>
#include <malloc.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef EXIT_ERROR
#define EXIT_ERROR 2
#endif
#ifndef EX_USAGE
#define EX_USAGE EXIT_ERROR
#endif
#ifndef EX_UNAVAILABLE
#define EX_UNAVAILABLE EXIT_ERROR
#endif

#if 0
#define OPTSIZE BUFSIZ  /* define this only if not 4.2 BSD or beyond */
#endif

static void
sigpipe(int signum)
{
    write(2, "pcat: caught SIGPIPE\n", 21);
    exit(1);
}

typedef enum line_numbering_e {
    /* 0 = no-action default */
    lnsHide = 0,
    lnsShow = 1,
    lnsSkipBlanks = 2
} LineNumbering;

typedef enum show_control_chars_e {
    /* 0 = no-action default */
    sccLiteral = 0,
    sccShow = 1,
    sccExceptTab = 2
} ShowControlChars;

typedef struct cat_opts_s {
    LineNumbering show_line_numbers;
    bool squash_blank_lines;
    ShowControlChars show_nonprinting;
    bool squash_space;
    bool show_eol_marker;
    bool use_stdio;
    size_t ibsize;
    size_t obsize;
    dev_t dev;
    ino_t ino;
} Options;

#ifndef LaxArrayCount
#define array_count(Array) (sizeof(Array) / sizeof(*Array) / ((void*)Array == (void*)&Array))
#else
#define array_count(Array) (sizeof(Array) / sizeof(*Array))
#endif

static inline char const *describe_bits(uintmax_t x, char const*const*descriptions, size_t array_size, size_t modulus) {
    if (!x)
        return descriptions[0];

    static char *b = NULL;
    static size_t cap = 0;
    char *p = b;

    if (!cap) {
        //fprintf(stderr, "before realloc b=%p, p=%p, p-b=%zd (state 1)\n", b, p, p-b);
        p = b = malloc(cap = 0x100);
        //fprintf(stderr, " after realloc b=%p, p=%p, p-b=%zd\n", b, p, p-b);
    }
    for (uintmax_t z = 1 ; z && x ; z<<=1 ) {
        size_t i = z%modulus;
        if (i >= array_size) continue;
        char const *r = descriptions[i];
        if (!r || !*r) continue;
        size_t needed = p-b + strlen(r) + 1 + 1;
        if (cap < needed) {
            //fprintf(stderr, "before realloc b=%p, p=%p, p-b=%zd (state 2)\n", b, p, p-b);
            char *n = realloc(b, cap = needed |= cap * 99 / 70);    /* ≈√2 */
            p = n+(p-b);
            b = n;
            //fprintf(stderr, " after realloc b=%p, p=%p, p-b=%zd\n", b, p, p-b);
        }
        p = stpcpy(p, r);
        *p++ = ',';
        x &=~ z;
    }
    if (x) {
        size_t needed = p-b + 2+16+1;   /* assuming 64-bit, "0x" + 16 digits + NUL */
        if (cap < needed) {
            //fprintf(stderr, "before realloc b=%p, p=%p, p-b=%zd (state 3)\n", b, p, p-b);
            char *n = realloc(b, cap = needed);
            p = n+(p-b);
            b = n;
            //fprintf(stderr, " after realloc b=%p, p=%p, p-b=%zd\n", b, p, p-b);
        }
        snprintf(p, cap - (p-b), "%#jx", (intmax_t) x);
    }
    else if (p>b)
        *--p = 0;   /* trim off trailing comma */
    else
        *p = 0,
        assert(0);  /* should never happen */
    return b;
}

static inline char const * lns(LineNumbering x) {
    enum { m = 67 };   /* modulus */
    static char const *_lns[m] = {
        [lnsHide%m]         = "hide",
        [lnsShow%m]         = "show",
        [lnsSkipBlanks%m]   = "skip-blanks",
    };
    return describe_bits(x, _lns, array_count(_lns), m);
}

static inline char const * scc(ShowControlChars x) {
    enum { m = 67 };   /* modulus */
    static char const *_scc[m] = {
        [sccLiteral%m]      = "literal",
        [sccShow%m]         = "show",
        [sccExceptTab%m]    = "except-tabs",
    };
    return describe_bits(x, _scc, array_count(_scc), m);
}

static inline char const * ny(bool x) {
    static char const *_ny[2] = {
        [false]="no",
        [true]="yes",
    };
    return _ny[x] ?: "unknown";
}

static inline intmax_t ji(intmax_t x) { return x; } /* tidier and shorter than a cast */

int cat_one(char const *arg,
            Options o);

int
main(int argc, char **argv)
{
    Options o = {0};

    signal(SIGPIPE, sigpipe);
    for(; --argc>0 && *++argv && **argv == '-' ;) {
        char *arg = *argv+1;

        /* lone "-" counts as a non-option arg, so stop processing options */
        if (!*arg)
            break;

        if (*arg == '-') {
            /* "--" means "end of options", so skip it and stop processing options */
            if (!arg[1]) {
                ++argv;
                break;
            }
            fprintf(stderr, "GNU-style \"--long-options\" not supported at \"%s\"\n", *argv);
            return EX_USAGE;
        }

        for (; arg && *arg; ++arg)
            switch(*arg) {
                case 'u':
                    setbuf(stdout, (char *)NULL);
                    o.use_stdio = true;
                    continue;
                case 'n':
                    o.show_line_numbers |= lnsShow;
                    continue;
                case 'b':
                    o.show_line_numbers |= lnsShow | lnsSkipBlanks;
                    continue;
                case 'v':
                    o.show_nonprinting |= sccShow;
                    continue;
                case 's':
                    o.squash_blank_lines = true;
                    continue;
                case 'S':
                    o.squash_space = true;
                    continue;
                case 'e':
                    o.show_eol_marker = true;
                    o.show_nonprinting |= sccShow;
                    continue;
                case 't':
                    o.show_nonprinting |= sccShow|sccExceptTab;
                    continue;
                default:
                    fprintf(stderr, "Invalid option '%c' in '%s'\n", *arg, *argv);
                    return EX_USAGE;
            }
    }

    o.obsize = 0;
    {
    struct stat statb;
    if (fstat(fileno(stdout), &statb) == 0) {
        if (S_ISREG(statb.st_mode)) {
            o.dev = statb.st_dev;
            o.ino = statb.st_ino;
        }
#ifndef OPTSIZE
        if (S_ISREG(statb.st_mode))     /* hint only valid for a regular file */
            o.obsize = statb.st_blksize;
#endif
    }
    }
    if (argc < 2)
        return cat_one("-", o);
    int retval = EXIT_SUCCESS;
    for (; argc > 0 ; --argc, ++argv) {
        int err = cat_one(*argv, o);
        if (err > retval)
            retval = err;
    }
    return retval;
}

/*
 * pretty_cat can modify the output in various ways, controlled by the flags.
 */

int
pretty_cat(FILE *fi,
           Options o)
{
    static int line_no = 0;
    static int output_column = 0;
    static int pending_spaces = 0;
    int consecutive_newlines = 0;

    for (int c; (c = getc(fi)) != EOF ;) {
        if (c == '\n') {
            if (output_column == 0) {
                if (o.squash_blank_lines && consecutive_newlines > 0)
                    continue;
                ++consecutive_newlines;
            }
            if (o.show_line_numbers == lnsShow && output_column == 0)
                printf("%6d\t", ++line_no);
            if (o.show_eol_marker)
                putchar('$');
            putchar('\n');
            output_column = 0;
            continue;
        }
        if (o.show_line_numbers != lnsHide && output_column == 0) {
            printf("%6d\t", ++line_no);
            output_column = 8;
            if (line_no >= 100000000)
                output_column = 16;
        }
        consecutive_newlines = 0;
        if (o.squash_space && (c == ' ' || c == '\t')) {
            int prev = output_column;
            if (c == '\t')
                output_column |= 7;
            ++output_column;
            pending_spaces += output_column - prev;
            continue;
        }
        if (o.show_nonprinting != sccLiteral && !(c == '\t' && o.show_nonprinting & sccExceptTab)) {
            if (c & 0x80) {
                putchar('M'); ++output_column;
                putchar('-'); ++output_column;
                c &= 0x7f;
            }
            if (c < ' ' || c == 0x7f) {
                putchar('^'); ++output_column;
                c ^= '@';
            }
        }
        putchar(c);
        if (c == '\t')
            output_column |= 7;
        ++output_column;
    }
    return EXIT_SUCCESS;
}

int
simple_cat(FILE *fi,
           Options o)
{
    int c;
    while ((c = getc(fi)) != EOF)
        putchar(c);
            while ((c = getc(fi)) != EOF)
                putchar(c);
    return EXIT_SUCCESS;
}

int
fast_cat(FILE *fi,
         Options o)
{
    int fd = fileno(fi);
    /* TODO: implement Linux-specific splice+tee method that avoids copying
     * data in and out of userspace. */
#ifndef OPTSIZE
    int buffsize = o.obsize ?: o.ibsize ?: BUFSIZ;
    if (o.obsize)
        buffsize = o.obsize;    /* common case, use output blksize */
    else if (o.ibsize)
        buffsize = o.ibsize;
    else
        buffsize = BUFSIZ;
#else
    int buffsize = OPTSIZE;
#endif

    char *buff;
    if ((buff = malloc(buffsize)) == NULL) {
        perror("cat: no memory");
        return EX_UNAVAILABLE;
    }

    /*
     * Note that on some systems (V7), very large writes to a pipe
     * return less than the requested size of the write.
     * In this case, multiple writes are required.
     */
    int nread, nwritten;
    while ((nread = read(fd, buff, buffsize)) > 0) {
        int offset = 0;
        do {
            nwritten = write(fileno(stdout), buff+offset, nread);
            if (nwritten <= 0) {
                perror("cat: write error");
                exit(2);
            }
            offset += nwritten;
        } while ((nread -= nwritten) > 0);
    }

    free(buff);
    if (nread < 0) {
        perror("cat: read error");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int cat_one(char const *arg,
            Options o)
{
    fprintf(stderr,
                    "cat_one(name=%s\n"
                    "        show_line_numbers=%s\n"
                    "        squash_blank_lines=%s\n"
                    "        show_nonprinting=%s\n"
                    "        squash_space=%s\n"
                    "        show_eol_marker=%s\n"
                    "        use_stdio=%s\n"
                    "        block_size=[in=%jd,out=%jd]\n"
                    "        dev:inode=%jd:%jd)\n",
                    arg,
                    lns(o.show_line_numbers),
                    ny(o.squash_blank_lines),
                    scc(o.show_nonprinting),
                    ny(o.squash_space),
                    ny(o.show_eol_marker),
                    ny(o.use_stdio),
                    ji(o.ibsize), ji(o.obsize),
                    ji(o.dev), ji(o.ino));

    FILE *fi = stdin;
    bool close_when_done = false;
    if (!(arg[0] == '-' && arg[1] == 0)) {
        fi = fopen(arg, "r");
        if (fi == NULL) {
            perror(arg);
            return EXIT_FAILURE;
        }
        close_when_done = false;
    }

    o.ibsize = 0;
    {
    struct stat statb;
    if (fstat(fileno(fi), &statb) == 0) {
        if (S_ISREG(statb.st_mode) &&
          statb.st_dev==o.dev &&
          statb.st_ino==o.ino) {
            fprintf(stderr, "cat: input %s is output\n", arg);
            fclose(fi);
            return EXIT_FAILURE;
        }
#ifndef OPTSIZE
        if (S_ISREG(statb.st_mode))     /* hint only valid for a regular file */
            o.ibsize = statb.st_blksize;
#endif
    }
    }

    int retval = 0;
    if (o.show_line_numbers || o.squash_blank_lines || o.show_nonprinting != sccLiteral)
        retval = pretty_cat(fi, o);
    else if (o.use_stdio)
        retval = simple_cat(fi, o);
    else
        retval = fast_cat(fi, o);       /* no flags specified */

    if (close_when_done)
        fclose(fi);
    else
        clearerr(fi);                   /* reset sticky eof on stdin */

    if (ferror(stdout)) {
        fprintf(stderr, "cat: output write error\n");
        return EXIT_FAILURE;
    }
    return retval;
}
