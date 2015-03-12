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
#include<iostream>
#include"SocketOps.hpp"


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
    struct connIPPort c;
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
        int ret = m_reactor.dispatch();
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



void ConnectPort::update(socket_t fd, int events)
{
    //由文件描述符找到对应的connIPPort结构体
    auto it = std::find_if(m_conns.begin(), m_conns.end(),
                           [fd](struct connIPPort &c)->bool{
                               return fd == c.fd;
                            });

    bool del_conn = driveMachine(*it);

    if(del_conn)
    {
        m_conns.erase(it);
    }
}


void ConnectPort::newResult(const std::string &domain, int port, std::vector<std::string> &ips)
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


bool ConnectPort::tryNewConnect(connIPPort &c)
{
    assert(c.fd == -1);

    while( c.curr_ip < c.ips.second.size() )
    {
        int ret = SocketOps::tcp_connect_server(c.ips.second[c.curr_ip].c_str(),
                c.port, &c.fd);

        if( ret == -1 )
        {
            perror("ConnectPort::addNewIPPort fail to connect domain ");
            ++c.curr_ip;
            //cannot transmit to connect_fail, because c.fd will not trigger any event
            continue;
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

        break;
    }

    return c.curr_ip < c.ips.second.size();
}


void ConnectPort::addNewIPPort()
{
    char buff[4];
    int ret = SocketOps::readn(m_fd[0], buff, 4);
    assert(ret == 4);

    int port;
    memcpy(&port, buff, 4);


    struct connIPPort c;
    c.curr_ip = 0;
    c.port = port;
    c.ips = m_ip_queue.getIP();
    c.fd = -1;


    if( tryNewConnect(c) )
        m_conns.push_back(c);
}


bool ConnectPort::driveMachine(struct connIPPort &c)
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
            ++c.curr_ip;
            if( !tryNewConnect(c) )//has tried all ips in c
                c.state = stop_conn_domain;
            else //has changed state in tryNewConnect            {
                stop = true;

            break;

        case connecting_443:
            ret = SocketOps::connecting_server(c.fd);
            if( ret == 1 )//need to try again
                stop = true;
            else if(ret == -1 )//some error to this socket
            {
                fprintf(stderr, "fail to connect domain %s with ip %s:%d, error is %s\n",
                        c.ips.first.c_str(), c.ips.second[c.curr_ip].c_str(),
                        c.port, strerror(errno));

                c.state = connect_fail;
            }
            else if(ret == 0 )//connect success
            {
                c.state = connect_success;
                m_reactor.updateEvent(c.fd, EV_WRITE);
            }
            break;

        case connect_fail:
            SocketOps::close_socket(c.fd);
            c.fd = -1;
            c.state = new_connect;
            break;

        case connect_success:
            c.state = write_result;
            break;

        case write_result:
            //write to file
            if( m_writer )
                m_writer(c.ips.first.c_str(), c.ips.second[c.curr_ip].c_str(), c.port);

            m_reactor.delFd(c.fd);
            SocketOps::close_socket(c.fd);
            c.fd = -1;
            c.state = new_connect;

            break;

        case stop_conn_domain:
            del_conn = true;

            stop = true;
            //fprintf(stdout, "stop conn domain %s \n", c.ips.first.c_str());
            break;

        default:
            fprintf(stderr, "unexpect case\n");
            break;
        }
    }

    return del_conn;
}
