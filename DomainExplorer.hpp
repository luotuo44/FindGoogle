//Filename: DomainExplorer.hpp
//Date: 2015-6-28

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef DOMAINEXPLORER_HPP
#define DOMAINEXPLORER_HPP

#include<string>
#include<vector>
#include<map>
#include<memory>
#include<functional>
#include<utility>
#include<list>

#include"MutexLock.hpp"

namespace Net
{
class Reactor;
typedef std::shared_ptr<Reactor> ReactorPtr;
class Event;
typedef std::shared_ptr<Event> EventPtr;
}

typedef std::function<void (const char *domain, const char *ip, int port, int success_times)> ResultCB;


class DomainExplorer
{
public:
    DomainExplorer();
    ~DomainExplorer();

    DomainExplorer(const DomainExplorer&)=delete;
    DomainExplorer& operator = (const DomainExplorer&)=delete;

    void run();
    void newDnsResult(std::string &&domain, int port, std::vector<std::string> &&ips);

    void eventCB(int fd, int events, void *arg);
    void pipeEventCB(int fd, int events, void *arg);
    void setResultCB(ResultCB cb) { m_result_cb = cb; }

private:
    class DomainConn;
    typedef std::shared_ptr<DomainConn> DomainConnPtr;


    typedef std::vector< std::string > StringVec;
    typedef std::pair<std::string, StringVec> DomainIPS;

private:
    void updateEvent(DomainConnPtr &d, int new_events, int millisecond=0);
    bool driveMachine(DomainConnPtr &d);
    bool tryNewConnect(DomainConnPtr &d);

private:
    bool m_dns_query_is_stop;
    int m_fd[2]; //pipe
    Net::EventPtr m_pipe_ev;
    ResultCB m_result_cb;

    Net::ReactorPtr m_reactor;
    std::map<int, DomainConnPtr> m_conns;

    Mutex m_mutex;
    std::list<DomainIPS> m_new_domain_ips;

};

#endif // DOMAINEXPLORER_HPP
