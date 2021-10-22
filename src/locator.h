#ifndef FILE__LOCATOR_H
#define FILE__LOCATOR_H

#include <QString>
#include <QVector>

#include "tagmanager.h"

class Location
{
public:
    Location(QString filename_, int lineno_);
    Location() {};

    void dump();

public:
    QString m_filename;
    int m_lineNo;
};

class Locator
{
public:
    Locator(TagManager *mgr, QList<FileInfo> *sourceFiles);
    virtual ~Locator();
    
    void setCurrentFile(QString filename);
    QVector<Location> locate(QString expr);
    QVector<Location> locateFunction(QString name);

    QStringList searchExpression(QString expressionStart);
     
    QStringList searchExpression(QString filename, QString expressionStart);

private:
    QStringList findFile(QString defFilename);
    
public:
    TagManager *m_mgr;
    QString m_currentFilename;
    QList<FileInfo> *m_sourceFiles;
};

#endif // FILE__LOCATOR_H

