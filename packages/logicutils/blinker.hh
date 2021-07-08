#ifndef _BLINKER_HH_
#define _BLINKER_HH_

#include <map>
#include <string>

// struct for handling an individual blinker's properties
class Blinker {
public:
    int* blink_state;               // pointer to DCAPP variable for storing on/off state (defined in the XML file)
    int repetitions;                // number of transition repetitions before stopping blinking
    int interval_ms;                // time interval each blink lasts (ms)

    int repetitions_remaining;
    int current_interval_ms;
    bool is_blinking;

    /* constructor for a blinker
        params:
        * bs: DCAPP blink state variable
        * rp: number of repetitions
        * iv: interval of each blink (s)
    */
    Blinker();
    Blinker(int* bs, int reps, float iv);

    void start();         // start blinker
    void stop();          // stop blinker  
};

// manage multiple blinkers (useful for multiple blinking attributes)
class BlinkerManager {
public:

    std::map<std::string, Blinker> b_map;
    long long t1;

    BlinkerManager();

    void processAllBlinkers();
    void startBlinker(std::string name);
    void stopBlinker(std::string name);
    void addBlinker(std::string name, Blinker b);
};

#endif
