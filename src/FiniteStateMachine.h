/**
 * Created Time: 2022.09.06
 * File name:    FiniteStateMachine.h
 * Author:       zgy(zhangguangyan@wicri.org)
 * Brief:        有限状态机框架
 * Include:      Class: State, Transition, Condition, Machine
 */

#ifndef __WICRI_FINITE_STATE_MACHINE__
#define __WICRI_FINITE_STATE_MACHINE__

#include <functional> // 实现通过bind()对模型类的函数成员转换
#include <iostream>
#include <map>
#include <string>
#include <vector>

/* 定义格式简写 */
using StateList  = std::vector<int>;
using TransTable = std::vector<std::vector<int>>;
enum FSM_GET_NONE { NO_TRANS = -1, NO_STATE = -2 };


/* 前置声明，因为下列类之间有互相依赖 */
template <class T>
class State;
template <class T>
class Transition;
template <class T>
class Condition;
template <class T>
class Machine;


/***********
 ** State **
 ***********/

/** TODO by zgy
 * 1.应当存在state不可在上层状态机退出时正在运行：
 *      目前是令其主状态机在此状态运行时不可退出，见“bool could_exit_”，应该是完备的功能，待测试。
 * 2.应当存在state在上层状态机退出时选择暂停还是停止。
 *      若暂停，上层状态机重新运行时此状态继续运行；若停止，上层状态机重新运行时执行默认状态。
 */
template <class T>
class State {
  private:
    /* 单层状态相关 */
    T*                             obj_;                 // 绑定的模型实例
    int                            name_;                // 状态名
    Machine<T>*                    machine_;             // 绑定的状态机对象
    std::vector<void (T::*)(void)> enter_functions_;     // 所有进入状态函数
    std::vector<void (T::*)(void)> exit_functions_;      // 所有退出状态函数
    std::vector<void (T::*)(void)> continous_functions_; // 所有状态持续函数
    std::vector<int>               subsequent_states_;   // 所有能够转移到的后续状态

    /* 多层状态相关 */
    Machine<T>* submachine_;     // 绑定的子状态机
    bool        has_submachine_; // 有子状态机
    bool        could_exit_;     // 上层状态可以从当前状态退出

  public:
    State() = default;

    /** 构造函数
     * 初始化: name_
     * 待初始化: machine_, functions, state_could_switch_ */
    State(int name, bool could_exit = true) : name_(name), could_exit_(could_exit) {
        has_submachine_ = false;
    }

    /* 获取当前状态的名称 */
    auto name() -> int {
        return name_;
    }

    /* 判断状态是否存在，只要名为NO_STATE(-2)就认为该状态为不存在（即使该实例是存在的） */
    auto exist() -> bool {
        return name_ != FSM_GET_NONE::NO_STATE;
    }

    /* 执行所有状态进入函数 */
    auto on_enter() -> void {
        for (const auto& f : enter_functions_) {
            auto func = std::bind(f, obj_);
            func();
        };
    }

    /* 执行所有状态退出函数 */
    auto on_exit() -> void {
        for (const auto& f : exit_functions_) {
            auto func = std::bind(f, obj_);
            func();
        }
    }

    /* 执行所有状态保持函数 */
    auto in_state() -> void {
        for (const auto& f : continous_functions_) {
            auto func = std::bind(f, obj_);
            func();
        }
    }

    /* 执行所有状态转移，当转移成功时return */
    auto try_switch() -> void {
        for (const auto& state_name : subsequent_states_) {
            auto switch_success = machine_->to_state(state_name);
            if (switch_success) {
                return;
            }
        }
    }

    /* 获取当前状态机 */
    auto get_machine() -> Machine<T>* {
        return machine_;
    }

    /* 状态关联状态机与模型实例（调用状态机的类实例）注册 */
    auto machine_regist(Machine<T>* machine, T* obj) -> void {
        machine_ = machine;
        obj_     = obj;
    }

    /* 状态关联下层子状态机，并将有子状态置为true */
    auto submachine_regist(Machine<T>* submachine) -> void {
        submachine_     = submachine;
        has_submachine_ = true;
    }

    /* 获取是否有子状态 */
    auto has_submachine() -> bool {
        return has_submachine_;
    }

    /* 获取下层子状态机 */
    auto get_submachine() -> Machine<T>* {
        return submachine_;
    }

    /* 返回上层状态是否可从当前状态退出 */
    auto could_exit() -> bool {
        return could_exit_;
    }

    /* 设置上层状态是否可以从当前状态退出 */
    auto set_could_exit(bool could) -> void {
        could_exit_ = could;
    }

    /** 状态关联函数注册
     * type: "on_enter", "on_exit", "in_state"
     */
    auto function_regist(void (T::*func)(void), std::string type) -> void {
        if (type == "on_enter") {
            enter_functions_.emplace_back(func);
        }
        else if (type == "on_exit") {
            exit_functions_.emplace_back(func);
        }
        else if (type == "in_state") {
            continous_functions_.emplace_back(func);
        }
        else {
            std::cout << "type must be one of \"on_enter\", \"on_exit\",\"in_state\"." << std::endl;
        }
    }

    /* 可转移的后续状态注册 */
    auto transition_regist(int to_state_name) -> void {
        subsequent_states_.emplace_back(to_state_name);
    }
};

/****************
 ** Transition **
 ****************/

template <class T>
class Transition {
  private:
    T*                             obj_;               // 绑定的模型实例
    int                            name_;              // 转移名
    int                            from_name_;         // 转移源状态名
    int                            to_name_;           // 转移目标状态名
    Machine<T>*                    machine_;           // 绑定的状态机实例
    Condition<T>                   condition_;         // 判断条件
    std::vector<void (T::*)(void)> prepare_functions_; // 所有转移准备函数
    std::vector<void (T::*)(void)> before_functions_;  // 所有转移前函数
    std::vector<void (T::*)(void)> after_functions_;   // 所有转移后函数

  public:
    Transition() = default;

    /** 构造函数
     * 初始化: name_, from_name_, to_name_
     * 待初始化: from_, to_, machine_, conditions_, functions_ */
    Transition(int name, int from_name, int to_name) : name_(name), from_name_(from_name), to_name_(to_name){};

    /* 获取转移名称 */
    auto name() -> int {
        return name_;
    }

    /* 判断转移是否存在，只要名为NO_TRANS(-1)就认为该转移为不存在（即使该实例是存在的） */
    auto exist() -> bool {
        return name_ != FSM_GET_NONE::NO_TRANS;
    }

    /* 获取转移源状态 */
    auto from() -> State<T>& {
        return machine_->get_state(from_name_);
    }

    /* 获取转移目标状态 */
    auto to() -> State<T>& {
        return machine_->get_state(to_name_);
    }

    /* 判断转移是否就绪 */
    auto is_ready() -> bool {
        return condition_.eval();
    }

    /* 执行所有转移准备函数，即发起转移后 and 判断转移就绪前要执行的函数 */
    auto prepare() -> void {
        for (const auto& f : prepare_functions_) {
            auto func = std::bind(f, obj_);
            func();
        };
    }

    /* 执行所有转移前函数，即判断转移就绪后要执行的函数 */
    auto before() -> void {
        for (const auto& f : before_functions_) {
            auto func = std::bind(f, obj_);
            func();
        };
    }

    /* 执行所有转移后函数 */
    auto after() -> void {
        for (const auto& f : after_functions_) {
            auto func = std::bind(f, obj_);
            func();
        };
    }

    /* 转移关联状态机与模型实例（调用状态机的类实例）注册 */
    auto machine_regist_and_state_init(Machine<T>* machine, T* obj) -> void {
        machine_ = machine;
        obj_     = obj;
        condition_.object_regist(obj_);
        from().transition_regist(to_name_);
    }

    /** 转移关联函数注册
     * type: "prepare", "before", "after" */
    auto function_regist(void (T::*func)(void), std::string type) -> void {
        if (type == "prepare") {
            prepare_functions_.emplace_back(func);
        }
        else if (type == "before") {
            before_functions_.emplace_back(func);
        }
        else if (type == "after") {
            after_functions_.emplace_back(func);
        }
    }

    /* 转移条件注册 */
    auto condition_regist(bool (T::*func)(void)) -> void {
        condition_.add_condition(func);
    }
};

/***************
 ** Condition **
 ***************/

template <class T>
class Condition {
  private:
    T*                             obj_;        // 绑定的模型实例
    std::vector<bool (T::*)(void)> conditions_; // 所有判断条件（并）[TODO] 加入或条件

  public:
    Condition() = default;

    /* 注册模型实例 */
    auto object_regist(T* obj) {
        obj_ = obj;
    }

    /* 添加判断条件，参数必须为输出bool的无参函数 */
    auto add_condition(bool (T::*func)(void)) -> void {
        conditions_.emplace_back(func);
    }

    /* 计算所有判断条件，返回是否通过 */
    auto eval() -> bool {
        for (const auto f : conditions_) {
            auto func = std::bind(f, obj_);
            if (!func()) {
                return false;
            }
        }
        return true;
    }
};

/*************
 ** Machine **
 *************/

template <class T>
class Machine {
  private:
    T*                           obj_; // 绑定的模型实例
    int                          default_state_name_;
    State<T>*                    current_state_;   // 当前状态
    State<T>*                    history_state_;   // 状态机退出时历史状态
    Transition<T>*               last_transition_; // 上一次转移
    std::map<int, State<T>>      states_;          // 所有状态
    std::map<int, Transition<T>> transitions_;     // 所有转移

  public:
    Machine() = default;

    /** 构造函数
     * 初始化: states_, transitions_, name_, current_state_
     * 帮助初始化: states_.machine, transitions_.machine, states_.to */
    Machine(std::vector<int> state_names, std::vector<std::vector<int>> transitions, int curstate_name, T* obj)
            : obj_(obj) {
        /* 初始化states_与transitions_ */
        states_.clear();
        transitions_.clear();
        for (const auto& state_name : state_names) {
            states_.emplace(state_name, State<T>(state_name));
        }
        for (const auto& trans : transitions) {
            transitions_.emplace(trans[0], Transition<T>(trans[0], trans[1], trans[2]));
        }

        /* 添加“不存在”的state与transition */
        states_.emplace(FSM_GET_NONE::NO_STATE, State<T>(FSM_GET_NONE::NO_STATE));
        transitions_.emplace(FSM_GET_NONE::NO_TRANS,
                             Transition<T>(FSM_GET_NONE::NO_TRANS, FSM_GET_NONE::NO_STATE, FSM_GET_NONE::NO_STATE));

        /* 初始化current_state_ */
        default_state_name_ = curstate_name;
        current_state_      = &get_state(default_state_name_);

        /* 将自身(machine)关联到states_与transitions_ */
        for (auto iter = states_.begin(); iter != states_.end(); iter++) {
            iter->second.machine_regist(this, obj_);
        }
        for (auto iter = transitions_.begin(); iter != transitions_.end(); iter++) {
            iter->second.machine_regist_and_state_init(this, obj_);
        }
        // /* 将自身(machine)关联到states_与transitions_ */
        // for (auto& state : states_) {
        //     state.machine_regist(this, obj_);
        // }
        // for (auto& trans : transitions_) {
        //     trans.machine_regist_and_state_init(this, obj_);
        // }
    }

    /* 获取状态机当前状态 */
    auto get_curstate() -> State<T>& {
        return *current_state_;
    }

    /* 获取此子状态机退出时的状态 */
    auto get_hisstate() -> State<T>& {
        return *history_state_;
    }

    /* 给出当前子状态机是否可以从当前状态退出，向子状态机递归判断 */
    auto could_exit() -> bool {
        return ((not current_state_->has_submachine()) or
                (current_state_->has_submachine() and current_state_->get_submachine()->could_exit())) and
               current_state_->could_exit();
    }

    /* 状态机退出，记录退出前状态为当前状态 */
    /** TODO by zgy
     * TODO: 退出模式要有pause和shutdown两种
     * Done: 仅在可以退出的状态退出当前状态机，递归退出子状态机
     */
    auto exit() -> void {
        if (current_state_->has_submachine()) {
            current_state_->get_submachine()->exit();
        }
        history_state_ = current_state_;
    }

    /* 重置状态机 */
    auto reset() -> void {
        history_state_ = &get_state(default_state_name_);
        current_state_ = &get_state(default_state_name_);
    }

    /* 状态机进入，重置当前状态为退出前状态，一般无需重置 */
    auto enter() -> void {
        current_state_ = history_state_;
    }

    /* 获取名为state_name的状态，若不存在则返回一个name=-2的State实例 */
    auto get_state(int state_name) -> State<T>& {
        if (states_.count(state_name) == 1) {
            return states_[state_name];
        }
        else {
            std::cout << "state " << state_name << " Not exists!" << std::endl;
            return states_[FSM_GET_NONE::NO_STATE];
        }
    }

    /* 判断当前状态是否（名）为state_name */
    auto is_state(int state_name) -> bool {
        return current_state_->name() == state_name;
    }

    /** 状态转移
     * 依次执行:
     * 准备函数->判断->转移前函数->子状态机退出->状态退出函数->状态切换->状态进入函数->转移后函数->记录本次转移 */
    auto to_state(int to_state_name) -> bool {
        auto trans = get_transition(current_state_->name(), to_state_name);
        if ((not current_state_->has_submachine()) or
            (current_state_->has_submachine() and current_state_->get_submachine()->could_exit())) {
            trans.prepare();
            if (trans.is_ready()) {
                trans.before();
                if (current_state_->has_submachine() and current_state_->get_submachine()->could_exit()) {
                    current_state_->get_submachine()->exit();
                }
                current_state_->on_exit();
                current_state_ = &get_state(to_state_name);
                current_state_->on_enter();
                trans.after();
                last_transition_ = &trans;
                return true;
            }
        }
        return false;
    }

    /* 添加状态 */
    auto add_state(int new_state_name) -> bool {
        if (!get_state(new_state_name).exist()) {
            states_.emplace(new_state_name, State<T>(new_state_name));
            get_state(new_state_name).machine_regist(this, obj_);
            return true;
        }
        else {
            return false;
        }
    }

    /* 注册状态函数，将名为state_name的状态通过type与func关联 */
    auto state_function_regist(int state_name, void (T::*func)(void), std::string type) -> void {
        get_state(state_name).function_regist(func, type);
    }

    /* 获取上一次转移 */
    auto get_last_transition() -> Transition<T>& {
        return last_transition_;
    }

    /* 获取名为trans_name的转移，若不存在则返回一个name=-1的Transition实例 */
    auto get_transition(int trans_name) -> Transition<T>& {
        if (transitions_.count(trans_name) == 1) {
            return transitions_[trans_name];
        }
        else {
            std::cout << "trans " << trans_name << " Not exists!" << std::endl;
            return transitions_[FSM_GET_NONE::NO_TRANS];
        }
    }

    /* 获取源名为from_name、目标名为to_name的转移，不存在则返回一个name=-1的Transition实例 */
    auto get_transition(int from_name, int to_name) -> Transition<T>& {
        for (auto iter = transitions_.begin(); iter != transitions_.end(); iter++) {
            if (iter->second.from().name() == from_name and iter->second.to().name() == to_name) {
                return iter->second;
            }
        }
        std::cout << "trans " << from_name << " TO " << to_name << " Not exists!" << std::endl;
        return transitions_[FSM_GET_NONE::NO_TRANS];
    }

    /* 添加转移 */
    auto add_transition(int new_trans_name, int from_name, int to_name) -> bool {
        auto got_trans      = get_transition(new_trans_name);
        auto got_from_state = get_state(from_name);
        auto got_to_state   = get_state(to_name);
        if (!got_trans.exist() and got_from_state.exist() and got_to_state.exist()) {
            transitions_.emplace(new_trans_name, Transition<T>(new_trans_name, got_from_state, got_to_state));
            get_transition(new_trans_name).machine_regist_and_state_init(this, obj_);
            return true;
        }
        else {
            return false;
        }
    }

    /* 转移函数注册，type: "prepare", "before", "after" */
    auto transition_function_regist(int trans_name, void (T::*func)(void), std::string type) -> void {
        get_transition(trans_name).function_regist(func, type);
    }

    /* 转移函数注册，type: "prepare", "before", "after" */
    auto transition_function_regist(int from_name, int to_name, void (T::*func)(void), std::string type) -> void {
        get_transition(from_name, to_name).function_regist(func, type);
    }

    /* 通过转移名进行转移条件注册 */
    auto transition_condition_regist(int trans_name, bool (T::*func)(void)) -> void {
        get_transition(trans_name).condition_regist(func);
    }

    /* 通过源状态与目标状态进行转移条件注册 */
    auto transition_condition_regist(int from_name, int to_name, bool (T::*func)(void)) -> void {
        get_transition(from_name, to_name).condition_regist(func);
    }

    /* 状态机运行一次 */
    auto on_going() -> void {
        current_state_->try_switch();
        current_state_->in_state();
        if (current_state_->has_submachine()) {
            current_state_->get_submachine()->on_going();
        }
    }
};

#endif //__WICRI_FINITE_STATE_MACHINE__