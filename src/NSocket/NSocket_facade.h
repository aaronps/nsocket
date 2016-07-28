#ifndef _NSOCKET_FACADE_H_
#define _NSOCKET_FACADE_H_

#ifdef _WIN32
//#define _WIN32_WINNT 0x0501
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #define socklen_t int
  #define SETSOCKOPT_TYPE (const char *)
  #define GET_NET_ERROR() WSAGetLastError()
  #define NET_ERROR_NONAME WSAHOST_NOT_FOUND
  
  #define SETMAXFD(d,o)

  // XXX CHECK WINSOCK EAGAIN
  #define NET_ERROR_EAGAIN          WSAEWOULDBLOCK
  #define NET_ERROR_CONINPROGRESS   WSAEWOULDBLOCK
  #define NET_ERROR_ENOTCONN        WSAENOTCONN
  #define NET_ERROR_ENOTSOCK        WSAENOTSOCK
  #define NET_ERROR_EINTR           WSAEINTR
  #define NET_ERROR_EFAULT          WSAEFAULT
  #define NET_ERROR_EINVAL          WSAEINVAL
  #define NET_ERROR_ECONNREFUSED    WSAECONNREFUSED


#else
  #include <sys/socket.h>
  #include <sys/types.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <netdb.h>
  #include <fcntl.h>
  #include <errno.h>

  #define SOCKET_ERROR -1
  #define INVALID_SOCKET -1
  #define closesocket(a) close(a)

  #define SETSOCKOPT_TYPE
  #define GET_NET_ERROR() errno
  #define NET_ERROR_NONAME EAI_NONAME
  #define SETMAXFD(d,o) d=(d>o)?d:o;

  #define NET_ERROR_EAGAIN          EAGAIN
  #define NET_ERROR_CONINPROGRESS   EINPROGRESS
  #define NET_ERROR_ENOTCONN        ENOTCONN
  #define NET_ERROR_ENOTSOCK        ENOTSOCK
  #define NET_ERROR_EINTR           EINTR
  #define NET_ERROR_EFAULT          EFAULT
  #define NET_ERROR_EINVAL          EINVAL
  #define NET_ERROR_ECONNREFUSED    ECONNREFUSED
  typedef int SOCKET;
#endif

//#include "NTimer.h"

// XXX EABDF == invalid descriptor NOT IN WINDOWS
// XXX EINVAL == invalid argument NOT IN MAC OSX

#ifndef sockaddr_storage
  //#error "sockaddr_storage is undefined"
#endif

#ifndef NI_MAXHOST
  #define NI_MAXHOST 1025
#endif

#ifndef NI_MAXSERV
  #define NI_MAXSERV 32
#endif

#ifndef NSBUFLEN
  #define NSBUFLEN 2048
#endif




#endif // _NSOCKET_FACADE_H_
