//Filename: typedefine.hpp
//Date: 2015-2-20

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#ifndef TYPEDEFINE_HPP
#define TYPEDEFINE_HPP



#define EV_READ 1
#define EV_WRITE 2


#ifdef WIN32
#define socket_t intptr_t
#else
#define socket_t int
#endif

#ifdef WIN32
#define socket_len_t int
#else
#define socket_len_t socklen_t
#endif


//maximum file descriptor number
#define MAXFILENO 1000

//dns packet state
enum DNS_STATE
{
    connecting_dns, //conn init state
    new_try, //one round start state
    send_query,
    recv_query_result,
    parse_query_result,
    stop_conn_dns
};


//test domain port state
enum TEST_STATE
{
    new_ip,
    new_connect,
    connecting_443,//can test other port
    connect_success,
    connect_fail,
    write_result,
    stop_conn_domain
};


typedef void (*writer_fun)(const char *domain, const char *ip, int port);






#endif // TYPEDEFINE_HPP
