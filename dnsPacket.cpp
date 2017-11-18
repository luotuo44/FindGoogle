//Author: luotuo44@gmail.com
//Use of this source code is governed by a BSD-style license


#include"dnsPacket.h"

#include<netinet/in.h>
#include<arpa/inet.h>

#include<string.h>
#include<stddef.h>

#include"helper.h"



namespace Net
{

class DnsFlag
{
public:
    DnsFlag(const std::vector<uchar> &vec = std::vector<uchar>(2, '\0'))
        : flag(vec)
    {
    }

    void setQR(bool b) { Helper::setNBits<1>(flag[0], 7, b); }
    void setOpcode(size_t code) { Helper::setNBits<4>(flag[0], 3, code); }
    void setAA(bool b) { Helper::setNBits<1>(flag[0], 2, b); }
    void setTC(bool b) { Helper::setNBits<1>(flag[0], 1, b); }
    void setRD(bool b) { Helper::setNBits<1>(flag[0], 0, b); }
    void setRA(bool b) { Helper::setNBits<1>(flag[1], 7, b); }
    void setRcode(size_t code) { Helper::setNBits<4>(flag[1], 0, code); }
    std::vector<uchar> getData()const { return flag; }

    bool getQR()const { return Helper::getNBits<1>(flag[0], 7); }
    size_t getOpcode()const { return Helper::getNBits<4>(flag[0], 3); }
    bool getAA()const { return Helper::getNBits<1>(flag[0], 2); }
    bool getTC()const { return Helper::getNBits<1>(flag[0], 1); }
    bool getRD()const { return Helper::getNBits<1>(flag[0], 0); }
    bool getRA()const { return Helper::getNBits<1>(flag[1], 7); }
    size_t getRcode()const { return Helper::getNBits<4>(flag[1], 0); }

private:
    std::vector<uchar> flag;
};



enum QueryKind
{
    A = 1,
    NS = 2,
    CNAME = 5,
    PTR = 12,
    HINFO = 13,
    MX = 15
};


std::vector<uchar> getDnsQuestionPart(const std::string &host, QueryKind kind)
{
    StrVec vec(Helper::StringSplit(host, "."), Helper::StringSplit());

    std::string str;
    str.reserve(host.size() + vec.size() + 10);

    for(const auto &e : vec)
    {
        str.append(1, static_cast<char>(e.size())).append(e);
    }
    str.append(1, static_cast<char>(0));

    std::vector<uchar> vvec = Helper::toUVec(str);

    vvec += Helper::toUVec(int16_t{kind});
    vvec += Helper::toUVec(int16_t{1});//query class


    return vvec;
}



std::vector<uchar> getDnsPacketByHost(const std::string &host)
{
    std::vector<uchar> packet;
    packet.reserve(12 + host.size() + 4 + 6);

    packet += Helper::toUVec(int16_t{13});//id


    DnsFlag flag;
    flag.setQR(0);
    flag.setOpcode(0);
    flag.setAA(0);
    flag.setTC(0);
    flag.setRD(1);
    flag.setRA(0);
    flag.setRcode(0);
    packet += flag.getData();


    packet += Helper::toUVec(int16_t{1});//question num
    packet += Helper::toUVec(int16_t{0});//resource num
    packet += Helper::toUVec(int16_t{0});//authorized num
    packet += Helper::toUVec(int16_t{0});//extra num


    packet += getDnsQuestionPart(host, QueryKind::A);


    return packet;
}


std::string parseIP(const uchar *data)
{
    unsigned int *p = (unsigned int*)(data);
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = *p;
    return ::inet_ntoa(addr.sin_addr);
}


}


//==========================================================================

namespace Net
{

DnsPacketDecoder::DnsPacketDecoder(const std::vector<uchar> &data)
{
    if(data.size() < 12)
        return ;

    m_origin_data = data;

    parsePacket();
}


void DnsPacketDecoder::parsePacket()
{
    m_parse_res = parseHeader();
    if(m_parse_res == 0)
        m_parse_res = parseBody();
}


int DnsPacketDecoder::parseHeader()
{
    m_packet_id = Helper::getSizeValue(m_origin_data.data(), m_origin_data.data()+2);


    DnsFlag flag(Helper::subVec(m_origin_data, 2, 2));
    if(flag.getQR() != 1 || flag.getRcode() != 0)
        return 2;//data invalid


    const uchar *pos = m_origin_data.data() + 4;
    size_t question_num = Helper::getSizeValue(pos, pos+2);
    size_t res_num = Helper::getSizeValue(pos+2, pos+4);
    if( question_num == 0 || res_num == 0)
        return 2;//data invalid


    return m_origin_data.size() > 12 ? 0 : 1;
}



int DnsPacketDecoder::parseBody()
{
    int ret = parseQuestion();
    ret = (ret == 0) ? parseAnswer() : ret;
    if( ret == 0 && m_cname_record.size() == 1 )
        m_cname_record.clear();//没有使用cname

    return ret;
}


int DnsPacketDecoder::parseQuestion()
{
    m_pos = 12;
    m_query_host = parseHost(m_pos);

    if(m_pos+4 >= m_origin_data.size())
        return 1;//imcomplete

    const uchar *data = m_origin_data.data() + m_pos;
    size_t query_kind = Helper::getSizeValue(data, data+2);
    size_t query_class = Helper::getSizeValue(data+2, data+4);
    m_pos += 4;

    return query_kind == QueryKind::A && query_class == 1 ? 0 : 2;
}


int DnsPacketDecoder::parseAnswer()
{
    std::string answer_name = parseHost(m_pos);
    Helper::trim(answer_name, '.');
    if(m_pos+10 >= m_origin_data.size())
        return 1;//imcomplete

    const uchar *data = m_origin_data.data() + m_pos;
    size_t answer_kind = Helper::getSizeValue(data, data+2);
    size_t answer_class = Helper::getSizeValue(data+2, data+4);
    if(answer_class != 1 || (answer_kind != QueryKind::A && answer_kind != QueryKind::CNAME))
        return 2;//data invalid

    Helper::getSizeValue(data+4, data+8);//ttl
    size_t res_len = Helper::getSizeValue(data+8, data+10);
    m_pos += 10;


    if(m_pos+res_len > m_origin_data.size())
        return 1;//imcomplete

    if(answer_kind == QueryKind::A)
        m_a_record.push_back(parseIP(m_origin_data.data()+m_pos));

    if(m_cname_record.empty() || answer_name != m_cname_record.back())
        m_cname_record.push_back(std::move(answer_name));

    m_pos += res_len;
    return m_pos < m_origin_data.size() ? parseAnswer() : 0;//finish
}



std::string DnsPacketDecoder::parseHost(size_t &pos)
{
    const uchar *start = m_origin_data.data();
    std::string host;

    while(pos < m_origin_data.size())
    {
        size_t len = Helper::getSizeValue(start+pos, start+pos+1);
        if( len == 0)
        {
            pos += 1;
            break;
        }

        if(len < 192)
        {
            host.append(1, '.').append(reinterpret_cast<const char*>(start+pos+1), len);
            pos += len + 1;
        }
        else
        {
            size_t offset = (Helper::getNBits<6>(start[pos], 0)<<8) + start[pos+1];
            host += parseHost(offset);
            pos += 2;
            break;//出现跳转后，即可结束。后面并不会有结束符号\0
        }
    }

    return host;
}

}
