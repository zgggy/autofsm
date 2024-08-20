# auto_finite_state_machine

under progress...

## Usage

1. add `autofsm.h` to your project, then just `#include` it.
1. init the machine:
   1. use `class YourObj`,`State<YourObj>` and all the `func_reg()`s to init the machine.
   1. use `class YourObj`,`Machine<YourObj>(states, transitions)` and all the `func_reg()`s to init the machine.
   1. use macro `fsm::make_machine()` to init the machine fastest.
1. `machine.process()` in `while(true)`.
