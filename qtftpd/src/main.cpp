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

using namespace std::string_literals;

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
        throw std::runtime_error("Config file "s + configFile.fileName().toStdString() + "invalid: '" + key.toStdString() +"' key missing/invalid in section [" + section.toStdString() + ']');
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
            throw std::runtime_error("Config file "s + fileName.toStdString() + "invalid: 'port' in section [" + nextSection.toStdString() + "] not a valid portnr" );
        }
        if (longportnr >  std::numeric_limits<std::uint16_t>::max())
        {
            throw std::runtime_error("Config file "s + fileName.toStdString() + "invalid: 'port' in section [" + nextSection.toStdString() + "] too large" );
        }
        uint16_t portnr = static_cast<uint16_t>(longportnr);
        auto addrValue = getSectionValue(config, nextSection, "bind_addr");
        QHostAddress bindAddr;
        bool validAddr = bindAddr.setAddress(addrValue.toString());
        if (!validAddr)
        {
            throw std::runtime_error("Config file "s + fileName.toStdString() + "invalid: 'bind_addr' in section [" + nextSection.toStdString() + "] not a valid IP address" );
        }
        auto dirValue =  getSectionValue(config, nextSection, "files_dir");
        QFileInfo filesDirInfo(dirValue.toString());
        if (!filesDirInfo.exists() || !filesDirInfo.isDir())
        {
            throw std::runtime_error("Config file "s + fileName.toStdString() + "invalid: 'files_dir' in section [" + nextSection.toStdString() + "] does not exist or not a directory" );
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
            throw std::runtime_error("Config file "s + fileName.toStdString() + "invalid: 'disable_upload' in section [" + nextSection.toStdString() + "] should be 'true' or 'false'" );
        }
        if (uploadDisabled)
        {
            if (filesDirInfo.isWritable())
            {
                throw std::runtime_error("Config file "s + fileName.toStdString() + ": directory " + filesDirInfo.absoluteFilePath().toStdString() + " in section [" + nextSection.toStdString() + "] set to read-only, but is writable" );
            }
        }

        bindings.emplace_back(portnr, bindAddr, filesDirInfo.absoluteFilePath(), !uploadDisabled);
    }


    return bindings;
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

    parser.addOption(portOption);
    parser.addOption(dirOption);
    parser.addOption(configFileOption);
    parser.process(app);

    QTFTP::TftpServer tftpServer(std::make_shared<QTFTP::UdpSocketFactory>(), nullptr);

    if ( parser.isSet(dirOption))
    {
        QString portValue = parser.value(portOption);
        bool portOk = false;
        unsigned int portNr = portValue.toUInt(&portOk);
        if (!portOk || portNr > std::numeric_limits<std::uint16_t>::max())
        {
            std::cerr << QObject::tr("Error: invalid port number provided.").toStdString() << std::endl;
            return 1;
        }

        QString dirValue = parser.value(dirOption);
        if (dirValue.isEmpty())
        {
            std::cerr << QObject::tr("Error: directory where to read/write files not specified.").toStdString() << std::endl;
            parser.showHelp(2);
        }
        QFileInfo dirInfo(dirValue);
        if (!dirInfo.exists())
        {
            std::cerr << QObject::tr("Error: specified files directory does not exist.").toStdString() << std::endl;
            return 3;
        }

        if (!dirInfo.isReadable())
        {
            std::cerr << QObject::tr("Error: specified files directory is not readable and/or writable.").toStdString() << std::endl;
            return 4;
        }

        try
        {
            tftpServer.bind(dirValue, QHostAddress::Any, static_cast<uint16_t>(portNr));
        }
        catch(const QTFTP::TftpError &tftpErr)
        {
            std::cerr << QObject::tr("Error while binding to UDP port ").toStdString() << portNr << ": " << tftpErr.what() << std::endl;
            return 5;
        }
    }

    if (parser.isSet(configFileOption))
    {
        auto configFileName = parser.value(configFileOption);
        std::vector<TftpBindings> bindings;
        try
        {
            bindings = readConfigFile(configFileName);
        }
        catch(const std::runtime_error &configErr)
        {
            std::cerr << QObject::tr("Error").toStdString() << ": " << configErr.what() << std::endl;
            return 6;
        }
        for (const auto &nextBinding : bindings)
        {
            try
            {
                tftpServer.bind(nextBinding.m_filesDir, nextBinding.m_bindAddr, nextBinding.m_portNr);
            }
            catch(const QTFTP::TftpError &tftpErr)
            {
                std::cerr << QObject::tr("Error while binding to address %1 and portNr %s").arg(nextBinding.m_bindAddr.toString()).arg(nextBinding.m_portNr).toStdString() << ": " << tftpErr.what() << std::endl;
                return 5;
            }
        }
    }



    int returnCode = 0;
    try
    {
        returnCode = app.exec();
    }
    catch(const QTFTP::TftpError &tftpErr)
    {
        std::cerr << QObject::tr("qtftpd exited due to exception: ").toStdString() << tftpErr.what() << std::endl;
        return 6;
    }

    return returnCode;
}

