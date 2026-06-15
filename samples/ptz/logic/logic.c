#include "dcapp.h"

#define BaseWidth 46.0
#define BaseHeight 50.0

void display_init(DcAppContext *app_ctx) {
    (void)app_ctx;
}

void display_draw(DcAppContext *app_ctx) {
    (void)app_ctx;
    *IMAGE_WIDTH  = *ZOOM * BaseWidth;
    *IMAGE_HEIGHT = *ZOOM * BaseHeight;
    *IMAGE_X      = 50.0 + ((*PAN) * (*ZOOM));
    *IMAGE_Y      = 50.0 + ((*TILT) * (*ZOOM));
}

void display_close(DcAppContext *app_ctx) {
    (void)app_ctx;
}
