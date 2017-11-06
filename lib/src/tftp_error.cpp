#include "include/tftp_error.h"

namespace QTFTP
{



TftpError::TftpError(const std::string &what) : std::runtime_error(what)
{
}



} // namespace QTFTP end
