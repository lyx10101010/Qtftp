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

#ifndef SIMULATEDNETWORKSTREAM_H
#define SIMULATEDNETWORKSTREAM_H

#include <QObject>
#include <sstream>

class QByteArray;

namespace QTFTP
{

/**
 * @brief The SimulatedNetworkStream class
 *
 * Used to unit test the QTFTP classes without need for a physical network
 */
class SimulatedNetworkStream : public QObject
{
    Q_OBJECT

    public:
        explicit SimulatedNetworkStream(QObject *parent=nullptr);

        std::string str() const;
        const std::string &sourceIpAddress() const;
        uint16_t sourcePort() const;
        operator bool() const;

        void reset();

        SimulatedNetworkStream &operator<<(const std::string &outData);
        SimulatedNetworkStream &operator<<(const QByteArray &outData);
        SimulatedNetworkStream &operator>>(QByteArray &outData);

    signals:
        void newData();

    private:
//        std::string m_sourceIpAddress;
//        uint16_t    m_sourcePort;
//        std::string m_destIpAddress;
//        uint16_t    m_destPort;
        std::stringstream m_stream;
};




} //namespace QTFTP end

#endif // SIMULATEDNETWORKSTREAM_H
