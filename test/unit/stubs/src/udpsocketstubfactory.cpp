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

#include "include/udpsocketstubfactory.h"
#include "udpsocketstub.h"
#include <QHostAddress>

namespace QTFTP
{


UdpSocketStubFactory::UdpSocketStubFactory()
{
}



std::shared_ptr<AbstractSocket> UdpSocketStubFactory::createNewSocket(QObject *parent)
{
    auto newSocketPtr = std::make_shared<UdpSocketStub>(parent);
    std::weak_ptr<UdpSocketStub> newSocketWeakPtr(newSocketPtr);
    m_socketList.push_back(newSocketWeakPtr);
    return  std::move( std::static_pointer_cast<AbstractSocket>(newSocketPtr) );
}


/**
 * @brief UdpSocketStubFactory::setSocketPeer
 * @param localAddr
 * @param localPort
 * @param peerAddr
 * @param peerPort
 * @throw std::logic_error if there is no socket with supplied local address and loca port
 *
 * Set peer address and port for socket stub that has local address \p localAddr and local port \p localPort.
 */
void UdpSocketStubFactory::setSocketPeer(const QHostAddress &localAddr, uint16_t localPort, const QHostAddress &peerAddr, uint16_t peerPort)
{
    auto socketFindLambda = [&localAddr, localPort](const std::weak_ptr<UdpSocketStub> &socketStubWeakPtr)
    {
        if (socketStubWeakPtr.expired())
        {
            return false;
        }
        std::shared_ptr<UdpSocketStub> stubSharedPtr(socketStubWeakPtr);
        return stubSharedPtr->localAddress() == localAddr && (stubSharedPtr->localPort()==localPort || stubSharedPtr->localPort()==0);
    };
    auto theSocketIter = std::find_if( m_socketList.begin(), m_socketList.end(), socketFindLambda);
    if (theSocketIter == m_socketList.end())
    {
        std::string errMsg("Could not find socket with local addr ");
        errMsg += localAddr.toString().toStdString();
        errMsg += " and local port ";
        errMsg += std::to_string(localPort);
        throw std::logic_error(errMsg);
    }

    //arriving here we should have a valid iterator that points to a non-expired weak_ptr
    std::shared_ptr<UdpSocketStub> stubSharedPtr(*theSocketIter);
    stubSharedPtr->setPeerAddress(peerAddr);
    stubSharedPtr->setPeerPort(peerPort);
}


/**
 * @brief UdpSocketStubFactory::getNetworkStreamBySource
 * @param direction
 * @param sourceAddr
 * @param sourcePortNr the source port that the socket is bound to, or 0 for any port
 * @return
 */
SimulatedNetworkStream &UdpSocketStubFactory::getNetworkStreamBySource(UdpSocketStubFactory::StreamDirection direction, const QHostAddress &sourceAddr, uint16_t sourcePortNr)
{
    auto socketFindLambda = [&sourceAddr, sourcePortNr](const std::weak_ptr<UdpSocketStub> &socketStubWeakPtr)
    {
        if (socketStubWeakPtr.expired())
        {
            return false;
        }
        std::shared_ptr<UdpSocketStub> stubSharedPtr(socketStubWeakPtr);
        return (stubSharedPtr->localAddress()==sourceAddr || sourceAddr==QHostAddress::Any) &&
               (stubSharedPtr->localPort()==sourcePortNr  || sourcePortNr==0);
    };
    auto theSocketIter = std::find_if( m_socketList.begin(), m_socketList.end(), socketFindLambda);
    if (theSocketIter == m_socketList.end())
    {
        std::string errMsg("Could not find socket with local addr ");
        errMsg += sourceAddr.toString().toStdString();
        errMsg += " and local port ";
        errMsg += std::to_string(sourcePortNr);
        throw std::logic_error(errMsg);
    }

    //arriving here we should have a valid iterator that points to a non-expired weak_ptr
    std::shared_ptr<UdpSocketStub> stubSharedPtr(*theSocketIter);
    if (direction == StreamDirection::Input)
    {
        return stubSharedPtr->getInputStream();
    }
    else
    {
        return stubSharedPtr->getOutputStream();
    }
}


/**
 * @brief UdpSocketStubFactory::getNetworkStreamByDest
 * @param direction
 * @param destAddr the destination port that the socket 'connects' to, or QHostAddress::Any for any address
 * @param destPortNr the destination port that the socket 'connects' to, or 0 for any port
 * @throw std::logic_error if no network stream for destination address could be found
 * @return
 */
SimulatedNetworkStream &UdpSocketStubFactory::getNetworkStreamByDest(UdpSocketStubFactory::StreamDirection direction, const QHostAddress &destAddr, uint16_t destPortNr)
{
    auto socketFindLambda = [&destAddr, destPortNr](const std::weak_ptr<UdpSocketStub> &socketStubWeakPtr)
    {
        if (socketStubWeakPtr.expired())
        {
            return false;
        }
        std::shared_ptr<UdpSocketStub> stubSharedPtr(socketStubWeakPtr);
        return stubSharedPtr->peerAddress() == destAddr && (stubSharedPtr->peerPort()==destPortNr || stubSharedPtr->peerPort()==0);
    };
    auto theSocketIter = std::find_if( m_socketList.begin(), m_socketList.end(), socketFindLambda);
    if (theSocketIter == m_socketList.end())
    {
        std::string errMsg("Could not find socket with peer addr ");
        errMsg += destAddr.toString().toStdString();
        errMsg += " and peer port ";
        errMsg += std::to_string(destPortNr);
        throw std::logic_error(errMsg);
    }

    //arriving here we should have a valid iterator that points to a non-expired weak_ptr
    std::shared_ptr<UdpSocketStub> stubSharedPtr(*theSocketIter);
    if (direction == StreamDirection::Input)
    {
        return stubSharedPtr->getInputStream();
    }
    else
    {
        return stubSharedPtr->getOutputStream();
    }
}



} // QTFTP namespace end
