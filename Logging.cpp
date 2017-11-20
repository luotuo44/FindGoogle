//Author: luotuo44@gmail.com
//Use of this source code is governed by a BSD-style license


#include"Logging.h"


#include<iostream>
#include<map>

static std::map<enum LogLevel, std::string> log_level{{DEBUG, "Debug"}, {INFO, "Info"},
                                {WARNING, "Warning"}, {ERROR, "Error"}, {FATAL, "Fatal"}};


void logMsg(enum LogLevel level, const std::string &msg)
{
    std::cout<<"["<<log_level[level]<<"] "<<msg<<std::endl;
}
