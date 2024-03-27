/**
 * Created Time: 2022.09.15
 * File name:    cat.cpp
 * Author:       zgy(zhangguangyan@wicri.org)
 * Brief:        一个使用多层状态机框架的关于猫的状态机模型示例
 * Include:      Enum: CatStates, CatTransitions; Class: Cat
 */

#include <stdlib.h>
#include <time.h>

#include <chrono>
#include <thread>
// #include <windows.h>

#include <sstream>
#include <string>
#include <vector>

#include "../include/FiniteStateMachine.h"

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

MAKE_ENUM(CatStates,             //
          SLEEP,                 //
          HUNGRY,                //
          PLAY,                  //
          PLAY_PlayingWithBall,  //
          PLAY_SearchingForBall, //
          CALM,                  //
          CALM_Dazing,           //
          CALM_Cleaning);

MAKE_ENUM(CatTransitions,                        //
          SLEEP_CALM,                            //
          CALM_SLEEP,                            //
          CALM_HUNGRY,                           //
          CALM_PLAY,                             //
          PLAY_CALM,                             //
          PALY_HUNGRY,                           //
          HUNGRY_CALM,                           //
          PLAY_PlayingWithBall_SearchingForBall, //
          PLAY_SearchingForBall_PlayingWithBall, //
          CALM_Dazing_Cleaning,                  //
          CALM_Cleaning_Dazing);

/* 定义用于初始化状态机的状态集合与转移集合 */
StateList topstates      = {SLEEP, HUNGRY, PLAY, CALM};
StateList substates_play = {PLAY_PlayingWithBall, PLAY_SearchingForBall};
StateList substates_calm = {CALM_Cleaning, CALM_Dazing};

TransTable toptrans = {
    {SLEEP_CALM,  SLEEP,  CALM  },
    {CALM_SLEEP,  CALM,   SLEEP },
    {CALM_HUNGRY, CALM,   HUNGRY},
    {CALM_PLAY,   CALM,   PLAY  },
    {PLAY_CALM,   PLAY,   CALM  },
    {PALY_HUNGRY, PLAY,   HUNGRY},
    {HUNGRY_CALM, HUNGRY, CALM  }
};
TransTable subtrans_play = {
    {PLAY_PlayingWithBall_SearchingForBall, PLAY_PlayingWithBall,  PLAY_SearchingForBall},
    {PLAY_SearchingForBall_PlayingWithBall, PLAY_SearchingForBall, PLAY_PlayingWithBall }
};
TransTable subtrans_calm = {
    {CALM_Dazing_Cleaning, CALM_Dazing,   CALM_Cleaning},
    {CALM_Cleaning_Dazing, CALM_Cleaning, CALM_Dazing  }
};

class Cat {
  public:
    float         feed_degree_;   // 0~20:hungry, 21~50:calm, 51~100(if don't stop eating:eating):play
    float         excite_degree_; // 0~20:sleep, 21~50:calm, 51~100:play
    Machine<Cat>* tomplay_;
    Machine<Cat>* tomcalm_;
    Machine<Cat>* tomtop_;

  private:
    bool  see_a_ball_;
    float see_a_ball_prob_;
    bool  nightmare_;
    float nightmare_prob_; // if cat gets nightmare, it will wakeup
    bool  dirty_;
    float clean_prob_;
    float stop_eating_prob_;

  public:
    Cat() {
        feed_degree_      = 60.0;
        excite_degree_    = 50.0;
        see_a_ball_       = false;
        see_a_ball_prob_  = 60;
        nightmare_        = false;
        nightmare_prob_   = 2;
        dirty_            = false;
        clean_prob_       = 80;
        stop_eating_prob_ = 30;

        /** 状态机初始化与注册
         * 不同层级的状态用不同的状态机包含，随后将子状态机注册到对应上层状态
         * 参数：状态集合，转移集合，初始状态，当前模型实例的地址（需要通过std::bind将状态机调用的类函数与实例绑定，否则函数类型不一致）*/
        tomtop_  = new Machine<Cat>(topstates, toptrans, CALM, this);
        tomplay_ = new Machine<Cat>(substates_play, subtrans_play, PLAY_SearchingForBall, this);
        tomcalm_ = new Machine<Cat>(substates_calm, subtrans_calm, CALM_Dazing, this);

        /* 状态机注册：通过get_state(state_name)获取状态，将子状态机地址注册进来 */
        tomtop_->get_state(PLAY).submachine_regist(tomplay_);
        tomtop_->get_state(CALM).submachine_regist(tomcalm_);
        tomcalm_->get_state(CALM_Cleaning).set_could_exit(false);

        /** 状态相关函数注册
         * 将函数加入到对应状态的对应函数集中
         * 参数：状态名，类名::函数名，函数类型 */
        tomtop_->state_function_regist(SLEEP, &Cat::sleep, "in_state");
        tomtop_->add_variable("sleep_clam", 1, 1);
        tomtop_->regist_variable_with_state("sleep_clam", {SLEEP, CALM});
        tomtop_->state_function_regist(HUNGRY, &Cat::eat, "in_state");
        tomtop_->add_variable("eat_play", 0.1, 0.1);
        tomtop_->regist_variable_with_state("eat_play", {PLAY, HUNGRY});
        tomtop_->state_function_regist(PLAY, &Cat::play, "in_state");
        tomtop_->state_function_regist(CALM, &Cat::clam, "in_state");
        tomtop_->state_function_regist(SLEEP, &Cat::out_sleep, "on_exit");

        tomplay_->state_function_regist(PLAY_PlayingWithBall, &Cat::playing_with_ball, "in_state");
        tomplay_->state_function_regist(PLAY_SearchingForBall, &Cat::searching_for_ball, "in_state");

        tomcalm_->state_function_regist(CALM_Cleaning, &Cat::clean, "in_state");

        /** 转移相关函数注册
         * 将判断条件加入到对应的转移函数集中
         * 参数：转移名，类名::函数名 */
        tomtop_->transition_condition_regist(SLEEP_CALM, &Cat::could_weakup);
        tomtop_->transition_condition_regist(CALM_SLEEP, &Cat::sleepy);
        tomtop_->transition_condition_regist(CALM_HUNGRY, &Cat::exhusted);
        tomtop_->transition_condition_regist(CALM_PLAY, &Cat::exciting);
        tomtop_->transition_condition_regist(PLAY_CALM, &Cat::boring);
        tomtop_->transition_condition_regist(PALY_HUNGRY, &Cat::exhusted);
        tomtop_->transition_condition_regist(HUNGRY_CALM, &Cat::feeded);

        tomplay_->transition_condition_regist(PLAY_PlayingWithBall_SearchingForBall, &Cat::lost_ball);
        tomplay_->transition_condition_regist(PLAY_SearchingForBall_PlayingWithBall, &Cat::find_ball);

        tomcalm_->transition_condition_regist(CALM_Dazing_Cleaning, &Cat::want_clean);
        tomcalm_->transition_condition_regist(CALM_Cleaning_Dazing, &Cat::is_clean);
    }

    /* state funcs */
    void sleep() {
        excite_degree_ += 2;
        feed_degree_ -= 0.25;
        nightmare_ = (rand() % 100) < nightmare_prob_;
        if (nightmare_) {
            excite_degree_ += 10;
            std::cout << " nightmare! ";
        }
        tomtop_->variables_.at("sleep_clam").Set(tomtop_->variables_.at("sleep_clam").Get<int>() + 1);
    }

    void out_sleep() {
        if (nightmare_) std::cout << " nightmare over~";
        std::cout << "out sleep~~~" << std::endl;
        nightmare_ = false;
    }

    void clam() {
        feed_degree_ -= 1;
        excite_degree_ -= 5;
        tomtop_->variables_.at("sleep_clam").Set(tomtop_->variables_.at("sleep_clam").Get<int>() + 1);
    }

    void clean() {
        std::cout << "clean! ";
        dirty_ = false;
    }

    void play() {
        dirty_      = true;
        see_a_ball_ = (rand() % 100) < see_a_ball_prob_;
        tomtop_->variables_.at("eat_play").Set(tomtop_->variables_.at("eat_play").Get<double>() + 0.1);
    }

    void playing_with_ball() {
        feed_degree_ -= 5;
        excite_degree_ -= 1;
        see_a_ball_ = (rand() % 100) < see_a_ball_prob_;
    }

    void searching_for_ball() {
        feed_degree_ -= 4;
        excite_degree_ -= 2;
        see_a_ball_ = (rand() % 100) < see_a_ball_prob_;
    }

    void eat() {
        feed_degree_ += 10;
        excite_degree_ -= 0.5;
        dirty_ = true;
        tomtop_->variables_.at("eat_play").Set(tomtop_->variables_.at("eat_play").Get<double>() + 0.1);
    }

    /* conditions */
    bool could_weakup() { return excite_degree_ > 90 or feed_degree_ <= 0 or nightmare_; }

    bool sleepy() { return excite_degree_ <= 20 and feed_degree_ > 20; }

    bool boring() { return excite_degree_ > 20 and excite_degree_ <= 50; }

    bool exciting() { return excite_degree_ > 50; }

    bool exhusted() { return feed_degree_ <= 20; }

    bool feeded() { return (feed_degree_ > 70 and (rand() % 100) < stop_eating_prob_) or feed_degree_ > 100; }

    bool lost_ball() { return ! see_a_ball_; }

    bool find_ball() { return see_a_ball_; }

    bool want_clean() { return (rand() % 100) < clean_prob_ and dirty_; }

    bool is_clean() { return ! dirty_; }
};

/** TODO by zgy
 * 继续简化初始化过程  */

int main() {
    /* Cat类实例化一个模型 */
    Cat tom;

    /* 主程序，只需要按照频率循环machine.on_going() */
    while (1) {
        srand((unsigned)time(NULL));

        /* print something */
        std::cout << "[" << CatStatesToStr(tom.tomtop_->get_curstate().name());
        if (tom.tomtop_->is_state(PLAY))
            std::cout << "." << CatStatesToStr(tom.tomplay_->get_curstate().name()) << "]\n";
        else if (tom.tomtop_->is_state(CALM))
            std::cout << "." << CatStatesToStr(tom.tomcalm_->get_curstate().name()) << "]\n";
        else
            std::cout << "]\n";
        std::cout << "f:" << tom.feed_degree_ << " e:" << tom.excite_degree_ << std::endl;

        tom.tomtop_->on_going();

        std::cout << "sleep_clam: " << tom.tomtop_->variables_.at("sleep_clam").Get<int>() << std::endl;
        std::cout << "eat_play: " << tom.tomtop_->variables_.at("eat_play").Get<double>() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}