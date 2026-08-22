#ifndef DC_APP_DISPLAY_LOGIC_CALLBACKS_H
#define DC_APP_DISPLAY_LOGIC_CALLBACKS_H

struct DcAppContext;
struct DcAppDrawContext;
struct DcAppDrawFuncArgs;
struct DcAppDisplayLogicInit;

typedef void (*DcAppDisplayLogicPreInitFn)(const struct DcAppDisplayLogicInit *init);
typedef void (*DcAppDisplayLogicInitFn)(struct DcAppContext *app_context, void **user_data);
typedef void (*DcAppDisplayLogicFunctionFn)(struct DcAppContext *app_context, void *user_data);
typedef void (*DcAppDisplayLogicDrawFunctionFn)(
    struct DcAppDrawContext *draw_context,
    const struct DcAppDrawFuncArgs *args,
    void *user_data);
typedef DcAppDisplayLogicFunctionFn DcAppDisplayLogicUpdateFn;
typedef DcAppDisplayLogicFunctionFn DcAppDisplayLogicCloseFn;

#endif
