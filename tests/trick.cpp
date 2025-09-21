// basic test app, used for verifying functionality

#include "../src/trick.hpp"

int main(int argc, char **argv) {

    DcTrick trick = dc_trick_create((char*)"localhost", 8932, .1, 1);

    dc_trick_add_rx_var(&trick, "time", NULL);
    while(1) {
        dc_trick_update(&trick);
    }
    dc_trick_cleanup(&trick);
    return 0;
}
