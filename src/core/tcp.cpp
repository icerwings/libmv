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
#include <sys/socket.h>
#include "log.h"
#include "net.h"
#include "tcp.h"
#include "buff.h"
#include "epoll.h"

#define     MIN_BUFF_SIZE           512

Tcp::Tcp(Epoll * epoll, int sockfd) : m_epoll(epoll), m_sockfd(sockfd) {
    m_epollOpr = 0;
    m_fin = false;
    m_wtbufsize  = MIN_BUFF_SIZE;
    m_rbuff = nullptr;
    m_wbuff = nullptr;
}

Tcp::~Tcp() {
    OnIoClose();
}

void Tcp::OnIoClose() {
    if (m_sockfd > 0 && m_epoll != nullptr) {
        m_epoll->Operate(m_sockfd, 0, this);
        Close(m_sockfd);
        m_sockfd = -1;
        if (m_closecb) {
            m_closecb();
        }
    }
    if (m_rbuff != nullptr) {
        delete m_rbuff;
        m_rbuff = nullptr;
    }
    if (m_wbuff != nullptr) {
        delete m_wbuff;
        m_wbuff = nullptr;
    }
}

void Tcp::SetReadFunc(function<int(Buff *)> readcb, uint32_t buffsize) {
    if (m_sockfd <= 0) {
        return;
    }
    if (buffsize < MIN_BUFF_SIZE) {
        buffsize = MIN_BUFF_SIZE;
    }
    if (m_rbuff != nullptr) {
        delete m_rbuff;
        m_rbuff = nullptr;
    }
    m_rbuff = new Buff(buffsize);
    if (m_rbuff != nullptr) {
        m_readcb = readcb;
    }
    m_epollOpr |= (EPOLLIN | EPOLLET);
    m_epoll->Operate(m_sockfd, m_epollOpr, this);
}

void Tcp::SetCloseFunc(function<void()> closecb) {
    m_closecb = closecb;
}

void Tcp::SetWtBuffSize(uint32_t size) {
    if (size > MIN_BUFF_SIZE) {
        m_wtbufsize = size;
    }
}

int Tcp::OnIoRead() {
    if (m_rbuff == nullptr || m_sockfd <= 0) {
        return -1;
    }

    while (true) {
        int ret = m_rbuff->Write([&](char *buff, uint32_t capacity){
            if (buff == nullptr || capacity == 0) {
                return -1;
            }
            int size = 0;
            do {
                size = read(m_sockfd, buff, capacity);
            } while (size == -1 && errno == EINTR);
            
            if (size == -1 && errno != EWOULDBLOCK && errno != EAGAIN) {
                return -1;
            } else if (size == -1) {
                return 0;
            } else if (size == 0) {
                return -1;
            }
            return size;
        }, true);

        if (ret < 0) {
            return -1;
        } else if (ret == 0) {
            break;
        }
        if (m_readcb) {
            do {
                ret = m_readcb(m_rbuff);
            } while (ret > 0);
            if (ret < 0) {
                return -1;
            }
        }
    }

    return 0;
}

int Tcp::SendMsg(const string & msg) {
    if (m_sockfd <= 0) {
        return -1;
    }
    if (m_wbuff == nullptr) {
        m_wbuff = new Buff(m_wtbufsize);
        if (m_wbuff == nullptr) {
            ErrorLog(m_wtbufsize).Flush();
            return -1;
        }
    }
    int ret = m_wbuff->Push(msg.c_str(), msg.size());
    if (ret < 0) {
        return -1;
    }
    m_epollOpr |= EPOLLOUT;
    m_epoll->Operate(m_sockfd, m_epollOpr, this);
    return ret;
}

void Tcp::SendFin() {
    if (m_sockfd > 0 && m_epoll != nullptr) {
        m_fin = true;
        m_epollOpr = EPOLLOUT;
        m_epoll->Operate(m_sockfd, m_epollOpr, this);
    }
}

void Tcp::TcpClose() {
    SendFin();
}

int Tcp::OnIoWrite() {
    return OnTcpWrite();
}

int Tcp::OnTcpWrite() {
     if (m_wbuff == nullptr || m_sockfd <= 0) {
         return -1;
     }
     
     int ret = m_wbuff->Read([&](const char *buff, uint32_t capacity){
        if (buff == nullptr || capacity == 0) {
                return -1;
        }
        int size = 0;
        do {
            size = send(m_sockfd, buff, capacity, MSG_NOSIGNAL);
        } while (size == -1 && errno == EINTR);
        
        if (size == -1 && errno != EWOULDBLOCK && errno != EAGAIN) {
            return -1;
        } else if (size == -1) {
            return 0;
        } else if (size == 0) {
            return -1;
        }
        return size;
    });
    if (ret < 0) {
        return -1;
    } else if (m_wbuff->Empty()) {
        if (m_fin) {
            return -1;
        }
        m_epollOpr &= ~EPOLLOUT;
        m_epoll->Operate(m_sockfd, m_epollOpr, this);
    }
    return 0;
}


