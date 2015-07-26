//Filename: DomainExplorer.cpp
//Date: 2015-6-28

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#include"DomainExplorer.hpp"

#include<unistd.h>

#include<stdlib.h>
#include<string.h>
#include<assert.h>

#include<iostream>
#include<system_error>

#include"SocketOps.hpp"
#include"Reactor.hpp"
#include"Logger.hpp"


enum class DomainState
{
    init,
    connecting_443,//can test other port
    connect_success,
    connect_fail,
    stop_conn_domain
};

class DomainExplorer::DomainConn
{
public:
    DomainConn()
        : fd(-1), try_times(0), success_times(0), state(DomainState::init)
    {}

    int fd;
    int port;
    int try_times;
    int success_times;

    Net::EventPtr ev;
    std::string domain;
    std::string ip;

    DomainState state;
};


DomainExplorer::DomainExplorer()
    :  m_result_cb(nullptr),
      m_reactor(Net::Reactor::newReactor())
{
    if( !Net::SocketOps::new_pipe(m_fd, false, false) )
        throw std::system_error(errno,  std::system_category(), "fail to create pipe: ");

    m_pipe_ev = m_reactor->createEvent(m_fd[0], EV_READ|EV_PERSIST, std::bind(&DomainExplorer::pipeEventCB, this, m_fd[0], EV_READ, nullptr), nullptr);
    Net::Reactor::addEvent(m_pipe_ev);
}


DomainExplorer::~DomainExplorer()
{
    Net::SocketOps::close_socket(m_fd[0]);
    Net::SocketOps::close_socket(m_fd[1]);
}



void DomainExplorer::newDnsResult(std::string &&domain, int port, std::vector<std::string> &&ips)
{
    //if port == -1, means that dnsseacher has finished dns query
    if( port == -1)
    {
        Net::Reactor::delEvent(m_pipe_ev);
        return ;
    }
    else
    {
        MutexLock lock(m_mutex);
        m_new_domain_ips.push_back({std::move(domain), std::move(ips)});
    }

    char buff[4];
    ::memcpy(buff, &port, 4);
    int ret = Net::SocketOps::writen(m_fd[1], buff, 4);
    assert(ret == 4);
}


void DomainExplorer::pipeEventCB(int fd, int events, void *arg)
{
    char buff[4];
    int ret = Net::SocketOps::readn(m_fd[0], buff, 4);
    assert(ret == 4);

    int port;
    ::memcpy(&port, buff, 4);

    DomainIPS ips;
    {
        MutexLock lock(m_mutex);
        assert( !m_new_domain_ips.empty() );
        auto it = m_new_domain_ips.begin();
        ips.first = std::move(it->first);
        ips.second = std::move(it->second);

        m_new_domain_ips.pop_front();
    }

    for(auto& e : ips.second)
    {
        DomainConnPtr d = std::make_shared<DomainConn>();
        d->port = port;
        d->state = DomainState::init;
        d->domain = ips.first;
        d->ip = std::move(e);

        if( tryNewConnect(d) )
            m_conns.insert({d->fd, d});
    }

    (void)fd; (void)events; (void)arg;
}



bool DomainExplorer::tryNewConnect(DomainConnPtr &d)
{
    assert(d->fd = -1);

    ++d->try_times;

    //最多只尝试4次
    if( d->try_times >= 4 )
        return false;

    int ret = Net::SocketOps::new_tcp_socket_connect_server(d->ip.c_str(), d->port, &d->fd);
    if( ret == -1 )
    {
        char buf[50];
        snprintf(buf, sizeof(buf), "%m");
        LOG(Log::ERROR)<<"new_tcp_socket_connect_server fail "<<buf;
        return false;
        //cannot transmit to connect_fail, because c.fd will not trigger any event
    }

    updateEvent(d, EV_WRITE|EV_PERSIST);
    if( ret == 0 )//connect success
        d->state = DomainState::connect_success;
    else
        d->state = DomainState::connecting_443;

    Net::Reactor::addEvent(d->ev);

    return true;
}


void DomainExplorer::updateEvent(DomainConnPtr &d, int new_events, int millisecond)
{
    Net::Reactor::delEvent(d->ev);
    d->ev = m_reactor->createEvent(d->fd, new_events, std::bind(&DomainExplorer::eventCB, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),  nullptr);
    Net::Reactor::addEvent(d->ev, millisecond);
}


void DomainExplorer::eventCB(int fd, int events, void *arg)
{
    auto it = m_conns.find(fd);
    if( it == m_conns.end() )
    {
        LOG(Log::ERROR)<<"event update a fd that does exist";
        return ;
    }

    bool need_del = driveMachine(it->second);
    if( need_del )
    {
        Net::Reactor::delEvent(it->second->ev);
        Net::SocketOps::close_socket(it->second->fd);
        DomainConnPtr d = it->second;
        m_conns.erase(it);

        d->fd = -1;
        if( d->try_times < 4 && tryNewConnect(d))//这个conn已经测试完毕，尝试下一次
        {
            m_conns.insert({d->fd, d});
        }
        else if( m_result_cb ) 
        {
            m_result_cb(d->domain.c_str(), d->ip.c_str(), d->port, d->success_times+1);
        }
    }

    (void)events;(void)arg;
}


void DomainExplorer::run()
{
    int ret = m_reactor->dispatch();
    if(ret == -1)
    {
        char buf[50];
        snprintf(buf, sizeof(buf), "%m");
        LOG(Log::ERROR)<<"Reactor::dispatch fail";
    }

    std::cout<<"finish domain connection try"<<std::endl;
}


bool DomainExplorer::driveMachine(DomainConnPtr &d)
{
    bool stop = false;
    int ret;
    bool del_conn = false;

    while( !stop )
    {
        switch(d->state)
        {
        case DomainState::connecting_443:
            ret = Net::SocketOps::connecting_server(d->fd);
            if( ret == 1 )//need to try again
                stop = true;
            else if(ret == -1 )//some error to this socket
                d->state = DomainState::connect_fail;
            else if(ret == 0 )//connect success
                d->state = DomainState::connect_success;
            break;


        case DomainState::connect_success:
            ++d->success_times;
            //fall

        case DomainState::connect_fail:
            d->state = DomainState::stop_conn_domain;

            del_conn = true;
            stop = true;
            break;

        default:
            LOG(Log::ERROR)<<"unexpect case";
            break;
        }
    }

    return del_conn;
}


