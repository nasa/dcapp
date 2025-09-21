#include <cstdlib>  // malloc, realloc, free, exit
#include <cstddef>  // size_t
#include <cstring>  // memmove

#define sbinitcap 16

typedef struct {
    int sbcap;
    int sbcnt;
} sbhdr;

static inline sbhdr *sbheader(void *a) {
    return ((sbhdr *)a) - 1;
}

static inline int sbcount(void *a) {
    return a ? sbheader(a)->sbcnt : 0;
}

static inline int sbcapacity(void *a) {
    return a ? sbheader(a)->sbcap : 0;
}

#define sbclear(a) \
    do { if (a) sbheader(a)->sbcnt = 0; } while (0)

static inline void *sbgrow(void *a, int inc, int sz) {
    int newcap = sbcapacity(a) ? sbcapacity(a) * 2 : sbinitcap;
    newcap += inc;
    size_t total = sizeof(sbhdr) + (size_t)newcap * (size_t)sz;

    sbhdr *newhdr;
    if (a) {
        newhdr = (sbhdr *)std::realloc(sbheader(a), total);
    } else {
        newhdr = (sbhdr *)std::malloc(total);
        if (newhdr) newhdr->sbcnt = 0;
    }

    if (!newhdr) std::exit(1);
    newhdr->sbcap = newcap;
    return (void *)(newhdr + 1);
}

#define sbpush(a, v) \
    do { \
        if (!(a) || sbheader(a)->sbcnt >= sbheader(a)->sbcap) \
            (a) = (decltype(+a))sbgrow((a), 1, sizeof(*(a))); \
        (a)[sbheader(a)->sbcnt++] = (v); \
    } while (0)

#define sbpushn(a, v, n) \
    do { \
        if (!(a) || sbheader(a)->sbcnt + (n) > sbheader(a)->sbcap) \
            (a) = (decltype(+a))sbgrow((a), (n), sizeof(*(a))); \
        for (int sbpushn_i = 0; sbpushn_i < (n); sbpushn_i++) \
            (a)[sbheader(a)->sbcnt++] = (v)[sbpushn_i]; \
    } while (0)

#define sbpop(a) \
    ((a) && sbheader(a)->sbcnt > 0 ? (a)[--sbheader(a)->sbcnt] : (a)[0])

#define sbpopn(a, n) \
    do { \
        if ((a) && sbheader(a)->sbcnt >= (n)) \
            sbheader(a)->sbcnt -= (n); \
        else if (a) \
            sbheader(a)->sbcnt = 0; \
    } while (0)

template <class T>
static inline T sbshift_fn(T*& a) {
    T v = a[0];
    int cnt = sbheader(a)->sbcnt;
    if (cnt > 1) {
        std::memmove(&a[0], &a[1], (size_t)(cnt - 1) * sizeof(T));
    }
    --sbheader(a)->sbcnt;
    return v;
}

#define sbshift(a) sbshift_fn((a))

#define sbshiftn(a, n) \
    do { \
        if ((a) && sbheader(a)->sbcnt > (n)) { \
            std::memmove(&(a)[0], &(a)[(n)], (size_t)(sbheader(a)->sbcnt - (n)) * sizeof(*(a))); \
            sbheader(a)->sbcnt -= (n); \
        } else if (a) { \
            sbheader(a)->sbcnt = 0; \
        } \
    } while (0)

#define sbfree(a) \
    do { \
        if (a) std::free(sbheader(a)); \
        (a) = nullptr; \
    } while (0)
