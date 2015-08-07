//Filename: DnsSeacher.cpp
//Date: 2015-6-28

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#include"DnsSeacher.hpp"

#include<string.h>
#include<stdio.h>

#include<iostream>
#include<vector>
#include<functional>

#include"DNS.hpp"
#include"Reactor.hpp"
#include"DomainExplorer.hpp"
#include"SocketOps.hpp"

enum class DnsState
{
    init,
    connecting_dns, //conn init state
    new_try, //one round start state
    send_query,
    recv_query_result,
    parse_query_result,
    stop_query_dns
};

class DnsSeacher::DnsQuery
{
public:
    DnsQuery()
        : fd(-1), state(DnsState::init),
          r_curr(0), w_curr(0)
    {}

    int fd;
    Net::EventPtr ev;
    DnsState state;
    std::string domain;
    int port;
    std::string dns_server;//ip

    std::vector<std::string> ips;//dns query result

    std::vector<unsigned char> r_buff;
    int r_curr;
    std::vector<unsigned char> w_buff;
    int w_curr;
};



DnsSeacher::DnsSeacher()
    : m_reactor(Net::Reactor::newReactor())
{

}


void DnsSeacher::setObserver(const DomainExplorerPtr &observer)
{
    m_observer = observer;
}



void DnsSeacher::addQuery(const std::string &domain, int port, const std::string &dns_server)
{
    DnsQueryPtr q = std::make_shared<DnsQuery>();

    int ret = Net::SocketOps::new_tcp_socket_connect_server(dns_server.c_str(), 53, &q->fd);
    if( ret == -1 )
    {
        char buf[50];
        snprintf(buf, sizeof(buf), "%m");
        std::cout<<"new_tcp_socket_connect_server fail "<<buf<<std::endl;
        return ;
    }

    updateEvent(q, EV_WRITE|EV_PERSIST);
    if( ret == 0)//connect success
        q->state = DnsState::new_try;
    else
        q->state = DnsState::connecting_dns;

    q->domain = domain;
    q->dns_server = dns_server;
    q->port = port;

    Net::Reactor::addEvent(q->ev);

    m_querys[q->fd] = q;
}


void DnsSeacher::updateEvent(DnsQueryPtr &query, int new_events, int milliseconds)
{
    Net::Reactor::delEvent(query->ev);
    query->ev = m_reactor->createEvent(query->fd, new_events, std::bind(&DnsSeacher::eventCB, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), nullptr);
    Net::Reactor::addEvent(query->ev, milliseconds);
}


void DnsSeacher::eventCB(int fd, int events, void *arg)
{
    auto it = m_querys.find(fd);
    if( it == m_querys.end() )
    {
        std::cout<<"event update a fd that does exist"<<std::endl;
        return ;
    }

    if( events & EV_TIMEOUT )
    {
        std::cout<<"timeout to wait for dns_server "<<it->second->dns_server<<std::endl;
        Net::Reactor::delEvent(it->second->ev);
        Net::SocketOps::close_socket(it->second->fd);
        m_querys.erase(it);
        return ;
    }

    bool need_del = driveMachine(it->second);
    if( need_del )
    {
        Net::Reactor::delEvent(it->second->ev);
        Net::SocketOps::close_socket(it->second->fd);
        m_querys.erase(it);
    }

    (void)events;(void)arg;
}


void DnsSeacher::run()
{
    int ret = m_reactor->dispatch();
    if( ret == -1)
    {
        char buf[50];
        snprintf(buf, sizeof(buf), "%m");
        std::cout<<"Reactor::dispatch fail "<<buf<<std::endl;
    }
    else
    {
        DomainExplorerPtr explorer = m_observer.lock();
        std::cout<<"finish dns query"<<std::endl;
        //tell DomainExplorer finished dns query
        explorer->newDnsResult("", -1, std::vector< std::string>());
    }
}



bool DnsSeacher::driveMachine(DnsQueryPtr &q)
{
    bool stop = false;
    int ret;
    uint16_t left;
    bool del_query = false;

    while( !stop )
    {
        switch(q->state)
        {
        case DnsState::connecting_dns:
            ret = Net::SocketOps::connecting_server(q->fd);
            if( ret == 1 )//need to try again
                stop = true;
            else if(ret == -1 )//some error to this socket
            {
                char buf[200];
                snprintf(buf, sizeof(buf), "fail to connect dns %s for query %s, error is %m", q->dns_server.c_str(), q->domain.c_str());
                std::cout<<buf<<std::endl;
                q->state = DnsState::stop_query_dns;
            }
            else if(ret == 0 )//connect success
            {
                q->state = DnsState::new_try;
            }

            break;

        case DnsState::new_try:
            updateEvent(q, EV_WRITE|EV_PERSIST);
            getDNSQueryPacket(q);

            q->state = DnsState::send_query;
            break;

        case DnsState::send_query:
            left = q->w_buff.size() - q->w_curr;
            ret = Net::SocketOps::write(q->fd, reinterpret_cast<char*>(&q->w_buff[q->w_curr]), left);

            if( ret < 0 )//socket error
            {
                q->state = DnsState::stop_query_dns;
                break;
            }

            //else if( ret == 0)//send 0 bytes
            //   stop = true;

            q->w_curr += ret;
            if( ret == left)//has send all data
            {
                q->state = DnsState::recv_query_result;
                updateEvent(q, EV_READ|EV_PERSIST|EV_TIMEOUT, 10000);//10 seconds

                //init
                q->r_buff.resize(2);
                q->r_curr = 0;
            }
            stop = true;

            break;

        case DnsState::recv_query_result:
            //std::cout<<"recv_query_result"<<std::endl;
            left = q->r_buff.size() - q->r_curr;
            ret = Net::SocketOps::read(q->fd, reinterpret_cast<char*>(&q->r_buff[q->r_curr]), left);

            if( ret < 0 )//socket error
            {
                q->state = DnsState::stop_query_dns;
                break;
            }
            else if( ret == 0)
                stop = true;

            q->r_curr += ret;
            if( ret != left )
                stop = true;
            else //has read all data
            {
                if(q->r_buff.size() == 2)//read the length information
                {
                    ::memcpy(&left, &q->r_buff[0], 2);
                    left = Net::SocketOps::ntohs(left);
                    q->r_buff.resize(2+left);
                    q->r_curr = 2;
                }
                else //read dns result information
                {
                    q->state = DnsState::parse_query_result;
                }
            }

            break;

        case DnsState::parse_query_result:
        {
            //RVO
            std::vector<std::string> str_vec = DNS::parseDNSResultPacket(&q->r_buff[2], q->r_buff.size()-2);
            if( str_vec.size() == 0 )//error result
            {
                std::cout<<q->dns_server<<"has no answer for "<<q->domain<<std::endl;
            }
            else
            {
                std::cout<<q->dns_server<<" said " <<"domain "<<q->domain<<" has follow answers"<<std::endl;
                for(auto e : str_vec)
                    std::cout<<e<<std::endl;
                std::cout<<std::endl;

                //将结果转移给另外一个专门connnect 443端口的线程
                DomainExplorerPtr explorer = m_observer.lock();
                if( explorer )
                {
                    explorer->newDnsResult(std::move(q->domain), q->port, std::move(str_vec));
                }
            }

            q->state = DnsState::stop_query_dns;//这个conn的任务完成了
            break;
        }


        case DnsState::stop_query_dns:
            del_query = true;
            stop = true;
            break;

        default:
            std::cout<<"unexpect case"<<std::endl;
            q->state = DnsState::stop_query_dns;
            break;
        }
    }

    return del_query;
}


void DnsSeacher::getDNSQueryPacket(DnsQueryPtr &query)
{
    std::string query_packet = DNS::getDNSPacket(query->domain);

    query->w_buff.resize(query_packet.size() + 2);

    uint16_t t = Net::SocketOps::htons(query_packet.size());
    ::memcpy(&query->w_buff[0], &t, 2);
    ::memcpy(&query->w_buff[2], query_packet.c_str(), query_packet.size());
    query->w_curr = 0;
}
