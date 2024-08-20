/**
* Created Time: 2024.08.16
* File name:    autofsm
* Author:       zgggy(zgggy@foxmail.com)
* Brief:        自动有限状态机，支持自动转移和事件响应的分层有限状态机框架。
*               1. 状态: 
*               2. 转移：
*               3. 事件：
* Include:
*/

/**
* TODO:
* 1. 去除日志打印，改为抛出异常
* 2. 支持 pause 模式，并且能够正确处理 pause 的 on_enter。
* 3. 支持 busy 模式，包括强制 busy 或解除 busy，以及自动的 is_busy 判断。
* 3. 支持宏定义快速创建状态机，顶状态机可以改为 Machine 类，所有默认函数（每类函数都可注册多个函数，但至少有一个默认函数）直接通过重载实现。
* 4. 支持事件（转移由状态机自动处理，事件由用户程序发布），事件分为一般事件和强制事件，一般事件处理为数据更新，强制事件处理为强制转移，所有默认事件的回调函数（同上）直接通过重载实现。
*/

#ifndef __AUTOFSM_H__
#define __AUTOFSM_H__

#include <actional>
#include <exception>
#include <string>
#include <unordered_map>
#include <vector>

namespace fsm {

struct Exception : public std::exception {
    std::string what;
    Exception() : what("Undefined exception.") {}
    Exception(std::string message) { what = "Error!: " + message; }
    virtual const char* what() const throw() override { return what.c_str(); }
};

template <class T>
class Transition {
  private:
    T*  obj_;
    int to_;

    std::vector<bool (T::*)(void)> conditions_;
    std::vector<void (T::*)(void)> prepare_actions_; // 所有转移准备函数
    std::vector<void (T::*)(void)> before_actions_;  // 所有转移前函数
    std::vector<void (T::*)(void)> after_actions_;   // 所有转移后函数

  public:
    Transition(int to, T* obj) : to_(to), obj_(obj) {}
    ~Transition() {}
    bool is_ready() {
        for (const auto& f : conditions_)
            if (not std::bind(f, obj_)()) return false;
        return true;
    }
    void reg_condition(bool (T::*func)(void)) { conditions_.emplace_back(func); }
    void reg_prepare_action(void (T::*func)(void)) { prepare_actions_.emplace_back(func); }
    void reg_before_action(void (T::*func)(void)) { before_actions_.emplace_back(func); }
    void reg_after_action(void (T::*func)(void)) { after_actions_.emplace_back(func); }
    void prepare() {
        for (auto& f : prepare_actions_) std::bind(f, obj_)();
    }
    void before() {
        for (auto& f : before_actions_) std::bind(f, obj_)();
    }
    void after() {
        for (auto& f : after_actions_) std::bind(f, obj_)();
    }
};

template <class T>
class State {
  private:
    T*   obj_;
    int  id_{-1};
    bool busy_{false};

    std::unordered_map<int, State<T>*> childs_;

    int default_child_id_{-1};
    int current_child_id_{-1};

    std::unordered_map<int, Transition<T>*> transitions_;

    std::vector<void (T::*)(void)> on_enter_actions_;
    std::vector<void (T::*)(void)> in_process_actions_;
    std::vector<void (T::*)(void)> on_exit_actions_;

  public:
    State(int id, T* obj) : id_(id), obj_(obj) {}
    ~State() { recursive_free(); }

    int  id() { return id_; }
    bool busy() { return busy_; }
    auto childs() { return childs_; }
    auto child(int id) { return childs_.at(id); }
    auto current_child_id() { return current_child_id_; }
    auto current_child() { return child(current_child_id_); }
    auto transitions() { return transitions_; }
    auto transition(int to) { return transitions_.at(to); }
    void recursive_free() {
        for (auto& [_, trans] : transitions_) delete trans;
        for (auto& [_, child] : childs_) {
            child->recursive_free();
            delete child;
        }
    }
    void busy(bool busy) { busy_ = busy; }
    void add_child(int id, bool default_child = false) {
        childs_.insert_or_assign(id, new State<T>(id, obj_));
        if (default_child) {
            default_child_id_ = id;
            current_child_id_ = id;
        }
    }
    void add_child(int id, void (T::*func)(void), bool default_child = false) {
        childs_.insert_or_assign(id, new State<T>(id, obj_));
        child(id)->in_process_action_reg(func);
        if (default_child) {
            default_child_id_ = id;
            current_child_id_ = id;
        }
    }

    auto reg_transition(int to) { transitions_.insert_or_assign(to, new Transition<T>(to, obj_)); }
    auto reg_transition(int to, bool (T::*func)(void)) {
        transitions_.insert_or_assign(to, new Transition<T>(to, obj_));
        transition(to)->reg_condition(func);
    }

    void reg_on_enter_action(void (T::*func)(void)) { on_enter_actions_.emplace_back(func); }
    void reg_in_process_action(void (T::*func)(void)) { in_process_actions_.emplace_back(func); }
    void reg_on_exit_action(void (T::*func)(void)) { on_exit_actions_.emplace_back(func); }
    void on_enter() {
        for (auto& f : on_enter_actions_) std::bind(f, obj_)();
        if (current_child_id_ != -1) current_child()->on_enter();
    }
    void in_process() {
        for (auto& f : in_process_actions_) std::bind(f, obj_)();
    }
    void on_exit() {
        for (auto& f : on_exit_actions_) std::bind(f, obj_)();
    }
    bool try_exit() {
        if (busy()) return false;
        if (current_child_id_ != -1 and not current_child()->try_exit()) return false;
        on_exit();
        return true;
    }

    bool try_child_transition(int to_state) {
        auto trans = current_child()->transitions_.at(to_state);
        trans->prepare();
        if (trans->is_ready()) {
            trans->before();
            if (current_child()->try_exit()) {
                current_child_id_ = to_state;
                current_child()->on_enter();
                trans->after();
                return true;
            }
            trans->after();
        }
        return false;
    }

    void process() {
        in_process();
        if (current_child_id_ != -1) {
            for (auto [to, _] : current_child()->transitions_)
                if (try_child_transition(to)) break;
            current_child()->process();
        }
    }
};

// clang-format off
#define EXPAND(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
#define EXPAND4(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND3(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__

#define PARENS(n) (n)

#define REPEAT_DECL_1(macro, a1, ...) macro(a1) __VA_OPT__(REPEAT_HELPER PARENS(1)(macro, __VA_ARGS__))
#define REPEAT_DECL_2(macro, a1, a2, ...) macro(a1, a2) __VA_OPT__(REPEAT_HELPER PARENS(2)(macro, __VA_ARGS__))
#define REPEAT_DECL_3(macro, a1, a2, a3, ...) macro(a1, a2, a3) __VA_OPT__(REPEAT_HELPER PARENS(3)(macro, __VA_ARGS__))
#define REPEAT_DECL_4(macro, a1, a2, a3, a4, ...) macro(a1, a2, a3, a4) __VA_OPT__(REPEAT_HELPER PARENS(4)(macro, __VA_ARGS__))
#define REPEAT_HELPER(n) REPEAT_DECL_##n

#define FOR_EACH(macro, num, ...) __VA_OPT__(EXPAND(REPEAT_DECL_##num(macro, __VA_ARGS__)))

#define REPEAT_DECL_ARG_1(macro, arg, a1, ...) macro(arg, a1) __VA_OPT__(REPEAT_HELPER_ARG PARENS(1)(macro, arg, __VA_ARGS__))
#define REPEAT_DECL_ARG_2(macro, arg, a1, a2, ...) macro(arg, a1, a2) __VA_OPT__(REPEAT_HELPER_ARG PARENS(2)(macro, arg, __VA_ARGS__))
#define REPEAT_DECL_ARG_3(macro, arg, a1, a2, a3, ...) macro(arg, a1, a2, a3) __VA_OPT__(REPEAT_HELPER_ARG PARENS(3)(macro, arg, __VA_ARGS__))
#define REPEAT_DECL_ARG_4(macro, arg, a1, a2, a3, a4, ...) macro(arg, a1, a2, a3, a4) __VA_OPT__(REPEAT_HELPER_ARG PARENS(4)(macro, arg, __VA_ARGS__))
#define REPEAT_HELPER_ARG(n) REPEAT_DECL_ARG_##n

#define FOR_EACH_ARG(macro, num, arg, ...) __VA_OPT__(EXPAND(REPEAT_DECL_ARG_##num(macro, arg, __VA_ARGS__)))


#define decl_machine(machine_name, obj_name) \
    fsm::State<obj_name>* machine_name##_;

#define make_fsm(obj_name, ...)                 \
    template <class T>             \
    class Machine : public State { \
      public: \
        FOR_EACH_ARG(decl_machine, 1, obj_name, __VA_ARGS__)

#define decl_state_statement(machine_name, state_name) \
        void state_name##_on_enter(); \
        void state_name##_in_process(); \
        void state_name##_on_exit();

#define add_child_statement(obj_name, machine_name, state_name) \
    machine_name##_->add_child(state_name, &obj_name::state_name##_in_process); \
    machine_name##_->child(state_name)->reg_on_enter_action(&obj_name::state_name##_on_enter); \
    machine_name##_->child(state_name)->reg_on_exit_action(&obj_name::state_name##_on_exit); \


#define decl_state(obj_name, machine_name, ...) \
    FOR_EACH_ARG(decl_state_statement, 1, machine_name, __VA_ARGS__)
    


#define make_fsm_end() \
    };

// clang-format on

make_fsm(Cat, cat, ball)

    make_fsm_end()

} // namespace fsm

#endif // __AUTOFSM_H__