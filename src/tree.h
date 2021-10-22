/*
 * Copyright (C) 2014-2017 Johan Henriksson.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#ifndef FILE__TREE_H
#define FILE__TREE_H

#include <QString>
#include <QVector>
#include <QList>
#include <QStringList>
#include <QHash>


class TreeNode
{
public:
    TreeNode();
    TreeNode(QString name);
    virtual ~TreeNode();
    
    TreeNode *findChild(QString path) const;

    void addChild(TreeNode *child);
    TreeNode *getChild(int i) const { return m_children[i]; };
    int getChildCount() const { return m_children.size(); };
    QString getData() const { return m_data; };
    int getDataInt(int defaultValue = 0) const;

    QString getChildDataString(QString childName) const;
    int getChildDataInt(QString path, int defaultValue = 0) const;
    long long getChildDataLongLong(QString path, long long defaultValue = 0) const;

    void setData(QString data) { m_data = data; };
    void dump();

    QString getName() const { return m_name; };

    void removeAll();


    void copy(const TreeNode &other);
private:
    void dump(int parentCnt);


private:
    TreeNode *m_parent;
    QString m_name;
    QString m_data;
    QVector<TreeNode*> m_children;
    QHash<QString, TreeNode*> m_childMap;
    
private:
    TreeNode(const TreeNode &) { };

};


class Token;


class Tree
{
public:
    Tree();
    


    void dump() { m_root.dump();};

    
    QString getString(QString path) const;
    int getInt(QString path, int defaultValue = 0) const;
    long long getLongLong(QString path) const;

    TreeNode *getChildAt(int idx) { return m_root.getChild(idx);};
    int getRootChildCount() const { return m_root.getChildCount();};
    
    TreeNode* findChild(QString path) const;

    TreeNode* getRoot() { return &m_root; };
    void copy(const Tree &other);

    void removeAll();
    
private:

    TreeNode fromStringToTree(QString str);
    QList<Token*> tokenize(QString str);
    Token* pop_token();
    Token* peek_token();
    
    void parseGroup(TreeNode *parentNode);
    void parseVar(TreeNode *parentNode);
    void parseVarList(TreeNode *parentNode);
    void parseData(TreeNode *ownerNode);
    void parseDataList(TreeNode *ownerNode);

private:
    Tree(const Tree &) {}; 
private:

    TreeNode m_root;
};

#endif // FILE__TREE_H
