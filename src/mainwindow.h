/*
 * Copyright (C) 2014-2017 Johan Henriksson.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#ifndef FILE__MAINWINDOW_H
#define FILE__MAINWINDOW_H

#include <QMainWindow>
#include <QApplication>
#include <QMap>
#include <QLabel>
#include <QRegExp>

#include "consolewidget.h"
#include "ui_mainwindow.h"
#include "core.h"
#include "codeview.h"
#include "settings.h"
#include "tagscanner.h"
#include "autovarctl.h"
#include "watchvarctl.h"
#include "codeviewtab.h"
#include "tagmanager.h"
#include "log.h"


class FileInfo
{
public:
    QString m_name; //!< The name of the file (Eg: "main.c").
    QString m_fullName; //!< The full path (Eg: "/a/dir/main.c").
};

#include "locator.h"


class MainWindow : public QMainWindow, public ICore, public ICodeView, public ILogger
{
  Q_OBJECT
public:
    MainWindow(QWidget *parent);
    virtual ~MainWindow();

    CodeViewTab* open(Location loc);
    CodeViewTab* open(QString filename);
    CodeViewTab* open(QString filename, int lineNo);

public:
    void insertSourceFiles();
    void setStatusLine(Settings &cfg);
    
public:
    void ICore_onStopped(ICore::StopReason reason, QString path, int lineNo);
    void ICore_onLocalVarChanged(QStringList varNames);
    void ICore_onWatchVarChanged(VarWatch &watch);
    void ICore_onConsoleStream(QString text);
    void ICore_onBreakpointsChanged();
    void ICore_onThreadListChanged();
    void ICore_onCurrentThreadChanged(int threadId);
    void ICore_onStackFrameChange(QList<StackFrameEntry> stackFrameList);
    void ICore_onFrameVarReset();
    void ICore_onFrameVarChanged(QString name, QString value);
    void ICore_onMessage(QString message);
    void ICore_onCurrentFrameChanged(int frameIdx);
    void ICore_onSignalReceived(QString sigtype);
    void ICore_onTargetOutput(QString msg);
    void ICore_onStateChanged(TargetState state);
    void ICore_onSourceFileListChanged();
    void ICore_onSourceFileChanged(QString filename);

    void ICodeView_onRowDoubleClick(int lineNo);
    void ICodeView_onContextMenu(QPoint pos, int lineNo, QStringList text);
    void ICodeView_onContextMenuIncFile(QPoint pos, int lineNo, QString incFile);
    
    void ICore_onWatchVarChildAdded(VarWatch &watch);
    void ICore_onWatchVarDeleted(VarWatch &watch);
    
    
    
    void ILogger_onWarnMsg(QString text);
    void ILogger_onErrorMsg(QString text);
    void ILogger_onInfoMsg(QString text);
    void ILogger_onCriticalMsg(QString text);

    
private:
    void showEvent(QShowEvent *);
    void closeEvent(QCloseEvent *e);

    void showWidgets();
    void fillInFuncList();
    void fillInClassList();
    
public:
        
private:
    void setConfig();
    
    void wrapSourceTree(QTreeWidget *treeWidget);

    QTreeWidgetItem *addTreeWidgetPath(QTreeWidget *treeWidget, QTreeWidgetItem *parent, QString path);
    void fillInStack();

    bool eventFilter(QObject *obj, QEvent *event);
    void loadConfig();
    QTreeWidgetItem *insertTreeWidgetItem(
                    VarCtl::DispInfoMap *map,
                    QString fullPath,
                    QString name,
                    QString value);
    void addVariableDataTree(
                QTreeWidget *treeWidget,
                VarCtl::DispInfoMap *map,
                QTreeWidgetItem *item, TreeNode *rootNode);

    CodeViewTab* findTab(QString filename);
    CodeViewTab* createTab(QString filename);
    CodeViewTab* currentTab();
    void updateCurrentLine(QString filename, int lineno);
    void onCurrentLineChanged(int lineno);
    void onCurrentLineDisabled();
    void hideSearchBox();


public slots:
    void onFuncFilter_textChanged(const QString &text);
    void onFuncFilterClear();
    void onFuncFilterCheckBoxStateChanged(int state);
    void onClassFilterCheckBoxStateChanged(int state);
    void onClassFilterClear();
    void onClassFilter_textChanged(const QString &text);

    void onIncSearch_textChanged(const QString &text);
    void onFolderViewItemActivated ( QTreeWidgetItem * item, int column );
    void onThreadWidgetSelectionChanged( );
    void onStackWidgetSelectionChanged();
    void onQuit();
    void onNext();
    void onStepIn();
    void onStepOut();
    void onAbout();
    void onSearch();
    void onSearchCheckBoxStateChanged(int state);
    void onSearchNext();
    void onSearchPrev();
    void onGoToLine();
    void onGoToMain();
    void onStop();
    void onBreakpointsWidgetItemDoubleClicked(QTreeWidgetItem * item,int column);
    void onRestart();
    void onContinue();
    void onCodeViewContextMenuAddWatch();
    void onCodeViewContextMenuOpenFile();
    void onCodeViewContextMenuShowDefinition();
    void onCodeViewContextMenuShowCurrentLocation();
    void onSettings();
    void onCodeViewContextMenuToggleBreakpoint();
    void onCodeViewTab_tabCloseRequested ( int index );
    void onCodeViewTab_currentChanged( int tabIdx);
    void onCodeViewTab_launchContextMenu(const QPoint&);
    void onCodeViewTab_closeTabsToLeft();
    void onCodeViewTab_closeOtherTabs();
    void onCodeViewContextMenuJumpToLocation();

    void onViewStack();
    void onViewBreakpoints();
    void onViewThreads();
    void onViewWatch();
    void onViewAutoVariables();
    void onViewTargetOutput();
    void onViewGedeOutput();
    void onViewGdbOutput();
    void onViewFileBrowser();
    void onViewFuncFilter();
    void onViewClassFilter();
    void onDefaultViewSetup();

    void onBreakpointsRemoveSelected();
    void onBreakpointsRemoveAll();
    void onBreakpointsGoTo();
    void onBreakpointsWidgetContextMenu(const QPoint& pt);

    void onAllTagScansDone();
    void onFuncWidgetItemSelected(QTreeWidgetItem * item, int column);
    void onClassWidgetItemSelected(QTreeWidgetItem * item, int column);

    
    void onNewInfoMsg(QString text);
    void onNewWarnMsg(QString text);
    void onNewErrorMsg(QString text);
    void onNewCritMsg(QString text);

signals:
    void newInfoMsg(QString text);
    void newWarnMsg(QString text);
    void newErrorMsg(QString text);
    void newCritMsg(QString text);
    
private:
    QByteArray m_gui_default_mainwindowState;
    QByteArray m_gui_default_mainwindowGeometry;
    QByteArray m_gui_default_splitter1State;
    QByteArray m_gui_default_splitter2State;
    QByteArray m_gui_default_splitter3State;
    QByteArray m_gui_default_splitter4State;

private:
    Ui_MainWindow m_ui;
    QIcon m_fileIcon;
    QIcon m_folderIcon;
    QString m_currentFile; //!< The file which the program counter points to.
    int m_currentLine; //!< The linenumber (first=1) which the program counter points to.
    QList<StackFrameEntry> m_stackFrameList;
    QMenu m_popupMenu;
    QVector<QRegExp> m_funcFilterText; //!< Filter for the function list.
    QVector<QRegExp> m_classFilterText; //!< Filter for the class list.

    
    Settings m_cfg;
    TagManager m_tagManager;
    QList<FileInfo> m_sourceFiles;
    QList<Tag> m_tagList; // Current list of tags
    
    AutoVarCtl m_autoVarCtl;
    WatchVarCtl m_watchVarCtl;
    QFont m_outputFont;
    QFont m_gdbOutputFont;
    QFont m_gedeOutputFont;
    QLabel m_statusLineWidget;
    Locator m_locator;
};


#endif


