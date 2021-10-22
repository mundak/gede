/*
 * Copyright (C) 2014-2017 Johan Henriksson.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#ifndef FILE__LOG_H
#define FILE__LOG_H

#include <QDebug>

class ILogger
{
public:
    ILogger() {};
    virtual ~ILogger() {};

    virtual void ILogger_onWarnMsg(QString text) = 0;
    virtual void ILogger_onErrorMsg(QString text) = 0;
    virtual void ILogger_onInfoMsg(QString text) = 0;
    virtual void ILogger_onCriticalMsg(QString text) = 0;

};

#ifndef ENABLE_DEBUGMSG
#define debugMsg(fmt...)  do{}while(0)
#else
void debugMsg_(const char *file, int lineNo, const char *fmt,...);
#define debugMsg(fmt...)  debugMsg_(__FILE__, __LINE__, fmt)
#endif

void critMsg(const char *fmt,...);
void errorMsg(const char *fmt,...);
void warnMsg(const char *fmt,...);
void infoMsg(const char *fmt,...);


void loggerRegister(ILogger *logger);
void loggerUnregister(ILogger *logger);


#endif // FILE__LOG_H


