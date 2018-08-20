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

#ifndef UDPSOCKETSTUBFACTORY_H
#define UDPSOCKETSTUBFACTORY_H

#include "qtftp/udpsocketfactory.h"
#include "qtftp/abstractsocket.h"
#include <vector>


class QHostAddress;

namespace QTFTP
{

class UdpSocketStub;
class SimulatedNetworkStream;

class UdpSocketStubFactory : public UdpSocketFactory
{
    public:
        enum class StreamDirection { Input, Output };

        UdpSocketStubFactory();
        virtual ~UdpSocketStubFactory();

        std::shared_ptr<AbstractSocket> createNewSocket(QObject *parent=nullptr) override;
        void setSocketPeer(const QHostAddress &destAddr, uint16_t destPort, const QHostAddress &sourceAddr, uint16_t sourcePort);

        SimulatedNetworkStream &getNetworkStreamBySource(StreamDirection direction, const QHostAddress &sourceAddr, uint16_t sourcePortNr);
        SimulatedNetworkStream &getNetworkStreamByDest(StreamDirection direction, const QHostAddress &destAddr, uint16_t destPortNr);

    private:
        std::vector< std::weak_ptr<UdpSocketStub> > m_socketList;
};


} // QTFTP namespace end

#endif // UDPSOCKETSTUBFACTORY_H
