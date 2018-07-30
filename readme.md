# Qtftp

Qtftp is a TFTP library and daemon that uses the Qt cross-platform library.
It implements the TFTP functionality specified by 

- RFC1350 (TFTP Protocol rev. 2)
- RFC2347 (TFTP Option extension)
- RFC2348 (TFTP BLocksize Option)
- RFC2349 (TFTP Timeout Interval and Transfer Size Options)


Currently only the server side is implemented.
Only file download is supported at this moment, but this covers the majority of use cases in embedded systems, where TFTP is often used to download firmware.

Qtftp is a complete rewrite and port to qt5 of libtftp that was written by Diego Elio Pettenò (see https://www.flameeyes.eu/oldstuff).

## License
Qtftp is licensed under LGPL 2.1. See file COPYING for the text of the license.

## Supported platforms
Qtftp has been tested on Linux (x86_64 and arm), OSX 10.10 and Windows 7/10

## build instructions
See doc/how_to_build.txt

## Starting the daemon
On Unix the qtftpd daemon needs elevated rights to open ports below 1024 (default port for TFTP is 69). On Linux it is recommended to give the qtftpd executable the cap_net_bind_service capability with command

```setcap cap_net_bind_service+ep qtftpd```

and run qtftpd as a non-privileged user.

On other Unix flavours you can start qtftpd as root and it will drop privileges and change user to the username passed with the -u option (user 'tftp' by default).

For the simple use case where the TFTP daemon can bind to localhost and serve files from one directory you can start the daemon as:

```qtftpd [-p portnr] -d <files_directory>```

If you omit the portnr option, the default TFTP port (69) will be used.

If you want the TFTP daemon to serve different network addresses from different files directories, you can can start the daemon with a configuration file.
The configuration file should have a section with name of choice for each binding of the TFTP server. Each section should have the following keys:

- ```port = <portnr>```
- ```bind_addr = <ip_address or hostname>```
- ```files_dir = <directory where files are downloaded from>```
- ```disable_upload = true```

The 'disable_upload' key is a future extension. Currently upload of files is not implemented and the 'disable_upload' key should always be set to 'true'.

Start the daemon in this case as:

```qtftpd -c <configuration_file>```

Example configuration file:


```
[network1]
port = 69
bind_addr = 10.1.2.3
files_dir = "/srv/tftp/network1_directory"
disable_upload = true


[network2]
port = 69
bind_addr = 10.15.16.17
files_dir = "/srv/tftp/network2_directory"
disable_upload = true
```

