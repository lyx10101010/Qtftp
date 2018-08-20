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

#ifndef UDPSOCKETSTUB_H
#define UDPSOCKETSTUB_H

#include "simulatednetworkstream.h"
#include "qtftp/abstractsocket.h"
#include <QByteArray>
#include <list>
#include <vector>
#include <random>

namespace QTFTP
{

struct Datagram
{
    public:
        QHostAddress m_sourceIpAddress;
        uint16_t    m_sourcePort;
        QByteArray  m_data;
};


/**
 * @brief The UdpSocketStub class
 *
 * Stub for class QUdpSocket, so that we can unit test the QTFTP classes without the
 * need for a physical network. It uses stringstream objects (wrapped in class SimulatedNetworkStream)
 * to capture the data that is sent and received by the socket.
 */
class UdpSocketStub : public AbstractSocket
{
    Q_OBJECT

    public:
        UdpSocketStub(QObject *parent = nullptr);
        virtual ~UdpSocketStub();

        bool hasPendingDatagrams() const override;
        qint64 pendingDatagramSize() const override;
        QHostAddress localAddress() const override;
        quint16 localPort() const override;
        QHostAddress peerAddress() const override;
        quint16 peerPort() const override;
        virtual QString errorString() const override;

        SimulatedNetworkStream &getInputStream();
        SimulatedNetworkStream &getOutputStream();

        qint64 readDatagram(char * data, qint64 maxSize, QHostAddress * address = 0, quint16 * port = 0) override;
        qint64 writeDatagram(const QByteArray &datagram, const QHostAddress &host, quint16 port) override;

        virtual bool bind(const QHostAddress &address, quint16 port, QAbstractSocket::BindMode mode) override;

        virtual void close() override;

    protected:
        void setLocalAddress(const QHostAddress &address);
        void setLocalPort(quint16 port);
        void setPeerAddress(const QHostAddress &address);
        void setPeerPort(quint16 port);

    private:
        void handleIncomingDatagram();

        SimulatedNetworkStream m_inputStream;
        SimulatedNetworkStream m_outputStream;
        std::list<Datagram>    m_pendingInputDatagrams;
        QHostAddress           m_localAddress;
        uint16_t               m_localPort;
        QHostAddress           m_peerAddress;
        uint16_t               m_peerPort;
        static std::vector<uint16_t>  m_portsInUse;
        static std::random_device     m_portRandomizer;

        friend class UdpSocketStubFactory;
};


} //namespace QTFTP end

#endif // UDPSOCKETSTUB_H
