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

#ifndef UDPSOCKETFACTORY_H
#define UDPSOCKETFACTORY_H

#include "abstractsocket.h"
#include <memory>

class QObject;

namespace LIBTFTP
{



class UdpSocketFactory
{
    public:
        UdpSocketFactory();

        virtual std::shared_ptr<AbstractSocket> createNewSocket(QObject *parent=nullptr);
};


} // LIBTFTP namespace end

#endif // UDPSOCKETFACTORY_H
