#include <chrono>
#include <map>

#include "blinker.hh"

blinker::blinker() {}

blinker::blinker(int* bs, int reps, int iv) : blink_state(bs), repetitions(reps), interval_ms(iv), is_blinking(false) {}

/*
    reload the blink manager to restart the blinking process
*/
void blinker::start() {
    if (is_blinking) return;

    repetitions_remainining = repetitions;
    current_interval_ms = interval_ms;
    is_blinking = true;
}

/*
    disable the blinker
*/
void blinker::stop() {
    repetitions_remainining = 0;
    current_interval_ms = 0;
    is_blinking = false;
}

// ##################################################################################################################################################

blink_handler::blink_handler() : b_map() {
    return;
}

/*
    handler function for blinking
    * b_map: map of blinker values to process each iteration

    - essentially, all this does is toggle a DCAPP variable to 1 or 0 on a set interval for a number of iterations.
    - use startBlinker() on the intended blinker variable to reenable the blinker
*/
void blink_handler::processAllBlinkers() {
    static auto t1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    auto t2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    for ( auto& p : b_map ) {
        blinker& b = p.second;
        if (!b.is_blinking) continue;

        b.current_interval_ms -= (t2-t1);
        if (b.current_interval_ms <= 0) {
            *(b.blink_state) = 1 - *(b.blink_state);
            b.current_interval_ms = b.interval_ms;
            b.repetitions_remainining--;

            if (b.repetitions_remainining <= 0)
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