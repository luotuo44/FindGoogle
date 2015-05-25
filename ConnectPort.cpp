//Filename: ConnectPort.cpp
//Date: 2015-2-20

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#include"ConnectPort.hpp"
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<stdio.h>
#include<algorithm>
#include<assert.h>
#include"SocketOps.hpp"


typedef std::vector< std::string > StringVec;
typedef std::pair<std::string, StringVec> DomainIP;


ConnectPort::ConnectPort(writer_fun fun)
    : m_dns_query_is_stop(false),
      m_writer(fun)
{
    int ret = pipe(m_fd);
    if( ret == -1)
    {
        perror("pipe fail ");
        exit(1);
    }

    m_reactor.addFd(m_fd[0], EV_READ);

    struct connDomain c;
    c.fd = m_fd[0];
    c.state = new_ip;
    m_conns.push_back(c);
}


ConnectPort::~ConnectPort()
{
    close(m_fd[0]);
    close(m_fd[1]);
}


void ConnectPort::init()
{
    m_reactor.setObserver(this);
}


void ConnectPort::start()
{
    while(1)
    {
        int ret = m_reactor.dispatch(500);//timeout
        if( ret == -1 )
        {
            perror("poll fail ");
            break;
        }

        if( m_conns.size() == 1 && m_dns_query_is_stop )
        {
            fprintf(stderr, "finished all conn to domain\n");
            break;
        }
    }
}



//fd的值可能为-1,表示是超时触发。此时要扫描m_conns队列里面的所有connDomain节点。如果connDomain
//的fd成员等于-1,就说明需要这个connDomain需要尝试运行。不过这和正常的connDomain都统一调用driveMachine执行
void ConnectPort::update(socket_t fd, int events)
{
    auto start = m_conns.begin();

    while( 1 )
    {
        auto it = std::find_if(start, m_conns.end(),
                               [fd](struct connDomain &c)->bool{
                                    return c.fd == fd;
                                });

        if( it == m_conns.end() )
            break;

        bool del_conn = driveMachine(*it);
        if( del_conn )
            start = m_conns.erase(it);
        else
            start = ++it;
    }

    (void)events;
}


void ConnectPort::newDNSResult(const std::string &domain, int port, std::vector<std::string> &ips)
{
    if( port == -1 )//finish dns query
    {
        m_dns_query_is_stop = true;
        return ;
    }

    m_ip_queue.addIP(domain, ips);

    char buff[4];
    memcpy(buff, &port, 4);
    int ret = SocketOps::writen(m_fd[1], buff, 4);
    assert(ret == 4);
}



bool ConnectPort::tryNewConnect(struct connDomain &c)
{
    assert(c.fd = -1);

    ++c.try_times;


    //最多只尝试4次
    if( c.try_times >= 4 )
        return true;


    int ret = SocketOps::tcp_connect_server(c.ip.c_str(), c.port, &c.fd);
    if( ret == -1 )
    {
        if( errno == EMFILE )//太多文件描述符了,等待下一次的尝试
            --c.try_times;//不计算这次连接
        else
            perror("ConnectPort::tryNewConnect fail to connect domain ");
        //cannot transmit to connect_fail, because c.fd will not trigger any event
    }
    else if( ret == 0 )//connect success
    {
        c.state = connect_success;
        m_reactor.addFd(c.fd, EV_WRITE);
    }
    else
    {
        c.state = connecting_443;
        m_reactor.addFd(c.fd, EV_WRITE);
    }

    return false;
}


void ConnectPort::addNewIPPort()
{
    char buff[4];
    int ret = SocketOps::readn(m_fd[0], buff, 4);
    assert(ret == 4);

    int port;
    memcpy(&port, buff, 4);


    struct connDomain c;
    c.fd = -1;
    c.try_times = 0;
    c.success_times = 0;
    c.port = port;
    c.state = new_connect;

    DomainIP ips = m_ip_queue.getIP();
    c.domain = ips.first;

    for(auto e : ips.second )
    {
        c.ip = e;
        m_conns.push_back(c);
    }
}


bool ConnectPort::driveMachine(struct connDomain &c)
{
    bool stop = false;
    int ret;
    bool del_conn = false;

    while( !stop )
    {
        switch(c.state)
        {
        case new_ip:
            addNewIPPort();
            stop = true;
            break;

        case new_connect:
            if( tryNewConnect(c) )
                c.state = stop_conn_domain;
            else
                stop = true;

            break;

        case connecting_443:
            ret = SocketOps::connecting_server(c.fd);
            if( ret == 1 )//need to try again
                stop = true;
            else if(ret == -1 )//some error to this socket
            {
//                fprintf(stderr, "fail to connect domain %s with ip %s:%d, error is %s\n",
//                        c.domain.c_str(), c.ip.c_str(), c.port, strerror(errno));

                c.state = connect_fail;
            }
            else if(ret == 0 )//connect success
            {
                c.state = connect_success;
            }
            break;


        case connect_success:
            ++c.success_times;
            //fall

        case connect_fail:
            m_reactor.delFd(c.fd);
            SocketOps::close_socket(c.fd);
            c.fd = -1;

            if( c.try_times >= 4)
            {
                c.state = stop_conn_domain;
            }
            else
            {
                c.state = new_connect;
                stop = true;
            }
            break;


        case stop_conn_domain:
            del_conn = true;
            if(c.success_times > 0 )
                c.state = write_result;
            else
                stop = true;

            //fprintf(stdout, "stop conn domain %s \n", c.ips.first.c_str());
            break;


        case write_result:
            //write to file
            if( m_writer )
                m_writer(c.domain.c_str(), c.ip.c_str(), c.port, c.success_times+1);

            stop = true;
            break;

        default:
            fprintf(stderr, "unexpect case\n");
            break;
        }
    }

    return del_conn;
}
