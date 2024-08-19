#include <iostream>

#include "../include/autofsm.h"

enum States {
    play,
    play_find,
    play_with_ball,
    eat,
    silent,
    silent_calm,
    silent_sleep,
};

class Cat {
    using ST = State<Cat>;
    using TR = Transition<Cat>;

  public:
    ST machine_;
    Cat() : machine_(0, this) {
        machine_.add_child(play);
        machine_.child(play)->add_child(play_find);
        machine_.child(play)->add_child(play_with_ball);
        machine_.add_child(eat);
        machine_.add_child(silent);
        machine_.child(silent)->add_child(silent_calm);
        machine_.child(silent)->add_child(silent_sleep);

        machine_.child(play)->trans_reg(eat);
        machine_.child(play)->trans_reg(silent);
        machine_.child(play)->child(play_find)->trans_reg(play_with_ball);
        machine_.child(play)->child(play_with_ball)->trans_reg(play_find);
        machine_.child(eat)->trans_reg(silent);
        machine_.child(silent)->trans_reg(play);
        machine_.child(silent)->trans_reg(eat);
        machine_.child(silent)->child(silent_calm)->trans_reg(silent_sleep);
        machine_.child(silent)->child(silent_sleep)->trans_reg(silent_calm);
    }
};

int main() {
    Cat cat;
    cat.machine_.process();
    std::cout << "fsm state: " << cat.machine_.current_child_name() << " "
              << (cat.machine_.current_child()->childs().empty() ? 0 : cat.machine_.current_child()->current_child_name()) << std::endl;
}