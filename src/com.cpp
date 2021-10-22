/*
 * Copyright (C) 2014-2017 Johan Henriksson.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#include "com.h"

#include <QDateTime>
#include <QByteArray>
#include <QDebug>
#include <unistd.h>
#include <assert.h>
#include <sys/time.h>

#include "log.h"
#include "util.h"
#include "config.h"
#include "version.h"



const char* GdbCom::asyncClassToString(GdbComListener::AsyncClass ac)
{
    switch(ac)
    {
        case GdbComListener::AC_STOPPED:return "stopped";break;
        case GdbComListener::AC_RUNNING:return "running";break;
        case GdbComListener::AC_THREAD_CREATED:return "thread_created";break;
        case GdbComListener::AC_THREAD_GROUP_ADDED:return "thread_group_added";break;
        case GdbComListener::AC_THREAD_GROUP_STARTED:return "thread_group_started";break;
        case GdbComListener::AC_LIBRARY_LOADED:return "library_loaded";break;
        case GdbComListener::AC_BREAKPOINT_MODIFIED: return "breakpoint_modified";break;
        case GdbComListener::AC_BREAKPOINT_DELETED: return "breakpoint_deleted";break;
        case GdbComListener::AC_THREAD_EXITED: return "thread_exited";break;
        case GdbComListener::AC_THREAD_GROUP_EXITED: return "thread_group_exited";break;
        case GdbComListener::AC_LIBRARY_UNLOADED: return "library_unloaded";break;
        case GdbComListener::AC_THREAD_SELECTED: return "thread_selected";break;
        case GdbComListener::AC_DOWNLOAD: return "download";break;
        case GdbComListener::AC_CMD_PARAM_CHANGED: return "cmd_param_changed";break;
        case GdbComListener::AC_UNKNOWN: return "unknown";break;

    };
    return "?";
}

    
const char *Token::toString()
{
    strcpy(m_tmpBuff, (const char*)stringToCStr(m_text));
        
    return m_tmpBuff;
}


const char *Token::typeToString(Type type)
{
    const char *str = "?";
    switch(type)
    {
        case UNKNOWN: str = "unknown";break;
        case C_STRING:str = "c_string";break;
        case C_CHAR: str = "c_char";break;
        case KEY_EQUAL:str = "=";break;
        case KEY_LEFT_BRACE:str = "{";break;
        case KEY_RIGHT_BRACE:str = "}";break;
        case KEY_LEFT_BAR:str = "[";break;
        case KEY_RIGHT_BAR:str = "]";break;
        case KEY_UP:str = "^";break;
        case KEY_PLUS:str = "+";break;
        case KEY_COMMA:str = ",";break;
        case KEY_TILDE:str = "~";break;
        case KEY_SNABEL:str = "@";break;
        case KEY_STAR:str = "*";break;
        case KEY_AND:str = "&";break;
        case END_CODE: str = "endcode";break;
        case VAR: str = "var";break;
    }
    return str;
}

    

bool Resp::isResult()
{
        return (m_type == RESULT) ? true : false;
}

        
GdbCom::GdbCom()
 : m_listener(NULL)
 ,m_logFile(GDB_LOG_FILE)
 ,m_busy(0)
 ,m_enableLog(false)
 {
/*
    QByteArray array = m_process.readAllStandardOutput();
    qDebug() << "--->>";
    qDebug() << QString(array);
    qDebug() << "---<<";
*/


    connect(&m_process, SIGNAL(readyReadStandardError ()), this, SLOT(onReadyReadStandardError()));

    connect(&m_process, SIGNAL(readyReadStandardOutput ()), this, SLOT(onReadyReadStandardOutput()));

    connect(&m_process, SIGNAL(stateChanged (QProcess::ProcessState)), this, SLOT(onGdbStateChanged(QProcess::ProcessState)));
}

GdbCom::~GdbCom()
{
    disconnect(&m_process, SIGNAL(stateChanged (QProcess::ProcessState)), this, SLOT(onGdbStateChanged(QProcess::ProcessState)));
    
    // Send the command to gdb to exit cleanly
    QString text = "-gdb-exit\n";
    m_process.write((const char*)text.toLatin1());

    if(!m_process.waitForFinished(1000))
    {
        m_process.terminate();

        m_process.waitForFinished();
    }

    // Free tokens
    while(!m_freeTokens.isEmpty())
    {
        Token *token = m_freeTokens.takeFirst();
        delete token;
    }

    enableLog(false);
    if(m_enableLog)
    {
        writeLogEntry("#\n");
        m_logFile.close();
    }
}


GdbResult GdbCom::commandF(Tree *resultData, const char *cmdFmt, ...)
{
      va_list ap;
    char buffer[1024];

    va_start(ap, cmdFmt);
     vsnprintf(buffer, sizeof(buffer), cmdFmt, ap);

    GdbResult res = command(resultData, buffer);
    va_end(ap);

    return res;
}



/**
 * @brief Creates tokens from a single GDB output row.
 */
QList<Token*> GdbCom::tokenize(QString str)
{
    enum { IDLE, END_CODE, STRING, VAR} state = IDLE;
    QList<Token*> list;
    Token *cur = NULL;
    bool prevCharIsEscCode = false;
    
    if(str.isEmpty())
        return list;

    for(int i = 0;i < str.size();i++)
    {
        QChar c = str[i];
        bool isEscaped = false;
        if(c == '\\' && prevCharIsEscCode)
        {
            prevCharIsEscCode = false;
        }
        else if(prevCharIsEscCode)
        {
            isEscaped = true;
            prevCharIsEscCode = false;
        }
        else if(c == '\\')
        {
            prevCharIsEscCode = true;
            continue;
        }
        
        switch(state)
        {
            case IDLE:
            {
                if(c == '"')
                {
                    cur = new Token(Token::C_STRING);
                    list.push_back(cur);
                    state = STRING;
                }
                else if(c == '(')
                {
                    cur = new Token(Token::END_CODE);
                    list.push_back(cur);
                    cur->m_text += c;
                    state = END_CODE;
                }
                else if(c == '=' || c == '{' || c == '}' || c == ',' ||
                    c == '[' || c == ']' || c == '+' || c == '^' ||
                    c == '~' || c == '@' || c == '&' || c == '*')
                {
                    Token::Type type = Token::UNKNOWN;
                    if(c == '=')
                        type = Token::KEY_EQUAL;
                    if(c == '{')
                        type = Token::KEY_LEFT_BRACE;
                    if(c == '}')
                        type = Token::KEY_RIGHT_BRACE;
                    if(c == '[')
                        type = Token::KEY_LEFT_BAR;
                    if(c == ']')
                        type = Token::KEY_RIGHT_BAR;
                    if(c == ',')
                        type = Token::KEY_COMMA;
                    if(c == '^')
                        type = Token::KEY_UP;
                    if(c == '+')
                        type = Token::KEY_PLUS;
                    if(c == '~')
                        type = Token::KEY_TILDE;
                    if(c == '@')
                        type = Token::KEY_SNABEL;
                    if(c == '&')
                        type = Token::KEY_AND;
                    if(c == '*')
                        type = Token::KEY_STAR;
                    cur = new Token(type);
                    list.push_back(cur);
                    cur->m_text += c;
                    state = IDLE;
                }
                else if( c != ' ')
                {
                    cur = new Token(Token::VAR);
                    list.push_back(cur);
                    cur->m_text = c;
                    state = VAR;
                }
                
            };break;
            case END_CODE:
            {
                QString codeEndStr = "(gdb)";
                cur->m_text += c;

                if(cur->m_text.length() == codeEndStr.length())
                {
                    state = IDLE;
                    
                }
                else if(cur->m_text.compare(codeEndStr.left(cur->m_text.length())) != 0)
                {
                    cur->setType(Token::VAR);
                    state = IDLE;
                }
                
            };break;
            case STRING:
            {
                if(c == '"' && isEscaped == false)
                {
                    state = IDLE;
                }
                else if(isEscaped)
                {
                    if(c == 'n')
                        cur->m_text += '\n';
                    else if(c == 't')
                        cur->m_text += '\t';
                    else
                        cur->m_text += c;
                }
                else
                    cur->m_text += c;
            };break;
            case VAR:
            {
                if(c == '=' || c == ',' || c == '{' || c == '}')
                {
                    i--;
                    cur->m_text = cur->m_text.trimmed();
                    state = IDLE;
                }
                else
                    cur->m_text += c;
            };break;
            
        }
        
    }
    if(cur)
    {
        if(cur->getType() == Token::VAR)
            cur->m_text = cur->m_text.trimmed();
    }
    return list;
}

Token* GdbCom::pop_token()
{
    if(m_list.empty())
        return NULL;
    Token *tok = m_list.takeFirst();
    m_freeTokens += tok;    
    //debugMsg(">%s", stringToCStr(tok->getString()));
    return tok;
}


Token* GdbCom::peek_token()
{
    readTokens();
    
    if(m_list.empty())
        return NULL;
        
    return m_list.first();
}


/**
 * @brief Parses 'ASYNC-OUTPUT'
 * @return 0 on success
 */
int GdbCom::parseAsyncOutput(Resp *resp, GdbComListener::AsyncClass *ac)
{
    Token *tokVar;
    int rc = 0;
    
    // Get the class
    tokVar = eatToken(Token::VAR);
    if(tokVar == NULL)
    {
        return -1;
    }
    QString acString = tokVar->getString();
    
    if(acString == "stopped")
    {
        *ac = GdbComListener::AC_STOPPED;
    }
    else if(acString == "running")
    {
        *ac = GdbComListener::AC_RUNNING;
    }
    else if(acString == "thread-created")
    {
        *ac = GdbComListener::AC_THREAD_CREATED;
    }
    else if(acString == "thread-group-added")
    {
        *ac = GdbComListener::AC_THREAD_GROUP_ADDED;
    }
    else if(acString == "thread-group-started")
    {
        *ac = GdbComListener::AC_THREAD_GROUP_STARTED;
    }
    else if(acString == "library-loaded")
    {
        *ac = GdbComListener::AC_LIBRARY_LOADED;
    }
    else if(acString == "breakpoint-modified")
    {
        *ac = GdbComListener::AC_BREAKPOINT_MODIFIED;
    }
    else if(acString == "breakpoint-deleted")
    {
        *ac = GdbComListener::AC_BREAKPOINT_DELETED;
    }
    else if(acString == "thread-exited")
    {
        *ac = GdbComListener::AC_THREAD_EXITED;
    }
    else if(acString == "thread-group-exited")
    {
        *ac = GdbComListener::AC_THREAD_GROUP_EXITED;
    }
    else if(acString == "library-unloaded")
    {
        *ac = GdbComListener::AC_LIBRARY_UNLOADED;
    }
    else if(acString == "thread-selected")
    {
        *ac = GdbComListener::AC_THREAD_SELECTED;
    }
    else if(acString == "download")
    {
        *ac = GdbComListener::AC_DOWNLOAD;
    }
    else if(acString == "cmd-param-changed")
    {
        *ac = GdbComListener::AC_CMD_PARAM_CHANGED;
    }
    else if(acString == "tsv-created" ||
            acString == "tsv-deleted" ||
            acString == "tsv-modified")
    {
        *ac = GdbComListener::AC_UNKNOWN;
    }
    else
    {
        warnMsg("Unexpected response '%s'", stringToCStr(acString));
        assert(0);
        *ac = GdbComListener::AC_UNKNOWN;
    }


    
    while(checkToken(Token::KEY_COMMA))
    {
        parseResult(resp->tree.getRoot());
    }

    return rc;
}


Resp* GdbCom::parseExecAsyncOutput()
{
    Token *tokVar;
    Resp *resp = NULL;

    // Parse 'token'
    tokVar = checkToken(Token::VAR);
    if(tokVar)
    {
    }

    if(checkToken(Token::KEY_STAR) == NULL)
        return NULL;

    resp = new Resp;
    resp->setType(Resp::EXEC_ASYNC_OUTPUT);

    parseAsyncOutput(resp, &resp->reason);

    return resp;
}



Resp *GdbCom::parseStatusAsyncOutput()
{
    Resp *resp = NULL;
    Token *tokVar;

    // Parse 'token'
    tokVar = checkToken(Token::VAR);
    if(tokVar)
    {
    }
    
    if(checkToken(Token::KEY_PLUS) == NULL)
        return NULL;

    resp = new Resp;
    resp->setType(Resp::STATUS_ASYNC_OUTPUT);
    
    parseAsyncOutput(resp, &resp->reason);
    return resp;
}


Resp *GdbCom::parseNotifyAsyncOutput()
{
    Resp *resp = NULL;
    Token *tokVar;

    // Parse 'token'
    tokVar = checkToken(Token::VAR);
    if(tokVar)
    {
    }


    if(checkToken(Token::KEY_EQUAL) == NULL)
        return NULL;

    resp = new Resp;
    resp->setType(Resp::NOTIFY_ASYNC_OUTPUT);

    parseAsyncOutput(resp, &resp->reason);
    return resp;
}

    

Resp *GdbCom::parseAsyncRecord()
{
    Resp *resp = NULL;
    if(isTokenPending() && resp == NULL)
        resp = parseExecAsyncOutput();
    if(isTokenPending() && resp == NULL)
        resp = parseStatusAsyncOutput();
    if(isTokenPending() && resp == NULL)
        resp = parseNotifyAsyncOutput();
    return resp;
}



Resp *GdbCom::parseStreamRecord()
{
    Resp *resp = NULL;
    Token *tok;
    if(checkToken(Token::KEY_TILDE))
    {
        resp = new Resp;
        tok = eatToken(Token::C_STRING);

        resp->setType(Resp::CONSOLE_STREAM_OUTPUT);
        resp->setString(tok->getString());
        
    }
    else if(checkToken(Token::KEY_SNABEL))
    {
        resp = new Resp;
        tok = eatToken(Token::C_STRING);

        resp->setType(Resp::TARGET_STREAM_OUTPUT);
        resp->setString(tok->getString());
        
    }
    else if(checkToken(Token::KEY_AND))
    {
        resp = new Resp;
        tok = eatToken(Token::C_STRING);

        resp->setType(Resp::LOG_STREAM_OUTPUT);
        resp->setString(tok->getString());
        
    }

    return resp;
}



Token* GdbCom::eatToken(Token::Type type)
{
    Token *tok = peek_token();
    while(tok == NULL)
    {
        m_process.waitForReadyRead(100);
        readTokens();
        tok = peek_token();
    }
    if(tok == NULL || tok->getType() != type)
    {
        errorMsg("Expected '%s' but got '%s'",
            Token::typeToString(type), tok ? stringToCStr(tok->getString()) : "<NULL>");
        return NULL;
    }
    pop_token();
    return tok;
}


/**
 * @brief Checks if the read queue is empty.
 */
bool GdbCom::isTokenPending()
{
    Token *tok = peek_token();
    if(tok == NULL)
    {
        return false;
    }
    else
        return true;
}


/**
 * @brief Checks and pops a token if the kind is as expected.
 * @return The found token or NULL if no hit.
 */
Token* GdbCom::checkToken(Token::Type type)
{
    Token *tok = peek_token();
    if(tok == NULL)
        readTokens();
    if(tok == NULL || tok->getType() != type)
    {
        return NULL;
    }
    pop_token();
    return tok;
}


/**
 * @brief Parses 'VALUE'
 * @param item   The tree item to put the result of the parse in.
 * @return 0 on success.
 */
int GdbCom::parseValue(TreeNode *item)
{
    Token *tok;
    int rc = 0;

    tok = pop_token();

    // Const?
    if(tok->getType() == Token::C_STRING)
    {
        item->setData(tok->getString());
    }
    // Tuple?
    else if(tok->getType() == Token::KEY_LEFT_BRACE)
    {
        do
        {
            parseResult(item);
        } while(checkToken(Token::KEY_COMMA) != NULL);

        if(eatToken(Token::KEY_RIGHT_BRACE) == NULL)
            return -1;
        
    }
    // List?
    else if(tok->getType() == Token::KEY_LEFT_BAR)
    {
        if(checkToken(Token::KEY_RIGHT_BAR) != NULL)
        {
            return 0;
        }
        
        tok = peek_token();
        if(tok->getType() == Token::VAR)
        {
            do
            {
                
                parseResult(item);

            } while(checkToken(Token::KEY_COMMA) != NULL);
        }
        else
        {
            int idx = 1;
            QString name;

            do
            {
                name.sprintf("%d", idx++);
                TreeNode *node = new TreeNode(name);
                item->addChild(node); 
                rc = parseValue(node);
            } while(checkToken(Token::KEY_COMMA) != NULL);
            
        }
    
        if(eatToken(Token::KEY_RIGHT_BAR) == NULL)
            return -1;
        
        
    }
    else
        errorMsg("Unexpected token: '%s'", stringToCStr(tok->getString()));
    return rc;
}


/**
 * @brief Parses 'RESULT'
 * @return 0 on success.
 */
int GdbCom::parseResult(TreeNode *parent)
{
    QString name;

    Token *tok = peek_token();
    if(tok != NULL && tok->getType() == Token::KEY_LEFT_BRACE)
    {
    }
    else
    {
        //
        Token *tokVar = eatToken(Token::VAR);
        if(tokVar == NULL)
            return -1;
        name = tokVar->getString();
        
        //
        if(eatToken(Token::KEY_EQUAL) == NULL)
            return -1;
    }

    TreeNode *item = new TreeNode(name);
    parent->addChild(item);
        
    parseValue(item);
    
    return 0;
}


Resp *GdbCom::parseResultRecord()
{
    Token *tokVar;
    Resp *resp = NULL;
    int rc = 0;
    
    // Parse 'token'
    tokVar = checkToken(Token::VAR);
    if(tokVar)
    {
    }

    // Parse '^'
    if(checkToken(Token::KEY_UP) == NULL)
        return NULL;


    
    // Parse 'result class'
    Token *tok = eatToken(Token::VAR);
    if(tok == NULL)
        return NULL;

    resp = new Resp;
    QString resultClass = tok->getString();
    GdbResult res;
    if(resultClass == "done")
        res = GDB_DONE;
    else if(resultClass == "running")
        res = GDB_RUNNING;
    else if(resultClass == "connected")
        res = GDB_CONNECTED;
    else if(resultClass == "error")
        res = GDB_ERROR;
    else if(resultClass == "exit")
        res = GDB_EXIT;
    else
    {
        delete resp;
        errorMsg("Invalid result class found: %s", stringToCStr(resultClass));
        return NULL;
    }
    resp->m_result = res;
    

    
    while(checkToken(Token::KEY_COMMA) != NULL && rc == 0)
    {
        rc = parseResult(resp->tree.getRoot());
    }

    if(!m_pending.isEmpty())
    {
        PendingCommand cmd = m_pending.takeFirst();

        debugMsg("%s done", stringToCStr(cmd.m_cmdText));
    }

    resp->setType(Resp::RESULT);

    return resp;
}



Resp* GdbCom::parseOutOfBandRecord()
{
    Resp *resp = NULL;
    
    if(isTokenPending() && resp == NULL)
        resp = parseAsyncRecord();
    if(isTokenPending() && resp == NULL)
        resp = parseStreamRecord();

    return resp;
}

    
Resp *GdbCom::parseOutput()
{
    Resp *resp = NULL;
    
    if(isTokenPending())
        resp = parseOutOfBandRecord();

    if(isTokenPending() && resp == NULL)
        resp = parseResultRecord();
            
    if(isTokenPending() && resp == NULL)
    {
        resp = new Resp;
        Token *token = checkToken(Token::END_CODE);
        if(token)
        {
            resp->setType(Resp::TERMINATION);
        }
    }


/*
    token = peek_token();
    if(token)
        errorMsg("Unexpected token '%s'", stringToCStr(token->getString()));
*/
    return resp;
}


void GdbCom::writeLogEntry(QString logText)
{
    assert(m_enableLog == true);

    // 
    struct timeval tv;
    gettimeofday(&tv, NULL);
    QString timeStr;
    timeStr.sprintf("%02d.%03d", (int)(tv.tv_sec%100), (int)(tv.tv_usec/1000));
    
    QString fullText = timeStr + "|" + logText;
    
    m_logFile.write(fullText.toUtf8());
    m_logFile.flush();
}
               
/**
 * @brief Reads output from GDB.
 */
void GdbCom::readTokens()
{
    m_inputBuffer += m_process.readAllStandardOutput();

    // Any characters received?
    while(m_inputBuffer.size() > 0)
    {
        // Newline received?
        int subLen = m_inputBuffer.indexOf('\n');
        if(subLen != -1)
        {
            QString row = QString(m_inputBuffer.left(subLen));
            m_inputBuffer = m_inputBuffer.mid(subLen+1);

            if(!row.isEmpty())
            {
                debugMsg("row:%s", stringToCStr(row));
     
                if(m_enableLog)
                {
                    QString logText;
                    logText = ">> ";
                    logText += row;
                    logText += "\n";
                    writeLogEntry(logText);
                }

                QList<Token*> list;
                char firstChar = row[0].toLatin1();
                if(firstChar == '(' ||
                    firstChar == '^' ||
                    firstChar == '*' ||
                    firstChar == '+' ||
                    firstChar == '~' ||
                    firstChar == '@' ||
                    firstChar == '&' ||
                    firstChar == '=')
                {
                    list = tokenize(row);
                    m_list += list;
                }
                else if(m_listener)
                {
                    m_listener->onTargetStreamOutput(row);
                }
            }
        }

        // Half a line received?
        if(m_inputBuffer.isEmpty() == false)
        {
            int timeout = 20;
            // Wait for the complete line to be received
            while(m_inputBuffer.indexOf('\n') == -1)
            {
                m_process.waitForReadyRead(100);
                m_inputBuffer += m_process.readAllStandardOutput();
                timeout--;
                assert(timeout > 0);
            }
        }

    }
}


/**
 * @brief Reads response from GDB
 * @return 0 on success otherwise an errorcode.
 */
int GdbCom::readFromGdb(GdbResult *m_result, Tree *m_resultData)
{
    int rc = 0;
    //debugMsg("## '%s'",stringToCStr(row));

    Resp *resp = NULL;

    
    if(m_result == NULL)
    {
        
        // Parse any data received from GDB
        resp = parseOutput();
        if(resp == NULL)
        {
            if(!m_process.waitForReadyRead(100))
            {
                QProcess::ProcessState  state = m_process.state();
                if(state == QProcess::NotRunning)
                {
                    rc = -1;
                }
            }
        }
        
        while(!m_freeTokens.isEmpty())
        {
            Token *token = m_freeTokens.takeFirst();
            delete token;
        }

        if(resp)
        {
            m_respQueue.push_back(resp);

            if(resp->getType() == Resp::RESULT)
            {
                assert(m_resultData != NULL);
            
                m_resultData->copy(resp->tree);
            }
        }

    }
    else
    {
        *m_result = GDB_DONE;
    
    do
    {
        
        // Parse any data received from GDB
        do
        {
            resp = parseOutput();
            if(resp == NULL)
            {
                if(!m_process.waitForReadyRead(100))
                {
                    QProcess::ProcessState  state = m_process.state();
                    if(state == QProcess::NotRunning)
                    {
                        assert(0);
                        
                        rc = -1;
                    }
                }
            }
        }while(resp == NULL && rc == 0);
       
        
        while(!m_freeTokens.isEmpty())
        {
            Token *token = m_freeTokens.takeFirst();
            delete token;
        }


        if(resp != NULL)
            m_respQueue.push_back(resp);

        
        if(resp != NULL && resp->getType() == Resp::RESULT)
        {
            assert(m_resultData != NULL);
            
            if(m_result)
                *m_result = resp->m_result;
            m_resultData->copy(resp->tree);
        }

    }while(m_result != NULL && resp != NULL && resp->getType() != Resp::TERMINATION);

    if(resp == NULL && m_result != NULL)
        *m_result = GDB_ERROR;
}

    //  debugMsg("# ---<< \n");


//for(int i = 0;i < m_respQueue.size();i++)
{
//    debugMsg("%d > %s", i, stringToCStr(m_respQueue[i].getString()));
}

 

    return rc;
}


GdbResult GdbCom::command(Tree *resultData, QString text)
{
    Tree resultDataNull;
    int rc = 0;

    assert(m_busy == 0);
    
    m_busy++;
    
    if(resultData == NULL)
        resultData = &resultDataNull;
    
    debugMsg("# Cmd: '%s'", stringToCStr(text));

    GdbResult result;
    
    assert(resultData != NULL);

    resultData->removeAll();

    
    //
    PendingCommand cmd;
    cmd.m_cmdText = text;
    m_pending.push_back(cmd);

    // Send the command to gdb
    text += "\n";
    QByteArray wtext = text.toLatin1();
    m_process.write(wtext);


    if(m_enableLog)
    {
        //
        QString logText;
        writeLogEntry("\n");
        logText = "<< ";
        logText += text;
        writeLogEntry(logText);
    }
    

    do
    {
        if(readFromGdb(&result,resultData))
        {
                rc = -1;
        }
    }while(!m_pending.isEmpty() && rc == 0);

    
    while(!m_list.isEmpty())
    {
        readFromGdb(NULL,resultData);
    }
     

    m_busy--;

    
    dispatchResp();

    onReadyReadStandardOutput();

    if(rc)
        return GDB_ERROR;
    return result;
}


/**
 * @brief Starts gdb
 * @return 0 on success and gdb was started.
 */
int GdbCom::init(QString gdbPath, bool enableDebugLog)
{
    QString commandLine;

    enableLog(enableDebugLog);

        
    commandLine.sprintf("%s --interpreter=mi2", stringToCStr(gdbPath));

    if(m_enableLog)
    {
        QString logStr;
        logStr = "# Gdb commandline: '" + commandLine + "'\n";
        writeLogEntry(logStr);
    }

    // Make sure that gdb understands that we can handle color output
    setenv("TERM", "xterm",1);

    m_process.start(commandLine);
    m_process.waitForStarted(6000);

    if(m_process.state() == QProcess::NotRunning)
    {
        assert(0);
        return 1;
    }

    return 0;
}


int GdbCom::getPid()
{
    return m_process.pid();
}


GdbCom& GdbCom::getInstance()
{
    static GdbCom core;
    return core;
}



void GdbCom::onReadyReadStandardError ()
{
    // Dump all stderr content
    QByteArray stderrBuffer = m_process.readAllStandardError();
    if(!stderrBuffer.isEmpty())
    {
        QString respString = QString(stderrBuffer);
        QStringList respList = respString.split("\n");
        for(int r = 0;r < respList.size();r++)
        {
            QString row = respList[r];
            if(!row.isEmpty())
                debugMsg("GDB|E>%s", stringToCStr(row));
        }
    }


}

void GdbCom::onReadyReadStandardOutput ()
{
    if(m_busy != 0)
        return;


    while(m_process.bytesAvailable() || m_list.isEmpty() == false)
    {
        Tree resultDataNull;
        readFromGdb(NULL, &resultDataNull);

        assert(m_pending.isEmpty() == true);
    }
    
    dispatchResp();

}


void GdbCom::dispatchResp()
{
    
    // Dispatch the response
    while(!m_respQueue.isEmpty())
    {
        Resp *resp = m_respQueue.takeFirst();
        assert(resp != NULL);
        // Dispatch the response
        if(m_listener)
        {
            if(resp->getType() == Resp::EXEC_ASYNC_OUTPUT)
                m_listener->onExecAsyncOut(resp->tree, resp->reason);
            if(resp->getType() == Resp::STATUS_ASYNC_OUTPUT)
                m_listener->onStatusAsyncOut(resp->tree, resp->reason);
            if(resp->getType() == Resp::NOTIFY_ASYNC_OUTPUT)
                m_listener->onNotifyAsyncOut(resp->tree,resp->reason);
            if(resp->getType() == Resp::LOG_STREAM_OUTPUT)
                m_listener->onLogStreamOutput(resp->getString());
            if(resp->getType() == Resp::TARGET_STREAM_OUTPUT)
                m_listener->onTargetStreamOutput(resp->getString());
            if(resp->getType() == Resp::CONSOLE_STREAM_OUTPUT)
                m_listener->onConsoleStreamOutput(resp->getString());
            if(resp->getType() == Resp::RESULT)
                m_listener->onResult(resp->tree);
        }
        delete resp;
    }

}

void GdbCom::enableLog(bool enable)
{
    if(m_enableLog == enable)
        return;

    if(m_enableLog)
    {
        writeLogEntry("#\n");
        QDateTime now = QDateTime::currentDateTime();
        QString logStr;
        logStr = "# Closed: " + now.toString("yyyy-MM-dd hh:mm:ss") + "\n";
        writeLogEntry(logStr);
        m_logFile.close();
    }
    m_enableLog = false;

    if(enable)
    {
        if(m_logFile.open(QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text))
        {
            infoMsg("Created %s", (const char*)GDB_LOG_FILE);

            m_enableLog = true;

            QString logStr;
            QDateTime now = QDateTime::currentDateTime();
            logStr = "# Created: " + now.toString("yyyy-MM-dd hh:mm:ss") + "\n";
            writeLogEntry(logStr);
            logStr.sprintf("# Gede version: %d.%d.%d \n", GD_MAJOR,GD_MINOR,GD_PATCH);
            writeLogEntry(logStr);

            QString distroDesc;
            detectDistro(NULL, &distroDesc);
            logStr = "# Host: " + distroDesc + "\n";
            writeLogEntry(logStr);

        }
        else
            critMsg("Failed to create log file %s", (const char*)GDB_LOG_FILE);
        
    }
}

void GdbCom::onGdbStateChanged ( QProcess::ProcessState newState )
{
    if(m_enableLog)
    {
        QString logText;
        logText = "# GDB process state changed to ";
        switch(newState)
        {
            case QProcess::NotRunning:
                logText += "'not running'";break;
            case QProcess::Running:
                logText += "'running'";break;
            case QProcess::Starting:
                logText += "'starting'";break;
            default:
                logText += "?";break;
        };
        logText += "\n";
        writeLogEntry(logText);
    }
    if(newState == QProcess::NotRunning)
    {
        critMsg("GDB unexpected terminated");
    }
}


    
        
