#include <stdlib.h>

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

#define sbclear(a)                  \
    do {                            \
        if (a)                      \
            sbheader(a)->sbcnt = 0; \
    } while (0)

static inline void *sbgrow(void *a, int inc, int sz) {
    int newcap = sbcapacity(a) ? sbcapacity(a) * 2 : sbinitcap;
    newcap += inc;
    size_t total = sizeof(sbhdr) + newcap * sz;

    sbhdr *newhdr;
    if (a) {
        newhdr = (sbhdr *)realloc(sbheader(a), total);
    } else {
        newhdr = (sbhdr *)malloc(total);
        if (newhdr)
            newhdr->sbcnt = 0;
    }

    if (!newhdr)
        exit(1);
    newhdr->sbcap = newcap;
    return (void *)(newhdr + 1);
}

#define sbpush(a, v)                                          \
    do {                                                      \
        if (!(a) || sbheader(a)->sbcnt >= sbheader(a)->sbcap) \
            (a) = sbgrow((a), 1, sizeof(*(a)));               \
        (a)[sbheader(a)->sbcnt++] = (v);                      \
    } while (0)

#define sbpushn(a, v, n)                                           \
    do {                                                           \
        if (!(a) || sbheader(a)->sbcnt + (n) > sbheader(a)->sbcap) \
            (a) = sbgrow((a), (n), sizeof(*(a)));                  \
        for (int sbpushn_i = 0; sbpushn_i < (n); sbpushn_i++)      \
            (a)[sbheader(a)->sbcnt++] = (v)[sbpushn_i];            \
    } while (0)

#define sbpop(a) \
    ((a) && sbheader(a)->sbcnt > 0 ? (a)[--sbheader(a)->sbcnt] : (a)[0])

#define sbpopn(a, n)                          \
    do {                                      \
        if ((a) && sbheader(a)->sbcnt >= (n)) \
            sbheader(a)->sbcnt -= (n);        \
        else if (a)                           \
            sbheader(a)->sbcnt = 0;           \
    } while (0)

#define sbshift(a)                                                           \
    ((a) && sbheader(a)->sbcnt > 0 ? ({                                      \
        typeof(*(a)) sbshift_val = (a)[0];                                   \
        for (int sbshift_i = 1; sbshift_i < sbheader(a)->sbcnt; ++sbshift_i) \
            (a)[sbshift_i - 1] = (a)[sbshift_i];                             \
        --sbheader(a)->sbcnt;                                                \
        sbshift_val;                                                         \
    })                                                                       \
                                   : (a)[0])

#define sbshiftn(a, n)                                                                    \
    do {                                                                                  \
        if ((a) && sbheader(a)->sbcnt > (n)) {                                            \
            for (int sbshiftn_i = 0; sbshiftn_i < sbheader(a)->sbcnt - (n); ++sbshiftn_i) \
                (a)[sbshiftn_i] = (a)[sbshiftn_i + (n)];                                  \
            sbheader(a)->sbcnt -= (n);                                                    \
        } else if (a) {                                                                   \
            sbheader(a)->sbcnt = 0;                                                       \
        }                                                                                 \
    } while (0)

#define sbfree(a)              \
    do {                       \
        if (a)                 \
            free(sbheader(a)); \
        (a) = NULL;            \
    } while (0)
