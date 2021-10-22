/*
 * Copyright (C) 2014-2021 Johan Henriksson.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#include "opendialog.h"

#include "version.h"
#include "log.h"
#include "util.h"
#include "processlistdialog.h"

#include <QFileDialog>
#include <QDir>
#include <QtGlobal>


OpenDialog::OpenDialog(QWidget *parent)
    : QDialog(parent)
{
    
    m_ui.setupUi(this);

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    m_ui.comboBox_projDir->setCurrentText(QDir::currentPath());
#else
    m_ui.comboBox_projDir->setEditText(QDir::currentPath());
#endif

    connect(m_ui.pushButton_runningPid, SIGNAL(clicked()), SLOT(onSelectRunningPid()));
    connect(m_ui.pushButton_selectFile, SIGNAL(clicked()), SLOT(onSelectProgram()));
    connect(m_ui.pushButton_selectCoreFile, SIGNAL(clicked()), SLOT(onSelectCoreFile()));
    
    connect(m_ui.radioButton_runningProgram, SIGNAL(toggled(bool)), SLOT(onConnectionTypePid(bool)));
    connect(m_ui.radioButton_localProgram, SIGNAL(toggled(bool)), SLOT(onConnectionTypeLocal(bool)));
    connect(m_ui.radioButton_gdbServerTcp, SIGNAL(toggled(bool)), SLOT(onConnectionTypeTcp(bool)));
    connect(m_ui.radioButton_openCoreDump, SIGNAL(toggled(bool)), SLOT(onConnectionTypeCoreDump(bool)));

    connect(m_ui.comboBox_projDir, SIGNAL(currentIndexChanged(int)), SLOT(onProjDirComboChanged(int)));

    m_ui.comboBox_gdbCommand->setSearchAreas(ExeComboBox::UseEnvPath);
    m_ui.comboBox_gdbCommand->setFilter(QRegExp("gdb(?!tui|server)"));

#if QT_VERSION >= QT_VERSION_CHECK(5,3,0)
    m_ui.plainTextEdit_initCommands->setPlaceholderText("Example: \"set substitute-path '/src' '/src2' # a comment\"");
#endif
}


void OpenDialog::setMode(ConnectionMode mode)
{
    m_ui.radioButton_localProgram->setChecked(false);
    m_ui.radioButton_gdbServerTcp->setChecked(false);
    m_ui.radioButton_openCoreDump->setChecked(false);
    m_ui.radioButton_runningProgram->setChecked(false);
    onConnectionTypeLocal(false);
    onConnectionTypeTcp(false);
    onConnectionTypeCoreDump(false);
    onConnectionTypePid(false);
    
    if(mode == MODE_TCP)
    {
        m_ui.radioButton_gdbServerTcp->setChecked(true);
        onConnectionTypeTcp(true);        
    }
    else if(mode == MODE_COREDUMP)
    {
        m_ui.radioButton_openCoreDump->setChecked(true);
        onConnectionTypeCoreDump(true);
    }
    else if(mode == MODE_PID)
    {
        m_ui.radioButton_runningProgram->setChecked(true);
        onConnectionTypePid(true);
    }
    else // if(mode == MODE_LOCAL)
    {
        m_ui.radioButton_localProgram->setChecked(true);
        onConnectionTypeLocal(true);
    }
    
}

ConnectionMode OpenDialog::getMode()
{
    if(m_ui.radioButton_gdbServerTcp->isChecked())    
        return MODE_TCP;
    else if(m_ui.radioButton_openCoreDump->isChecked())    
        return MODE_COREDUMP;
    else if(m_ui.radioButton_runningProgram->isChecked())    
        return MODE_PID;
    else
        return MODE_LOCAL;
}

QString OpenDialog::getCoreDumpFile()
{
    return m_ui.lineEdit_coreFile->text();
}



QString OpenDialog::getProgram()
{
    return m_ui.lineEdit_program->text();
}





int OpenDialog::getRunningPid()
{
    return m_ui.lineEdit_pid->text().toInt();
}

void OpenDialog::setRunningPid(int pid)
{
    return m_ui.lineEdit_pid->setText(QString::number(pid));
}

QString OpenDialog::getArguments()
{
    return m_ui.lineEdit_arguments->text();
}
    


void OpenDialog::setCoreDumpFile(QString coreDumpFile)
{
    m_ui.lineEdit_coreFile->setText(coreDumpFile);
}

void OpenDialog::setProgram(QString program)
{
    m_ui.lineEdit_program->setText(program);
}

void OpenDialog::setInitCommands(QStringList commandList)
{
    QString str;
    str = commandList.join("\n");
    m_ui.plainTextEdit_initCommands->setPlainText(str);
}


QStringList OpenDialog::getInitCommands()
{
    return m_ui.plainTextEdit_initCommands->toPlainText().split("\n");
}    

void OpenDialog::setArguments(QString arguments)
{
    m_ui.lineEdit_arguments->setText(arguments);

}

void OpenDialog::onBrowseForProgram(QString *path)
{
    // Get start dir
    QString startPath = *path;
    if(!startPath.isEmpty())
    {
        dividePath(startPath, NULL, &startPath);
    }
    else
        startPath = QDir::currentPath();
        
    // Open dialog
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Select Program"), startPath, tr("All Files (*)"));
    if(!fileName.isEmpty())
        *path = fileName;
}


void OpenDialog::onSelectProgram()
{
    QString path = m_ui.lineEdit_program->text();

    onBrowseForProgram(&path);
    
    // Fill in the selected path
    m_ui.lineEdit_program->setText(path);

    //
    QString coreFilePath = m_ui.lineEdit_coreFile->text();
    if(coreFilePath.isEmpty())
    {
        QString filename;
        QString folderPath;
        
        dividePath(path, &filename, &folderPath);
        
        m_ui.lineEdit_coreFile->setText(folderPath + "/core");
    }

}



void OpenDialog::onSelectRunningPid()
{
    
    ProcessListDialog dlg;
    dlg.selectPid(m_ui.lineEdit_pid->text().toInt());
    if(dlg.exec() == QDialog::Accepted)
    {
        ProcessInfo selectedProcess = dlg.getSelectedProcess();
        m_ui.lineEdit_pid->setText( QString::number(selectedProcess.getPid()));

        m_ui.lineEdit_program->setText(selectedProcess.getExePath());
    }
        
}



void OpenDialog::onSelectCoreFile()
{
    QString path = m_ui.lineEdit_coreFile->text();

    onBrowseForProgram(&path);
    
    // Fill in the selected path
    m_ui.lineEdit_coreFile->setText(path);
}



void OpenDialog::onConnectionTypeLocal(bool checked)
{
    m_ui.pushButton_selectFile->setEnabled(checked);
    m_ui.lineEdit_arguments->setEnabled(checked);
}

void OpenDialog::onConnectionTypeTcp(bool checked)
{
    m_ui.lineEdit_tcpHost->setEnabled(checked);
    m_ui.lineEdit_tcpPort->setEnabled(checked);
    m_ui.checkBox_download->setEnabled(checked);
    
}

void OpenDialog::onConnectionTypePid(bool checked)
{
    m_ui.pushButton_selectFile->setEnabled(checked);
    m_ui.pushButton_runningPid->setEnabled(checked);
    m_ui.lineEdit_pid->setEnabled(checked);

}

void OpenDialog::onConnectionTypeCoreDump(bool checked)
{
    m_ui.pushButton_selectCoreFile->setEnabled(checked);
    m_ui.lineEdit_coreFile->setEnabled(checked);
    
    m_ui.lineEdit_initialBreakpoint->setEnabled(checked ? false : true);
    m_ui.checkBox_reloadBreakpoints->setEnabled(checked ? false : true);
}


void OpenDialog::setTcpRemoteHost(QString host)
{
    m_ui.lineEdit_tcpHost->setText(host);
}

QString OpenDialog::getTcpRemoteHost()
{
    return m_ui.lineEdit_tcpHost->text();
}



    
void OpenDialog::setTcpRemotePort(int port)
{
    QString portStr;
    portStr.sprintf("%d", port);
    m_ui.lineEdit_tcpPort->setText(portStr);
}

int OpenDialog::getTcpRemotePort()
{
    return m_ui.lineEdit_tcpPort->text().toInt();
}


QString OpenDialog::getGdbPath()
{
    return m_ui.comboBox_gdbCommand->currentText();
}


void OpenDialog::setGdbPath(QString path)
{
    return m_ui.comboBox_gdbCommand->setEditText(path);
}

QString OpenDialog::getProjectDir()
{
    return m_ui.comboBox_projDir->currentText();
}

/**
 * @brief Returns the settings selected by the user.
 */
void OpenDialog::saveConfig(Settings *cfg)
{
    OpenDialog &dlg = *this;
    cfg->m_coreDumpFile = dlg.getCoreDumpFile();
    cfg->setProgramPath(dlg.getProgram());
    cfg->m_argumentList = dlg.getArguments().split(' ');
    cfg->m_connectionMode = dlg.getMode();
    cfg->m_tcpPort = dlg.getTcpRemotePort();
    cfg->m_download = dlg.getDownload();
    cfg->m_tcpHost = dlg.getTcpRemoteHost();
    cfg->m_initCommands = dlg.getInitCommands();
    cfg->m_gdbPath = dlg.getGdbPath();
    cfg->m_initialBreakpoint = dlg.getInitialBreakpoint().trimmed();
    cfg->m_runningPid = dlg.getRunningPid();
    
    if(cfg->m_initialBreakpoint.isEmpty())
        cfg->m_initialBreakpoint = "main";
    if(dlg.m_ui.checkBox_reloadBreakpoints->checkState() == Qt::Checked)
        cfg->m_reloadBreakpoints = true;
    else
        cfg->m_reloadBreakpoints = false;
    
    cfg->m_projDir = getProjectDir();

}

bool OpenDialog::getDownload()
{
    return m_ui.checkBox_download->checkState() == Qt::Checked ? true : false;
}

void OpenDialog::setDownload(bool enable)
{
    m_ui.checkBox_download->setChecked(enable);
}

void OpenDialog::loadConfig(Settings &cfg)
{
    OpenDialog &dlg = *this;
    dlg.setMode(cfg.m_connectionMode);

    dlg.setTcpRemotePort(cfg.m_tcpPort);
    dlg.setDownload(cfg.m_download);
    dlg.setTcpRemoteHost(cfg.m_tcpHost);
    dlg.setInitCommands(cfg.m_initCommands);
    dlg.setGdbPath(cfg.m_gdbPath);

    dlg.m_ui.checkBox_reloadBreakpoints->setChecked(cfg.m_reloadBreakpoints);

    dlg.setCoreDumpFile(cfg.m_coreDumpFile);

    dlg.setRunningPid(cfg.m_runningPid);

    // Fill in the paths
    QString currentProjDir = m_ui.comboBox_projDir->currentText();
    if(dlg.m_ui.comboBox_projDir->count() == 0)
    {
    
        dlg.m_ui.comboBox_projDir->clear();
        QStringList projectDirs;
        projectDirs.append(currentProjDir);
        projectDirs.append(cfg.getLastUsedProjectsDir());
        projectDirs.removeDuplicates();
        for(int i = 0;i < projectDirs.size();i++)
        {
            QString projBasePath = projectDirs[i];
            if(!projBasePath.isEmpty())
            {
                QString projDir = QFileInfo(projBasePath).absoluteFilePath();
                dlg.m_ui.comboBox_projDir->insertItem(i, projDir, projBasePath);
            }
        }
    }
    dlg.setProgram(cfg.getProgramPath());

    QStringList defList;
    dlg.setArguments(cfg.m_argumentList.join(" "));
    dlg.setInitialBreakpoint(cfg.m_initialBreakpoint);

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    m_ui.comboBox_projDir->setCurrentText(currentProjDir);
#else
    m_ui.comboBox_projDir->setEditText(currentProjDir);
#endif


}


void OpenDialog::setInitialBreakpoint(QString list)
{
    m_ui.lineEdit_initialBreakpoint->setText(list);
}
    
QString OpenDialog::getInitialBreakpoint()
{
    return m_ui.lineEdit_initialBreakpoint->text();
}


void OpenDialog::onProjDirComboChanged(int idx)
{
    // Which project config file was selected?
    QString projConfPath = m_ui.comboBox_projDir->itemData(idx).toString();
    if(!projConfPath.isEmpty())
    {
        // Load config file
        Settings projCfg;
        projCfg.setProjectConfig(projConfPath + "/" + PROJECT_CONFIG_FILENAME);
        projCfg.load();
        projCfg.m_projDir = projConfPath;

        loadConfig(projCfg);        
    }
}

