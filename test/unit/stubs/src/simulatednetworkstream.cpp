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

#include "simulatednetworkstream.h"

namespace LIBTFTP
{



SimulatedNetworkStream::SimulatedNetworkStream(QObject *parent) : QObject(parent)
{

}


std::string SimulatedNetworkStream::str() const
{
    return m_stream.str();
}


//const std::string &SimulatedNetworkStream::sourceIpAddress() const
//{
//    return m_sourceIpAddress;
//}


//uint16_t SimulatedNetworkStream::sourcePort() const
//{
//    return m_sourcePort;
//}


//const std::string &SimulatedNetworkStream::destIpAddress() const
//{
//    return m_destIpAddress;
//}


//uint16_t SimulatedNetworkStream::destPort() const
//{
//    return m_destPort;
//}


LIBTFTP::SimulatedNetworkStream::operator bool() const
{
    return m_stream ? true : false;
}


//void SimulatedNetworkStream::setSourceIpAddress(const std::string &sourceAddress)
//{
//    m_sourceIpAddress = sourceAddress;
//}



//void SimulatedNetworkStream::setSourcePort(uint16_t sourcePort)
//{
//    m_sourcePort = sourcePort;
//}

//void SimulatedNetworkStream::setDestIpAddress(const std::string &destAddress)
//{
//    m_destIpAddress = destAddress;
//}

//void SimulatedNetworkStream::setDestPort(uint16_t destPort)
//{
//    m_destPort = destPort;
//}


SimulatedNetworkStream &SimulatedNetworkStream::operator<<(const std::string &outData)
{
    if ( ! outData.empty() )
    {
        m_stream << outData;
        emit newData();
    }

    return *this;
}


SimulatedNetworkStream &SimulatedNetworkStream::operator<<(const QByteArray &outData)
{
    if ( ! outData.isEmpty() )
    {
        for(int byteNr=0; byteNr<outData.length(); ++byteNr)
        {
            m_stream << outData[byteNr];
        }

        emit newData();
    }
    return *this;
}


SimulatedNetworkStream &SimulatedNetworkStream::operator>>(QByteArray &inData)
{
    inData.resize(m_stream.str().length());
    m_stream.read(inData.data(), inData.size());
    //the read call doesn't remove the data from the stringstream, so make it empty ourself
    m_stream.clear(); //clears any state bits that are set
    m_stream.str(std::string()); //makes the stringstream empty
    return *this;
}


} //namespace LIBTFTP end
