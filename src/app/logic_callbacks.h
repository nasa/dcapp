#ifndef DC_APP_LOGIC_CALLBACKS_H
#define DC_APP_LOGIC_CALLBACKS_H

struct DcAppContext;
struct DcAppDrawContext;
struct DcAppDrawFuncArgs;
struct DcAppInit;

typedef void (*DcAppLogicPreInitFn)(const struct DcAppInit *init);
typedef void (*DcAppLogicInitFn)(struct DcAppContext *app_context, void **user_data);
typedef void (*DcAppLogicFunctionFn)(struct DcAppContext *app_context, void *user_data);
typedef void (*DcAppLogicDrawFunctionFn)(
    struct DcAppDrawContext *draw_context,
    const struct DcAppDrawFuncArgs *args,
    void *user_data);
typedef DcAppLogicFunctionFn DcAppLogicUpdateFn;
typedef DcAppLogicFunctionFn DcAppLogicCloseFn;

#endif
