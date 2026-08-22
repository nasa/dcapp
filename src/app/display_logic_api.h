#ifndef DC_APP_DISPLAY_LOGIC_API_H
#define DC_APP_DISPLAY_LOGIC_API_H

typedef struct DcAppContext DcAppContext;
typedef struct DcAppDrawApi DcAppDrawApi;
typedef struct DcAppMouseApi DcAppMouseApi;
typedef struct DcAppTextureApi DcAppTextureApi;
typedef struct DcAppPlanetApi DcAppPlanetApi;

typedef struct DcAppDisplayLogicApi {
    void *(*get_variable)(DcAppContext *app_ctx, const char *name);
} DcAppDisplayLogicApi;

typedef struct DcAppDisplayLogicInit DcAppDisplayLogicInit;

struct DcAppDisplayLogicInit {
    DcAppContext *app_ctx;
    const DcAppDisplayLogicApi *app;
    const DcAppDrawApi *draw;
    const DcAppMouseApi *mouse;
    const DcAppTextureApi *texture;
    const DcAppPlanetApi *planet;
};

#endif
