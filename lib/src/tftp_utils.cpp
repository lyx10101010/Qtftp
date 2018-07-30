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

#include "tftp_utils.h"


namespace QTFTP
{

/**
\brief Creates the payload for an UDP datagram that indicates a TFTP error
\param ec Error code to send
\param errMsg Error message string

The format of the payload that is created is this:
<pre>
    2 bytes      2 bytes        string    1 byte
    ----------------------------------------
    05 (ERROR) |  ErrorCode |   ErrMsg   |   0  |
    ----------------------------------------
</pre>

*/
QByteArray assembleTftpErrorDatagram(TftpCode::ErrorCode ec, const QString &errMsg)
{
    QByteArray dgram;
    dgram.fill('\0', errMsg.length() + 5 );

    assignWordInByteArray(dgram, 0, htons(uint16_t(TftpCode::TFTP_ERROR)));
    assignWordInByteArray(dgram, 2, htons(uint16_t(ec)));

    strcpy( dgram.data() + 4, static_cast<const char*>(errMsg.toLatin1()) );
    return dgram;
}


} // QTFTP namespace end
