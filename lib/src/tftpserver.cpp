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


using namespace std::string_literals;

namespace LIBTFTP
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
                                                                                                                    m_socketFactory(socketFactory)
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
void TftpServer::bind(QHostAddress hostAddr, uint16_t portNr)
{
    if ( ! m_mainSocket )
    {
        m_mainSocket = std::static_pointer_cast<UdpSocket>( m_socketFactory->createNewSocket() );
        auto debugPtr = m_mainSocket.get();
        connect(m_mainSocket.get(), &UdpSocket::readyRead, this, &TftpServer::dataReceived);
    }

    if ( ! m_mainSocket->bind(hostAddr, portNr) )
    {
        std::string errorMsg( "Could not bind tftp server to host address ");
        errorMsg += hostAddr.toString().toStdString();
        errorMsg += " at port ";
        errorMsg += std::to_string(portNr);
        throw TftpError(errorMsg);
    }
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
            throw TftpError("Error while reading data from tftp socket (port "s + std::to_string(m_mainSocket->localPort()) + ")");
        }

        auto opcode = ntohs( readWordInByteArray(dgram, 0) );
        switch( opcode )
        {
            case TftpCode::TFTP_RRQ:
                {
                    std::shared_ptr<ReadSession> readSession = findReadSession(SessionIdent(peerAddress, peerPort));
                    if ( readSession )
                    {
                        QByteArray errorDgram = assembleTftpErrorDatagram(TftpCode::IllegalOp, "Duplicate read request from same peer");
                        if (m_mainSocket->writeDatagram(errorDgram, peerAddress, peerPort) == -1)
                        {
                            throw TftpError("Error while sending error datagram to client "s + peerAddress.toString().toStdString() + ":" + std::to_string(peerPort));
                        }
                        return;
                    }

                    auto newReadSession = std::make_shared<ReadSession>(peerAddress, peerPort, dgram, m_filesDir, m_socketFactory);
                    connect(newReadSession.get(), &Session::finished, this, &TftpServer::removeSession);
                    connect(newReadSession.get(), &Session::error, this, &TftpServer::removeSession);
                    m_readSessions.push_back( newReadSession );
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
        m_readSessions.erase(readSessionIter);
        return;
    }

    //TODO: check if the session that ended is a write session
}


std::shared_ptr<ReadSession> TftpServer::findReadSession(const SessionIdent &sessionIdent) const
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
