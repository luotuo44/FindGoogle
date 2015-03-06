//Filename: MutexLock.hpp
//Date: 2015-2-20

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef MUTEXLOCK_HPP
#define MUTEXLOCK_HPP

#include"Uncopyable.hpp"
#include<pthread.h>


class Mutex : Utility::Uncopyable
{
public:
    Mutex()
    {
        pthread_mutex_init(&m_mutex, NULL);
    }

    ~Mutex()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    void lock()
    {
        pthread_mutex_lock(&m_mutex);
    }

    void unlock()
    {
        pthread_mutex_unlock(&m_mutex);
    }

    pthread_mutex_t* nativeMutex()
    {
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex;
};


class MutexLock : public Utility::Uncopyable
{
public:
    explicit MutexLock(Mutex& mutex)
        : m_mutex(mutex)
    {
        m_mutex.lock();
    }

    ~MutexLock()
    {
        m_mutex.unlock();
    }

private:
    Mutex& m_mutex;
};

#define MutexLock(x) error "Missing lock a mutex"

#endif // MUTEXLOCK_HPP
