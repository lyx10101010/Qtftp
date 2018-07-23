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

#include "udpsocket.h"
#include <QObject>
#include <QHostAddress>
#include <memory>
#include <vector>
#include <map>

class QAbstractSocket;
class QHostAddress;

namespace QTFTP
{

constexpr int DefaultTftpPortNr = 69;

struct SessionIdent;
class ReadSession;
class UdpSocketFactory;

class ConnectionRequestSocket : public QObject
{
    Q_OBJECT

    public:
        ConnectionRequestSocket(const QString &filesDir, std::shared_ptr<UdpSocketFactory> socketFactory);
        virtual ~ConnectionRequestSocket() = default;

        const QString &filesDir() const;
        QString errorString() const;
        bool hasPendingDatagrams() const;
        qint64 pendingDatagramSize() const;
        quint16 localPort() const;

        bool bind(const QHostAddress &address, quint16 port = 0, QAbstractSocket::BindMode mode = QAbstractSocket::DefaultForPlatform);
        qint64 readDatagram(char *data, qint64 maxSize, QHostAddress *address = nullptr, quint16 *port = nullptr);
        qint64 writeDatagram(const QByteArray &datagram, const QHostAddress &host, quint16 port);
        void close();

    signals:
        void readyRead();

    private:
        std::shared_ptr<AbstractSocket> m_socket;
        QString m_filesDir;
};


class TftpServer : public QObject
{
    Q_OBJECT

    public:
        explicit TftpServer(std::shared_ptr<UdpSocketFactory> socketFactory, QObject *parent = nullptr);
        //explicit TftpServer(std::shared_ptr<UdpSocketFactory> socketFactory, QObject *parent = nullptr);
        virtual ~TftpServer() = default;

        virtual void bind(const QString &filesDir, const QHostAddress &hostAddr=QHostAddress(QHostAddress::LocalHost), uint16_t port=69);
        virtual void close();

        void setSlowNetworkDetectionThreshold(unsigned int ackLatencyUs);

        std::shared_ptr<const ReadSession> findReadSession(const SessionIdent &sessionIdent) const;

    signals:
        void newReadSession(std::shared_ptr<const ReadSession> newSession);
        void receivedFile();

    private slots:
        void dataReceived();
        void removeSession();

    private:
        std::shared_ptr<ReadSession> doFindReadSession(const SessionIdent &sessionIdent) const;
        void handleNewData(std::shared_ptr<ConnectionRequestSocket> mainSocket);

        std::shared_ptr<UdpSocketFactory> m_socketFactory;  ///creates real sockets in production code, test stub sockets in unit tests
        //std::shared_ptr<UdpSocket> m_mainSocket; //could have been unique_ptr, but shared_ptr needed in socket stub for testing
        std::vector<std::shared_ptr<ConnectionRequestSocket>> m_mainSockets; /// sockets that listen for new connection requests
        std::vector< std::shared_ptr<ReadSession> > m_readSessions;
        unsigned int m_slowNetworkThreshold;
        //std::map<std::pair<QHostAddress, uint16_t>, QString> m_filesDirs;

};



} //namespace end

#endif // QTFTPSERVER_H
