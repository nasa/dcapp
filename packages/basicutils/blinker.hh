#include <map>

// struct for handling an individual blinker's properties
class blinker {
public:
    int* blink_state;               // pointer to DCAPP variable for storing on/off state (defined in the XML file)
    int repetitions;                // number of transition repetitions before stopping blinking
    int interval_ms;                // time interval each blink lasts (ms)

    int repetitions_remainining;
    int current_interval_ms;
    bool is_blinking;

    /* constructor for a blinker
        params:
        * bs: DCAPP blink state variable
        * rp: number of repetitions
        * iv: interval of each blink (in ms)
    */
    blinker();
    blinker(int* bs, int reps, int iv);

    void start();         // start blinker
    void stop();          // stop blinker  
};

// manage multiple blinkers (useful for multiple blinking attributes)
class blink_handler {
public:

    std::map<std::string, blinker> b_map;

    blink_handler();

    void processAllBlinkers();
    void startBlinker(std::string name);
    void stopBlinker(std::string name);
    void addBlinker(std::string name, blinker b);
};
