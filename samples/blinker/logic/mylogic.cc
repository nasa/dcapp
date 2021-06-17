#include <cstring>
#include <ctime>
#include <sys/time.h>
#include "dcapp.hh"
#include "blinker.hh"

static BlinkerManager bh;

extern "C" void DisplayInit(void)
{
    /* map defining the different blinkers, with:
        * key for accessing blinker
        * DCAPP variable being blinked
        * blink cycles before stopping
        * time spent per blink state (s)
    */
    bh.addBlinker("CIRCLE", Blinker(CIRCLE_BLINK_STATE, 12, .25));          // CIRCLE_BLINK_STATE set to blink 12 times, at 250ms per state
    // bh.addBlinker("CIRCLE2", Blinker(CIRCLE2_BLINK_STATE, 4, 1));        // runs 4 iterations at 1Hz
    // bh.addBlinker("TRIANGLE", Blinker(TRIANGLE_BLINK_STATE, -1, .1));    // runs indefinitely at 10Hz until stopped
}

extern "C" void DisplayLogic(void)
{
    // if the button is clicked, start the circle blink
    if ( *CIRCLE_START_BLINK )
        bh.startBlinker("CIRCLE");

    // if this button is clicked, stop blinking
    if ( *CIRCLE_STOP_BLINK ) {
        bh.stopBlinker("CIRCLE");

        // set the blink state to 1, in case blinking is stopped while the graphic is "off" (optional)
        *CIRCLE_BLINK_STATE = 1;
    }

    // run this every refresh cycle, to update the boolean state of the DCAPP variable
    bh.processAllBlinkers();
}
