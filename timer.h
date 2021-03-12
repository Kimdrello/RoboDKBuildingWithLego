#ifndef TIMER_H
#define TIMER_H

#include <iostream>
#include <thread>

class Timer{
/// A timer for printing out runtime, this timer runs in its own thread.
/// Full credit goes towards its creator @ https://www.fluentcpp.com/2018/12/28/timer-cpp/

    bool clear = false;

public:
    template<typename Function>
    void setTimeout(Function function, int delay){
        this->clear = false;
        std::thread t([=]() {
            if(this->clear) return;
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            if(this->clear) return;
            function();
        });
        t.detach();
    }

    template<typename Function>
    void setInterval(Function function, int interval){
        this->clear = false;
            std::thread t([=]() {
                while(true) {
                    if(this->clear) return;
                    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                    if(this->clear) return;
                    function();
                }
            });
            t.detach();
    }

    void stop(){
       this->clear = true;
    }
};

#endif // TIMER_H
