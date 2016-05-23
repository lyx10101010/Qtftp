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

#include "udpsocket.h"
#include <QAbstractSocket>

namespace LIBTFTP
{


UdpSocket::UdpSocket(QObject *parent) : AbstractSocket(parent),
                                        m_socket(this)
{
    connect(&m_socket, &QAbstractSocket::readyRead, this, &UdpSocket::readyRead);
    //debug start
    //connect(&m_socket, &QAbstractSocket::readyRead, this, &UdpSocket::dataReceived);
    //debug end
    connect(&m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SIGNAL(error(QAbstractSocket::SocketError)));
}


UdpSocket::~UdpSocket()
{
}


QHostAddress UdpSocket::localAddress() const
{
    return m_socket.localAddress();
}


quint16 UdpSocket::localPort() const
{
    return m_socket.localPort();
}

QHostAddress UdpSocket::peerAddress() const
{
    return m_socket.peerAddress();
}

quint16 UdpSocket::peerPort() const
{
    return m_socket.peerPort();
}


bool UdpSocket::bind(const QHostAddress &address, quint16 port, QAbstractSocket::BindMode mode)
{
    return m_socket.bind(address, port, mode);
}

qint64 UdpSocket::readDatagram(char *data, qint64 maxSize, QHostAddress *address, quint16 *port)
{
    return m_socket.readDatagram(data, maxSize, address, port);
}

qint64 UdpSocket::writeDatagram(const QByteArray &datagram, const QHostAddress &host, quint16 port)
{
    return m_socket.writeDatagram(datagram, host, port);
}

//void UdpSocket::dataReceived()
//{
//    bool hasDatagrams = m_socket.hasPendingDatagrams();
//    emit readyRead();
//}

qint64 UdpSocket::pendingDatagramSize() const
{
    return m_socket.pendingDatagramSize();
}

bool UdpSocket::hasPendingDatagrams() const
{
    return m_socket.hasPendingDatagrams();
}


} // LIBTFTP namespace end
