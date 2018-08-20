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

#include "include/session.h"

#include "udpsocketfactory.h"
#include "tftp_error.h"
#include <QFileInfo>
#include <QDir>
#include <string>
#include <cassert>

using namespace std::string_literals;

namespace QTFTP
{

SessionIdent::SessionIdent(const QHostAddress &address, uint16_t port) : m_address(address), m_port(port)
{
}


bool SessionIdent::operator==(const SessionIdent &otherIdent) const
{
    return m_address==otherIdent.m_address && m_port==otherIdent.m_port;
}


unsigned int Session::m_retransmitTimeOut = DefaultRetransmitTimeOutms;
unsigned int Session::m_maxRetransmissions = DefaultMaxRetryCount;


Session::Session(const QHostAddress &peerAddr, uint16_t peerPort,
                 std::shared_ptr<UdpSocketFactory> socketFactory, QObject *parent) : QObject(parent),
                                                                    m_file(nullptr),
                                                                    m_sessionSocket(socketFactory->createNewSocket()),
                                                                    m_retransmitCount(0),
                                                                    m_peerIdent(peerAddr, peerPort),
                                                                    m_transferMode(TftpCode::Octet),
                                                                    m_state(State::Busy)
{
    //port==0 means: choose random free port
    m_sessionSocket->bind(QHostAddress::Any, 0);
    m_retransmitTimer.setSingleShot(true);
    connect(m_sessionSocket.get(), &AbstractSocket::readyRead, this, &Session::dataReceived);
    connect(&m_retransmitTimer, &QTimer::timeout, this, &Session::handleExpiredRetransmitTimer);
}


Session::~Session()
{
}

Session::State Session::state() const
{
    return m_state;
}


QString Session::filePath() const
{
    return m_file.fileName();
}


bool Session::fileExists() const
{
    return m_file.exists();
}


bool Session::atEndOfFile() const
{
    return m_file.atEnd();
}


qint64 Session::posInFile() const
{
    return m_file.pos();
}


qint64 Session::fileSize() const
{
    return m_file.size();
}

quint16 Session::localPort() const
{
    return m_sessionSocket ? m_sessionSocket->localPort() : 0;
}


QString Session::lastFileError() const
{
    return m_file.errorString();
}


TftpCode::Mode Session::transferMode() const
{
    return m_transferMode;
}


bool Session::operator==(const SessionIdent &sessionIdent) const
{
    return m_peerIdent == sessionIdent;
}

bool Session::operator==(const Session &otherSession) const
{
    return m_peerIdent == otherSession.m_peerIdent;
}


/**
 * @brief Session::peerIdent get the identification information of the peer of this TFTP session
 * @return a reference to a SessionIdent struct that contains IP address and port of the peer
 */
const SessionIdent & Session::peerIdent() const
{
    return m_peerIdent;
}


bool Session::openFile(QIODevice::OpenModeFlag openMode)
{
    return m_file.open(openMode);
}


/**
 * @brief Session::readFromFile read data from source file into buffer
 * @param buffer the destination where the data read from file will be stored
 * @param maxSize the maximum size that the buffer will have after this function completed (unless end of file was reached before)
 * @return true if read action succeedded, false if an error occurred.
 *
 * Note that \p maxSize is the maximum size of \p buffer after completion of this function. In other words,
 * if \p buffer contains already \p maxSize characters when this function is called, no additional characters will
 * be read from file.
 */
bool Session::readFromFile(QByteArray &buffer, qint64 maxSize)
{
    //function QByteArray QFile::read(qint64 maxSize) has no way to report read errors, so use an overloaded variant
    qint64 bytesToRead = std::min(maxSize, m_file.bytesAvailable());
    bytesToRead = std::min(maxSize-buffer.size(), bytesToRead);
    qint64 bytesInBuffer = buffer.size();
    if (bytesToRead)
    {
        buffer.resize( buffer.size() + bytesToRead );
    }
    assert(bytesInBuffer+bytesToRead <= buffer.size());
    qint64 bytesRead = m_file.read(buffer.data()+bytesInBuffer, bytesToRead);

    return (bytesRead != -1);
}


/**
 * @brief Session::setRetransmitTimeOut
 * @param newTimeOut time to wait for acknowledgement of sent datagram before retransmitting in msec
 *
 * Change the time to wait for acknowledgement of a datagram that was sent to our peer before retransmitting the same
 * datagram again. The new time-out value will be used for new datagrams. Datagrams that are already sent and are waiting
 * for an acknowledgement will continue to use the time-out value that was set at the time the datagram was sent.
 */
void Session::setRetransmitTimeOut(unsigned int newTimeOut)
{
    m_retransmitTimeOut = newTimeOut;
}


void Session::setMaxRetransmissions(unsigned int newMax)
{
    m_maxRetransmissions = newMax;
}


bool Session::isFileOpen() const
{
    return m_file.isOpen();
}

void Session::setTransferMode(TftpCode::Mode newMode)
{
    m_transferMode = newMode;
}


void Session::setFilePath(const QString &directory, const QString &fileName)
{
    if (m_file.isOpen())
    {
        m_file.close();
    }

    m_file.setFileName( QFileInfo(QDir(directory), fileName).absoluteFilePath() );
}


/**
 * @brief Session::sendDatagram send a datagram to our session peer
 * @param datagram the payload of the datagram to send
 * @throw TftpError if there was an error while sending the datagram
 */
void Session::sendDatagram(QByteArray datagram, bool startRetransmitTimer)
{
    if( m_sessionSocket->writeDatagram(datagram, m_peerIdent.m_address, m_peerIdent.m_port) == -1 )
    {
        throw TftpError( "Error sending tftp error datagram to "s + m_peerIdent.m_address.toString().toStdString() +
                         " port " + std::to_string(m_peerIdent.m_port));
    }

    if (startRetransmitTimer)
    {
        m_retransmitTimer.start(m_retransmitTimeOut);
    }
}


void Session::stopRetransmitTimer()
{
    m_retransmitTimer.stop();
}


void Session::handleExpiredRetransmitTimer()
{
    if (m_retransmitCount < m_maxRetransmissions)
    {
        retransmitData();
        ++m_retransmitCount;
        return;
    }

    //max nr of retransmissions reached
    setState(State::InError, QObject::tr("Maximum nr of re-transmissions reached"));
}


void Session::setState(Session::State newState, QString msg)
{
    m_state = newState;
    if (m_state == State::Finished)
    {
        emit finished();
    }
    else if (m_state == State::InError)
    {
        emit error(msg);
    }
}


/**
 * @brief Session::readDatagram read a datagram from the session socket
 * @param datagram reference to the bytearray where the datagram will be stored
 * @param peerAddress if not null the address of the sender will be stored in this parameter
 * @param peerPort if not null the port of the sender will be stored in this parameter
 * @throw TftpError if an error occurrs while reading from the session socket
 */
void Session::readDatagram(QByteArray &datagram, QHostAddress *peerAddress, quint16 *peerPort)
{
    int dgramSize = static_cast<int>(m_sessionSocket->pendingDatagramSize());
    datagram.resize( dgramSize );
    if (m_sessionSocket->readDatagram( datagram.data(), dgramSize, peerAddress, peerPort ) == -1)
    {
        throw TftpError("Error while reading data from read session socket (port "s + std::to_string(m_sessionSocket->localPort()) + ")");
    }
}


} // namespace QTFTP end
