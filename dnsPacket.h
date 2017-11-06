//Author: luotuo44@gmail.com
//Use of this source code is governed by a BSD-style license

#ifndef DNSPACKET_H
#define DNSPACKET_H


#include<stddef.h>

#include<string>
#include<vector>

using uchar = unsigned char;

namespace Net
{

std::vector<uchar> getDnsPacketByHost(const std::string &host);

class DnsPacketDecoder
{
public:
    DnsPacketDecoder(const std::vector<uchar> &data);

    bool isValid()const { return m_valid; }
    std::vector<std::string> getCNameRecord()const { return m_cname_record; }
    std::vector<std::string> getARecord()const { return m_a_record; }
    std::string queryHost()const { return m_query_host; }

private:
    void parsePacket();

    bool parseHeader();
    bool parseBody();
    bool parseQuestion();
    bool parseAnswer();
    void doParsePacket();
    std::string parseHost(size_t &pos);

private:
    bool m_valid = false;
    bool m_complete = false;
    std::vector<uchar> m_origin_data;
    size_t m_pos = 0;
    std::string m_query_host;
    std::vector<std::string> m_cname_record;
    std::vector<std::string> m_a_record;

    uint16_t m_packet_id = 0;
};

}

#endif // DNSPACKET_H

