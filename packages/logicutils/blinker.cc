#include <chrono>
#include <map>

#include "blinker.hh"

blinker::blinker() 
{}

/*
    initialize a blinker

    params:
    * bs: pointer to trick variable being toggled. Switches between 0 and 1 on blink cycles
    * reps: blinks performed per cycle. e.g. reps=4 => on(starting state)-off-on-off-on
    * iv: interval per blink (ms). e.g. iv=500 => .5s staying on, .5s being off
*/
blinker::blinker(int* bs, int reps, int iv) : 
    blink_state(bs), 
    repetitions(reps), 
    interval_ms(iv), 
    is_blinking(false) 
{}

/*
    reload the blink manager to restart the blinking process
*/
void blinker::start() {
    if (is_blinking) return;

    repetitions_remaining = repetitions;
    current_interval_ms = interval_ms;
    is_blinking = true;
}

/*
    disable the blinker
*/
void blinker::stop() {
    repetitions_remaining = 0;
    current_interval_ms = 0;
    is_blinking = false;
}

// ##################################################################################################################################################

blink_handler::blink_handler() : 
    b_map() {
    t1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

/*
    handler function for blinking
    * b_map: map of blinker values to process each iteration

    - essentially, all this does is toggle a DCAPP variable to 1 or 0 on a set interval for a number of iterations.
    - use startBlinker() on the intended blinker variable to reenable the blinker
*/
void blink_handler::processAllBlinkers() {
    long long t2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    for ( auto& p : b_map ) {
        blinker& b = p.second;
        if (!b.is_blinking) continue;

        b.current_interval_ms -= (t2-t1);
        if (b.current_interval_ms <= 0) {
            *(b.blink_state) = !(*(b.blink_state));
            b.current_interval_ms = b.interval_ms;

            if (b.repetitions_remaining > 0)
                b.repetitions_remaining--;

            if (b.repetitions_remaining == 0)
                b.is_blinking = false;
        }
    }

    t1 = t2;
}

void blink_handler::startBlinker(std::string name) {
    b_map[name].start();
}

void blink_handler::stopBlinker(std::string name) {
    b_map[name].stop();
}

void blink_handler::addBlinker(std::string name, blinker b) {
    b_map[name] = b;
}