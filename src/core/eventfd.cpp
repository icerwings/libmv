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

#include "log.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <errno.h>
#include "net.h"
#include "eventfd.h"
#include "epoll.h"

Eventfd::Eventfd(Epoll *epoll) : m_epoll(epoll) {    
    m_eventfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (m_eventfd == - 1 && errno == EINVAL) {
        m_eventfd = eventfd(0, 0);        
        if (m_eventfd > 0) {
            FdSet(m_eventfd, O_NONBLOCK);
            FdSet(m_eventfd, FD_CLOEXEC);
        }
    }
}

Eventfd::~Eventfd() {
    OnIoClose();
}

void Eventfd::SetEvFunc(function<void()> evcb) {
    if (m_eventfd == -1 || m_epoll == nullptr) {
        return;
    }
    m_evcb = evcb;
    m_epoll->Operate(m_eventfd, EPOLLIN, this);
}

void Eventfd::SetCloseFunc(function<void()> closecb) {
    if (m_eventfd == -1) {
        return;
    }
    m_closecb = closecb;
}

int Eventfd::SendEvent() {
    if (m_eventfd == -1) {
        return -1;
    }
    int         ret         = 0;
    uint64_t    value       = 1;
    do {
        ret = write(m_eventfd, &value, sizeof(value));
    }while (ret == -1 && errno == EINTR);
    
    return (ret != -1) ? 0 : -1;
}

void Eventfd::OnIoClose() {
    if (m_eventfd > 0 && m_epoll != nullptr) {
        m_epoll->Operate(m_eventfd, 0, this);
        Close(m_eventfd);
        m_eventfd = -1;
        if (m_closecb) {
            m_closecb();
        }
    }    
}

int Eventfd::OnIoRead() {
    if (m_eventfd == -1) {
        return -1;
    }
    int         ret         = 0;
    uint64_t    value       = 0;
    while (true) {
        ret = read(m_eventfd, &value, sizeof(value));
        if (ret == -1 && errno == EAGAIN) {
            break;
        } else if (ret == -1 && errno == EINTR) {
            continue;
        } else if (ret == -1) {
            return 0;
        }
    }
    if (m_evcb) {
        m_evcb();
    }
    return 0;
}


