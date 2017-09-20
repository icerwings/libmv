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

#include <chrono>
#include <errno.h>
#include "epoll.h"
#include "net.h"
#include "io.h"
#include "log.h"

using namespace std;
using namespace chrono;

#define     MAX_EPOLL_NUM       1024

Epoll::Epoll() : m_running(false), m_fdnum(0) {
    m_epfd = epoll_create1(EPOLL_CLOEXEC);
}

Epoll::~Epoll(){
    if (m_epfd > 0) {
        Close(m_epfd);
    }
}

int Epoll::Loop(int timeOut) {
    if (m_epfd <= 0) {
        return -1;
    }

    struct epoll_event  events[MAX_EPOLL_NUM];
    int     curTimeOut  = timeOut;

    while (true) {
        auto start = steady_clock::now();
        if (!m_running) {
            m_running = true;
            __sync_synchronize();
        }
        int nfds = epoll_wait(m_epfd, events, MAX_EPOLL_NUM, timeOut);
        if (nfds == 0) {
            if (timeOut == -1) {
                return -1;
            }
        } else if (nfds < 0) {
            if (errno != EINTR) {
                return -1;
            }
            else if (timeOut == 0) {
                return 0;
            }
            else if (timeOut > 0) {
                timeOut -= (duration_cast<milliseconds>(steady_clock::now() - start)).count();
                if (timeOut <= 0) {
                    timeOut = curTimeOut;
               }
            }
            continue;
        }
        for (auto iLoop = 0; iLoop < nfds; iLoop++) {
            timeOut = curTimeOut;
            IO * io = (IO *)events[iLoop].data.ptr;
            if (io == NULL) {
                return -1;
            }
            if ((events[iLoop].events & (EPOLLERR | EPOLLHUP)) != 0) {
                io->OnIoClose();
                continue;
            }
            if ((events[iLoop].events & EPOLLIN) != 0 && io->OnIoRead() != 0) {
                io->OnIoClose();
                continue;
            }
            if ((events[iLoop].events & EPOLLOUT) != 0 && io->OnIoWrite() != 0) {
                io->OnIoClose();
                continue;
            }
        }
        if (!m_running) {
            if (m_stopcb) {
                m_stopcb();
            }
            break;
        }
    }

    return 0;
}

int Epoll::Operate(int sockfd, int events, void *io) {
    struct epoll_event  ev;
    if (sockfd <= 0 || m_epfd <= 0) {
        return -1;
    }
    ev.events   = events;
    ev.data.ptr = io;
    if (events == 0) {
        epoll_ctl(m_epfd, EPOLL_CTL_DEL, sockfd, &ev);
        __sync_fetch_and_sub(&m_fdnum, 1);
        return 0;
    }

    int ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, sockfd, &ev);
    if (ret == -1 && errno == EEXIST) {
        ret = epoll_ctl(m_epfd, EPOLL_CTL_MOD, sockfd, &ev);
    } else if (ret != -1) {
        __sync_fetch_and_add(&m_fdnum, 1);
    }
    return ret;
}

bool Epoll::IsRunning() {
    bool  running = m_running;
    asm volatile("lfence" ::: "memory");
    
    return running;
}

void Epoll::SetStopFunc(function<void()> stopcb) {
    m_stopcb = stopcb;
}

