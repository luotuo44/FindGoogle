#include <iostream>

#include<vector>
#include<utility>
#include<stdexcept>
#include<memory>
#include<iterator>
#include<set>
#include<map>
#include<algorithm>
#include<functional>


#include"helper.h"
#include"EventLoop.h"
#include"TcpClient.h"
#include"dnsPacket.h"


using namespace std;


using TcpClientUPtr = std::unique_ptr<Net::TcpClient>;

inline std::ostream& operator << (std::ostream &os, const std::vector<uchar> &vec)
{
    std::copy(vec.begin(), vec.end(), std::ostream_iterator<uchar>(os, ""));
    return os;
}



class DnsQueryTask
{
public:
    DnsQueryTask(const std::string &query_host, int query_port)
        : m_query_host(query_host),
          m_query_port(query_port)
    {}


    //-1 data is invalid, 0 data is incomplete, 1 data is success to parse
    int appendRspData(const std::vector<uchar> &data);


    const std::string& queryHost()const { return m_query_host; }
    int queryPort()const { return m_query_port; }
    const std::vector<std::string>& queryHostIp()const { return m_host_ip; }

private:
    std::string m_query_host;
    int m_query_port;
    std::vector<uchar> m_dns_rsp;
    std::vector<std::string> m_host_ip;
};


//-1 data is invalid, 0 data is incomplete, 1 data is success to parse
int DnsQueryTask::appendRspData(const std::vector<uchar> &data)
{
    m_dns_rsp += data;
    Net::DnsPacketDecoder decoder(m_dns_rsp);

    if(decoder.successDecode())
    {
        m_host_ip = decoder.getARecord();
        return 1;
    }

    return decoder.isImcomplete() ? 0 : -1;
}


class HostConnectTask
{
public:
    HostConnectTask(const std::string &host, const std::string &ip, int port)
        : m_host(host), m_ip(ip), m_port(port)
    {}

    const std::string& connectHost()const { return m_host; }
    const std::string& connectIp()const { return m_ip; }
    int connectPort()const { return m_port; }


private:
    std::string m_host;
    std::string m_ip;
    int m_port;
};

class BigBoss
{
public:
    explicit BigBoss(const std::string &filename);

    void run() { m_loop.run(); }

private:
    void readQueryTask(const string &filename);
    void addTimeoutEvent();

private:
    void readCb(const std::vector<uchar> &data, void *arg);
    void eventCb(Net::EVENT_KIND kind, const std::string &msg, void *arg);
    void hostConnectEventCb(Net::EVENT_KIND kind, const std::string &msg, void *arg);


private:
    Net::EventLoop m_loop;

    std::map<TcpClientUPtr, DnsQueryTask> m_dns_query_task;
    std::map<TcpClientUPtr, HostConnectTask> m_host_connect_task;
};


BigBoss::BigBoss(const string &filename)
{
    readQueryTask(filename);
    addTimeoutEvent();
}


void BigBoss::addTimeoutEvent()
{

}

void BigBoss::hostConnectEventCb(Net::EVENT_KIND kind, const std::string &msg, void *arg)
{
    TcpClientUPtr tmp(reinterpret_cast<Net::TcpClient*>(arg));
    auto it = m_host_connect_task.find(tmp);
    tmp.release();//必须释放

    if(it == m_host_connect_task.end())
        return ;

    if(kind == Net::EK_CONNECT )
    {
        std::cout<<"success connect "<<it->second.connectHost()<<"\t"<<it->second.connectIp()<<":"<<it->second.connectPort()<<std::endl;
    }

    m_host_connect_task.erase(it);
}


void BigBoss::readQueryTask(const string &filename)
{
    auto dns_domain_pair = Helper::parseConfig(filename);

    for(auto &e : dns_domain_pair.first )
    {
        for(auto &ee : dns_domain_pair.second )
        {
            DnsQueryTask task(ee.first, ee.second);
            TcpClientUPtr p(new Net::TcpClient());
            Net::CallbackParam cb_param;
            cb_param.read_cb = std::bind(&BigBoss::readCb, this, std::placeholders::_1, std::placeholders::_2);
            cb_param.event_cb = std::bind(&BigBoss::eventCb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
            cb_param.arg = p.get();
            p->connect(m_loop, e, 53, cb_param);

            m_dns_query_task.emplace(std::move(p), std::move(task));
        }
    }
}




void BigBoss::readCb(const std::vector<uchar> &data, void *arg)
{
    //std::map::find只能接受value_type作为参数
    TcpClientUPtr tmp(reinterpret_cast<Net::TcpClient*>(arg));
    auto it = m_dns_query_task.find(tmp);
    tmp.release();//必须释放

    if(it == m_dns_query_task.end() )
        return ;

    DnsQueryTask &dns_task = it->second;
    int ret = dns_task.appendRspData(data);
    if(ret == 0)//imcomplete
        return ;
    else if(ret == 1)//success decode
    {
        auto ips = dns_task.queryHostIp();
        std::cout<<dns_task.queryHost()<<" has ";
        std::copy(ips.begin(), ips.end(), std::ostream_iterator<std::string>(std::cout, " "));
        std::cout<<" ips. "<<std::endl;

        for(const auto &ip : ips)
        {
            HostConnectTask task(dns_task.queryHost(), ip, dns_task.queryPort());
            TcpClientUPtr p(new Net::TcpClient());
            Net::CallbackParam cb_param;
            cb_param.read_cb = std::bind(&BigBoss::readCb, this, std::placeholders::_1, std::placeholders::_2);
            cb_param.event_cb = std::bind(&BigBoss::hostConnectEventCb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
            cb_param.arg = p.get();
            p->connect(m_loop, ip, dns_task.queryPort(), cb_param);

            m_host_connect_task.emplace(std::move(p), std::move(task));
        }
    }

    m_dns_query_task.erase(it);
}



void BigBoss::eventCb(Net::EVENT_KIND kind, const std::string &msg, void *arg)
{
    //std::map::find只能接受value_type作为参数
    TcpClientUPtr tmp(reinterpret_cast<Net::TcpClient*>(arg));
    auto it = m_dns_query_task.find(tmp);
    tmp.release();//必须要释放


    if( it == m_dns_query_task.end() )
        return ;

    if(kind != Net::EK_CONNECT )
    {
        m_dns_query_task.erase(it);
    }
    else //succes connect
    {
        auto query_packet = Net::getDnsPacketByHost(it->second.queryHost());
        it->first->writeData(query_packet);
    }
}



int main()
{
    BigBoss boss("dns_query.txt");
    boss.run();

    std::cout<<"hello world"<<std::endl;
    return 0;
}

