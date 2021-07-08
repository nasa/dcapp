#include <chrono>
#include <map>
#include <string>

#include "blinker.hh"

Blinker::Blinker() 
{}

/*
    initialize a Blinker

    params:
    * bs: pointer to trick variable being toggled. Switches between 0 and 1 on blink cycles
    * reps: blinks performed per cycle. e.g. reps=4 => on(starting state)-off-on-off-on
    * iv: interval per blink (s). e.g. iv=0.5 => .5s staying on, .5s being off
*/
Blinker::Blinker(int* bs, int reps, float iv) : 
    blink_state(bs), 
    repetitions(reps), 
    interval_ms( (int)(1000 * iv) ), 
    is_blinking(false) 
{}

/*
    reload the blink manager to restart the blinking process
*/
void Blinker::start() {
    if (is_blinking) return;

    repetitions_remaining = repetitions;
    current_interval_ms = interval_ms;
    is_blinking = true;
}

/*
    disable the Blinker
*/
void Blinker::stop() {
    repetitions_remaining = 0;
    current_interval_ms = 0;
    is_blinking = false;
}

// ##################################################################################################################################################

BlinkerManager::BlinkerManager() : 
    b_map() {
    t1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

/*
    handler function for blinking
    * b_map: map of Blinker values to process each iteration

    - essentially, all this does is toggle a DCAPP variable to 1 or 0 on a set interval for a number of iterations.
    - use startBlinker() on the intended Blinker variable to reenable the Blinker
*/
void BlinkerManager::processAllBlinkers() {
    long long t2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    for ( auto& p : b_map ) {
        Blinker& b = p.second;
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

void BlinkerManager::startBlinker(std::string name) {
    b_map[name].start();
}

void BlinkerManager::stopBlinker(std::string name) {
    b_map[name].stop();
}

void BlinkerManager::addBlinker(std::string name, Blinker b) {
    b_map[name] = b;
}