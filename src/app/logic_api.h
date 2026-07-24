#ifndef DC_APP_LOGIC_API_H
#define DC_APP_LOGIC_API_H

typedef struct DcAppContext DcAppContext;
typedef struct DcAppDrawApi DcAppDrawApi;
typedef struct DcAppMouseApi DcAppMouseApi;
typedef struct DcAppTextureApi DcAppTextureApi;
typedef struct DcAppPlanetApi DcAppPlanetApi;

typedef struct DcAppApi {
    void *(*get_variable)(DcAppContext *app_ctx, const char *name);
} DcAppApi;

typedef struct DcAppInit DcAppInit;

struct DcAppInit {
    DcAppContext *app_ctx;
    const DcAppApi *app;
    const DcAppDrawApi *draw;
    const DcAppMouseApi *mouse;
    const DcAppTextureApi *texture;
    const DcAppPlanetApi *planet;
};

#endif
