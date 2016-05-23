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

#include "udpsocketstub.h"
#include <QHostAddress>


namespace LIBTFTP {


//static class members initialization
std::vector<uint16_t>  UdpSocketStub::m_portsInUse;
std::random_device UdpSocketStub::m_portRandomizer;


//member function implemetations

UdpSocketStub::UdpSocketStub(QObject *parent) : AbstractSocket(parent),
                                                m_localPort(0),
                                                m_peerPort(0)
{
    connect(&m_inputStream, &SimulatedNetworkStream::newData, this, &UdpSocketStub::handleIncomingDatagram);
}



UdpSocketStub::~UdpSocketStub()
{
}


bool UdpSocketStub::hasPendingDatagrams() const
{
    return ! m_pendingInputDatagrams.empty();
}


qint64 UdpSocketStub::pendingDatagramSize() const
{
    if (m_pendingInputDatagrams.empty())
    {
        return -1;
    }

    return m_pendingInputDatagrams.front().m_data.size();
}


SimulatedNetworkStream &UdpSocketStub::getInputStream()
{
    return m_inputStream;
}


SimulatedNetworkStream &UdpSocketStub::getOutputStream()
{
    return m_outputStream;
}

#ifdef DISABLE_ME
/**
 * @brief UdpSocketStub::assignLocalAddress
 * @param address
 *
 * Because QAbstractSocket::setLocalAddress(...) is not a virtual function, we can't override it directly. Instead we use this function with
 * different name.
 */
void QUdpSocketStub::assignLocalAddress(const QHostAddress &address)
{
    QAbstractSocket::setLocalAddress(address);
}

/**
 * @brief QUdpSocketStub::assignLocalPort
 * @param port
 *
 * Because QAbstractSocket::setLocalAddress(...) is not a virtual function, we can't override it directly. Instead we use this function with
 * different name.
 */
void QUdpSocketStub::assignLocalPort(quint16 port)
{
    QAbstractSocket::setLocalPort(port);
}

void QUdpSocketStub::assignPeerAddress(const QHostAddress &address)
{
    QAbstractSocket::setPeerAddress(address);
}

void QUdpSocketStub::assignPeerPort(quint16 port)
{
    QAbstractSocket::setPeerPort(port);
}
#endif


qint64 LIBTFTP::UdpSocketStub::readDatagram(char *data, qint64 maxSize, QHostAddress *address, quint16 *port)
{
    if (m_pendingInputDatagrams.empty())
    {
        return -1;
    }

    Datagram pendingDatagram(m_pendingInputDatagrams.front());
    m_pendingInputDatagrams.pop_front();
    auto endIter = std::copy_n( pendingDatagram.m_data.data(), std::min<size_t>(pendingDatagram.m_data.size(), maxSize), data);
    qint64 charsCopied = endIter - data;
    if (address)
    {
        *address = pendingDatagram.m_sourceIpAddress;
    }
    if (port)
    {
        *port = pendingDatagram.m_sourcePort;
    }
    return charsCopied;
}


void UdpSocketStub::handleIncomingDatagram()
{
    Datagram newDatagram;
    m_inputStream >> newDatagram.m_data;
    newDatagram.m_sourceIpAddress = m_peerAddress;
    newDatagram.m_sourcePort = m_peerPort;
    m_pendingInputDatagrams.push_back(newDatagram);
    emit readyRead();
}


qint64 UdpSocketStub::writeDatagram(const QByteArray &datagram, const QHostAddress &host, quint16 port)
{
    m_outputStream << datagram;
    if (!m_outputStream)
    {
        return -1;
    }
//    m_outputStream.setDestIpAddress(host.toString().toStdString());
//    m_outputStream.setDestPort(port);
    m_peerAddress = host;
    m_peerPort = port;
    return datagram.length();
}


void UdpSocketStub::setLocalAddress(const QHostAddress &address)
{
    m_localAddress = address;
}


void UdpSocketStub::setLocalPort(quint16 port)
{
    m_localPort = port;
}


void UdpSocketStub::setPeerAddress(const QHostAddress &address)
{
    m_peerAddress = address;
}


void UdpSocketStub::setPeerPort(quint16 port)
{
    m_peerPort = port;
}


QHostAddress UdpSocketStub::localAddress() const
{
    return m_localAddress;
}


quint16 UdpSocketStub::localPort() const
{
    return m_localPort;
}


QHostAddress UdpSocketStub::peerAddress() const
{
    return m_peerAddress;
}


quint16 UdpSocketStub::peerPort() const
{
    return m_peerPort;
}


bool UdpSocketStub::bind(const QHostAddress &address, quint16 port, QAbstractSocket::BindMode mode)
{

    if (port==0)
    {
        //we have to select a random free port ourself (> 1024)
        auto portsInUseIter = m_portsInUse.end();
        std::uniform_int_distribution<uint16_t> distrib(1025, 65535);
        do
        {
            port = distrib(m_portRandomizer);
            //check if the random port nr was not issued before
            portsInUseIter = std::find(m_portsInUse.begin(), m_portsInUse.end(), port);
        }
        while (portsInUseIter != m_portsInUse.end());
        m_portsInUse.push_back(port);

    }
    setLocalAddress(address);
    setLocalPort(port);
    return true;
}


} // namespace LIBTFTP end
