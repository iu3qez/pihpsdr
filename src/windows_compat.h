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
#include <io.h>        // For _close(), _open(), etc.
#include <stdarg.h>    // For va_list, va_start, va_end

/*
 * Socket type compatibility
 * On Windows: SOCKET is UINT_PTR (unsigned integer)
 * On POSIX: socket is int
 * We use SOCKET type everywhere and define it as int on POSIX
 */
// SOCKET is already defined by winsock2.h on Windows

/*
 * Socket function compatibility
 * Provide wrapper functions instead of macros to avoid conflicts
 */
#define ioctl(s, cmd, arg) ioctlsocket(s, cmd, arg)

// Wrapper for close() - handles both sockets (positive values) and files
// On Windows, socket() returns SOCKET (unsigned), but code uses int
// We'll try closesocket first, if it fails try _close for file descriptors
static inline int close_compat(int fd) {
    // Attempt socket close first (most common in this codebase)
    int result = closesocket((SOCKET)fd);
    if (result == SOCKET_ERROR) {
        // If that failed, might be a file descriptor
        // Try _close (but this might also fail if it was actually a bad socket)
        return _close(fd);
    }
    return result;
}
#define close(fd) close_compat(fd)

/*
 * Error code compatibility
 * Windows uses WSAGetLastError() instead of errno
 * Note: MinGW already defines some of these in errno.h, so use #ifndef guards
 */
#define ERRNO           WSAGetLastError()
#ifndef EWOULDBLOCK
#define EWOULDBLOCK     WSAEWOULDBLOCK
#endif
#ifndef EINPROGRESS
#define EINPROGRESS     WSAEINPROGRESS
#endif
#ifndef ECONNREFUSED
#define ECONNREFUSED    WSAECONNREFUSED
#endif
#ifndef ETIMEDOUT
#define ETIMEDOUT       WSAETIMEDOUT
#endif
#ifndef ECONNRESET
#define ECONNRESET      WSAECONNRESET
#endif
#ifndef EHOSTUNREACH
#define EHOSTUNREACH    WSAEHOSTUNREACH
#endif
#ifndef ENETUNREACH
#define ENETUNREACH     WSAENETUNREACH
#endif

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
static inline void sleep_compat(unsigned int seconds) {
    Sleep(seconds * 1000);
}
static inline void usleep_compat(unsigned int microseconds) {
    Sleep(microseconds / 1000);
}
// Only define macros if not already defined
#ifndef sleep
#define sleep(seconds)      sleep_compat(seconds)
#endif
#ifndef usleep
#define usleep(microseconds) usleep_compat(microseconds)
#endif

/*
 * Directory separator for paths
 * Windows accepts both / and \ but native is \
 * GLib's G_DIR_SEPARATOR should be used in GTK code
 */

/*
 * Byte-order conversion functions
 * Windows doesn't have htobe64/be64toh, provide equivalents
 */
#include <stdlib.h>
#ifndef htobe64
#define htobe64(x) _byteswap_uint64(x)
#endif
#ifndef be64toh
#define be64toh(x) _byteswap_uint64(x)
#endif
#ifndef htobe32
#define htobe32(x) _byteswap_ulong(x)
#endif
#ifndef be32toh
#define be32toh(x) _byteswap_ulong(x)
#endif
#ifndef htobe16
#define htobe16(x) _byteswap_ushort(x)
#endif
#ifndef be16toh
#define be16toh(x) _byteswap_ushort(x)
#endif

/*
 * Socket options compatibility
 */
// SO_REUSEPORT doesn't exist on Windows, map to SO_REUSEADDR
#ifndef SO_REUSEPORT
#define SO_REUSEPORT SO_REUSEADDR
#endif

/*
 * fcntl() compatibility
 * Windows doesn't have fcntl, provide minimal implementation for socket flags
 */
#define F_GETFL 3
#define F_SETFL 4
static inline int fcntl(int fd, int cmd, ...) {
    // Only support getting/setting socket flags for O_NONBLOCK
    // For F_GETFL, return 0 (assume blocking by default)
    // For F_SETFL with O_NONBLOCK, use ioctlsocket
    if (cmd == F_GETFL) {
        return 0;  // Return 0, actual flags unknown
    }
    else if (cmd == F_SETFL) {
        // Extract flags from varargs
        va_list args;
        va_start(args, cmd);
        int flags = va_arg(args, int);
        va_end(args);

        // If O_NONBLOCK is set, make socket non-blocking
        if (flags & O_NONBLOCK) {
            u_long mode = 1;
            return ioctlsocket((SOCKET)fd, FIONBIO, &mode);
        }
    }
    return 0;
}

/*
 * BSD compatibility functions
 */
// bcopy is obsolete, use memmove
#define bcopy(src, dst, len) memmove(dst, src, len)

/*
 * Other Windows-specific compatibility
 */
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp

// Windows doesn't have these POSIX file flags
#ifndef O_NONBLOCK
#define O_NONBLOCK 0x0800
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
