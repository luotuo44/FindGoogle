//Filename: Reactor.hpp
//Date: 2015-2-20

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef REACTOR_HPP
#define REACTOR_HPP

#include"Uncopyable.hpp"
#include<vector>


#include"typedefine.hpp"


struct pollfd;

class Observer;

//use poll as the IO multiplexing
class Reactor : public Utility::Uncopyable
{
public:
    Reactor();
    ~Reactor();

    void setObserver(Observer *observer);

    bool addFd(socket_t sockfd, int events);
    void delFd(socket_t sockfd);
    void updateEvent(socket_t sockfd, int new_events);

    int fdNum()const;

    int dispatch(int timeout = -1);

private:
    void expand(socket_t sockfd);

private:
    //vector的大小不是fd的个数，而是fd最大值+1(类似select的第一个参数)
    std::vector<struct pollfd> m_event_set;
    Observer *m_observer;
    int m_fd_num;
};



#endif // REACTOR_HPP
