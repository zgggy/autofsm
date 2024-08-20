/**
 * Created Time: 2024.08.19
 * File name:    logdef
 * Author:       zgggy(zgggy@foxmail.com)
 * Brief:        自定义log
 * Include:
 */

#ifndef __LOGDEF_H__
#define __LOGDEF_H__

#ifdef USE_EASYLOGGINGPP
#define INFO LOG(INFO)
#define DEBUG LOG(DEBUG)
#define WARN LOG(WARNING)
#define ERROR LOG(ERROR)
#define ENDL ""
#else
#include <cstring>
#ifndef LOGOUTPUT
#define LOGOUTPUT true
#endif
#define LOG_PUREFILENAME [](const char* a, const char* b) { return a > b ? a : b; }(strrchr(__FILE__, '/'), strrchr(__FILE__, '\\')) + 1
#define LOG_MESSAGES LOG_PUREFILENAME << ":" << __LINE__ << "|" << __FUNCTION__ << "] "
#define INFO \
    if (LOGOUTPUT) std::cout << "[I|" << LOG_MESSAGES
#define DEBUG \
    if (LOGOUTPUT) std::cout << "[D|" << LOG_MESSAGES
#define WARN \
    if (LOGOUTPUT) std::cout << "[W|" << LOG_MESSAGES
#define ERROR \
    if (LOGOUTPUT) std::cout << "[E|" << LOG_MESSAGES
#define ENDL "\n"
#endif

#endif // __LOGDEF_H__