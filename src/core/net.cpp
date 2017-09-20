/* Copyright Hengfeng Lang. Inspiredd by libuv link: http://www.libuv.org
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), toi
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <sys/types.h>
#include <linux/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "net.h"
#include "defer.h"
#include "log.h"

int FdSet(int fd, int flags) {
    int                 ret       = 0;

    do {
        ret = fcntl(fd, F_GETFL);
    }while (ret == -1 && errno == EINTR);

    if (ret != -1) {
        if ((ret & flags) != 0) {
            return 0;
        }
        do {
            ret = fcntl(fd, F_SETFL, ret | flags);
        }while (ret == -1 && errno == EINTR);
    }
    return (ret != -1) ? 0 : -1;
}

static inline int Socket(int domain, int type, int protocol) {
    int                 sockfd      = -1;

#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
    sockfd = socket(domain, type | SOCK_NONBLOCK | SOCK_CLOEXEC, protocol);
    if (sockfd > 0) {
       return sockfd;
    } else if (errno != EINVAL) {
        ErrorLog(domain)(type)(protocol).Flush();
        return -1;
    }
#endif
    sockfd = socket(domain, type, protocol);
    if (sockfd != -1) {
        FdSet(sockfd, O_NONBLOCK);
        FdSet(sockfd, SOCK_CLOEXEC);
    }
    return sockfd;
}

int TcpSocket(uint16_t port, bool nodelay) {
    int                 opt         = 1;

    int sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd > 0) {
        Defer   gurad([&]{
            Close(sockfd);
        });

#if defined(MSG_NOSIGNAL)
        setsockopt(sockfd, SOL_SOCKET, MSG_NOSIGNAL, (char *)&opt, sizeof(opt));
#else
#if defined(SO_NOSIGPIPE)
        setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, (char *)&opt, sizeof(opt));
#endif
#endif

        if (nodelay) {
            if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&opt, sizeof(opt)) == -1) {
                return -1;
            }
        }
        //tcp_tw_reuse
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == -1) {
            return -1;
        }

        if (port > 0) {
            struct sockaddr_in  saddr;
            saddr.sin_family        = AF_INET;
            saddr.sin_addr.s_addr   = 0;
            saddr.sin_port          = htons(port);

            if (bind(sockfd, (struct sockaddr*)&saddr, sizeof(saddr)) != 0 && errno != EADDRINUSE) {
                return -1;
            }
        }
        gurad.Dismiss();
    }

    return sockfd;
}

int Close(int fd) {
    int         ret         = -1;
    if (fd > 0) {
        int         firstNo     = errno;
        ret = close(fd);
        if (ret == -1 && errno == EINTR) {
            errno = firstNo;
        }
    }
    return ret;
}

