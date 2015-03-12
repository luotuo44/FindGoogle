//Filename: ConnectPort.hpp
//Date: 2015-2-20

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef CONNECTPORT_HPP
#define CONNECTPORT_HPP

#include<string>
#include<list>

#include"typedefine.hpp"
#include"Observer.hpp"
#include"Reactor.hpp"
#include"IPQueue.hpp"
#include"conn.hpp"

class ConnectPort : public Observer
{
public:
    ConnectPort(writer_fun fun);
    ~ConnectPort();

    void init();
    void update(socket_t fd, int events);

    void start();

    void newResult(const std::string &domain, int port, std::vector<std::string> &ips);

    bool driveMachine(struct connIPPort &c);

private:
    void addNewIPPort();
    bool tryNewConnect(struct connIPPort &c);

private:
    bool m_dns_query_is_stop;
    Reactor m_reactor;
    IPQueue m_ip_queue;
    int m_fd[2]; //pipe
    std::list<struct connIPPort> m_conns;

    writer_fun m_writer;
};

#endif // CONNECTPORT_HPP
