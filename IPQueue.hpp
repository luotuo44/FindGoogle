//Filename: IPQueue.hpp
//Date: 2015-2-20

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef IPQUEUE_HPP
#define IPQUEUE_HPP

#include"Uncopyable.hpp"
#include<string>
#include<vector>
#include<list>
#include<utility>

#include"MutexLock.hpp"

typedef std::vector< std::string > StringVec;
typedef std::pair<std::string, StringVec> DomainIP;

class IPQueue : public Utility::Uncopyable
{
public:
    void addIP(const std::string &domain, const StringVec &str_vec);
    DomainIP getIP();

private:
    std::list< DomainIP > m_ip_queue;

    Mutex m_mutex;
};


#endif // IPQUEUE_HPP
