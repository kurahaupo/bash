#ifndef INCLUDED_sxbuf_h
#define INCLUDED_sxbuf_h

#ifdef no_inline_sxbuf
#undef no_inline_sxbuf
#define SCOPE
#define EXTERN extern
#else
#define SCOPE static
#define EXTERN static
#endif

#ifndef sxDebug
int sxDebug = 0;
#endif

typedef struct sxbuf {
    char *start;
    char *cur;
    char *end;
    bool bufalloc;
    bool selfalloc;
} SX[1], *SXP;

#define SX_arg(p) struct sxbuf (p) [static 1]

EXTERN inline size_t sxavailable(SX_arg(b)) ;
SCOPE  inline size_t sxavailable(SX_arg(b)) {
    return b->end || b->cur ? b->end - b->cur : 0;    /* (NULL-NULL) yields 0 on all known architectures, but let's be 100% standards-compliant */
}

EXTERN inline size_t sxcapacity(SX_arg(b)) ;
SCOPE  inline size_t sxcapacity(SX_arg(b)) {
    return b->end || b->start ? b->end - b->start : 0;  /* (NULL-NULL) yields 0 on all known architectures, but let's be 100% standards-compliant */
}

EXTERN inline size_t sxlength(SX_arg(b)) ;
SCOPE  inline size_t sxlength(SX_arg(b)) {
    return b->cur || b->start ? b->cur - b->start : 0;  /* (NULL-NULL) yields 0 on all known architectures, but let's be 100% standards-compliant */
}

EXTERN inline void sxinfo(char const*step, char const*func, SX_arg(b)) ;
SCOPE  inline void sxinfo(char const*step, char const*func, SX_arg(b)) {
    fprintf(stderr,
            "%-7s %s: b=%p [start=%p, end=%p, cap=%zd, cur=%p, len=%zd%s%s]",
            step, func,
            b,
            b->start,
            b->end, b->end - b->start,
            b->cur, b->cur - b->start,
            b->bufalloc  ? ", +balloc" : "",
            b->selfalloc ? ", +salloc" : "");
}

EXTERN inline void sxdump(SX_arg(b)) ;
SCOPE  inline void sxdump(SX_arg(b)) {
    if (!b->cur) {
        fputs(" (empty)\n", stderr);
        return;
    }
    size_t len = sxlength(b);
    for ( size_t i = 0 ; i < len ; i += 16 ) {
        fprintf(stderr,
                " %05zx:\t", i);
        size_t j=i;
        for (; j < i+16 && j<len ; ++j )
            fprintf(stderr,
                    "%02hhx ",
                    b->start[j]);
        for (; j < i+16 ; ++j )
            fputs("   ",stderr);
        fputs(": ", stderr);
        for ( j=i ; j < 16 && j < len ; ++j ) {
            char c = b->start[j];
            if (c<' ' || c > '~')
                c = '.';
            fputc(c, stderr);
        }
        fputs("\n", stderr);
    }
}

# define sxstaticinit() {0}
EXTERN inline void sxinit(SX_arg(b)) ;
SCOPE  inline void sxinit(SX_arg(b)) {
    static SX empty;
    *b = *empty;
}

EXTERN inline SXP sxnew(void) ;
SCOPE  inline SXP sxnew(void) {
    SXP b = malloc(sizeof(SX));
    sxinit(b);
    b->selfalloc = true;
    return b;
}

# if 0
EXTERN inline struct sxbuf sxnew(void) ;
SCOPE  inline struct sxbuf sxnew(void) {
    struct sxbuf *b = malloc(sizeof *b);
    if (!b)
        die(2, "malloc(%zu) failed", sizeof *b);
    sxinit(b);
    b->selfalloc = true;
}
# endif

EXTERN inline void sxreset(SX_arg(b)) ;
SCOPE  inline void sxreset(SX_arg(b)) {
    b->cur = b->start;
    if (b->cur != b->end)
        b->cur[0] = 0;
}

EXTERN inline void sxdestroy(SXP b) ;
SCOPE  inline void sxdestroy(SXP b) {
    if (!b)
        return;
    if (b->bufalloc)
        free(b->start);
    b->start = b->cur = b->end = NULL;
    if (b->selfalloc)   /* from sxnew? */
        free(b);
}

EXTERN inline char const *sxpeek(SX_arg(b)) ;
SCOPE  inline char const *sxpeek(SX_arg(b)) {
    // Look at the internal buffer; this will be invalidated by the next sx
    // operation, so don't keep it too long.
    return b->start;
}

EXTERN inline char *sxfinal(SX_arg(b)) ;
SCOPE  inline char *sxfinal(SX_arg(b)) {
    // Return a mallocked "copy" of the string within b, and destroy b;
    // since it's already mallocked, this is very cheap.
    char *ret = b->start;
    if (!b->bufalloc)
        ret = strdup(ret);
    b->start = b->end = b->cur = NULL;
    b->bufalloc = false;
    sxdestroy(b);
    return ret;
}

EXTERN inline void sxsoftsetcap(SX_arg(b), size_t new_length, bool force_alloc) ;
SCOPE  inline void sxsoftsetcap(SX_arg(b), size_t new_length, bool force_alloc) {
    if (sxDebug) {
        sxinfo("START:", __FUNCTION__, b);
        fprintf(stderr, ", req_sz=%zu%s\n", new_length, force_alloc ? ", +force" : "");
        if (sxDebug > 1)
            sxdump(b);
    }
    size_t capacity = sxcapacity(b);
    size_t old_length = sxlength(b);
    if (new_length <= capacity) {
        fprintf(stderr,
                "QUIT:   %s: new_length(%zu) <= capacity(%zu)\n",
                __FUNCTION__,
                new_length,
                capacity);
        return;
    }
    int exp = 0;
    frexp(new_length-1, &exp);
    size_t new_capacity = 1 << exp;    /* round up to power of 2 in [request..request*2-1] */
    if (sxDebug)
        fprintf(stderr, "        %s: round up request from %zu to %zu\n", __FUNCTION__, new_length, new_capacity);
    char * np = b->start;
    if (b->bufalloc) {
        np = realloc(b->start, new_capacity);
        if (!np)
            die(88, "realloc(%zu) failed within %s", new_capacity, __FUNCTION__);
    } else {
        /* make a copy */
        np = malloc(new_capacity);
        if (!np)
            die(88, "malloc(%zu) failed within %s", new_capacity, __FUNCTION__);
        size_t min_capacity = capacity < new_capacity
                            ? capacity : new_capacity;
        if (min_capacity)
            memcpy(np, b->start, min_capacity);
        b->bufalloc = true;
    }
    b->start = np;
    b->end   = np + new_capacity;
    b->cur = b->start + old_length;
    if (sxDebug)
        if (np != b->start) {
            sxinfo("DONE", __FUNCTION__, b);
            fprintf(stderr, " moved block from %p (%zu bytes) to %p (%zu bytes)\n", b->start, capacity, np, new_capacity);
        }
}

EXTERN inline void sxsoftresizecap(SX_arg(b), ssize_t make_room_for, bool force_alloc) ;
SCOPE  inline void sxsoftresizecap(SX_arg(b), ssize_t make_room_for, bool force_alloc) {
    if (sxDebug) {
        sxinfo("START:", __FUNCTION__, b);
        fprintf(stderr, ", change=%zd%s\n", make_room_for, force_alloc ? ", +force" : "");
    }
    size_t old_length = sxlength(b);
    size_t new_length = 0 > make_room_for &&
                           -make_room_for > old_length
                        ? 0
                        : old_length + make_room_for;
    sxsoftsetcap(b, new_length, force_alloc);
}

EXTERN inline void sxtrim(SX_arg(b), size_t how_many, bool force_alloc) ;
SCOPE  inline void sxtrim(SX_arg(b), size_t how_many, bool force_alloc) {
    if (sxDebug) {
        sxinfo("START:", __FUNCTION__, b);
        fprintf(stderr, ", how_many=%zu%s\n", how_many, force_alloc ? ", +force" : "");
    }
    size_t old_length = sxlength(b);
    size_t new_length = old_length < how_many ? 0 : old_length - how_many;
    sxsoftsetcap(b, new_length+1, force_alloc);
    b->cur = &b->start[new_length];
    *b->cur = 0;
}

////////////////////////////////////////

EXTERN inline void sxcat(SX_arg(b), char const *addition) ;
SCOPE  inline void sxcat(SX_arg(b), char const *addition) {
    size_t len = strlen(addition);
    if (len >= sxavailable(b))
        sxsoftresizecap(b, len+1, false);
    memcpy(b->cur, addition, len);
    b->cur += len;
}

EXTERN inline size_t sxprintf(SX_arg(b), char const *fmt, ...) ;
SCOPE  inline size_t sxprintf(SX_arg(b), char const *fmt, ...) {
    size_t result;
    va_list ap;
    va_start(ap, fmt);
    if (sxDebug) {
        sxinfo("BEGIN:", __FUNCTION__, b);
        fprintf(stderr, ", fmt=\"%s\", args... [", fmt);
        va_list aq;
        va_copy(aq, ap);
        ssize_t n = vfprintf(stderr, fmt, aq);
        va_end(aq);
        fprintf(stderr, "] (output=%zd)\n", n);
    }
    bool done_once = false;
    for(;;) {
        size_t avail = sxavailable(b);
        va_list aq;
        va_copy(aq, ap);
        ssize_t output_size = vsnprintf(b->cur, avail, fmt, aq);
        va_end(aq);
        if (output_size < 0)
            pdie(88, "FAIL:   sxprintf: vsnprint failed");
        if (output_size < avail) {
            result = output_size;
            b->cur += output_size;
            break;
        }
        if (done_once)
            die(88, "FAIL:   sxprintf: resizing more than once!!");
        done_once = true;
        sxsoftresizecap(b, output_size, false);
    }
    if (sxDebug) {
        sxinfo("RESULT:", __FUNCTION__, b);
        fputs("\n", stderr);
    }
    va_end(ap);
    return result;
}

#undef EXTERN
#undef SCOPE
#undef SX_arg

#endif  /* inclusion guard */
