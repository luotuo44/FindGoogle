//Author: luotuo44@gmail.com
//Use of this source code is governed by a BSD-style license

#include"TcpClient.h"


#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include<assert.h>
#include<string.h>

#include<stdexcept>

#include"event2/bufferevent.h"
#include"event.h"


#include"EventLoop.h"
#include"Logging.h"


namespace Net
{


static inline struct sockaddr* initSocket(struct sockaddr_in &server_addr, const std::string &server_ip, int server_port)
{
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = ::htons(server_port);
    ::inet_aton(server_ip.c_str(), &server_addr.sin_addr);

    return reinterpret_cast<struct sockaddr *>(&server_addr);
}



TcpClient::~TcpClient()
{
    if( m_bev != nullptr )
        bufferevent_free(m_bev);
}



void TcpClient::connect(EventLoop &loop, const std::string &server_ip, int server_port, const CallbackParam &cb_param)
{
    if( m_bev != nullptr )
        throw std::logic_error("this TcpClient has connected one server");

    bufferevent *bev = loop.newBufferevent();
    std::string msg;
    bool flag = false;
    do
    {
        struct sockaddr_in server_addr;
        auto s_addr = initSocket(server_addr, server_ip, server_port);
        int ret = bufferevent_socket_connect(bev, s_addr, sizeof(server_addr));
        if( ret != 0 )
        {
            msg = "fail to create socket";
            flag = true; break;
        }

        bufferevent_setcb(bev, TcpClient::buffereventDataReadCb, nullptr, TcpClient::buffereventEventCb, this);

        ret = bufferevent_enable(bev, EV_READ | EV_PERSIST);
        if( ret != 0 )
        {
            msg = "fail to enable bufferevent";
            flag = true; break;
        }

    }while(0);


    if( flag )
    {
        bufferevent_free(bev);
        logMsg(WARNING, msg);
        throw std::logic_error(msg);
    }
    else
    {
        m_bev = bev;
        m_server_ip = server_ip;
        m_server_port = server_port;
        m_cb_param = cb_param;
    }
}


void TcpClient::close()
{
    bufferevent_free(m_bev);
    m_bev = nullptr;
}


void TcpClient::writeData(const std::vector<uchar> &data)
{
    bufferevent_write(m_bev, data.data(), data.size());
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void TcpClient::buffereventDataReadCb(struct bufferevent *, void *ctx)
{
    TcpClient *tc = reinterpret_cast<TcpClient*>(ctx);

    std::vector<uchar> data(4096, '\0');
    size_t len = bufferevent_read(tc->m_bev, &data[0], data.size());
    data.resize(len);

    if(data.size() > 0 && tc->m_cb_param.read_cb)
    {
        tc->m_cb_param.read_cb(data, tc->m_cb_param.arg);
    }
}



static std::string parseErrorMsg(short what)
{
    /*
    @param bev the bufferevent for which the error condition was reached
    @param what a conjunction of flags: BEV_EVENT_READING or BEV_EVENT_WRITING
       to indicate if the error was encountered on the read or write path,
       and one of the following flags: BEV_EVENT_EOF, BEV_EVENT_ERROR,
       BEV_EVENT_TIMEOUT, BEV_EVENT_CONNECTED.
    */
    std::string msg;
    if(what & BEV_EVENT_READING)
        msg += " [error encountered while reading] ";
    if(what & BEV_EVENT_WRITING)
        msg += " [error encountered while writing] ";
    if(what & BEV_EVENT_EOF)
        msg += " [eof file reached] ";
    if(what & BEV_EVENT_ERROR)
        msg += " [unrecoverable error encountered] ";
    if(what & BEV_EVENT_TIMEOUT)
        msg += " [user-specified timeout reached] ";

    return msg;
}


void TcpClient::buffereventEventCb(struct bufferevent *bev, short what, void *ctx)
{
    TcpClient *tc = reinterpret_cast<TcpClient*>(ctx);
    assert(bev == tc->m_bev);

    std::string msg;
    enum EVENT_KIND kind = EK_ERROR;

    if(what & BEV_EVENT_CONNECTED )//客户端成功连接服务器
    {
        msg = "connect server " + tc->m_server_ip + ":" + std::to_string(tc->m_server_port);
        kind = EK_CONNECT;
    }
    else if( what & BEV_EVENT_EOF )
    {
        msg = tc->m_server_ip + ":" + std::to_string(tc->m_server_port) + " closed";
        kind = EK_CLOSE;
    }else
    {
        msg = parseErrorMsg(what);
    }


    if(tc->m_cb_param.event_cb)
        tc->m_cb_param.event_cb(kind, msg, tc->m_cb_param.arg);
}

}
