//Author: luotuo44@gmail.com
//Use of this source code is governed by a BSD-style license


#include<iostream>
#include<algorithm>
#include<iterator>
#include<sstream>

#include"helper.h"
#include"dnsPacket.h"



std::string join(const std::vector<std::string> &vec, const std::string &delim)
{
    std::string str;
    do
    {
        if(vec.empty())
            break;

        std::stringstream ss;
        std::copy(vec.begin(), std::prev(vec.end()), std::ostream_iterator<std::string>(ss, delim.c_str()));

        ss<<vec.back();
        str = ss.str();
    }while(0);


    return str;
}

int char2int(char input)
{
    if( input >= '0' && input <= '9' )
        return input - '0';
    else if( input >= 'a' && input <= 'f')
        return input - 'a' + 10;
    else if( input >= 'A' && input <= 'F')
        return input - 'A' + 10;
    else
        throw "wrong input";
}

std::vector<uchar> hexToUVec(const std::string &origin_data)
{
    if(origin_data.size()%2 != 0)
        throw "length must be 2 times";

    std::vector<uchar> vec;
    vec.reserve(origin_data.size()/2);

    for(size_t pos = 0; pos < origin_data.size(); pos+=2)
    {
        size_t h = char2int(origin_data[pos]);
        size_t l = char2int(origin_data[pos+1]);
        size_t c = (h<<4) + l;

        vec.push_back(reinterpret_cast<uchar&>(c));
    }

    return vec;
}

void do_testParseDnsPacket(const std::string &host, const std::string &hex_str)
{
    std::cout<<"host "<<host<<std::endl;

    std::vector<uchar> vec = hexToUVec(hex_str);
    Net::DnsPacketDecoder decoder(vec);

    if(!decoder.isValid())
    {
        std::cout<<"is not valid"<<std::endl;
        return ;
    }


    auto cname = decoder.getCNameRecord();
    if(!cname.empty())
    {
        std::cout<<join(cname, " **cname** ")<<std::endl;
    }

    auto a_name = decoder.getARecord();
    std::copy(a_name.begin(), a_name.end(), std::ostream_iterator<std::string>(std::cout, "\t"));
    std::cout<<std::endl<<std::endl;
}




void testParseDnsPacket()
{
    std::cout<<"+++++++++++++++++++++parseDnsPacket test++++++++++++++++++++++"<<std::endl;
    std::string qq_hex = "0002818000010003000000000377777702717103636f6d0000010001c00c000100010000002a00040e1120d3c00c000100010000002a00043b25603fc00c000100010000002a00040e112a28";
    do_testParseDnsPacket("www.qq.com", qq_hex);//没有cname

    std::string baidu_hex = "8f7e818000010003000000000377777705626169647503636f6d0000010001c00c0005000100000187000f0377777701610673686966656ec016c02b000100010000006000040ed7b127c02b000100010000006000040ed7b126";
    do_testParseDnsPacket("www.baidu.com", baidu_hex);//一层cname

    std::string sina_hex = "000281800001000300000000037777770473696e6103636f6d0000010001c00c000500010000001300100275730473696e6103636f6d02636e00c02a000500010000001300160573706f6f6c04677269640873696e6165646765c015c04600010001000000130004de4cd63c";
    do_testParseDnsPacket("www.sina.com", sina_hex);//两层cname



    std::cout<<"============ invalid paseDnsPacket test==============="<<std::endl;
    do_testParseDnsPacket("www.qq.com", qq_hex.substr(0, 20));
    do_testParseDnsPacket("www.qq.com", qq_hex.substr(1, 36));

    do_testParseDnsPacket("www.baidu.com", baidu_hex.substr(0, 60));
    sina_hex.pop_back();sina_hex.pop_back();
    do_testParseDnsPacket("www.sina.com", sina_hex);
}


int main()
{
    testParseDnsPacket();
}
