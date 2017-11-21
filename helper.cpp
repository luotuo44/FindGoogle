//Author: luotuo44@gmail.com
//Use of this source code is governed by a BSD-style license

#include"helper.h"

#include<string.h>

#include<fstream>
#include<set>
#include<stdexcept>


namespace Helper
{


void StringSplit::next()
{
    if( m_pos == std::string::npos )
    {
        m_last_pos = m_pos;
    }
    else
    {
        auto tmp = m_pos;
        m_pos = m_str.find(m_delim, m_pos + m_delim.size());
        m_last_pos = tmp + m_delim.size();
    }
}


bool StringSplit::operator == (const StringSplit &ss)const
{
    /*
    return  (m_last_pos == std::string::npos && m_last_pos == ss.m_last_pos)
         || (m_last_pos == ss.m_last_pos && m_pos == ss.m_pos
             && m_str.data() == ss.m_str.data() && m_delim.data() == ss.m_delim.data());
    */

    //严谨的写法应该如上面那样，但对于StringSplit的正常使用来说，不需要搞得那么复杂
    return m_last_pos == std::string::npos && m_last_pos == ss.m_last_pos;
}



//==========================================================================

std::string& trim(std::string &str)
{
    do
    {
        auto pos = str.find_first_not_of("\t ");
        if(pos == std::string::npos)
            break;

        auto rpos = str.find_last_not_of("\t ");

        str = str.substr(pos, rpos-pos+1);
    }while(0);

    return str;
}


std::string& trim(std::string &str, char ch)
{
    do
    {
        auto pos = str.find_first_not_of(ch);
        if(pos == std::string::npos)
            break;

        auto rpos = str.find_last_not_of(ch);

        str = str.substr(pos, rpos-pos+1);
    }while(0);

    return str;
}


std::pair<StrVec, std::vector<StrIntPair>> parseConfig(const std::string &filename)
{
    std::ifstream in(filename.c_str());
    if( !in )
        throw std::logic_error("config file " + filename + " doesnot exist");


    std::set<std::string> dns_server_set;
    std::set<std::string> domain_set;

    std::string line;
    while( in>>line )
    {
        trim(line);
        std::string::size_type pos = 0;
        if(line.empty() || line[0] == '#' || (pos = line.find(':')) == std::string::npos)
            continue;


        std::string kind = line.substr(0, pos);
        if(kind == "dns_server")
            dns_server_set.insert(StringSplit(line.substr(pos+1), ";"), StringSplit());
        else if(kind == "domain")
            domain_set.insert(StringSplit(line.substr(pos+1), ";"), StringSplit());
    }


    std::vector<StrIntPair> domain_port;
    StrVec vec;
    for(const auto &e : domain_set)
    {
        vec.assign(StringSplit(e, ":"), StringSplit());
        if(vec.size() == 2)
            domain_port.emplace_back(std::move(vec[0]), std::stoi(vec[1]));
        else if( vec.size() == 1 && domain_set.find(vec[0] + ":443") == domain_set.end())
            domain_port.emplace_back(std::move(vec[0]), 443);
    }


    StrVec dns_server_vec(dns_server_set.begin(), dns_server_set.end());
    return std::make_pair(std::move(dns_server_vec), std::move(domain_port));
}





std::vector<uchar> subVec(const std::vector<uchar> &vec, size_t pos, size_t n)
{
    std::vector<uchar> ret;
    do
    {
        if(pos >= vec.size())
            break;

        size_t real_n = std::min(n, vec.size()-pos);
        ret.assign(vec.begin()+pos, vec.begin()+pos+real_n);

    }while(0);

    return ret;
}


std::vector<uchar> toUVec(const std::string &str)
{
    std::vector<uchar> vec;
    vec.resize(str.size());

    memcpy(&vec[0], str.data(), str.size());

    return vec;
}

}
