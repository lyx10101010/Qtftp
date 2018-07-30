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

#ifndef READSESSION_H
#define READSESSION_H

#include "session.h"
#include "udpsocketfactory.h"
#include <QByteArray>
#include <chrono>

namespace QTFTP
{


class ReadSession : public Session
{
    Q_OBJECT

    public:
        ReadSession(const QHostAddress &peerAddr, uint16_t peerPort, QByteArray rrqDatagram, QString filesDir,
                    unsigned int slowNetworkThresholdUs, std::shared_ptr<UdpSocketFactory> socketFactory=std::make_shared<UdpSocketFactory>());

        unsigned averageAckDelayUs() const;
        uint16_t currBlockNr() const;

    signals:
        void progress(unsigned int progressPerc);

    protected slots:
        void dataReceived() override;
        void retransmitData() override;

    private:
        void loadNextBlock();
        void sendDataPacket(bool isRetransmit=false);
        bool handleRrqOptions(const QByteArray &rrqDgram, unsigned int offset);

        uint16_t     m_blockNr;
        unsigned int m_blockSize;
        QByteArray   m_blockToSend;
        QByteArray   m_asciiOverflowBuffer; /// used if block size is exceeded after CR/LF conversions
        char         m_lastCharRead;        ///needed for CR/LF conversion in netascii mode
        std::chrono::high_resolution_clock::time_point m_previousSendTime;
        std::vector<unsigned int> m_ackTimes; /// used to calculate average time delay between data sent and ack received
        bool         m_slowNetworkReported;
        unsigned int m_slowNetworkThresholdUs; /// threshold for time between data package sending and ack receipt
};


} // QTFTP namespace end

#endif // READSESSION_H
