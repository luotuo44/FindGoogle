//Filename: main.cpp
//Date: 2015-2-20

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#include<iostream>
#include<stdio.h>
#include<pthread.h>
#include<memory>
#include<string>
#include<vector>
#include<map>
#include<set>
#include<assert.h>
#include<sys/resource.h>

#include"Reactor.hpp"
#include"DnsSeacher.hpp"
#include"DomainExplorer.hpp"
#include"Thread.hpp"

using namespace std;

void saveIPResult(const char *domain, const char *ip, int port, int success_times)
{
    fprintf(stdout, "  ### can connect %s---%s:%d, with %d%% success\n", domain, ip, port, 25*success_times);
}


typedef std::set<std::string> StringSet; //dns server
typedef std::map<std::string, int> StringMap;//domain, port

void parseDNSServer(const std::string &line, StringSet &dns_server)
{
    auto pos = line.find(':');

    do
    {
        auto last = pos + 1;
        pos = line.find(';', last);
        dns_server.insert(line.substr(last, pos-last));
    }while(pos != string::npos);
}


void parseDomain(const std::string &line, StringMap &domain)
{
    auto pos = line.find(':');
    int port = 443;

    do
    {
        port = 443;
        auto last = pos + 1;
        pos = line.find(';', last);
        auto pos2 = line.find(':', last);

        if( pos2 < pos ) //this domain specify port
        {
            port = atoi(line.substr(pos2+1, pos-(pos2+1)).c_str());
        }
        else
            pos2 = pos;

        domain[line.substr(last, pos2-last)] = port;

    }while( pos != string::npos );

}

void readQueryList(StringSet &dns_server, StringMap &domain)
{
    char buff[4096];
    FILE *finput = fopen("dns_query.txt", "r");
    if( finput == NULL )
    {
        fprintf(stderr, "can't open dns_query.txt to read\n");
        return ;
    }

    while( fgets(buff, sizeof(buff), finput) != NULL )
    {
        std::string line(buff);

        //"\r\n"
        while( line.back() == 10 || line.back() == 13)
        {
            line.pop_back();
        }

        if( line.find("dns_server:") == 0)
            parseDNSServer(line, dns_server);
        else if(line.find("domain:") == 0)
            parseDomain(line, domain);
    }

    fclose(finput);
}



void threadFun(void *arg)
{
    DnsSeacher *dns_seacher = reinterpret_cast<DnsSeacher*>(arg);
    assert( dns_seacher != nullptr);

    StringSet dns_server;
    StringMap domain;
    readQueryList(dns_server, domain);

    cout<<"dns_server : ";
    for(auto& e : dns_server)
        cout<<e<<'\t';
    cout<<endl;

    cout<<"domain : ";
    for(auto& e : domain)
        cout<<e.first<<":"<<e.second<<endl;
    cout<<endl;


    for(auto& e : dns_server)
    {
        for(auto& ee : domain)
        {
            dns_seacher->addQuery(ee.first, ee.second, e);
        }
    }

    dns_seacher->run();
}


int main()
{
    DnsSeacher dns_seacher;
    DomainExplorerPtr domain_explorer = std::make_shared<DomainExplorer>();

    dns_seacher.setObserver(domain_explorer);

    domain_explorer->setResultCB(saveIPResult);

    Thread thread(threadFun);
    thread.start(&dns_seacher);

    domain_explorer->run();

    return 0;
}
