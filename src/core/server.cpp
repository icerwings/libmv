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
#include <sys/stat.h>
#include <fcntl.h>
#include <thread>
#include <sys/socket.h>

#include "log.h"
#include "epoll.h"
#include "net.h"
#include "tcp.h"
#include "signalfd.h"
#include "server.h"

Server::Server() : m_epoll(nullptr), m_sockfd(-1) {
    m_epoll = new Epoll();
    m_signalfd = new Signalfd(m_epoll);
    m_exitsig = SIGQUIT;
}

Server::~Server() {
    OnIoClose();
}

int Server::Open(uint16_t port, uint32_t poolsize) {
    if (port == 0 || m_epoll == nullptr || m_signalfd == nullptr) {
        ErrorLog(port)(m_epoll).Flush();
        return -1;
    }
    m_sockfd = TcpSocket(port, true);
    if (m_sockfd == -1) {
        ErrorLog(m_sockfd)(errno).Flush();
        return -1;
    }
    m_epoll->Operate(m_sockfd, EPOLLIN, this);
    if (listen(m_sockfd, 1000) != 0) {
        ErrorLog(m_sockfd)(errno).Flush();
        return -1;
    }
    OpenSpareFile("/");
    
    for (int iLoop = 0; iLoop < (int)poolsize; iLoop++) {
        Epoll * epoll = new Epoll();
        Signalfd * sigfd = new Signalfd(epoll);
        if (epoll == nullptr || sigfd == nullptr) {
            return -1;
        }        
        thread     work([epoll]{
            epoll->Loop();
        });
        work.detach();
        m_pool.push_back(epoll);
    }
    while (true) {
        int     count = 0;
        for (const auto & thread : m_pool) {
            if (!thread->IsRunning()) {
                break;
            }
            count++;
        }
        if (count == (int)poolsize) {
            break;
        }
        usleep(100000);
    }
    
    m_signalfd->SetCloseFunc([&]{
        delete m_signalfd;
    });
    m_signalfd->SetSigFunc(m_exitsig, [&]{
        m_epoll->StopLoop();
    });
    
    return 0;
}

void Server::SetIoReadFunc(function<int(Buff *, Tcp *)> svrcb) {
    m_svrcb = svrcb;
}
void Server::SetIoCloseFunc(function<void(Tcp *)> ioclosecb) {
    m_ioclosecb = ioclosecb;
}
void Server::SetCloseFunc(function<void()> closecb) {
    m_closecb = closecb;
}

void Server::SetExitSig(int exitsig) {
    if (exitsig != 0) {
        m_exitsig = exitsig;
    }
}

void Server::OnIoClose() {
    if (m_sockfd > 0 && m_epoll != nullptr) {
        m_epoll->Operate(m_sockfd, 0, this);
        Close(m_sockfd);
        m_sockfd = -1;
        if (m_closecb) {
            m_closecb();
        }
    }    
}

int Server::Accept() {
    int peerfd = 0;
    do {
        peerfd = accept4(m_sockfd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
    }while (peerfd == -1 && errno == EINTR);
    
    if (peerfd > 0) {
        return peerfd;
    } else if (errno != EINVAL) {
        return -1;
    } 
    do {
        peerfd = accept(m_sockfd, nullptr, nullptr);
    }while (peerfd == -1 && errno == EINTR);

    if (peerfd > 0) {
        FdSet(peerfd, O_NONBLOCK);
        FdSet(peerfd, FD_CLOEXEC);
    }
    return peerfd;
}

int Server::OpenSpareFile(const string & path) {
    m_emfile = open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (m_emfile == -1 && errno == EINVAL) {
        m_emfile = open(path.c_str(), O_RDONLY);
        if (m_emfile > 0) {
            FdSet(m_emfile, FD_CLOEXEC);
        }
    }
    return 0;
}

void Server::OnEmfile() {
    if (m_emfile > 0) {
        Close(m_emfile);
        m_emfile = -1;
        int peerfd = Accept();
        if (peerfd > 0) {
            Close(peerfd);
            OpenSpareFile("/");
        }
    }    
}

int Server::OnIoRead() {
    if (m_sockfd < 0) {
        return -1;
    }
    int peerfd = Accept();
    if (peerfd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        } else if (errno == EMFILE || errno == ENFILE) {
            ErrorLog(errno).Flush();
            OnEmfile();
        }
    }
    Epoll * epoll   = m_epoll;
    int    fdnum    = 0;
    for (auto thread : m_pool) {
        if (fdnum == 0 || thread->GetFdNum() < fdnum) {
            epoll = thread;
            fdnum = thread->GetFdNum();
        }
    }
    Tcp * tcp = new Tcp(epoll, peerfd);
    if (tcp == nullptr) {
        return -1;
    }
    tcp->SetCloseFunc([&, tcp]{
        if (m_ioclosecb) {
            m_ioclosecb(tcp);
        }
        delete tcp;
    });
    tcp->SetReadFunc([&, tcp](Buff * buff){
        if (m_svrcb) {
            return m_svrcb(buff, tcp);
        }
        return -1;
    });
    
    return 0;
}

void Server::SetExitFunc(function<void()> exitcb) {
    if (m_epoll != nullptr) {
        m_epoll->SetStopFunc(exitcb);
    }
}

int Server::Run() {
    if (m_epoll == nullptr || m_sockfd == -1) {
        return -1;
    }
    if (m_epoll->Loop() != 0) {
        return -1;
    }
    return 0;
}


