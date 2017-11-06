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
#include "readsession.h"
#include "tftp_error.h"
#include "tftp_constants.h"
#include "tftp_utils.h"
#include "udpsocketfactory.h"
#include "udpsocket.h"
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
 * @brief TftpServer::TftpServer
 * @param filesDir
 * @param hostAddr
 * @param portNr
 * @param parent
 *
 * @throw TftpError if could not bind server socket to \p hostAddr and \p portNr
 */
TftpServer::TftpServer(const QString &filesDir, std::shared_ptr<UdpSocketFactory> socketFactory, QObject *parent) : QObject(parent),
                                                                                                                    m_filesDir(filesDir),
                                                                                                                    m_socketFactory(socketFactory),
                                                                                                                    m_mainSocket( std::static_pointer_cast<UdpSocket>(m_socketFactory->createNewSocket()) ),
                                                                                                                    m_tftpServerPort(69),
                                                                                                                    m_slowNetworkThreshold(2000)
{
    connect(m_mainSocket.get(), &UdpSocket::readyRead, this, &TftpServer::dataReceived);
}


TftpServer::TftpServer(std::shared_ptr<UdpSocketFactory> socketFactory, QObject *parent) : TftpServer(QString(), socketFactory, parent)
{
}


TftpServer::~TftpServer()
{
}


/**
 * @brief TftpServer::bindSocket
 * @param hostAddr
 * @param portNr
 *
 * @throw TtftpError if an error occurred while binding this server's socket to
 * \p hostAddr and \p portNr.
 */
void TftpServer::bind(const QHostAddress &hostAddr)
{
//    if ( ! m_mainSocket )
//    {
//        m_mainSocket = std::static_pointer_cast<UdpSocket>( m_socketFactory->createNewSocket() );
//        auto debugPtr = m_mainSocket.get();
//        connect(m_mainSocket.get(), &UdpSocket::readyRead, this, &TftpServer::dataReceived);
//    }

    if ( m_filesDir.isEmpty() )
    {
        throw TftpError("File directory for tftp server not set before binding server port");
    }

    if ( ! m_mainSocket->bind(hostAddr, m_tftpServerPort) )
    {
        std::string errorMsg( "Could not bind tftp server to host address ");
        errorMsg += hostAddr.toString().toStdString();
        errorMsg += " at port ";
        errorMsg += std::to_string(m_tftpServerPort);
        throw TftpError(errorMsg);
    }
}


/**
 * @brief TftpServer::close stop listening for new tftp requests
 */
void TftpServer::close()
{
    m_mainSocket->close();
}


/**
 * @brief TftpServer::setServerPort set the udp port on which the tftp server will listen for new connections
 * @param port udp port to use
 *
 * If the tftp server should listen on a non-standard port, call this function before calling bind(QHostAddress&).
 */
void TftpServer::setServerPort(uint16_t port)
{
    m_tftpServerPort = port;
}


/**
 * @brief TftpServer::setFilesDir change the directory where the tftp server looks for requested files
 * @param filesDir
 *
 * If you call this function when the server is active (e.g. after you call bind(...) ), \p filesDir will only
 * be used for new tftp read requests.
 */
void TftpServer::setFilesDir(const QString &filesDir)
{
    m_filesDir = filesDir;
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


/**
 * @brief TftpServer::serverPort get the UDP port that we are listening on for tftp requests
 * @return the UDP port nr or 0 if the main socket wasn't bind yet.
 *
 * Note: the main TFTP port is returned, e.g. the port that the server listens on for new TFTP transfer requests.
 * For each transfer a new session with its own UDP port is created.
 */
quint16 TftpServer::serverPort() const
{
    return m_mainSocket->localPort();
}


const QString &TftpServer::filesDir() const
{
    return m_filesDir;
}

std::shared_ptr<const ReadSession> TftpServer::findReadSession(const SessionIdent &sessionIdent) const
{
    auto readSesPtr = doFindReadSession(sessionIdent);
    return readSesPtr;
}


/**
 * @brief TftpServer::dataReceived
 * @throw TftpError if an error occurs while reading data from tftp server socket or while sending respons to sender
 * Slot that handles data coming in at the tftp server's main socket
 */
void TftpServer::dataReceived()
{
    QByteArray dgram;
    int dgramSize = m_mainSocket->pendingDatagramSize();
    dgram.resize( dgramSize );
    QHostAddress peerAddress;
    quint16 peerPort;
    while (m_mainSocket->hasPendingDatagrams())
    {
        if (m_mainSocket->readDatagram( dgram.data(), dgramSize, &peerAddress, &peerPort ) == -1)
        {
            auto lastErrStr = m_mainSocket->errorString().toStdString();
            throw TftpError("Error while reading data from tftp socket (port "s + std::to_string(m_mainSocket->localPort()) + ")" + lastErrStr);
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

                    readSession = std::make_shared<ReadSession>(peerAddress, peerPort, dgram, m_filesDir, m_slowNetworkThreshold, m_socketFactory);
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
                if (m_mainSocket->writeDatagram(errorDgram, peerAddress, peerPort) == -1)
                {
                    throw TftpError("Error while sending error datagram to client "s + peerAddress.toString().toStdString() + ":" + std::to_string(peerPort));
                }
                return;
        }
    } //while
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
