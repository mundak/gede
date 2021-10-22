/*
 * Copyright (C) 2014-2017 Johan Henriksson.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#ifndef FILE__COM_H
#define FILE__COM_H


#include <QProcess>
#include <QList>
#include <QFile>
#include <assert.h>
#include "tree.h"
#include "config.h"


class Token
{
    public:

        enum Type{
            UNKNOWN,
            C_STRING,         // "string"
            C_CHAR,           // 'c'
            KEY_EQUAL,        // '='
            KEY_LEFT_BRACE,   // '{'
            KEY_RIGHT_BRACE,  // '}'
            KEY_LEFT_BAR,     // '['
            KEY_RIGHT_BAR,    // ']'
            KEY_UP,           // '^'
            KEY_PLUS,         // '-'
            KEY_COMMA,        // ','
            KEY_TILDE,        // '~'
            KEY_SNABEL,       // '@'
            KEY_STAR,         // '*'
            KEY_AND,          // '&'
            END_CODE,
            VAR
        };
    public:

        Token(Type type) : m_type(type) {};
    
        static const char *typeToString(Type type);
        Type getType() const { return m_type; };
        void setType(Type type) { m_type = type; };
        QString getString() const { return m_text; };

        const char *toString();

    private:
        Type m_type;
        char m_tmpBuff[128];
    public:
        QString m_text;
};



class GdbComListener : public QObject
{
    
private:
    Q_OBJECT
    
    public:

        enum AsyncClass
        {
            AC_STOPPED,
            AC_RUNNING,
            AC_THREAD_CREATED,
            AC_THREAD_GROUP_ADDED,
            AC_THREAD_GROUP_STARTED,
            AC_LIBRARY_LOADED,
            AC_BREAKPOINT_MODIFIED,
            AC_BREAKPOINT_DELETED,
            AC_THREAD_EXITED,
            AC_THREAD_GROUP_EXITED,
            AC_LIBRARY_UNLOADED,
            AC_THREAD_SELECTED,
            AC_DOWNLOAD,
            AC_CMD_PARAM_CHANGED,
            AC_UNKNOWN
        };


        virtual void onStatusAsyncOut(Tree &tree, AsyncClass ac) = 0;
        virtual void onNotifyAsyncOut(Tree &tree, AsyncClass ac) = 0;
        virtual void onExecAsyncOut(Tree &tree, AsyncClass ac) = 0;
        virtual void onResult(Tree &tree) = 0;
        virtual void onConsoleStreamOutput(QString str) = 0;
        virtual void onTargetStreamOutput(QString str) = 0;
        virtual void onLogStreamOutput(QString str) = 0;
};

enum GdbResult
{
    GDB_DONE = 0,
    GDB_RUNNING,
    GDB_CONNECTED,
    GDB_ERROR,
    GDB_EXIT
};


class PendingCommand
{
    public:
        PendingCommand() {};

        QString m_cmdText;


};


class Resp
{
    public:
        Resp() : m_type(UNKNOWN) {};

        typedef enum {
            UNKNOWN = 0,
            RESULT,
            CONSOLE_STREAM_OUTPUT,
            TARGET_STREAM_OUTPUT,
            LOG_STREAM_OUTPUT,
            TERMINATION,
            STATUS_ASYNC_OUTPUT,
            NOTIFY_ASYNC_OUTPUT,
            EXEC_ASYNC_OUTPUT,
        } Type;

        bool isResult();
        Type getType() { return m_type; };
        void setType(Type t) { assert(m_type == UNKNOWN); m_type = t; };
        
        bool isTermination() { return m_type == TERMINATION ? true : false; };
        QString getString() { return m_str; };
        void setString(QString str) { m_str = str; };

        QString getDataStr() { return m_dataStr; };
    private:
        Type m_type;
        QString m_str;
        QString m_dataStr;
    public:
        Tree tree;
        GdbComListener::AsyncClass reason;
        GdbResult m_result;
        
        
};



class GdbCom : public QObject
{
    private:

        Q_OBJECT

        GdbCom();
        ~GdbCom();

    public:


        static const char* asyncClassToString(GdbComListener::AsyncClass ac);

        static GdbCom& getInstance();
        int init(QString gdbPath, bool enableDebugLog);

        void setListener(GdbComListener *listener) { m_listener = listener; };

        int getPid();

        GdbResult commandF(Tree *resultData, const char *cmd, ...);
        GdbResult command(Tree *resultData, QString cmd);

        static QList<Token*> tokenize(QString str);

        void enableLog(bool enable);
        
    private:
        int parseAsyncOutput(Resp *resp, GdbComListener::AsyncClass *ac);
        Resp *parseAsyncRecord();
        Resp *parseExecAsyncOutput();
        Resp *parseNotifyAsyncOutput();
        Resp *parseOutOfBandRecord();
        Resp *parseOutput();
        int parseResult(TreeNode *parent);
        Resp *parseResultRecord();
        Resp *parseStatusAsyncOutput();
        Resp *parseStreamRecord();
        int parseValue(TreeNode *item);



    public slots:
        void onReadyReadStandardOutput ();
        void onReadyReadStandardError();
        void onGdbStateChanged(QProcess::ProcessState newState );
        

    private:
        int readFromGdb(GdbResult *m_result, Tree *m_resultData);
        void decodeGdbResponse();
        Token* pop_token();
        Token* peek_token();
        Token* checkToken(Token::Type type);
        Token* eatToken(Token::Type type);
        void dispatchResp();
        bool isTokenPending();
        void readTokens();
        void writeLogEntry(QString logText);
        
    private:
        QProcess m_process;
        QList<Resp*> m_respQueue; //!< List of responses received from GDB
        QList<PendingCommand> m_pending;
        GdbComListener *m_listener;
        
        QList<Token*> m_freeTokens; //!< List of tokens allocated but not in use.
        QList<Token*> m_list;
        QFile m_logFile;
        QByteArray m_inputBuffer; //!< List of raw characters received from the GDB process.
        int m_busy;
        bool m_enableLog;
};


#endif // FILE__COM_H
