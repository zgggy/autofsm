/**
* Created Time: 2024.08.16
* File name:    autofsm
* Author:       zgggy(zgggy@foxmail.com)
* Brief:        自动有限状态机
* Include:      
*/

#ifndef __AUTOFSM_H__
#define __AUTOFSM_H__

#include <functional>
#include <unordered_map>
#include <vector>

template <class T>
class Transition {
  private:
    T*  obj_;
    int to_;

    std::vector<bool (T::*)(void)> conditions_;
    std::vector<void (T::*)(void)> prepare_functions_; // 所有转移准备函数
    std::vector<void (T::*)(void)> before_functions_;  // 所有转移前函数
    std::vector<void (T::*)(void)> after_functions_;   // 所有转移后函数

  public:
    Transition(int to, T* obj) : to_(to), obj_(obj) {}
    ~Transition() {}
    void condition_reg(bool(T::*func)) { conditions_.emplace_back(func); }
    bool is_ready() {
        for (const auto f : conditions_)
            if (! std::bind(f, obj_)()) return false;
        return true;
    }
    void prepare_func_reg(void (T::*func)(void)) { prepare_functions_.emplace_back(func); }
    void before_func_reg(void (T::*func)(void)) { before_functions_.emplace_back(func); }
    void after_func_reg(void (T::*func)(void)) { after_functions_.emplace_back(func); }
    void prepare() {
        for (auto& f : prepare_functions_) std::bind(f, obj_)();
    }
    void before() {
        for (auto& f : before_functions_) std::bind(f, obj_)();
    }
    void after() {
        for (auto& f : after_functions_) std::bind(f, obj_)();
    }
};

template <class T>
class State {
  private:
    T*   obj_;
    int  name_{-1};
    bool busy_{false};

    std::unordered_map<int, State<T>*> childs_;
    int                                default_child_name_{-1};
    int                                current_child_name_{-1};

    std::unordered_map<int, Transition<T>*> transitions_;

    std::vector<void (T::*)(void)> on_enter_functions_;
    std::vector<void (T::*)(void)> in_process_functions_;
    std::vector<void (T::*)(void)> on_exit_functions_;

  public:
    State(int name, T* obj) : name_(name), obj_(obj) {}
    ~State() { recursive_free(); }

    int  name() { return name_; }
    bool busy() { return busy_; }
    auto childs() { return childs_; }
    auto child(int name) { return childs_.at(name); }
    auto current_child_name() { return current_child_name_; }
    auto current_child() { return child(current_child_name_); }
    auto transitions() { return transitions_; }
    auto transition(int to) { return transitions_.at(to); }

    void recursive_free() {
        for (auto& [_, trans] : transitions_) delete trans;
        for (auto& [_, child] : childs_) {
            child->recursive_free();
            delete child;
        }
    }
    void allowence(bool allowance_to_abort) { busy = allowance_to_abort; }
    void add_child(int name, bool default_child = false) {
        childs_.insert_or_assign(name, new State<T>(name, obj_));
        if (default_child) {
            default_child_name_ = name;
            current_child_name_ = name;
        }
    }

    auto trans_reg(int to) { transitions_.insert_or_assign(to, new Transition<T>(to, obj_)); }

    void on_enter_func_reg(void (T::*func)(void)) { on_enter_functions_.emplace_back(func); }
    void in_process_func_reg(void (T::*func)(void)) { in_process_functions_.emplace_back(func); }
    void on_exit_func_reg(void (T::*func)(void)) { on_exit_functions_.emplace_back(func); }
    void on_enter() {
        for (auto& f : on_enter_functions_) std::bind(f, obj_)();
        if (current_child_name_ != -1) childs_.at(current_child_name_)->on_enter();
    }
    void in_process() {
        for (auto& f : in_process_functions_) std::bind(f, obj_)();
    }
    void on_exit() {
        for (auto& f : on_exit_functions_) std::bind(f, obj_)();
    }
    bool try_exit() {
        if (busy()) return false;
        if (current_child_name_ == -1) return false;
        if (not childs_.at(current_child_name_)->try_exit()) return false;
        on_exit();
        return true;
    }

    bool try_child_trans(int to_state) {
        auto trans = transitions_.at(to_state);
        trans->prepare();
        if (trans->is_ready()) {
            trans->before();
            if (childs_.at(current_child_name_)->try_exit()) {
                current_child_name_ = to_state;
                childs_.at(current_child_name_)->on_enter();
                trans->after();
                return true;
            }
            trans->after();
        }
        return false;
    }

    void process() {
        in_process();
        if (current_child_name_ != -1) {
            for (auto [to, _] : childs_.at(current_child_name_)->transitions_)
                if (try_child_trans(to)) break;
            childs_.at(current_child_name_)->process();
        }
    }
};

#endif // __AUTOFSM_H__