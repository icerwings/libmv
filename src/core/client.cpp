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

#include <netinet/in.h>
#include <arpa/inet.h>
#include "epoll.h"
#include "log.h"
#include "net.h"
#include "buff.h"
#include "client.h"

TcpClient::~TcpClient() {
    OnIoClose();
}

int TcpClient::Connect(const struct sockaddr * inaddr) {
    socklen_t       addrlen     = 0;
    int             ret         = 0;
    if (inaddr == nullptr) {
        return -1;
    }
    
    if (inaddr->sa_family == AF_INET) {
        addrlen = sizeof(struct sockaddr_in);
    } else if (inaddr->sa_family == AF_INET6) {
        addrlen = sizeof(struct sockaddr_in6);
    } else {
        ErrorLog(inaddr->sa_family).Flush();
        return -1;
    }
    
    if (m_sockfd > 0) {
        Close(m_sockfd);
    }
    m_sockfd = TcpSocket(0, true);
    if (m_sockfd == -1) {
        return -1;
    }

    do {
        ret = connect(m_sockfd, inaddr, addrlen);
    } while (ret == -1 && errno == EINTR);
    
    if (0 == ret) {
        m_status = ConnStatus::CONNECTING;
        return 0;
    } else if (errno == EINPROGRESS) {
        m_epollOpr |= EPOLLOUT;
        m_epoll->Operate(m_sockfd, m_epollOpr, this);
        return 0;
    } else {
        char *ipstr = inet_ntoa(((struct sockaddr_in *)inaddr)->sin_addr);
        ErrorLog(ipstr)(errno).Flush();
        return -1;
    }
    
    return 0;
}

bool TcpClient::IsConnected() {
    return (m_status == ConnStatus::CONNECTED);
}

int TcpClient::OnIoWrite() {
    if (m_status == ConnStatus::CONNECTING) {
        int                 optval      = 0;
        socklen_t           optlen      = 0;
        int ret = getsockopt(m_sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen);
        if (ret < 0 || optval != 0) {
            return -1;
        }
        m_status = ConnStatus::CONNECTED;
    }
    if (m_wbuff == nullptr || m_wbuff->Empty()) {
        m_epollOpr &= ~EPOLLOUT;
        m_epoll->Operate(m_sockfd, m_epollOpr, this);
    } else {
        return OnTcpWrite();
    }

    return 0;
}


