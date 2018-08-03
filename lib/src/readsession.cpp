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

#include "readsession.h"
#include "tftp_utils.h"
#include "tftp_constants.h"
#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif
#include <cassert>
#include <cmath>
#include <iostream>

using namespace std::string_literals;

namespace QTFTP
{

static constexpr unsigned int PopulationForAckTimeAverage = 20;

/**
 * @brief ReadSession::ReadSession
 * @param peerAddr
 * @param peerPort
 * @param rrqDatagram must contain a RRQ packet
 * @param filesDir
 * @param socketFactory
 *
 * ReadRequest package consists of:
 * <pre>
 *           2 bytes     string    1 byte     string   1 byte
 *           ------------------------------------------------
 *          | Opcode |  Filename  |   0  |    Mode    |   0  |
 *           ------------------------------------------------
 * </pre>
 *
 * Opcode for read request is 1. Mode should be either 'netascii' or 'octet'.
 */
ReadSession::ReadSession(const QHostAddress &peerAddr, uint16_t peerPort, QByteArray rrqDatagram,
                         QString filesDir, unsigned int slowNetworkThresholdUs, std::shared_ptr<UdpSocketFactory> socketFactory) : Session(peerAddr, peerPort, socketFactory),
                                                                                              m_blockNr(0),
                                                                                              m_blockSize(DefaultTftpBlockSize),
                                                                                              m_lastCharRead('\0'),
                                                                                              m_slowNetworkReported(false),
                                                                                              m_slowNetworkThresholdUs(slowNetworkThresholdUs)
{
    assert( ntohs( readWordInByteArray(rrqDatagram, 0) ) == TftpCode::TFTP_RRQ );

    unsigned int rrqOffset = 2;
    QString recvdFileName = QString(rrqDatagram.data() + rrqOffset);
    setFilePath(filesDir, recvdFileName);
    rrqOffset += static_cast<unsigned int>(recvdFileName.length()) + 1;

    QString mode = QString( rrqDatagram.data() + rrqOffset ).toLower();

    if ( mode == "netascii" )
    {
        setTransferMode(TftpCode::NetAscii);
    }
    else if ( mode == "octet" )
    {
        setTransferMode(TftpCode::Octet);
    }
    else if ( mode == "mail" )
    {
        // mail transfer mode not supported
        setState(State::InError, "'mail' transfer mode not supported");
        QByteArray errorDgram = assembleTftpErrorDatagram(TftpCode::IllegalOp, "Mail transfer not supported");
        sendDatagram(errorDgram);
        return;
        //TODO: destroy read session after return
    }
    else
    {
        //illegal transfer mode
        setState(State::InError, QString("RRQ contains illegal transfer mode ")+mode);
        QByteArray errorDgram = assembleTftpErrorDatagram(TftpCode::IllegalOp, "Illegal transfer mode");
        sendDatagram(errorDgram);
        return;
        //TODO: destroy read session after return
    }
    rrqOffset += static_cast<unsigned int>(mode.length()) + 1;

    if ( ! fileExists() )
    {
        setState(State::InError, "File not found");
        QByteArray errorDgram = assembleTftpErrorDatagram(TftpCode::FileNotFound, "File not found");
        sendDatagram(errorDgram);
        return;
        //TODO: destroy read session after return
    }
    if ( ! openFile(QIODevice::ReadOnly) )
    {
        setState(State::InError, "Could not open file");
        QByteArray errorDgram = assembleTftpErrorDatagram(TftpCode::Undefined, lastFileError());
        sendDatagram(errorDgram);
        return;
        //TODO: destroy read session after return
    }

    auto waitForOAck = handleRrqOptions(rrqDatagram, rrqOffset);
    if (!waitForOAck)
    {
        loadNextBlock();
        sendDataPacket();
    }
}


/**
 * @brief handle options appended after TFTP RRQ according to RFC2347
 * @param rrqDgram tftp Read Request datagram
 * @param offset offset where possible options start
 * @return true if options were present and an OACK was sent. False if no (recognised) options were present.
 *
 * The format of an OACK payload to acknowledge two options is this:
 * <pre>
 *   2 bytes      string     1 byte   string    1 byte   string    1 byte   string   1 byte
 *   --------------------------------------------------------------------------------------
 *   06 (OACK) |  optionName | 0  | optionValue  | 0 |  optionName | 0 | optionValue  | 0 |
 *   --------------------------------------------------------------------------------------
 * </pre>
 */
bool ReadSession::handleRrqOptions(const QByteArray &rrqDgram, unsigned int offset)
{
    unsigned int maxOffset = static_cast<unsigned int>(rrqDgram.size()) - 1;
    uint16_t oackOpCode(TftpCode::TFTP_OACK);
    uint16_t tempWord( htons(oackOpCode) );
    QByteArray oackDatagram = QByteArray::fromRawData( reinterpret_cast<char*>(&tempWord), sizeof(tempWord) );
    while (offset < maxOffset)
    {
        QString optionName = QString(rrqDgram.data() + offset).toLower();
        offset += static_cast<unsigned int>(optionName.size()) + 1;
        QString optionValueStr = QString(rrqDgram.data() + offset);
        if (optionName == "blksize")
        {
            //RFC2348
            bool convOk;
            unsigned int blockSize = optionValueStr.toUInt(&convOk, 10);
            if (!convOk || blockSize<8 || blockSize>65464)
            {
                continue;
            }
            m_blockSize = blockSize;
            oackDatagram.append(static_cast<const char*>(optionName.toLatin1()));
            oackDatagram.append(char(0x0));
            oackDatagram.append(static_cast<const char*>(optionValueStr.toLatin1()));
            oackDatagram.append(char(0x0));
        }
        else if (optionName == "timeout")
        {
            //RFC2349
            bool convOk;
            unsigned int retransmitTimeout = optionValueStr.toUInt(&convOk, 10);
            if (!convOk || retransmitTimeout<1 || retransmitTimeout>255)
            {
                continue;
            }
            setRetransmitTimeOut(retransmitTimeout * 1000);
            oackDatagram.append(static_cast<const char*>(optionName.toLatin1()));
            oackDatagram.append(char(0x0));
            oackDatagram.append(static_cast<const char*>(optionValueStr.toLatin1()));
            oackDatagram.append(char(0x0));
        }
        else if (optionName == "tsize")
        {
            //RFC2349
            bool convOk;
            unsigned int tsize = optionValueStr.toUInt(&convOk, 10);
            if (!convOk || tsize != 0)
            {
                continue;
            }
            oackDatagram.append(static_cast<const char*>(optionName.toLatin1()));
            oackDatagram.append(char(0x0));
            QString fileSizeStr = QString::number(fileSize(), 10);
            oackDatagram.append(static_cast<const char*>(fileSizeStr.toLatin1()));
            oackDatagram.append(char(0x0));
        }
    }

    if (static_cast<unsigned long>(oackDatagram.size()) > sizeof(u_int16_t))
    {
        sendDatagram(oackDatagram, false); //TODO: start retransmit timer after OACK sent ?
        setState(State::OptionsNegotation);
        return true;
    }

    return false;
}

/**
 * @brief ReadSession::averageAckDelayMs calculate average time between sending data package and receiving its ACK datagram
 * @return average time between sent and ACK in us or 0 if no ACK msg has been received yet
 */
unsigned ReadSession::averageAckDelayUs() const
{
    if (m_ackTimes.size() == 0)
    {
        return 0;
    }
    auto delaySum = std::accumulate(m_ackTimes.begin(), m_ackTimes.end(), 0);
    float averageDelay = delaySum / static_cast<float>(m_ackTimes.size());
    return static_cast<unsigned int>(averageDelay + 0.5f);
}


/**
 * @brief ReadSession::dataReceived handles incoming data for this read session
 *
 * Call this function when new data on the UDP socket of this session is available. It will
 * validate the data (only ACK datagrams are expected for read sessions) and if valid, load
 * the next block of data from the source file and send it over the UDP session socket.
 */
void ReadSession::dataReceived()
{
    if (state() == State::InError)
    {
        //we already send an error response, ignore further datagrams
        return;
    }
    if (state() == State::Finished)
    {
        setState(State::InError, "Received data when already finished");
        QByteArray errorDgram = assembleTftpErrorDatagram(TftpCode::IllegalOp, "Unexpected TFTP opcode");
        sendDatagram(errorDgram);
        return;
    }

    QByteArray datagram;
    readDatagram(datagram);

    uint16_t opCode =  ntohs( reinterpret_cast<uint16_t*>(datagram.data())[0] );
    if (state() == State::OptionsNegotation)
    {
        if (opCode == TftpCode::TFTP_ERROR)
        {
            uint16_t errCode =  ntohs( reinterpret_cast<uint16_t*>(datagram.data())[1] );
            if (errCode == TftpCode::OptionNegotiationAbort || errCode == TftpCode::DiskFull)
            {
                //client aborted the transfer during options negatiation
                setState(State::Finished);
                return;
            }
        }
    }

    // From here ReadSession should only receive ACK datagrams
    if (datagram.size() < 4)
    {
        setState(State::InError, "Received malformed datagram");
        QByteArray errorDgram = assembleTftpErrorDatagram(TftpCode::Undefined, "Malformed datagram");
        sendDatagram(errorDgram);
        return;
    }

    if (opCode != TftpCode::TFTP_ACK)
    {
        setState(State::InError, QString("Unexpected opcode ")+QString::number(opCode, 10));
        QByteArray errorDgram = assembleTftpErrorDatagram(TftpCode::IllegalOp, "Unexpected TFTP opcode");
        sendDatagram(errorDgram);
        return;
    }

    //we received an ACK, so stop re-transmit timer
    stopRetransmitTimer();

    uint16_t ackBlockNr =  ntohs( reinterpret_cast<uint16_t*>(datagram.data())[1] );
    //static auto lastSend = std::chrono::high_resolution_clock::now();
    //auto currTime = std::chrono::high_resolution_clock::now();
    //std::cerr <<  std::chrono::duration_cast<std::chrono::microseconds>(currTime-m_debugStartTime).count()  << ": received ack " << ackBlockNr << "after "
    //          << std::chrono::duration_cast<std::chrono::microseconds>(currTime-lastSend).count() << " us since send" << std::endl;
    if (m_blockNr > 0 && ackBlockNr == (m_blockNr-1))
    {
        //duplicate ACK received, ignore because data packet was already sent when we received the previous ACK
    //    std::cerr << std::chrono::duration_cast<std::chrono::microseconds>(currTime-m_debugStartTime).count() << ": duplicate ack recvd" << std::endl;
        return;
    }
    if (ackBlockNr != m_blockNr)
    {
        setState(State::InError, QString("Received ACK with wrong blocknr"));
        QByteArray errorDgram = assembleTftpErrorDatagram(TftpCode::IllegalOp, "Ack contains wrong block number");
    //    std::cerr << std::chrono::duration_cast<std::chrono::microseconds>(currTime-m_debugStartTime).count() << ": wrong ack recvd, send error" << std::endl;
        sendDatagram(errorDgram);
        return;
    }

    if (state() == State::OptionsNegotation)
    {
        setState(State::Busy);
    }
    else if (static_cast<unsigned int>(m_blockToSend.size()) < m_blockSize)
    {
        //valid ACK received, this transfer is finished when the last block that was sent is less than the maximum block size
        setState(State::Finished);
        return;
    }

    // keep running average of time it takes to receive ack from client
    std::chrono::high_resolution_clock::time_point ackRecvTime = std::chrono::high_resolution_clock::now();
    if (!m_slowNetworkReported && m_blockNr > 0)
    {
        assert(m_previousSendTime != std::chrono::high_resolution_clock::time_point());
        if (m_ackTimes.size() > PopulationForAckTimeAverage)
        {
            m_ackTimes.erase(m_ackTimes.begin());
        }
        auto ackDelay = ackRecvTime - m_previousSendTime;
        auto ackTimeus = std::chrono::duration_cast<std::chrono::microseconds>(ackDelay).count();
        //std::cerr << "ack time: " << ackTimeus << " , blocknr " << m_blockNr << std::endl;
        if (ackTimeus < 0)
        {
            ackTimeus = 0;
        }
        m_ackTimes.push_back(static_cast<unsigned int>(ackTimeus));
        if ( (m_blockNr % 5 == 0) &&
             (averageAckDelayUs() > m_slowNetworkThresholdUs) )
        {
            emit slowNetwork();
            m_slowNetworkReported = true;
        }
    }

    //load and send next block of file
    loadNextBlock();
    //currTime = std::chrono::high_resolution_clock::now();
    //std::cerr << std::chrono::duration_cast<std::chrono::microseconds>(currTime-m_debugStartTime).count() << ": after loadNextBlock" << std::endl;
    sendDataPacket();
    //currTime = lastSend = std::chrono::high_resolution_clock::now();
    //std::cerr << std::chrono::duration_cast<std::chrono::microseconds>(currTime-m_debugStartTime).count() << ": after sendDataPacket" << std::endl;
}


/**
 * @brief ReadSession::retransmitData retransmit the last data block message
 */
void ReadSession::retransmitData()
{
    //TODO: when state is optionsNegotation re-send OACK
    sendDataPacket(true);
}


/**
 * @brief ReadSession::loadNextBlock loads the next block of data from the source file
 *
 * If this function is called for a netascii session, line endings will be converted (CR -> CR,0 and LF -> CR,LF).
 */
void ReadSession::loadNextBlock()
{
    assert( isFileOpen() );

    m_blockToSend.clear();

    if (atEndOfFile())
    {
        //Already read entire file. This can happen when the file size is an exact multiple of the block size.
        //In this case we don't read anything, and an empty block will be sent as the last DATA datagram.
        return;
    }

    int lineEndConversionStartIndex = 0;
    if (transferMode() == TftpCode::NetAscii && !m_asciiOverflowBuffer.isEmpty())
    {
        //There is some leftover from previous file read after line ending conversion. Add this to the next block
        //that we will send before reading contents from file.
        m_blockToSend = m_asciiOverflowBuffer;
        m_asciiOverflowBuffer.clear();
        //Make sure that line endings in overflow buffer are not converted for 2nd time.
        lineEndConversionStartIndex = m_blockToSend.size();
    }

    if ( ! readFromFile(m_blockToSend, m_blockSize))
    {
        throw TftpError("Read error while reading from file "s + filePath().toStdString());
    }

    if (transferMode() == TftpCode::NetAscii)
    {
        //convert line endings and CR char, making sure that block size is not exceeded
        for (int index=lineEndConversionStartIndex; index<m_blockToSend.size(); ++index)
        {
            if ( static_cast<char>(m_blockToSend[index]) == 0x0D )
            {
                //convert CR -> CR,0
                ++index;
                m_blockToSend.insert(index, '\0');
            }
            else if ( static_cast<char>(m_blockToSend[index]) == 0x0A)
            {
                //convert LF -> CR,LF
                m_blockToSend.insert(index, 0x0D);
                ++index;
            }
        }

        //If m_blockToSend became bigger than TftpBlockSize due to linefeed conversions,
        //store the surplus in an overflow buffer for the next block.
        if (static_cast<unsigned int>(m_blockToSend.size()) > m_blockSize)
        {
            int surplusNrOfChars = m_blockToSend.size() - static_cast<int>(m_blockSize);
            m_asciiOverflowBuffer = m_blockToSend.right(surplusNrOfChars);
            m_blockToSend.chop(surplusNrOfChars);
        }
    }
}


/**
 * @brief ReadSession::sendDataPacket send a prepared data block
 * @param isRetransmit if true the data block sequence number will not be increased
 *
 * sendDataPacket() will assemble a TFTP data message and include the data prepared in m_blockToSend. If the previously sent
 * datablock has to be retransmitted, set \p isRetransmit to true. The data block sequence nr will not be increased in that case.
 */
void ReadSession::sendDataPacket(bool isRetransmit)
{
    uint16_t dataOpCode(TftpCode::TFTP_DATA);
    uint16_t tempWord( htons(dataOpCode) );
    QByteArray datagram = QByteArray::fromRawData( reinterpret_cast<char*>(&tempWord), sizeof(tempWord) );
    if (! isRetransmit)
    {
        ++m_blockNr;
    }
    uint16_t tempWord2 = htons(m_blockNr); //don't re-use tempWord because QByteArray::fromRawData makes a shallow copy
    datagram.append( reinterpret_cast<char*>(&tempWord2), sizeof(tempWord2) );
    datagram.append( m_blockToSend );
    assert( static_cast<unsigned int>(datagram.size()) <= (m_blockSize + 4) );

    m_previousSendTime = std::chrono::high_resolution_clock::now();
    sendDatagram(datagram, true);
    unsigned int progressPerc = static_cast<unsigned int>(float(posInFile()) / fileSize() * 100.0f + 0.5f);
    assert(progressPerc <= 100);
    emit progress(progressPerc);
}




} // QTFTP namespace end
