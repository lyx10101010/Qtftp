/****************************************************************************
* Copyright (c) Contributors as noted in the AUTHORS file
*
* This file is part of LIBTFTP.
*
* LIBTFTP is free software; you can redistribute it and/or modify it under
* the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2.1 of the License, or
* (at your option) any later version.
*
* LIBTFTP is distributed in the hope that it will be useful,
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
#include <iostream>
#include <limits>
#include <cstdint>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("qt_tftpd");
    QCoreApplication::setApplicationVersion("1.0.0");

    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("Tftp server daemon"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption portOption( {"p", "port"}, QObject::tr("UDP port to listen on"), "portValue", "69");
    QCommandLineOption dirOption( {"d", "directory"}, QObject::tr("Directory where to read/write files"), "dirValue" );
    parser.addOption(portOption);
    parser.addOption(dirOption);
    parser.process(app);

    QString portValue = parser.value(portOption);
    bool portOk = false;
    unsigned int portNr = portValue.toUInt(&portOk);
    if (!portOk || portNr > std::numeric_limits<std::uint16_t>::max())
    {
        std::cerr << (const char*)QObject::tr("Error: invalid port number provided.").toLatin1() << std::endl;
        return 1;
    }

    QString dirValue = parser.value(dirOption);
    if (dirValue.isEmpty())
    {
        std::cerr << (const char*)QObject::tr("Error: directory where to read/write files not specified.").toLatin1() << std::endl;
        parser.showHelp(2);
    }
    QFileInfo dirInfo(dirValue);
    if (!dirInfo.exists())
    {
        std::cerr << (const char*)QObject::tr("Error: specified files directory does not exist.").toLatin1() << std::endl;
        return 3;
    }
    if (!dirInfo.isReadable() || !dirInfo.isWritable())
    {
        std::cerr << (const char*)QObject::tr("Error: specified files directory is not readable and/or writable.").toLatin1() << std::endl;
        return 4;
    }

    LIBTFTP::TftpServer tftpServer(dirValue, std::make_shared<LIBTFTP::UdpSocketFactory>(), nullptr);
    try
    {
        tftpServer.bind(QHostAddress::Any, portNr);
    }
    catch(const LIBTFTP::TftpError &tftpErr)
    {
        std::cerr << (const char*)QObject::tr("Error while binding to UDP port ").toLatin1() << portNr << ": " << tftpErr.what() << std::endl;
        return 5;
    }

    int returnCode = 0;
    try
    {
        returnCode = app.exec();
    }
    catch(const LIBTFTP::TftpError &tftpErr)
    {
        std::cerr << (const char*)QObject::tr("qt_tftpd exited due to exception: ").toLatin1() << tftpErr.what() << std::endl;
        return 6;
    }

    return returnCode;
}
