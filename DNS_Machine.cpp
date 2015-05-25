//Filename: DNS_Machine.cpp
//Date: 2015-2-20

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#include"DNS_Machine.hpp"

#include<stdio.h>
#include<string.h>
#include<algorithm>
#include<assert.h>

#include"typedefine.hpp"
#include"Reactor.hpp"

#include"SocketOps.hpp"
#include"DNS.hpp"
#include"ConnectPort.hpp"

#include<iostream>

typedef std::vector< std::string > StringVec;
typedef std::pair<std::string, StringVec> DomainIP;


void DNS_Machine::addConn(const std::string &domain, int port,
                          const std::string &dns_server)
{
    struct connDNS c;
    int ret = SocketOps::tcp_connect_server(dns_server.c_str(), 53,
                                            &c.fd);

    if( ret == -1 )
    {
        perror("tcp_connect_server fail to connect dns");
        return ;
    }
    else if( ret == 0 )//connect success
    {
        c.state = new_try;
        m_reactor.addFd(c.fd, EV_WRITE);
    }
    else
    {
        c.state = connecting_dns;
        m_reactor.addFd(c.fd, EV_WRITE);
    }

    c.port = port;
    c.domain = domain;
    c.dns_server = dns_server;
    m_conns.push_back(c);
}




void DNS_Machine::start()
{
    while(1)
    {
        int ret = m_reactor.dispatch();
        if( ret == -1 )
        {
            perror("poll fail ");
            break;
        }


        if( m_conns.size() == 0 )
        {
            //fprintf(stderr, "finished all dns query\n");
            break;
        }
    }

    if( m_observer)//tell ConnectPort finished dns query
    {
        std::vector< std::string> str_vec;
        m_observer->newDNSResult("", -1, str_vec);
    }
}


void DNS_Machine::init()
{
    m_reactor.setObserver(this);
}



void DNS_Machine::setObserver(std::shared_ptr<ConnectPort> &observer)
{
    m_observer = observer;
}


void DNS_Machine::update(socket_t fd, int events)
{
    //由文件描述符找到对应的connDNS结构体
    auto it = std::find_if(m_conns.begin(), m_conns.end(),
                           [fd](struct connDNS &c)->bool{
                               return fd == c.fd;
                            });


    bool del_conn = driveMachine(*it);

    if(del_conn)//need to delete this connDNS struct
    {
        m_conns.erase(it);
    }

    (void)events;
}

typedef std::vector<std::string> StringVec;

bool DNS_Machine::driveMachine(struct connDNS &c)
{
    bool stop = false;
    int ret;
    uint16_t left;
    bool del_conn = false;


    while( !stop )
    {
        switch(c.state)
        {
        case connecting_dns:

            ret = SocketOps::connecting_server(c.fd);
            if( ret == 1 )//need to try again
                stop = true;
            else if(ret == -1 )//some error to this socket
            {
                fprintf(stderr, " fail to connect dns %s for query %s, error is %s\n",
                        c.dns_server.c_str(), c.domain.c_str(), strerror(errno));
                c.state = stop_conn_dns;
            }
            else if(ret == 0 )//connect success
            {
                c.state = new_try;
                m_reactor.updateEvent(c.fd, EV_WRITE);
            }

            break;

        case new_try:

            m_reactor.updateEvent(c.fd, EV_WRITE);
            getDNSQueryPacket(c);

            c.state = send_query;
            break;

        case send_query:

            left = c.w_buff.size() - c.w_curr;
            ret = SocketOps::write(c.fd, &c.w_buff[c.w_curr], left);


            if( ret < 0 )//socket error
            {
                c.state = stop_conn_dns;
                break;
            }

            //            else if( ret == 0)//send 0 bytes
            //                stop = true;

            c.w_curr += ret;
            if( ret == left)//has send all data
            {
                c.state = recv_query_result;
                m_reactor.updateEvent(c.fd, EV_READ);

                //init
                c.r_buff.resize(2);
                c.r_curr = 0;
            }
            stop = true;

            break;

        case recv_query_result:

            left = c.r_buff.size() - c.r_curr;
            ret = SocketOps::read(c.fd, &c.r_buff[c.r_curr], left);

            if( ret < 0 )//socket error
            {
                c.state = stop_conn_dns;
                break;
            }
            else if( ret == 0)
                stop = true;

            c.r_curr += ret;
            if( ret != left )
                stop = true;
            else //has read all data
            {
                if(c.r_buff.size() == 2)//read the length information
                {
                    memcpy(&left, &c.r_buff[0], 2);
                    left = SocketOps::ntohs(left);
                    c.r_buff.resize(2+left);
                    c.r_curr = 2;
                }
                else //read dns result information
                {
                    c.state = parse_query_result;
                }
            }

            break;

        case parse_query_result:
        {
            //RVO
            StringVec str_vec = DNS::parseDNSResultPacket(&c.r_buff[2],
                                                c.r_buff.size()-2);
            if( str_vec.size() == 0 )//error result
            {
                std::cout<<c.dns_server<<" has no answer for "<<c.domain<<std::endl;
            }
            else
            {
                std::cout<<c.dns_server<<" said "
                            <<"domain "<<c.domain<<" has follow answers\n";
                for(auto e : str_vec)
                    std::cout<<e<<std::endl;
                std::cout<<std::endl;

                //将结果转移给另外一个专门connnect 443端口的线程
                if(m_observer)
                {
                    m_observer->newDNSResult(c.domain, c.port, str_vec);
                }
            }

            c.state = stop_conn_dns;//这个conn的任务完成了
            break;
        }


        case stop_conn_dns:
            m_reactor.delFd(c.fd);
            SocketOps::close_socket(c.fd);
            del_conn = true;
            c.fd = -1;

            stop = true;
            break;

        default:
            fprintf(stderr, "unexpect case\n");
            c.state = stop_conn_dns;
            break;
        }
    }

    return del_conn;
}


void DNS_Machine::getDNSQueryPacket(struct connDNS &c)
{

    std::string query_packet = DNS::getDNSPacket(c.domain);

    c.w_buff.resize(query_packet.size() + 2);

    uint16_t t = SocketOps::htons(query_packet.size());
    memcpy(&c.w_buff[0], &t, 2);
    memcpy(&c.w_buff[2], query_packet.c_str(), query_packet.size());
    c.w_curr = 0;
}
