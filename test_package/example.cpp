#include "qtftp/tftpserver.h"
#include "qtftp/udpsocketfactory.h"
#include <QDir>
#include <iostream>
#include <vector>

int main()
{
    QTFTP::TftpServer tftpServer(std::make_shared<QTFTP::UdpSocketFactory>(), nullptr);
    tftpServer.bind(QDir::homePath(), QHostAddress::LocalHost, 8869);    
    return 0;
}
