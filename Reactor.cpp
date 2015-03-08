//Filename: Reactor.cpp
//Date: 2015-2-20

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#include"Reactor.hpp"
#include"conn.hpp"

#include<poll.h> //poll
#include<stdlib.h> //realloc
#include<assert.h>
#include<errno.h>


#include"DNS_Machine.hpp"
#include"typedefine.hpp"

Reactor::Reactor()
    : m_observer(NULL),
      m_fd_num(0)
{

}


Reactor::~Reactor()
{
    m_observer = NULL;
}

void Reactor::setObserver(Observer *observer)
{
    m_observer = observer;
}


int Reactor::fdNum()const
{
    return m_event_set.size();
}


bool Reactor::addFd(socket_t sockfd, int events)
{
    assert(sockfd >= 0);

    if( sockfd >= m_event_set.capacity() )
    {
        int temp_size;
        if(m_event_set.size() < 32)
            temp_size = 32;
        else
            temp_size = 2*sockfd;

        m_event_set.reserve(temp_size);
    }

    if( sockfd >= m_event_set.size() )//new fd
    {
        int old_size = m_event_set.size();
        //不应该使用push_back, 因为m_event_set的大小不是fd的个数，而是fd最大值+1
        //而前后加入的fd值可能会出现跳跃
        m_event_set.resize(sockfd+1);

        //all fd that doesn't used are set -1 to identify
        while( old_size < m_event_set.size() )
            m_event_set[old_size++].fd = -1;
    }

    m_event_set[sockfd].fd = sockfd;
    m_event_set[sockfd].revents = 0;
    m_event_set[sockfd].events = 0;

    if( events & EV_WRITE )
        m_event_set[sockfd].events |= POLLOUT;
    if( events & EV_READ )
        m_event_set[sockfd].events |= POLLIN;

    ++m_fd_num;
    return true;
}





void Reactor::delFd(socket_t sockfd)
{
    assert(sockfd >= 0 && sockfd < m_event_set.size() );

    if( m_event_set[sockfd].fd != -1)
        --m_fd_num;

    m_event_set[sockfd].fd = -1;
}



void Reactor::updateEvent(socket_t sockfd, int new_events)
{
    //this sockfd number isn't used
    if( m_event_set[sockfd].fd == -1 )
        return ;

    m_event_set[sockfd].events = 0;

    if( new_events & EV_WRITE )
        m_event_set[sockfd].events |= POLLOUT;
    if( new_events & EV_READ )
        m_event_set[sockfd].events |= POLLIN;

}



//timetout 的默认值是-1, DNS_Machine使用默认值。ConnectPort将设置一个超时时长
int Reactor::dispatch(int timeout)
{
    int ret = 1;
    int i, what;

begin:
    ret = poll(&m_event_set[0], m_event_set.size(), timeout);

    if( ret == 0 )//timeout
    {
        m_observer->update(-1, 0);
        return 1;
    }

    if( ret == -1 )
    {
        if( errno == EINTR)
            goto begin;
        else
            return -1;
    }

    //Observer modern
    if( m_observer != NULL)
    {
        for(i = 0; i < m_event_set.size(); ++i)
        {
            what = m_event_set[i].revents;

            if( !what || (m_event_set[i].fd<0) )
                continue;

            if (what & (POLLHUP|POLLERR))
                what |= POLLIN|POLLOUT;

            int events = 0;
            if (what & POLLIN)
                events |= EV_READ;
            if (what & POLLOUT)
                events |= EV_WRITE;
            if( events == 0 )
                continue;

            m_observer->update(m_event_set[i].fd, events);
        }
    }

    if( m_fd_num == 0)
        ret = 0;


    return ret;
}
