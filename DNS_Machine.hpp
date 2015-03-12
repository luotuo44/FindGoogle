//Filename: DNS_Machine.hpp
//Date: 2015-2-20

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef DNS_MACHINE_HPP
#define DNS_MACHINE_HPP


#include<list>
#include<string>
#include<memory>

#include"Reactor.hpp"
#include"typedefine.hpp"
#include"Observer.hpp"

#include"conn.hpp"

class ConnectPort;

class DNS_Machine : public Observer
{
public:

    void addConn(const std::string &domain, int port, const std::string &dns_server);

    bool driveMachine(struct connDNS &c);

    void init();
    void update(socket_t fd, int events);

    void start();

    void setObserver(std::shared_ptr<ConnectPort> &observer);


private:
    void getDNSQueryPacket(struct connDNS &c);

private:
    std::list<struct connDNS> m_conns;
    //std::weak_ptr<Observer> m_reactor;
    Reactor m_reactor;

    std::shared_ptr<ConnectPort> m_observer;
};


#endif // DNS_MACHINE_HPP
