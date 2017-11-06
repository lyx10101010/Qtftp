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

#ifndef QTFTP_ABSTRACTCLASS_H
#define QTFTP_ABSTRACTCLASS_H

#include <QObject>
#include <QHostAddress>

namespace QTFTP
{


class AbstractSocket : public QObject
{
        Q_OBJECT

        public:
        explicit AbstractSocket(QObject *parent = 0);
        virtual ~AbstractSocket();

        virtual qint64 pendingDatagramSize() const = 0;
        virtual bool hasPendingDatagrams() const = 0;
        virtual QHostAddress localAddress() const = 0;
        virtual quint16 localPort() const = 0;
        virtual QHostAddress peerAddress() const = 0;
        virtual quint16 peerPort() const = 0;
        virtual QString errorString() const = 0;

        virtual bool bind(const QHostAddress &address, quint16 port = 0,
                          QAbstractSocket::BindMode mode = QAbstractSocket::DefaultForPlatform) = 0;
        virtual qint64 readDatagram(char *data, qint64 maxSize, QHostAddress *address = nullptr, quint16 *port = nullptr) = 0;
        virtual qint64 writeDatagram(const QByteArray &datagram, const QHostAddress &host, quint16 port) = 0;


    signals:
        void error(QAbstractSocket::SocketError socketError);
        void readyRead();

};


} // QTFTP namespace end

#endif //QTFTP_ABSTRACTCLASS_H
