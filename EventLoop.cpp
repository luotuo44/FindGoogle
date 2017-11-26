//Author: luotuo44@gmail.com
//Use of this source code is governed by a BSD-style license


#include"EventLoop.h"


#include<stdexcept>

#include"event.h"
#include"event2/bufferevent.h"


#include"Logging.h"


namespace Net
{


EventLoop::EventLoop()
{
    m_base = event_base_new();
    if(m_base == nullptr)
    {
        logMsg(ERROR, "fail to allocate event_base");
        throw std::logic_error("fail to allocate event_base");
    }
}


EventLoop::EventLoop(EventLoop &&el)
{
    std::swap(m_base, el.m_base);
}



EventLoop::~EventLoop()
{
    if(m_base != nullptr)
        event_base_free(m_base);
}



bufferevent* EventLoop::newBufferevent()
{
    bufferevent *bev = bufferevent_socket_new(m_base, -1, BEV_OPT_CLOSE_ON_FREE);

    if( bev == nullptr )
    {
        logMsg(WARNING, "fail to allocate bufferevent");
        throw  std::logic_error("fail to allocate bufferevent");
    }

    return bev;
}


event* EventLoop::newEvent()
{
    event *ev = new event;
    ev->ev_base = m_base;
    return ev;
}


void EventLoop::run()
{
    event_base_dispatch(m_base);
}


}

