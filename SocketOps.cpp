//Filename: SocketOps.cpp
//Date: 2015-2-20

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#include"SocketOps.hpp"
#include<errno.h>
#include<string.h>
#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<assert.h>


extern int g_max_fileno;

namespace SocketOps
{

uint16_t htons(uint16_t host)
{
    return ::htons(host);
}

uint16_t ntohs(uint16_t net)
{
    return ::ntohs(net);
}


uint32_t htonl(uint32_t host)
{
    return ::htonl(host);
}


uint32_t ntohl(uint32_t net)
{
    return ::ntohl(net);
}


void close_socket(socket_t fd)
{
    //ignore the error
#ifdef WIN32
    closesocket(fd);
#else
    close(fd);
#endif
}

typedef struct sockaddr SA;

//-1 system call error. 0 connect success. 1 wait to connect,
//cannot connect immediately, and need to try again
int tcp_connect_server(const char* server_ip, int port,
                       socket_t *sockfd)
{
    assert(sockfd != NULL);

    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if( *sockfd == -1 )
        return -1;

    if( *sockfd >= g_max_fileno )//too much fd
    {
        SocketOps::close_socket(*sockfd);
        *sockfd = -1;
        errno = EMFILE;
        return -1;
    }


    int ret, save_errno;
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr) );

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    ret = inet_aton(server_ip, &server_addr.sin_addr);
    if( ret == 0 ) //the server_ip is not valid value
    {
        errno = EINVAL;
        goto err;
    }


    if( make_socket_nonblocking(*sockfd) == -1 )
        goto err;

    ret = connect(*sockfd, (SA*)&server_addr, sizeof(server_addr));

    if( ret == 0 )//connect success immediately
        return 0;
    else if( ret == -1)
    {
        int e = SocketOps::get_socket_error(*sockfd);

        if( SocketOps::wait_to_connect(e) )
            return 1;
        else if(SocketOps::refuse_connect(e) )
            fprintf(stderr, "refuse connect\n");
        else
            fprintf(stderr, "unknown err\n");

        goto err;
    }

err:
    save_errno = errno;
    SocketOps::close_socket(*sockfd);
    *sockfd = -1;
    errno = save_errno;

    return -1;
}

//-1 system call error. 0 connect success. 1 wait to connect,
int connecting_server(socket_t fd)
{
    int err;
    socket_len_t len = sizeof(err);

    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)&err, &len) < 0)
        return -1;

    if( err == 0 ) //connect success
        return 0;

    if( wait_to_connect(err) )//try again
        return 1;

    errno = err;

    return -1;
}


int make_socket_nonblocking(socket_t fd)
{
#ifdef WIN32

    u_long nonblocking = 1;
    if (ioctlsocket(fd, FIONBIO, &nonblocking) == SOCKET_ERROR)
        return -1;
#else

    int flags;
    if ((flags = fcntl(fd, F_GETFL, NULL)) < 0)
        return -1;

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        return -1;

#endif

    return 0;
}


int get_socket_error(socket_t fd)
{
#ifdef WIN32
    int optval, optvallen = sizeof(optval);
    int err = WSAGetLastError();
    if (err == WSAEWOULDBLOCK && sock >= 0)
    {
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (void*)&optval,
                       &optvallen))
            return err;
        if (optval)
            return optval;
    }
    return err;

#else
    return errno;
#endif
}


bool wait_to_connect(int err)
{
#ifdef WIN32
    return (err == WSAEWOULDBLOCK) || (err == WSAEINTR)
            || (err == WSAEINPROGRESS) || (err == WSAEINVAL);
#else
    return (err == EINTR) || (err == EINPROGRESS);
#endif
}


bool refuse_connect(int err)
{
#ifdef WIN32
    return err == WSAECONNREFUSED;
#else
    return err == ECONNREFUSED;
#endif
}


//-1: system call error
//0: write 0 byte
//positive number : write bytes
int write(socket_t fd, unsigned char *buff, int len)
{
    assert(len > 0);

    int ret;
    int send_num = 0;

    while( len > 0 )
    {
        ret = ::send(fd, buff+send_num, len, 0);
        if( ret > 0 )
        {
            send_num += ret;
            len -= ret;
        }
        else if( ret == 0)//socket fd fail
            return -1;
        else //ret < 0
        {
            if( errno == EINTR)
                continue;
            else if( errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            else
                return -1;
        }
    }

    return send_num;//should not be 0
}

int read(socket_t fd, unsigned char *buff, int len)
{
    int ret;
    int recv_num = 0;

    while(len > 0)
    {
        ret = ::recv(fd, buff+recv_num, len, 0);
        if( ret > 0)
        {
            recv_num += ret;
            len -= ret;
        }
        else if( ret == 0)
            return -1;
        else //ret < 0
        {
            if( errno == EINTR)
                continue;
            else if( errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            else
                return -1;
        }
    }

    return recv_num; //should not be 0
}


std::string parseIP(const unsigned char *buff)
{
    unsigned int *p = (unsigned int*)(buff);
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = *p;
    return inet_ntoa(addr.sin_addr);
}


int writen(socket_t fd, const void *vptr, int n)
{
    size_t      nleft;
    ssize_t     nwritten;
    const char  *ptr;

    ptr = (char*)vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nwritten = ::write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;       /* and call write() again */
            else
                return(-1);         /* error */
        }

        nleft -= nwritten;
        ptr   += nwritten;
    }
    return(n);
}


int readn(socket_t fd, void *vptr, int n)
{
    size_t  nleft;
    ssize_t nread;
    char    *ptr;

    ptr = (char*)vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nread = ::read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0;      /* and call read() again */
            else
                return(-1);
        } else if (nread == 0)
            break;              /* EOF */

        nleft -= nread;
        ptr   += nread;
    }

    return(n - nleft);      /* return >= 0 */
}


}
