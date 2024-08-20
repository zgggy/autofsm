#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "../include/autofsm.h"
#define LOGOUTPUT true
#include "logdef.h"

#define MAKE_ENUM(name, ...)                              \
    enum name { __VA_ARGS__, name##__COUNT };             \
    template <typename NameOrInt>                         \
    inline std::string name##ToStr(NameOrInt value) {     \
        std::string              enumName = #name;        \
        std::string              str      = #__VA_ARGS__; \
        int                      len      = str.length(); \
        std::vector<std::string> strings;                 \
        std::ostringstream       temp;                    \
        for (int i = 0; i < len; i++) {                   \
            if (isspace(str[i]))                          \
                continue;                                 \
            else if (str[i] == ',') {                     \
                strings.emplace_back(temp.str());         \
                temp.str(std::string());                  \
            }                                             \
            else                                          \
                temp << str[i];                           \
        }                                                 \
        strings.emplace_back(temp.str());                 \
        return strings[static_cast<int>(value)];          \
    }

MAKE_ENUM(States,         //
          top,            // 0
          play,           // 1
          play_find,      // 2
          play_with_ball, // 3
          eat,            // 4
          silent,         // 5
          silent_calm,    // 6
          silent_sleep    // 7
)

class Cat {
    using ST = fsm::State<Cat>;
    using TR = fsm::Transition<Cat>;

  public:
    int satiety{0};
    int excitement{0};
    int find{0};

  public:
    ST machine_;
    Cat() : machine_(top, this) {
        machine_.add_child(play, &Cat::play_process);
        machine_.child(play)->add_child(play_find, &Cat::find_process, true);
        machine_.child(play)->add_child(play_with_ball, &Cat::ball_process);
        machine_.add_child(eat, &Cat::eat_process);
        machine_.add_child(silent, &Cat::silent_process, true);
        machine_.child(silent)->add_child(silent_calm, &Cat::calm_process, true);
        machine_.child(silent)->add_child(silent_sleep, &Cat::sleep_process);

        machine_.child(play)->trans_reg(eat, &Cat::play_eat);
        machine_.child(play)->trans_reg(silent, &Cat::play_silent);
        machine_.child(play)->child(play_find)->trans_reg(play_with_ball, &Cat::find_ball);
        machine_.child(play)->child(play_with_ball)->trans_reg(play_find, &Cat::loss_ball);
        machine_.child(eat)->trans_reg(silent, &Cat::eat_silent);
        machine_.child(silent)->trans_reg(play, &Cat::silent_play);
        machine_.child(silent)->trans_reg(eat, &Cat::silent_eat);
        machine_.child(silent)->child(silent_calm)->trans_reg(silent_sleep, &Cat::calm_sleep);
        machine_.child(silent)->child(silent_sleep)->trans_reg(silent_calm, &Cat::sleep_calm);
    }

    void play_process() {
        satiety -= 2;
        excitement -= 1;
    }

    void find_process() { find += 1; }
    void ball_process() { find -= 1; }

    void eat_process() { satiety += 3; }

    void silent_process() {}

    void calm_process() {
        satiety -= 1;
        excitement += 2;
    }
    void sleep_process() { excitement += 3; }

    bool play_eat() { return satiety <= 30; }
    bool play_silent() { return excitement <= 30; }
    bool find_ball() { return find > 5; }
    bool loss_ball() { return find <= 5; }
    bool eat_silent() { return satiety > 80; }
    bool silent_play() { return excitement > 80; }
    bool silent_eat() { return satiety <= 10; }
    bool calm_sleep() { return excitement <= 10; }
    bool sleep_calm() { return excitement > 10; }
};

int main() {
    Cat cat;

    while (1) {
        cat.machine_.process();
        INFO << "cat satiety: " << cat.satiety << " excitement: " << cat.excitement << ENDL;
        INFO << "fsm state: " << StatesToStr(cat.machine_.current_child_id()) << " "
             << (cat.machine_.current_child()->childs().empty() ? " " : StatesToStr(cat.machine_.current_child()->current_child_id()))
             << ENDL << ENDL;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}