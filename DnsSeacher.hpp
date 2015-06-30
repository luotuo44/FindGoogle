//Filename: DnsSeacher.hpp
//Date: 2015-6-28

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef DNSSEACHER_HPP
#define DNSSEACHER_HPP

#include<list>
#include<map>
#include<memory>
#include<string>

namespace Net
{
class Reactor;
typedef std::shared_ptr<Reactor> ReactorPtr;
}


class DomainExplorer;
typedef std::weak_ptr<DomainExplorer> DomainExplorerWPtr;
typedef std::shared_ptr<DomainExplorer> DomainExplorerPtr;

class DnsSeacher
{
public:
    DnsSeacher();
    ~DnsSeacher()=default;

    DnsSeacher(const DnsSeacher& )=delete;
    DnsSeacher& operator = (const DnsSeacher& )=delete;
    DnsSeacher(DnsSeacher &&)=default;
    DnsSeacher& operator = (DnsSeacher &&)=default;

public:
    void addQuery(std::string domain, int port, std::string dns_server);
    void setObserver(const DomainExplorerPtr &observer);
    void run();
    void eventCB(int fd, int events, void *arg);

private:
    class DnsQuery;
    typedef std::shared_ptr<DnsQuery> DnsQueryPtr;

private:
    bool driveMachine(DnsQueryPtr &q);
    void getDNSQueryPacket(DnsQueryPtr &query);
    void updateEvent(DnsQueryPtr &query, int new_events, int milliseconds=-1);

private:
    std::map<int, DnsQueryPtr> m_querys;
    Net::ReactorPtr m_reactor;
    DomainExplorerWPtr m_observer;
};

#endif // DNSSEACHER_HPP
