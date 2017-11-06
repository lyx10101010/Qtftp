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

#ifndef QTFTPSERVER_H
#define QTFTPSERVER_H

#include <QObject>
#include <QHostAddress>
#include <memory>
#include <vector>


class QAbstractSocket;
class QHostAddress;

namespace QTFTP
{

constexpr int DefaultTftpPortNr = 69;

struct SessionIdent;
class ReadSession;
class UdpSocket;
class UdpSocketFactory;

class TftpServer : public QObject
{
    Q_OBJECT

    public:
        explicit TftpServer(const QString &filesDir, std::shared_ptr<UdpSocketFactory> socketFactory, QObject *parent = 0);
        explicit TftpServer(std::shared_ptr<UdpSocketFactory> socketFactory, QObject *parent = 0);
        virtual ~TftpServer();

        virtual void bind(const QHostAddress &hostAddr=QHostAddress(QHostAddress::LocalHost));
        virtual void close();
        void setServerPort(uint16_t port);
        void setFilesDir(const QString &filesDir);
        void setSlowNetworkDetectionThreshold(unsigned int ackLatencyUs);

        quint16 serverPort() const;
        const QString & filesDir() const;
        std::shared_ptr<const ReadSession> findReadSession(const SessionIdent &sessionIdent) const;

    signals:
        void newReadSession(std::shared_ptr<const ReadSession> newSession);
        void receivedFile();

    private slots:
        void dataReceived();
        void removeSession();

    private:
        std::shared_ptr<ReadSession> doFindReadSession(const SessionIdent &sessionIdent) const;

        QString m_filesDir;
        std::shared_ptr<UdpSocketFactory> m_socketFactory;  ///creates real sockets in production code, test stub sockets in unit tests
        std::shared_ptr<UdpSocket> m_mainSocket; //could have been unique_ptr, but shared_ptr needed in socket stub for testing
        std::vector< std::shared_ptr<ReadSession> > m_readSessions;
        uint16_t m_tftpServerPort;
        unsigned int m_slowNetworkThreshold;

};



} //namespace end

#endif // QTFTPSERVER_H
