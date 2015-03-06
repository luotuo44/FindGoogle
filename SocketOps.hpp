//Filename: SocketOps.hpp
//Date: 2015-2-20

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef SOCKETOPS_HPP
#define SOCKETOPS_HPP


#include"typedefine.hpp"

#include<string>
#include<stdint-gcc.h>

namespace SocketOps
{

uint16_t htons(uint16_t host);
uint16_t ntohs(uint16_t net);

uint32_t htonl(uint32_t host);
uint32_t ntohl(uint32_t net);

int tcp_connect_server(const char* server_ip, int port,
                       socket_t *sockfd);

int connecting_server(socket_t fd);

int make_socket_nonblocking(socket_t fd);

int get_socket_error(socket_t fd);

bool wait_to_connect(int err);
bool refuse_connect(int err);

void close_socket(socket_t fd);

int write(socket_t fd, unsigned char *buff, int len);
int read(socket_t fd, unsigned char *buff, int len);

int writen(socket_t fd, const void *vptr, int n);
int readn(socket_t fd, void *vptr, int n);

std::string parseIP(const unsigned char *buff);

}

#endif // SOCKETOPS_HPP
