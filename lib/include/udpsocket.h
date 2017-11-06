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

#ifndef UDPSOCKET_H
#define UDPSOCKET_H

#include "abstractsocket.h"
#include <QObject>
#include <QUdpSocket>

class QHostAddress;

namespace QTFTP
{

/**
 * @brief UdpSocket class
 *
 * Class QUdpSocket has almost no virtual members, so it's implementation can't be overridden in a derived stub class.
 * This class is a wrapper around QUdpSocket that has the same (partial) interface, but uses virtual functions.
 */
class UdpSocket : public AbstractSocket
{
    Q_OBJECT

    public:
        explicit UdpSocket(QObject *parent = 0);
        virtual ~UdpSocket();

        qint64 pendingDatagramSize() const override;
        bool hasPendingDatagrams() const override;
        QHostAddress localAddress() const override;
        quint16	localPort() const override;
        QHostAddress peerAddress() const override;
        quint16	peerPort() const override;
        virtual QString errorString() const override;

        bool bind(const QHostAddress & address, quint16 port = 0, QAbstractSocket::BindMode mode = QAbstractSocket::DefaultForPlatform) override;
        void close();
        qint64 readDatagram(char * data, qint64 maxSize, QHostAddress *address = 0, quint16 *port = 0) override;
        qint64 writeDatagram(const QByteArray & datagram, const QHostAddress & host, quint16 port) override;

    private:
        QUdpSocket m_socket;
};


} // QTFTP namespace end

#endif // UDPSOCKET_H
