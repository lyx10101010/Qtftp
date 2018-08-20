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

#include "qtftp/tftpserver.h"
#include "qtftp/readsession.h"
#include "qtftp/tftp_error.h"
#include "qtftp/tftp_constants.h"
#include "qtftp/tftp_utils.h"
#include "qtftp/udpsocketfactory.h"
#include "qtftp/udpsocket.h"
#include <QDir>
#include <QByteArray>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif
#include <cassert>
#include <iostream> //TODO: remove after debug

using namespace std::string_literals;

namespace QTFTP
{


/**
 * @brief bind socket to provided address and port
 * @param address local address to bind to
 * @param port local port to bind to
 * @param mode bind mode to use
 * @throw TftpError if socket could not be bound successfully
 */
ConnectionRequestSocket::ConnectionRequestSocket(const QString &filesDir, std::shared_ptr<UdpSocketFactory> socketFactory) : m_socket(socketFactory->createNewSocket(this)),
                                                                                                                             m_filesDir(filesDir)

{
    connect(m_socket.get(), &AbstractSocket::readyRead, this, &ConnectionRequestSocket::readyRead);
}


const QString &ConnectionRequestSocket::filesDir() const
{
    return m_filesDir;
}

QString ConnectionRequestSocket::errorString() const
{
    return m_socket->errorString();
}

bool ConnectionRequestSocket::hasPendingDatagrams() const
{
    return m_socket->hasPendingDatagrams();
}

qint64 ConnectionRequestSocket::pendingDatagramSize() const
{
    return m_socket->pendingDatagramSize();
}

quint16 ConnectionRequestSocket::localPort() const
{
    return m_socket->localPort();
}

bool ConnectionRequestSocket::bind(const QHostAddress &address, quint16 port, QAbstractSocket::BindMode mode)
{
    return m_socket->bind(address, port, mode);
}

qint64 ConnectionRequestSocket::readDatagram(char *data, qint64 maxSize, QHostAddress *address, quint16 *port)
{
    return m_socket->readDatagram(data, maxSize, address, port);
}

qint64 ConnectionRequestSocket::writeDatagram(const QByteArray &datagram, const QHostAddress &host, quint16 port)
{
    return m_socket->writeDatagram(datagram, host, port);
}

void ConnectionRequestSocket::close()
{
    m_socket->close();
}



/**
 * @brief TftpServer::TftpServer
 * @param filesDir
 * @param hostAddr
 * @param portNr
 * @param parent
 *
 * @throw TftpError if could not bind server socket to \p hostAddr and \p portNr
 */

TftpServer::TftpServer(std::shared_ptr<UdpSocketFactory> socketFactory, QObject *parent) : QObject(parent),
                                                                                                                    m_socketFactory(socketFactory),
                                                                                                                    //m_mainSocket( std::static_pointer_cast<UdpSocket>(m_socketFactory->createNewSocket()) ),
                                                                                                                    m_slowNetworkThreshold(2000)
{
}


//TftpServer::TftpServer(std::shared_ptr<UdpSocketFactory> socketFactory, QObject *parent) : TftpServer(QString(), socketFactory, parent)
//{
//}


/**
 * @brief bind tftpserver main socket
 * @param filesDir read from or write to files requested from \p hostaddr in this directory
 * @param hostAddr listen only for tftp requests originating from this address
 * @param port udp port on which the tftp server will listen for new connections
 * @throw TtftpError if an error occurred while binding this server's socket to
 * \p hostAddr and \p portNr.
 */
void TftpServer::bind(const QString &filesDir, const QHostAddress &hostAddr, uint16_t port)
{
    QDir tftpFileDir(filesDir);
    if ( filesDir.isEmpty() || !tftpFileDir.exists() || !tftpFileDir.isReadable())
    {
        throw TftpError("File directory for tftp server "s + filesDir.toStdString() + " does not exist or is not readable");
    }

    auto newSocket = std::make_shared<ConnectionRequestSocket>(filesDir, m_socketFactory);
    connect(newSocket.get(), &ConnectionRequestSocket::readyRead, this, &TftpServer::dataReceived);
    if ( ! newSocket->bind(hostAddr, port) )
    {
        std::string errorMsg( "Could not bind tftp server to host address ");
        errorMsg += hostAddr.toString().toStdString();
        errorMsg += " at port ";
        errorMsg += std::to_string(port);
        errorMsg += ". "s + newSocket->errorString().toStdString();
        throw TftpError(errorMsg);
    }
    m_mainSockets.push_back(newSocket);
}


/**
 * @brief TftpServer::close stop listening for new tftp requests
 */
void TftpServer::close()
{
    for (auto &nextSocket : m_mainSockets)
    {
        nextSocket->close();
    }
}


/**
 * @brief TftpServer::setSlowNetworkDetectionThreshold set the threshold for what is considered a slow network
 * @param ackLatencyUs threshold in microsec for latency between sending data packet and receiving corresponding ack
 *
 * The TftpServer will for each session keep a running average of the time between sending a data package and receipt of
 * the corresponding ack datagram. If this average time exceeds \p ackLatencyUs the slowNetwork signal will be emitted
 * for that session. The signal will be emitted only once for each session.
 */
void TftpServer::setSlowNetworkDetectionThreshold(unsigned int ackLatencyUs)
{
    m_slowNetworkThreshold = ackLatencyUs;
}


std::shared_ptr<const ReadSession> TftpServer::findReadSession(const SessionIdent &sessionIdent) const
{
    auto readSesPtr = doFindReadSession(sessionIdent);
    return readSesPtr;
}

void TftpServer::handleNewData(std::shared_ptr<ConnectionRequestSocket> mainSocket)
{
    QByteArray dgram;
    int dgramSize = static_cast<int>(mainSocket->pendingDatagramSize());
    dgram.resize( dgramSize );
    QHostAddress peerAddress;
    quint16 peerPort;

    while (mainSocket->hasPendingDatagrams())
    {
        if (mainSocket->readDatagram( dgram.data(), dgramSize, &peerAddress, &peerPort ) == -1)
        {
            auto lastErrStr = mainSocket->errorString().toStdString();
            throw TftpError("Error while reading data from tftp socket (port "s + std::to_string(mainSocket->localPort()) + ")" + lastErrStr);
        }

        auto opcode = ntohs( readWordInByteArray(dgram, 0) );
        switch( opcode )
        {
            case TftpCode::TFTP_RRQ:
                {
                    std::shared_ptr<ReadSession> readSession = doFindReadSession(SessionIdent(peerAddress, peerPort));
                    if ( readSession )
                    {
                        // YMP-70: ignore duplicate RRQ until we have time to find out why client sends them
                        /*
                        QByteArray errorDgram = assembleTftpErrorDatagram(TftpCode::IllegalOp, "Duplicate read request from same peer");
                        if (m_mainSocket->writeDatagram(errorDgram, peerAddress, peerPort) == -1)
                        {
                            throw TftpError("Error while sending error datagram to client "s + peerAddress.toString().toStdString() + ":" + std::to_string(peerPort));
                        }
                        */
                        return;
                    }

                    readSession = std::make_shared<ReadSession>(peerAddress, peerPort, dgram, mainSocket->filesDir(), m_slowNetworkThreshold, m_socketFactory);
                    connect(readSession.get(), &Session::finished, this, &TftpServer::removeSession);
                    connect(readSession.get(), &Session::error, this, &TftpServer::removeSession);
                    m_readSessions.push_back( readSession );
                    emit newReadSession(readSession);
                }
                break;
            /*case ACK: {
                    ReadSession *rs = findRSession(ti);
                    if ( !rs )
                    {
                        sendError(ti, IllegalOp, "ACK packet without a RRQ");
                        return;
                    }

                    if ( rs->parseAck(dgram) )
                    {
                        emit sentFile(rs->currentFile(), rs->currentFilename());
                        std::vector<ReadSession*>::iterator newEnd = std::remove(reads.begin(), reads.end(), rs);
                        reads.erase(newEnd, reads.end());
                        //reads.remove(rs);
                        delete rs;
                    }
                } break;
            case WRQ: {
                    WriteSession *ws = findWSession(ti);
                    if ( ws )
                    {
                        qWarning("Duplicate write request from same peer");
                        sendError(ti, IllegalOp, "Duplicate write request from same peer");
                        return;
                    }

                    writes.push_back( new WriteSession(ti, dgram) );
                } break;
            case DATA: {
                    WriteSession *ws = findWSession(ti);
                    if ( !ws )
                    {
                        qWarning("DATA packet without a WRQ");
                        sendError(ti, IllegalOp, "DATA packet without a WRQ");
                        return;
                    }

                    if ( ws->parseData(dgram) )
                    {
                        emit receivedFile(ws->currentFile(), ws->currentFilename());
                        std::vector<WriteSession*>::iterator newEnd = std::remove(writes.begin(), writes.end(), ws);
                        writes.erase(newEnd, writes.end());
                        //writes.remove(ws);
                        delete ws;
                    }
                } break;
            case ERROR:
            {
                    Session *s;
                    if ( (s = findWSession(ti) ) )
                    {
                        std::vector<WriteSession*>::iterator newEnd = std::remove(writes.begin(), writes.end(), static_cast<WriteSession*>(s));
                        writes.erase(newEnd, writes.end());
                        //writes.remove(reinterpret_cast<WriteSession*>(s));
                    }
                    else if ( (s = findRSession(ti) ) )
                    {
                        std::vector<ReadSession*>::iterator newEnd = std::remove(reads.begin(), reads.end(), static_cast<ReadSession*>(s));
                        reads.erase(newEnd, reads.end());
                        //reads.remove(reinterpret_cast<ReadSession*>(s));
                    }
                    else
                    {
                        qWarning("Error received without a session opened for the peer");
                    }

                    if ( s )
                    {
                        qWarning(
                            "Error packet received, peer session aborted\n%s [%d]",
                            dgram.data() + 4,
                            ntohs(wordOfArray(dgram)[1])
                            );
                    }
            }
                break;
            */
            default:
                QByteArray errorDgram = assembleTftpErrorDatagram(TftpCode::IllegalOp, "Illegal TFTP opcode");
                if (mainSocket->writeDatagram(errorDgram, peerAddress, peerPort) == -1)
                {
                    throw TftpError("Error while sending error datagram to client "s + peerAddress.toString().toStdString() + ":" + std::to_string(peerPort));
                }
                return;
        }
    } //while
}


/**
 * @brief TftpServer::dataReceived
 * @throw TftpError if an error occurs while reading data from tftp server socket or while sending respons to sender
 * Slot that handles data coming in at the tftp server's main socket
 */
void TftpServer::dataReceived()
{
    for (auto &nextMainSocket : m_mainSockets)
    {
        handleNewData(nextMainSocket);
    }
}


/**
 * @brief TftpServer::removeSession remove the read or write session that emitted the signal connected to this slot
 */
void TftpServer::removeSession()
{
    QObject *finishedSession = sender(); //get the session that emitted the error() or finished() signal
    if (!finishedSession)
    {
        return;
    }

    auto session = static_cast<Session*>(finishedSession);
    auto &peerIdent = session->peerIdent();

    //check if the session that ended is a read session
    auto readSessionIter = std::find_if(m_readSessions.begin(), m_readSessions.end(), [&peerIdent](auto &nextSession) { return (*nextSession)==peerIdent; } );
    if (readSessionIter != m_readSessions.end())
    {
#ifndef _WIN32
        // on Windows the next line will cause a null-pointer exception.
        m_readSessions.erase(readSessionIter);
#endif
        return;
    }

    //TODO: check if the session that ended is a write session
}


std::shared_ptr<ReadSession> TftpServer::doFindReadSession(const SessionIdent &sessionIdent) const
{
    auto readSessionIter = std::find_if(m_readSessions.begin(), m_readSessions.end(), [&sessionIdent](auto &nextSession) { return (*nextSession)==sessionIdent; } );

    if (readSessionIter != m_readSessions.end())
    {
        return *readSessionIter;
    }
    else
    {
        return std::shared_ptr<ReadSession>();
    }
}



} // namespace end
