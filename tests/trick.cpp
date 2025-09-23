// basic test app, used for verifying functionality

#include "../src/trick.hpp"
#include <cstdio>

int main(int argc, char **argv) {

    DcTrick trick = dc_trick_create((char*)"localhost", 9123, 1, 1);

    DcTrickIndex var1 = dc_trick_add_rx_var(&trick, "dyn.cannon.pos[0]", NULL);
    DcTrickIndex var2 = dc_trick_add_rx_var(&trick, "dyn.cannon.pos[1]", NULL);
    DcTrickIndex var3 = dc_trick_add_rx_var(&trick, "dyn.cannon.vel[1]", NULL);

    DcTrickIndex var4 = dc_trick_add_tx_var(&trick, "dyn.cannon.g", NULL, false);
    DcTrickIndex var5 = dc_trick_add_tx_var(&trick, "dyn.cannon.pos0[0]", NULL, false);
    while(1) {
        dc_trick_update(&trick);
        if (trick.has_new_data) {
            char a[30], b[30], c[30];
            dc_trick_get_rx_var_value(&trick, var1, a);
            dc_trick_get_rx_var_value(&trick, var2, b);
            dc_trick_get_rx_var_value(&trick, var3, c);
            dc_trick_set_tx_var(&trick, var4, ".00222");
            dc_trick_set_tx_var(&trick, var5, ".00333");
            printf("%s %s %s\n", a, b, c);
        }
    }
    dc_trick_cleanup(&trick);
    return 0;
}
