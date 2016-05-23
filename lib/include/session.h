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

#ifndef SESSION_H
#define SESSION_H

#include <tftp_constants.h>
#include <QHostAddress>
#include <QFile>
#include <QTimer>
#include <memory>



namespace LIBTFTP
{

class AbstractSocket;
class UdpSocketFactory;

/**
 * @brief The SessionIdent struct uniquely identifies a TFTP session
 */
struct SessionIdent
{
    public:
        SessionIdent(const QHostAddress &address, uint16_t port);

        bool operator==(const SessionIdent &otherIdent) const;

        QHostAddress        m_address;
        uint16_t            m_port;
};


/**
 * @brief The Session class base class for TFTP sessions
 */
class Session : public QObject
{
    Q_OBJECT

    public:
        enum class State { Busy, Finished, InError };

        Session(const QHostAddress &peerAddr, uint16_t peerPort,
                std::shared_ptr<UdpSocketFactory> socketFactory, QObject *parent=nullptr);
        ~Session();

        State   state() const;
        QString filePath() const;
        bool    fileExists() const;
        bool    atEndOfFile() const;
        QString lastFileError() const;
        TftpCode::Mode transferMode() const;
        const SessionIdent &peerIdent() const;
        bool operator==(const SessionIdent &sessionIdent) const;
        bool operator==(const Session &otherSession) const;

        bool    openFile(QIODevice::OpenModeFlag openMode);
        bool    readFromFile(QByteArray &buffer, qint64 maxSize);
        static void setRetransmitTimeOut(unsigned int newTimeOut);
        static void setMaxRetransmissions(unsigned int newMax);

    protected:
        bool isFileOpen() const;

        void setTransferMode(TftpCode::Mode newMode);
        void setFilePath(const QString &directory, const QString &fileName);
        void setState(State newState);
        void readDatagram(QByteArray &datagram, QHostAddress *peerAddress=nullptr, quint16 *peerPort=nullptr);
        void sendDatagram(const QByteArray &datagram, bool startRetransmitTimer=false);
        void stopRetransmitTimer();
        virtual void retransmitData() = 0;


    protected slots:
        virtual void dataReceived() = 0;

    private slots:
        void handleExpiredRetransmitTimer();

    signals:
        void finished();
        void error();



    private:
        QFile               m_file; //file to read or write
        std::shared_ptr<AbstractSocket> m_sessionSocket;
        QTimer              m_retransmitTimer;  //to check for timeout on receiving ACK
        unsigned int        m_retransmitCount;
        SessionIdent        m_peerIdent;
        TftpCode::Mode      m_transferMode;
        State               m_state;
        static unsigned int m_retransmitTimeOut;
        static unsigned int m_maxRetransmissions;
};



} // namespace LIBTFTP end

#endif // SESSION_H
