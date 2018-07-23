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

#include "tftpserver.h"
#include "tftp_constants.h"
#include "udpsocketstubfactory.h"
#include "simulatednetworkstream.h"
#include "tftp_error.h"
#include <QByteArray>
#include <QTest>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

namespace QTFTP
{


class TftpServerTest : public QObject
{
    Q_OBJECT

    public:

        TftpServerTest() : m_tftpServer(m_socketFactory=std::make_shared<UdpSocketStubFactory>(), nullptr)
        {
            if (m_rrqOpcode == 0x0)
            {
                m_rrqOpcode = htons(0x0001);
            }
            try
            {
                m_tftpServer.bind(TFTP_TEST_FILES_DIR, QHostAddress::Any, 2345);
            }
            catch(const QTFTP::TftpError &/*tftpErr*/)
            {
                QFAIL("Main socket bind fail...");
            }

            //give the simulation socket of our tftp server the peer ip addr and port of a random tftp client
            m_socketFactory->setSocketPeer(QHostAddress::Any, 2345, QHostAddress("10.6.11.201"), 1923);
        }

        std::shared_ptr<UdpSocketStubFactory> m_socketFactory; //we need to hold on to this to retrieve sockets later
        TftpServer m_tftpServer;
        static uint16_t m_rrqOpcode;

    private slots:
        void readRequestSendsNoOutputOnMainSocket();
        void readRequestSendsDataPacketOnSessionSocket();
};

uint16_t TftpServerTest::m_rrqOpcode( 0x0 );




void TftpServerTest::readRequestSendsNoOutputOnMainSocket()
{
    SimulatedNetworkStream &inputNetworkStream = m_socketFactory->getNetworkStreamBySource(UdpSocketStubFactory::StreamDirection::Input, QHostAddress::Any, 2345);
    QByteArray rrqDatagram = QByteArray::fromRawData(reinterpret_cast<char*>(&m_rrqOpcode), sizeof(m_rrqOpcode));
    rrqDatagram.append("16_byte_file.txt"); // name of requested file
    rrqDatagram.append(char(0x0));           // terminating \0 of filename
    rrqDatagram.append("octet");             // transfer mode
    rrqDatagram.append(char(0x0));           // terminating \0 of transfer mode
    inputNetworkStream << rrqDatagram;
    SimulatedNetworkStream &outputNetworkStream = m_socketFactory->getNetworkStreamBySource(UdpSocketStubFactory::StreamDirection::Output, QHostAddress::Any, 2345);
    QCOMPARE(outputNetworkStream.str().size(), size_t(0));
}



void TftpServerTest::readRequestSendsDataPacketOnSessionSocket()
{
    //give the simulation socket of our tftp server the peer ip addr and port of a different tftp client,
    //so that we can retrieve the simulated network stream of the read session that will be constructed in this test
    m_socketFactory->setSocketPeer(QHostAddress::Any, 2345, QHostAddress("10.6.11.202"), 1456);

    SimulatedNetworkStream &inputNetworkStream = m_socketFactory->getNetworkStreamBySource(UdpSocketStubFactory::StreamDirection::Input, QHostAddress::Any, 2345);
    QByteArray rrqDatagram = QByteArray::fromRawData(reinterpret_cast<char*>(&m_rrqOpcode), sizeof(m_rrqOpcode));
    rrqDatagram.append("16_byte_file.txt"); // name of requested file
    rrqDatagram.append(char(0x0));           // terminating \0 of filename
    rrqDatagram.append("octet");             // transfer mode
    rrqDatagram.append(char(0x0));           // terminating \0 of transfer mode
    inputNetworkStream << rrqDatagram;
    //give the (simulated) main socket of our tftp server the peer ip addr and port of a different tftp client
    //so that we can find the created session socket
    m_socketFactory->setSocketPeer(QHostAddress::Any, 2345, QHostAddress("10.6.11.203"), 1345);

    try
    {
        SimulatedNetworkStream &outputSessionStream = m_socketFactory->getNetworkStreamByDest(UdpSocketStubFactory::StreamDirection::Output, QHostAddress("10.6.11.202"), 1456);
        QCOMPARE(outputSessionStream.str().size(), size_t(20)); //output to client should be 2 byte opcoe + 2 byte blocknr + 16 byte file data
    }
    catch (std::logic_error &/*err*/)
    {
        QFAIL("Test failed because simulated network stream couldn't be found.");
    }

    //correct contents of data packet is tested in unit tests of class ReadSession
}

//TODO: If a host receives a octet file and then returns it, the returned file must be identical to the original.


} // namespace QTFTP end

QTEST_MAIN(QTFTP::TftpServerTest)
#include "tftpserver_ut.moc"
