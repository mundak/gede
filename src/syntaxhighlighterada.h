/*
 * Copyright (C) 2018 Johan Henriksson.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#ifndef  FILE__SYNTAXHIGHLIGHTERADA_H
#define  FILE__SYNTAXHIGHLIGHTERADA_H


#include <QVector>
#include <QString>
#include <QColor>
#include <QHash>

#include "settings.h"
#include "syntaxhighlighter.h"



class SyntaxHighlighterAda : public SyntaxHighlighter
{
public:
    SyntaxHighlighterAda();
    virtual ~SyntaxHighlighterAda();
    
    void colorize(QString text);

    QVector<TextField*> getRow(unsigned int rowIdx);
    unsigned int getRowCount() { return m_rows.size(); };
    void reset();

    bool isKeyword(QString text) const;
    bool isSpecialChar(char c) const;
    bool isSpecialChar(TextField *field) const;
    void setConfig(Settings *cfg);

private:
    class Row
    {
    public:
        Row();

        TextField *getLastNonSpaceField();
        void appendField(TextField* field);
        int getCharCount();
        
        QVector<TextField*>  m_fields;
    };
private:
    void pickColor(TextField *field);

private:
    Settings *m_cfg;
    QVector <Row*> m_rows;
    QHash <QString, bool> m_keywords;
};

#endif // #ifndef FILE__SYNTAXHIGHLIGHTER_H
