/****************************************************************************
* Copyright (c) Contributors as noted in the AUTHORS file
*
* This file is part of QTFTP.
*
* QTFTP is free software; you can redistribute it and/or modify it under
* the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2.1 of the License, or
* (at your option) any later version.
*
* QTFTP is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
****************************************************************************/

#include "tftpserver.h"
#include "readsession.h"
#include "udpsocketfactory.h"
#include "tftp_error.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QHostAddress>
#include <QFileInfo>
#include <QFile>
#include <QSettings>
#include <vector>
#include <string>
#include <iostream>
#include <limits>
#include <cstdint>
#ifdef Q_OS_LINUX
#include <systemd/sd-journal.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

using namespace std::string_literals;

static bool g_disableLogToStderr = false;
static bool g_disableLogToSystem = false;

struct TftpBindings
{
    public:
        TftpBindings();
        TftpBindings(uint16_t portNr, const QHostAddress &bindAddr, const QString &filesDir, bool allowUploads);

        uint16_t m_portNr;
        QHostAddress m_bindAddr;
        QString m_filesDir;
        bool    m_allowUploads;
};

TftpBindings::TftpBindings() : m_portNr(0),
                               m_allowUploads(false)
{
}

TftpBindings::TftpBindings(uint16_t portNr, const QHostAddress &bindAddr, const QString &filesDir, bool allowUploads) : m_portNr(portNr),
                                                                                                                        m_bindAddr(bindAddr),
                                                                                                                        m_filesDir(filesDir),
                                                                                                                        m_allowUploads(allowUploads)
{

}



static QVariant getSectionValue(const QSettings &configFile, const QString &section, const QString &key)
{
    auto keyValue = configFile.value(section + "/" + key);
    if (keyValue.isNull() || !keyValue.isValid())
    {
        throw std::runtime_error("Config file "s + configFile.fileName().toStdString() + " invalid: '" + key.toStdString() +"' key missing/invalid in section [" + section.toStdString() + ']');
    }
    return keyValue;
}

static std::vector<TftpBindings> readConfigFile(const QString &fileName)
{
    QFileInfo configFileInfo(fileName);
    if (!configFileInfo.exists() || !configFileInfo.isReadable())
    {
        throw std::runtime_error("Config file "s + fileName.toStdString() + " does not exist or is not readable");
    }
    //On Unix refuse to start when config file is writable by any non-root user
#ifdef Q_OS_UNIX
#ifndef QTFTP_DEBUG_BUILD
    QFile::Permissions disAllowedPerms = QFileDevice::WriteOther;
    if (configFileInfo.groupId() != 0)
    {
        disAllowedPerms |= QFileDevice::WriteGroup;
    }
    if (configFileInfo.ownerId() != 0 || configFileInfo.permission(QFile::WriteOther) || (configFileInfo.groupId()!=0 && configFileInfo.permission(QFile::WriteGroup)))
    {
        throw std::runtime_error("Config file "s + fileName.toStdString() + " must not be writable for other users/groups than root");
    }
#endif
#endif
    std::vector<TftpBindings> bindings;
    QSettings config(fileName, QSettings::IniFormat);
    auto sections = config.childGroups();
    for (const auto &nextSection : sections)
    {
        auto portValue = getSectionValue(config, nextSection, "port");
        bool conversionOk = false;
        uint64_t longportnr = portValue.toULongLong(&conversionOk);
        if (!conversionOk)
        {
            throw std::runtime_error("Config file "s + fileName.toStdString() + " invalid: 'port' in section [" + nextSection.toStdString() + "] not a valid portnr" );
        }
        if (longportnr >  std::numeric_limits<std::uint16_t>::max())
        {
            throw std::runtime_error("Config file "s + fileName.toStdString() + " invalid: 'port' in section [" + nextSection.toStdString() + "] too large" );
        }
        uint16_t portnr = static_cast<uint16_t>(longportnr);
        auto addrValue = getSectionValue(config, nextSection, "bind_addr");
        QHostAddress bindAddr;
        bool validAddr = bindAddr.setAddress(addrValue.toString());
        if (!validAddr)
        {
            throw std::runtime_error("Config file "s + fileName.toStdString() + " invalid: 'bind_addr' in section [" + nextSection.toStdString() + "] not a valid IP address" );
        }
        auto dirValue =  getSectionValue(config, nextSection, "files_dir");
        QFileInfo filesDirInfo(dirValue.toString());
        if (!filesDirInfo.exists() || !filesDirInfo.isDir())
        {
            throw std::runtime_error("Config file "s + fileName.toStdString() + " invalid: 'files_dir' in section [" + nextSection.toStdString() + "] does not exist or not a directory" );
        }
        auto uploadValue = getSectionValue(config, nextSection, "disable_upload");
        QString uploadValueStr = uploadValue.toString().toLower();
        bool uploadDisabled = true;
        if (uploadValueStr == "true")
        {
            // no action needed
        }
        else if (uploadValueStr == "false")
        {
            uploadDisabled = false;
        }
        else
        {
            throw std::runtime_error("Config file "s + fileName.toStdString() + " invalid: 'disable_upload' in section [" + nextSection.toStdString() + "] should be 'true' or 'false'" );
        }
#ifndef QTFTP_DEBUG_BUILD
        if (uploadDisabled)
        {
            if (filesDirInfo.isWritable())
            {
                throw std::runtime_error("Config file "s + fileName.toStdString() + ": directory " + filesDirInfo.absoluteFilePath().toStdString() + " in section [" + nextSection.toStdString() + "] set to read-only, but is writable" );
            }
        }
#endif
        bindings.emplace_back(portnr, bindAddr, filesDirInfo.absoluteFilePath(), !uploadDisabled);
    }


    return bindings;
}


static void logTftpdMsg(int severity, const QString &msg)
{
    if (! g_disableLogToStderr)
    {
        std::cerr << msg.toStdString() << std::endl;
    }
    if (!g_disableLogToSystem)
    {
#ifdef Q_OS_LINUX
        auto utf8Msg = static_cast<const char*>(msg.toUtf8());
        sd_journal_print(severity, "%s", utf8Msg);
#endif
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("qtftpd");
    QCoreApplication::setApplicationVersion("1.0.0");

    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("Tftp server daemon"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption portOption( {"p", "port"}, QObject::tr("UDP port to listen on"), "portValue", "69");
    QCommandLineOption dirOption( {"d", "directory"}, QObject::tr("Directory where to read/write files"), "dirValue" );
    QCommandLineOption configFileOption( {"c", "config"}, QObject::tr("Read configuration from file"), "configValue", "/etc/qtftpd.conf" );
    QCommandLineOption stderrOption( {"e", "no-stderr"}, QObject::tr("Do not send errors to stderr"));
    QCommandLineOption systemLogOption( {"s", "no-systemlog"}, QObject::tr("Do not send errors to system log daemon"));
#ifdef Q_OS_UNIX
    QCommandLineOption userOption( {"u", "user"}, QObject::tr("User to run daemon"), "userValue", "tftp");
    parser.addOption(userOption);
#endif
    parser.addOption(portOption);
    parser.addOption(dirOption);
    parser.addOption(configFileOption);
    parser.addOption(stderrOption);
    parser.addOption(systemLogOption);
    parser.process(app);

    g_disableLogToStderr = parser.isSet(stderrOption);
    g_disableLogToSystem = parser.isSet(systemLogOption);

    QTFTP::TftpServer tftpServer(std::make_shared<QTFTP::UdpSocketFactory>(), nullptr);
    std::vector<TftpBindings> bindings;

    if ( parser.isSet(dirOption))
    {
        QString portValue = parser.value(portOption);
        bool portOk = false;
        unsigned int portNr = portValue.toUInt(&portOk);
        if (!portOk || portNr > std::numeric_limits<std::uint16_t>::max())
        {
            logTftpdMsg(LOG_ERR, QObject::tr("Error: invalid port number provided.") );
            return 1;
        }

        QString dirValue = parser.value(dirOption);
        if (dirValue.isEmpty())
        {
            logTftpdMsg(LOG_ERR, QObject::tr("Error: directory where to read/write files not specified.") );
            parser.showHelp(2);
        }
        QFileInfo dirInfo(dirValue);
        if (!dirInfo.exists())
        {
            logTftpdMsg(LOG_ERR, QObject::tr("Error: specified files directory does not exist.") );
            return 3;
        }

        if (!dirInfo.isReadable())
        {
            logTftpdMsg(LOG_ERR, QObject::tr("Error: specified files directory is not readable.") );
            return 4;
        }

        bindings.emplace_back(static_cast<uint16_t>(portNr), QHostAddress::LocalHost, dirInfo.absoluteFilePath(), true);
    }

    if (parser.isSet(configFileOption))
    {
        auto configFileName = parser.value(configFileOption);
        try
        {
            bindings = readConfigFile(configFileName);
        }
        catch(const std::runtime_error &configErr)
        {
            logTftpdMsg(LOG_ERR, QObject::tr("Error: ") + configErr.what() );
            return 6;
        }
    }

    if (bindings.empty())
    {
        logTftpdMsg(LOG_ERR, QObject::tr("Error: no directorie(s) given to serve files to/from") );
        return 7;
    }
    for (const auto &nextBinding : bindings)
    {
        try
        {
            tftpServer.bind(nextBinding.m_filesDir, nextBinding.m_bindAddr, nextBinding.m_portNr);
        }
        catch(const QTFTP::TftpError &tftpErr)
        {
            logTftpdMsg(LOG_ERR, QObject::tr("Error while binding to address %1 and portNr %2").arg(nextBinding.m_bindAddr.toString()).arg(nextBinding.m_portNr) + ": " + tftpErr.what() );
            return 5;
        }
    }

    //report successful or failed file download
    QObject::connect(&tftpServer, &QTFTP::TftpServer::newReadSession, [](std::shared_ptr<const QTFTP::ReadSession> newReadSession)
                                                                      { QObject::connect(newReadSession.get(), &QTFTP::ReadSession::finished, [newReadSession]()
                                                                                                                                              {
                                                                                                                                                  logTftpdMsg(LOG_INFO, QObject::tr("Download of file %1 by %2 finished").arg(newReadSession->filePath()).arg(newReadSession->peerIdent().m_address.toString()));
                                                                                                                                              });
                                                                        QObject::connect(newReadSession.get(), &QTFTP::ReadSession::error, [newReadSession](QString errMsg)
                                                                                                                                              {
                                                                                                                                                  logTftpdMsg(LOG_ERR, QObject::tr("Download of file %1 by %2 failed: %3").arg(newReadSession->filePath()).arg(newReadSession->peerIdent().m_address.toString()).arg(errMsg));
                                                                                                                                              });
                                                                      });

#ifdef Q_OS_UNIX
    //Recommended way to run qtftp on Linux is to add CAP_NET_BIND_SERVICE capability to qtftpd executable and run as normal user.
    //If we are running as root, we should drop privileges now that listening socket are opened.
    auto tftpUserName = parser.value(userOption);

    auto tftpUser = getpwnam(tftpUserName.toStdString().c_str());
    if (!tftpUser)
    {
        logTftpdMsg(LOG_ERR, QObject::tr("Error: user %1 not found").arg(tftpUserName));
        return 8;
    }
    if (getuid() == 0)
    {
        /* process is running as root, drop privileges */
        if (setgid(tftpUser->pw_gid) != 0)
        {
            logTftpdMsg(LOG_ERR, QObject::tr("Error dropping group priviliges to groupid %1").arg(tftpUser->pw_gid));
            return 8;
        }
        if (setuid(tftpUser->pw_uid) != 0)
        {
            logTftpdMsg(LOG_ERR, QObject::tr("Error dropping user priviliges to user %1").arg(tftpUserName));
            return 8;
        }
        //make sure it is not possible to get root privileges back
        if (setuid(0) != -1)
        {
            logTftpdMsg(LOG_ERR, QObject::tr("Managed to regain root privileges after dropping them.").arg(tftpUserName));
            return 8;
        }
    }
#endif

    int returnCode = 0;
    try
    {
        returnCode = app.exec();
    }
    catch(const QTFTP::TftpError &tftpErr)
    {
        logTftpdMsg(LOG_ERR, QObject::tr("qtftpd exited due to exception: ") + tftpErr.what() );
        return 9;
    }

    return returnCode;
}

