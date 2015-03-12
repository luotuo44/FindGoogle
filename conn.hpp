//Filename: conn.hpp
//Date: 2015-2-20

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef CONN_HPP
#define CONN_HPP

#include<string>
#include<vector>
#include<list>
#include<utility>

#include"typedefine.hpp"


struct conn
{
    conn(){}
    ~conn(){}

    socket_t fd;

    DNS_STATE state;


    std::vector<unsigned char> r_buff;
    int r_curr;


    std::vector<unsigned char> w_buff;
    int w_curr;


    //insure domain and dns_server are valid
    std::string domain;
    int port;
    std::string dns_server;
};


typedef std::vector< std::string > StringVec;
typedef std::pair<std::string, StringVec> DomainIP;

struct connIPPort
{
    socket_t fd;

    TEST_STATE state;

    int port;
    DomainIP ips;
    int curr_ip;//conn which ip
};

#endif // CONN_HPP
