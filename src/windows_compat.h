/* Copyright (C)
* 2025 - piHPSDR Windows Porting Team
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*
*/

/**
 * @file windows_compat.h
 * @brief Windows compatibility layer for piHPSDR
 *
 * This header provides compatibility between Windows and POSIX systems.
 * It abstracts differences in:
 * - Network/socket headers and APIs (Winsock2 vs POSIX sockets)
 * - File paths (backslash vs forward slash)
 * - Sleep functions
 * - Other platform-specific APIs
 *
 * Usage:
 *   #include "windows_compat.h"
 *
 * This should be included BEFORE any network-related headers in Windows builds.
 */

#ifndef WINDOWS_COMPAT_H
#define WINDOWS_COMPAT_H

/*
 * Platform detection
 */
#if defined(_WIN32) || defined(_WIN64) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WINDOWS)
  #define PLATFORM_WINDOWS 1
#else
  #define PLATFORM_WINDOWS 0
#endif

/*
 * =============================================================================
 * WINDOWS-SPECIFIC INCLUDES AND DEFINITIONS
 * =============================================================================
 */
#if PLATFORM_WINDOWS

/*
 * Windows Socket API (Winsock2)
 * IMPORTANT: winsock2.h must be included BEFORE windows.h to avoid conflicts
 */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <string.h>

/*
 * Socket type compatibility
 * On Windows: SOCKET is UINT_PTR (unsigned integer)
 * On POSIX: socket is int
 * We use SOCKET type everywhere and define it as int on POSIX
 */
// SOCKET is already defined by winsock2.h on Windows

/*
 * Socket function compatibility macros
 */
#define close(s)        closesocket(s)
#define ioctl(s, cmd, arg) ioctlsocket(s, cmd, arg)

/*
 * Error code compatibility
 * Windows uses WSAGetLastError() instead of errno
 */
#define ERRNO           WSAGetLastError()
#define EWOULDBLOCK     WSAEWOULDBLOCK
#define EINPROGRESS     WSAEINPROGRESS
#define ECONNREFUSED    WSAECONNREFUSED
#define ETIMEDOUT       WSAETIMEDOUT
#define ECONNRESET      WSAECONNRESET
#define EHOSTUNREACH    WSAEHOSTUNREACH
#define ENETUNREACH     WSAENETUNREACH

/*
 * socklen_t type (not defined in older Windows headers)
 */
#ifndef socklen_t
typedef int socklen_t;
#endif

/*
 * Network address conversion
 * inet_pton and inet_ntop compatibility
 */
// These are available in ws2tcpip.h on Windows Vista+
// If targeting older Windows, we'd need shims here

/*
 * File path separator
 */
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"

/*
 * Sleep functions
 * Windows: Sleep(milliseconds)
 * POSIX: sleep(seconds), usleep(microseconds)
 */
#define sleep(seconds)      Sleep((seconds) * 1000)
#define usleep(microseconds) Sleep((microseconds) / 1000)

/*
 * Directory separator for paths
 * Windows accepts both / and \ but native is \
 * GLib's G_DIR_SEPARATOR should be used in GTK code
 */

/*
 * Other Windows-specific compatibility
 */
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp

// Windows doesn't have these POSIX file flags
#ifndef O_NONBLOCK
#define O_NONBLOCK 0
#endif

/*
 * Windows Winsock initialization
 * This MUST be called before using any socket functions
 */
static inline int winsock_init(void) {
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0) {
        return -1;
    }
    return 0;
}

static inline void winsock_cleanup(void) {
    WSACleanup();
}

/*
 * =============================================================================
 * POSIX (Linux/macOS) INCLUDES AND DEFINITIONS
 * =============================================================================
 */
#else  /* PLATFORM_WINDOWS */

/*
 * Standard POSIX headers
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

/*
 * SOCKET type compatibility (define SOCKET as int on POSIX)
 */
typedef int SOCKET;
#define INVALID_SOCKET  (-1)
#define SOCKET_ERROR    (-1)

/*
 * Error code compatibility
 */
#define ERRNO errno

/*
 * File path separator
 */
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"

/*
 * No-op Winsock initialization on POSIX
 */
#define winsock_init()    (0)
#define winsock_cleanup() ((void)0)

#endif /* PLATFORM_WINDOWS */

/*
 * =============================================================================
 * COMMON UTILITY FUNCTIONS (all platforms)
 * =============================================================================
 */

/**
 * @brief Convert path to native format (Windows: \, POSIX: /)
 * @param path Path string to convert (modified in-place)
 * @return Pointer to the modified string
 */
static inline char *path_to_native(char *path) {
#if PLATFORM_WINDOWS
    // Convert forward slashes to backslashes
    for (char *p = path; *p; p++) {
        if (*p == '/') *p = '\\';
    }
#else
    // Convert backslashes to forward slashes (if any)
    for (char *p = path; *p; p++) {
        if (*p == '\\') *p = '/';
    }
#endif
    return path;
}

/**
 * @brief Get last socket error as string
 * @return Human-readable error message
 */
static inline const char *socket_error_string(void) {
#if PLATFORM_WINDOWS
    static char buf[256];
    int err = WSAGetLastError();

    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf,
        sizeof(buf),
        NULL
    );

    return buf;
#else
    return strerror(errno);
#endif
}

/**
 * @brief Set socket to non-blocking mode
 * @param sockfd Socket file descriptor
 * @return 0 on success, -1 on error
 */
static inline int set_socket_nonblocking(SOCKET sockfd) {
#if PLATFORM_WINDOWS
    u_long mode = 1;
    return ioctlsocket(sockfd, FIONBIO, &mode);
#else
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
#endif
}

/**
 * @brief Set socket to blocking mode
 * @param sockfd Socket file descriptor
 * @return 0 on success, -1 on error
 */
static inline int set_socket_blocking(SOCKET sockfd) {
#if PLATFORM_WINDOWS
    u_long mode = 0;
    return ioctlsocket(sockfd, FIONBIO, &mode);
#else
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(sockfd, F_SETFL, flags & ~O_NONBLOCK);
#endif
}

#endif /* WINDOWS_COMPAT_H */
