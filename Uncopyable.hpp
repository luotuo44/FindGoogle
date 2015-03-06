//Filename: Uncopyable.hpp
//Date: 2015-2-20

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef UNCOPYABLE_HPP
#define UNCOPYABLE_HPP

namespace Utility
{

class Uncopyable
{
public:
    Uncopyable() {}

private:
    Uncopyable(const Uncopyable& un);
    Uncopyable& operator = (const Uncopyable& un);
};

}




#endif // UNCOPYABLE_HPP
