//Filename: MutexLock.cpp
//Date: 2015-5-25

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#include"MutexLock.hpp"
#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#include"Logger.hpp"




static void assertMutex(const char *op, int result)
{
    if(result)
    {
        char buf[50];
        snprintf(buf, sizeof(buf), "%m");
        LOG(Log::FATAL)<<op<<" fail : "<<buf;
    }
}



Mutex::Mutex()
{
    assertMutex("init mutex", pthread_mutex_init(&m_mutex, NULL));
}


Mutex::~Mutex()
{
    assertMutex("destroy mutex", pthread_mutex_destroy(&m_mutex));
}


void Mutex::lock()
{
    assertMutex("lock mutex", pthread_mutex_lock(&m_mutex));
}


void Mutex::unlock()
{
    assertMutex("unlock mutex", pthread_mutex_unlock(&m_mutex));
}




