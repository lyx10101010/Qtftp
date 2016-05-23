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

#ifndef TFTP_UTILS_H
#define TFTP_UTILS_H

#include "tftp_constants.h"
#include "tftp_error.h"
#include "udpsocket.h"
#include <QByteArray>
#include <QHostAddress>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif
#include <memory>
#include <cassert>



namespace LIBTFTP
{

/**
 * @brief assignWordInByteArray
 * @param byteArray
 * @param indexInByteArray, must be an even number
 * @param word the value to assign
 * @pre byteArray.size() >= (indexInByteArray+1)
 */
inline void assignWordInByteArray(QByteArray &byteArray, unsigned int indexInByteArray, uint16_t word)
{
    assert( indexInByteArray % 2 == 0 ); //index should be even location in bytearray
    uint16_t *wordArrayPtr = reinterpret_cast<uint16_t*>( byteArray.data() );
    wordArrayPtr[indexInByteArray/2] = word;
}


/**
 * @brief readWordInByteArray
 * @param byteArray
 * @param indexInByteArray must be an even number
 * @return the word value at index \p indexInByteArray in \p byteArray
 * @pre byteArray.size() >= (indexInByteArray+1)
 */
inline uint16_t readWordInByteArray(QByteArray &byteArray, unsigned int indexInByteArray)
{
    assert( indexInByteArray % 2 == 0 ); //index should be even location in bytearray
    uint16_t *wordArrayPtr = reinterpret_cast<uint16_t*>( byteArray.data() );
    return wordArrayPtr[indexInByteArray/2];
}


QByteArray assembleTftpErrorDatagram(TftpCode::ErrorCode ec, const QString &errMsg);


} // LIBTFTP namespace end

#endif // TFTP_UTILS_H

