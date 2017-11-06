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

#ifndef TFTP_CONSTANTS_H
#define TFTP_CONSTANTS_H

namespace QTFTP
{

struct TftpCode
{
    public:

        //! List of opcodes which can be found in a TFTP header
        enum Opcode
        {
            OpcodeInvalid,	//!< Invalid opcode
            TFTP_RRQ,		//!< Read request
            TFTP_WRQ,		//!< Write request
            TFTP_DATA,		//!< Data packet
            TFTP_ACK,		//!< Acknowledge
            TFTP_ERROR		//!< Error
        };

        //! List of error codes which can be found in a TFtp error datagram
        enum ErrorCode
        {
            Undefined,	//!< Not defined, see error message (if any).
            FileNotFound,	//!< File not found.
            AccessViolation,//!< Access violation.
            DiskFull,	//!< Disk full or allocation exceeded.
            IllegalOp,	//!< Illegal TFTP operation.
            UnknownTID,	//!< Unknown transfer ID.
            FileExists,	//!< File already exists.
            NoUser		//!< No such user.
        };

        //! List of transfer modes
        enum Mode
        {
            ModeInvalid,	//!< Invalid mode
            NetAscii,	//!< ASCII-mode transfer
            Octet,		//!< Binary-mode transfer
            Mail		//!< Mail transfer (not supported)
        };
};


constexpr unsigned int TftpBlockSize = 512;
constexpr unsigned int DefaultRetransmitTimeOutms = 5000;
constexpr unsigned int DefaultMaxRetryCount = 3;

} //QTFTP namespace end

#endif // TFTP_CONSTANTS_H

