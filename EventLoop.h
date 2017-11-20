//Author: luotuo44@gmail.com
//Use of this source code is governed by a BSD-style license

#ifndef EVENTLOOP_H
#define EVENTLOOP_H


#include<string>
#include<memory>

struct event_base;
struct bufferevent;


namespace Net
{


class TcpClient;

class EventLoop
{

public:
    EventLoop();
    EventLoop(const EventLoop&)=delete;
    EventLoop& operator = (const EventLoop &)=delete;
    //没有移动赋值，因为被赋值的m_base所带的bufferevent将无地自容
    EventLoop(EventLoop &&el);
    ~EventLoop();

public:
    bufferevent* newBufferevent();

    void run();

private:
    event_base* getEventBase() { return m_base; }


private:
    event_base *m_base = nullptr;
};

}

#endif // EVENTLOOP_H

