//Filename: IPQueue.cpp
//Date: 2015-2-20

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license



#include"IPQueue.hpp"
#include<assert.h>


void IPQueue::addIP(const std::string &domain, const StringVec &str_vec)
{
    MutexLock lock(m_mutex);

    m_ip_queue.push_back(std::make_pair(domain, str_vec));
}


DomainIP IPQueue::getIP()
{
    MutexLock lock(m_mutex);

    assert(m_ip_queue.size() > 0);

    DomainIP ips = m_ip_queue.front();
    m_ip_queue.pop_front();

    return ips;
}
