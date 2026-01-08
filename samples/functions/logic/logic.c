#include "dcapp.h"
#include <stdio.h>

void display_init(void) {
    printf("Functions Demo: Initialized\n");
}

void display_draw(void) {
}

void display_close(void) {
    printf("Functions Demo: Closed\n");
}

// Called when Button A is clicked
void on_button_a_click(void) {
    printf("Button A clicked!\n");
}

// Called when Button B is clicked
void on_button_b_click(void) {
    printf("Button B clicked!\n");
}

// Called when Reset button is clicked
void on_reset_click(void) {
    printf("Reset clicked!\n");
}
