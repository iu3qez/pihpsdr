#ifndef WIN_COMPAT_H
#define WIN_COMPAT_H

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#ifndef socklen_t
typedef int socklen_t;
#endif
#ifndef close
#define close closesocket
#endif
#endif


#ifndef SO_REUSEPORT
#define SO_REUSEPORT SO_REUSEADDR
#endif

#endif // WIN_COMPAT_H
