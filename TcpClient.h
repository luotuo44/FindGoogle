//Author: luotuo44@gmail.com
//Use of this source code is governed by a BSD-style license

#ifndef TCPCLIENT_H
#define TCPCLIENT_H


#include<string>
#include<functional>
#include<vector>

using uchar = unsigned char;

struct bufferevent;

namespace Net
{

using ReadCbFunc = std::function<void (const std::vector<uchar>&, void*)>;

enum EVENT_KIND
{
    EK_CONNECT = 0,//成功连接上服务器
    EK_CLOSE,//连接关闭
    EK_ERROR
};

using EventCbFunc = std::function<void (EVENT_KIND, const std::string &, void *)>;


struct CallbackParam
{
    CallbackParam()=default;
    CallbackParam(const CallbackParam&)=default;
    CallbackParam& operator = (const CallbackParam&)=default;
    CallbackParam(CallbackParam &&)=default;
    CallbackParam& operator = (CallbackParam &&)=default;
    void swap(CallbackParam &cb)noexcept
    {
        read_cb.swap(cb.read_cb);
        event_cb.swap(cb.event_cb);
        std::swap(arg, cb.arg);
    }

    ReadCbFunc read_cb;
    EventCbFunc event_cb;
    void *arg = nullptr;
};


class EventLoop;

class TcpClient
{
public:
    TcpClient()=default;
    TcpClient(const TcpClient&)=delete;
    TcpClient& operator = (const TcpClient&)=delete;
    //不支持移动构造和移动赋值，因为回调函数中的void*参数就是this
    ~TcpClient();



    void connect(EventLoop &loop, const std::string &server_ip, int server_port, const CallbackParam &cb_param);
    void close();

    const std::string& serverIp()const { return m_server_ip; }
    int serverPort()const { return m_server_port; }

    void writeData(const std::vector<uchar> &data);

private:
    static void buffereventDataReadCb(struct bufferevent *, void *ctx);
    static void buffereventEventCb(struct bufferevent *bev, short what, void *ctx);


private:
    bufferevent *m_bev = nullptr;
    std::string m_server_ip;
    int m_server_port;
    CallbackParam m_cb_param;
};

}


#endif // TCPCLIENT_H

