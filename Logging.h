//Author: luotuo44@gmail.com
//Use of this source code is governed by a BSD-style license

#ifndef LOGGING_H
#define LOGGING_H

#include<string>

enum LogLevel
{
    DEBUG = 0,
    INFO,
    WARNING,
    ERROR,
    FATAL,

    NB
};

void logMsg(enum LogLevel level, const std::string &msg);

#endif // LOGGING_H

