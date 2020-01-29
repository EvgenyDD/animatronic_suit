#ifndef SMART_THREAD_H
#define SMART_THREAD_H

#include <thread>
#include <atomic>
#include <functional>
#include "debug_tools.h"
#include <condition_variable>

class basic_thread
{
public:
    void start(std::function<int(void)> fun)
    {
        this->fun = fun;
        flagActive = true;
        thread.reset(new std::thread(&basic_thread::process, this));
    }

    void terminate()
    {
        flagActive = false;

        cv_delay.notify_all(); // terminate delays

        if(thread)
            if(thread->joinable())
            {
                thread->join();
            }
    }

//    void delay(uint32_t delay_ms)
//    {
//        std::unique_lock<std::mutex> lock(mtx_delay);
//        cv_delay.wait_for(lock, delay_ms);
//    }

//    void delay(uint32_t delay_us)
//    {
//        std::unique_lock<std::mutex> lock(mtx_delay);
//        cv_delay.wait_for(lock, delay_us);
//    }

    bool isActive() const { return flagActive; }

private:
    void process()
    {
        while(flagActive)
        {
            if(fun())
            {
                flagActive = false;
                PRINT_INFO("[XX]\tThread has terminated itself");
            }
        }
    }

    std::atomic_bool flagActive{false};
    std::unique_ptr<std::thread> thread{};
    std::function<int(void)> fun;

    std::condition_variable cv_delay;
    std::mutex mtx_delay;
};

#endif // SMART_THREAD_H
