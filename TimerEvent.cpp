//Author: luotuo44@gmail.com
//Use of this source code is governed by a BSD-style license

#include"TimerEvent.h"

#include<sys/time.h>

#include<stdexcept>

#include"event.h"


#include"EventLoop.h"
#include"Logging.h"


namespace Net
{


TimerEvent::~TimerEvent()
{
    clear();
}


void TimerEvent::init(EventLoop &loop, size_t interval, bool need_persist, const TimerCbParam &cb_param)
{
    if(m_ev != nullptr )
        throw std::logic_error("this TimerEvent has init");


    struct event *ev = loop.newEvent();
    event_base *base = ev->ev_base;

    short e = EV_TIMEOUT | (need_persist ? EV_PERSIST : 0);
    event_assign(ev, base, -1, e, TimerEvent::eventCb, this);

    struct timeval tv;
    tv.tv_sec = interval / 1000;
    tv.tv_usec = (interval%1000) * 1000;

    int ret = event_add(ev, &tv);
    if( ret == 0 )//success
    {
        m_cb_param = cb_param;
        m_ev = ev;
    }
    else
    {
        ev->ev_base = nullptr;//只是简单的释放内存
        event_free(ev);
        logMsg(WARNING, "fail to add timer event");
        throw std::logic_error("fail to add timer event");
    }
}



void TimerEvent::cancal()
{
    clear();
}


void TimerEvent::clear()
{
    if(m_ev != nullptr)
    {
        event_del(m_ev);
        delete m_ev;
        m_ev = nullptr;
    }
}

void TimerEvent::eventCb(int , short e, void *arg)
{
    TimerEvent *te = reinterpret_cast<TimerEvent*>(arg);
    if(te->m_cb_param.cb)
        te->m_cb_param.cb(te->m_cb_param.arg);
}


}

