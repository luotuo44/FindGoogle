//Filename: Condition.cpp
//Date: 2015-5-25

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#include"Condition.hpp"

#include<sys/time.h>

#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<errno.h>

#include"MutexLock.hpp"
#include"Logger.hpp"


static void assertCondition(const char *op, int result)
{
    if(result)
    {
        char buf[50];
        snprintf(buf, sizeof(buf), "%m");
        LOG(Log::FATAL)<<op<<" fail : "<<buf;
    }
}


//set the t to abstime that after msecs Milliseconds from now
static void mkTime(struct timespec* t, int msecs)
{
    if( t == nullptr )
        return ;

    struct timeval now;
    gettimeofday(&now, nullptr);


    if( msecs > 0 )
    {
        int u = now.tv_usec + (msecs%1000)*1000; //the usecs
        if( u >= 1000 * 1000) //enough one second.
        {
            ++now.tv_sec;
            u -= 1000 * 1000;
        }

        now.tv_sec += msecs/1000;
        now.tv_usec = u;
    }

    t->tv_sec = now.tv_sec;
    t->tv_nsec = now.tv_usec * 1000;
}



Condition::Condition(Mutex &mutex)
    : m_mutex(mutex)
{
    assertCondition("init condition", pthread_cond_init(&m_cond, NULL));
}


Condition::~Condition()
{
    assertCondition("destroy conditon", pthread_cond_destroy(&m_cond));
}


void Condition::wait()
{
    assertCondition("wait condition", pthread_cond_wait(&m_cond, &m_mutex.m_mutex));
}


//if timeout return false. or return true
bool Condition::waitForMilliseconds(int msecs)
{
    struct timespec abs_time;
    mkTime(&abs_time, msecs);

    int ret = pthread_cond_timedwait(&m_cond, &m_mutex.m_mutex, &abs_time);

    if( ret != 0 && ret != ETIMEDOUT)
        assertCondition("wait condition with time", ret);//will abort


    return ret != ETIMEDOUT;
}


void Condition::notify()
{
    assertCondition("notify condition", pthread_cond_signal(&m_cond));
}


void Condition::notifyAll()
{
    assertCondition("notifyAll condtion", pthread_cond_broadcast(&m_cond));
}


