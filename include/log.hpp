#pragma once 

#include <cstring>
#include <fstream>
#include <iostream>
#include <iomanip>

enum LOG_LEVEL : size_t {
    ALL = 0,
    TRACE,
    DEBUG,
    WARN,
    INFO,
    ERROR,
    FATAL
};
inline std::string getLogLevelLiteral(LOG_LEVEL level)
{
    switch (level)
    {
    case TRACE: return "TRACE";
    case DEBUG: return "DEBUG";
    case WARN:  return "WARN";
    case INFO:  return "INFO";
    case ERROR: return "ERROR";
    case FATAL: return "FATAL";
    default:
        return "ULOG";
    }
}
#define STREAMADDRESS(address) "0x" << std::setw(16) << std::left << std::hex << address << std::dec

#ifndef NLOG
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define MAXfilenameLenght 16
#define MAXlevelLenght 6
#define MAXlineLenght 5


extern std::fstream LOGSTREAM;

constexpr LOG_LEVEL LOGLEVEL = LOG_LEVEL::ALL;

#define LOGMSG(level,text) LOGSTREAM << std::setw(5) << getLogLevelLiteral(level) << ":" << std::setw(MAXfilenameLenght) << __FILENAME__ << " line:" << std::setw(MAXlineLenght) << std::left << __LINE__ << " msg:" << text << std::endl

#define LOGINIT(filename) LOGSTREAM.open(filename, std::ios::out | std::ios::trunc)
#define LOG(level,text) ( ( level ) < LOGLEVEL) ? void() : void(LOGMSG(level,text))
#else

#define LOGINIT(filename)
#define LOG(level,text) 
#endif


#define TRACE(text) LOG(LOG_LEVEL::TRACE,text)
#define DEBUG(text) LOG(LOG_LEVEL::DEBUG,text)
#define INFO(text) LOG(LOG_LEVEL::INFO,text)
#define WARN(text) LOG(LOG_LEVEL::WARN,text)
#define ERROR(text) LOG(LOG_LEVEL::ERROR,text)
#define FATAL(text) LOG(LOG_LEVEL::FATAL,text)