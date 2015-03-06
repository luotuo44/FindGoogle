//Filename: Observer.hpp
//Date: 2015-2-20

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef OBSERVER_HPP
#define OBSERVER_HPP

#include"typedefine.hpp"

class Observer
{
public:
    virtual ~Observer() {}
    virtual void update(socket_t fd, int events)=0;
    virtual void init()=0;

protected:
    Observer(){}
};



#endif // OBSERVER_HPP
