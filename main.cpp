//Filename: main.cpp
//Date: 2015-2-20

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#include <iostream>
#include<stdio.h>
#include<pthread.h>
#include<memory>
#include<string>
#include<vector>
#include<map>
#include<set>
#include<fstream>

#include"Reactor.hpp"
#include"DNS_Machine.hpp"
#include"ConnectPort.hpp"


using namespace std;

void saveIPResult(const char *domain, const char *ip, int port)
{
    fprintf(stdout, "  ### can connect %s---%s:%d\n", domain, ip, port);
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
        dns_server.insert(std::string(line, last, pos-last));
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
            port = atoi(std::string(line, pos2+1, pos-(pos2+1)).c_str());
        }
        else
            pos2 = pos;

        domain[std::string(line, last, pos2-last)] = port;

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



void* threadFun(void *arg)
{
    DNS_Machine *dns_machine = (DNS_Machine*)(arg);

    StringSet dns_server;
    StringMap domain;
    readQueryList(dns_server, domain);

    for(auto e : dns_server)
    {
        for(auto ee : domain)
        {
            dns_machine->addConn(ee.first, ee.second, e);
        }
    }

//    //dns_machine->addConn("www.baidu.com", 443, "223.5.5.5");
//    //dns_machine->addConn("www.sina.com", 80, "223.5.5.5");
//    dns_machine->addConn("www.google.com.hk", 443, "208.76.50.50");
//    //dns_machine->addConn("ns1.google.com", 53, "208.67.222.222");
//    dns_machine->addConn("www.google.com", 443, "208.76.50.50");
//    dns_machine->addConn("www.google.com.hk", 443, "216.239.32.10");
//    dns_machine->addConn("www.google.com", 443, "74.82.42.42");
//    dns_machine->addConn("www.google.com.hk", 443, "91.239.100.100");
//    dns_machine->addConn("www.google.com.hk", 443, "37.235.1.174");
//    dns_machine->addConn("www.google.com", 443, "37.235.1.174");
//    dns_machine->addConn("www.google.com", 443, "216.146.35.35");
//    //dns_machine->addConn("www.google.com", 443, "208.67.222.222");
//    //dns_machine->addConn("www.taobao.com", 80, "223.5.5.5");
    dns_machine->start();

    cout<<endl<<"finish dns query"<<endl<<endl;

    return NULL;
}


int main()
{
    StringSet dns_server;
    StringMap domain;
    readQueryList(dns_server, domain);

    cout<<"dns_server : ";
    for(auto e : dns_server)
        cout<<e<<'\t';
    cout<<endl;

    cout<<"domain : ";
    for(auto e : domain)
        cout<<e.first<<":"<<e.second<<endl;
    cout<<endl;


    DNS_Machine dns_machine;
    dns_machine.init();

    std::shared_ptr<ConnectPort> test_port(new ConnectPort(saveIPResult));
    test_port->init();
    dns_machine.setObserver(test_port);

    pthread_t thread;
    pthread_create(&thread, NULL, threadFun, &dns_machine);

    test_port->start();

    //read configure and call dns_machine.addConn

    cout << "Hello World!" << endl;
    return 0;
}
