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
* 3. 支持宏定义快速创建状态机，顶状态机可以改为 Machine 类，所有默认函数（每类函数都可注册多个函数，但至少有一个默认函数）直接通过重载实现。
* 4. 支持事件（转移由状态机自动处理，事件由用户程序发布），事件分为一般事件和强制事件，一般事件处理为数据更新，强制事件处理为强制转移，所有默认事件的回调函数（同上）直接通过重载实现。
*/

#ifndef __AUTOFSM_H__
#define __AUTOFSM_H__

#include <exception>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace fsm {

class fsm_exception : public std::exception {
    std::string message_;

  public:
    fsm_exception() : message_("Undefined exception.") {}
    fsm_exception(std::string message) { message_ = "Error!: " + message; }
    virtual const char* what() const throw() override { return message_.c_str(); }
};

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
    void condition_reg(bool (T::*func)(void)) { conditions_.emplace_back(func); }
    bool is_ready() {
        if (conditions_.empty()) throw fsm_exception("[transition " + std::to_string(to_) + "] has no condition registied");
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
    int  id_{-1};
    bool busy_{false};

    std::unordered_map<int, State<T>*> childs_;

    int default_child_id_{-1};
    int current_child_id_{-1};

    std::unordered_map<int, Transition<T>*> transitions_;

    std::vector<void (T::*)(void)> on_enter_functions_;
    std::vector<void (T::*)(void)> in_process_functions_;
    std::vector<void (T::*)(void)> on_exit_functions_;

  public:
    State(int id, T* obj) : id_(id), obj_(obj) {}
    ~State() { recursive_free(); }

    int  id() { return id_; }
    bool busy() { return busy_; }
    auto childs() { return childs_; }
    auto child(int id) {
        if (id == -1) throw fsm_exception(std::string{""} + "[state " + std::to_string(id_) + "] trying to get child [-1]");
        return childs_.at(id);
    }
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
    void allowence(bool allowance_to_abort) { busy = allowance_to_abort; }
    void add_child(int id, bool default_child = false) {
        childs_.insert_or_assign(id, new State<T>(id, obj_));
        if (default_child) {
            default_child_id_ = id;
            current_child_id_ = id;
        }
    }
    void add_child(int id, void (T::*func)(void), bool default_child = false) {
        childs_.insert_or_assign(id, new State<T>(id, obj_));
        child(id)->in_process_func_reg(func);
        if (default_child) {
            default_child_id_ = id;
            current_child_id_ = id;
        }
    }

    auto trans_reg(int to) { transitions_.insert_or_assign(to, new Transition<T>(to, obj_)); }
    auto trans_reg(int to, bool (T::*func)(void)) {
        transitions_.insert_or_assign(to, new Transition<T>(to, obj_));
        transition(to)->condition_reg(func);
    }

    void on_enter_func_reg(void (T::*func)(void)) { on_enter_functions_.emplace_back(func); }
    void in_process_func_reg(void (T::*func)(void)) { in_process_functions_.emplace_back(func); }
    void on_exit_func_reg(void (T::*func)(void)) { on_exit_functions_.emplace_back(func); }
    void on_enter() {
        for (auto& f : on_enter_functions_) std::bind(f, obj_)();
        if (current_child_id_ != -1) current_child()->on_enter();
    }
    void in_process() {
        for (auto& f : in_process_functions_) std::bind(f, obj_)();
    }
    void on_exit() {
        for (auto& f : on_exit_functions_) std::bind(f, obj_)();
    }
    bool try_exit() {
        if (busy()) return false;
        if (current_child_id_ != -1 and not current_child()->try_exit()) return false;
        on_exit();
        return true;
    }

    bool try_child_trans(int to_state) {
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
                if (try_child_trans(to)) break;
            current_child()->process();
        }
    }
};

} // namespace fsm

#endif // __AUTOFSM_H__