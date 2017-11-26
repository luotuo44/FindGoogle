//Author: luotuo44@gmail.com
//Use of this source code is governed by a BSD-style license

#include<stddef.h>


#include<functional>

struct event;

namespace Net
{

class EventLoop;


using TimerCbFunc = std::function<void (void*)>;

struct TimerCbParam
{
    TimerCbFunc cb;
    void *arg = nullptr;
};


class TimerEvent
{
public:
    TimerEvent()=default;
    TimerEvent(const TimerEvent&)=delete;
    TimerEvent& operator = (const TimerEvent&)=delete;

    ~TimerEvent();

    //单位毫秒
    void init(EventLoop &loop, size_t interval, bool need_persist, const TimerCbParam &cb_param);
    void cancal();

private:
    void clear();
    static void eventCb(int, short e, void *arg);

private:
    struct event *m_ev = nullptr;
    TimerCbParam m_cb_param;
    size_t m_interval = 0;//单位毫秒
};


}
