#include <stdlib.h>

#define sbinitcap 16

typedef struct {
    int sbcap;
    int sbcnt;
    // Followed by actual data
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

static inline void *sbgrow(void *a, int inc, int sz) {
    int newcap = sbcapacity(a) ? sbcapacity(a) * 2 : sbinitcap;
    newcap += inc;
    size_t total = sizeof(sbhdr) + newcap * sz;

    sbhdr *newhdr;
    if (a) {
        newhdr = (sbhdr *)realloc(sbheader(a), total);
    } else {
        newhdr = (sbhdr *)malloc(total);
        if (newhdr) newhdr->sbcnt = 0;
    }

    if (!newhdr) exit(1);
    newhdr->sbcap = newcap;
    return (void *)(newhdr + 1);
}


#define sbpush(a, v) \
    do { \
        if (!(a) || sbheader(a)->sbcnt >= sbheader(a)->sbcap) \
            (a) = sbgrow((a), 1, sizeof(*(a))); \
        (a)[sbheader(a)->sbcnt++] = (v); \
    } while (0)

#define sbcat(a, v, n) \
    do { \
        if (!(a) || sbheader(a)->sbcnt + (n) > sbheader(a)->sbcap) \
            (a) = sbgrow((a), (n), sizeof(*(a))); \
        for (int sbcat_i = 0; sbcat_i < (n); sbcat_i++) \
            (a)[sbheader(a)->sbcnt++] = (v)[sbcat_i]; \
    } while (0)

#define sbfree(a) \
    do { \
        if (a) free(sbheader(a)); \
        (a) = NULL; \
    } while (0)
