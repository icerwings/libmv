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

#include <sys/timerfd.h>
#include "log.h"
#include "net.h"
#include "epoll.h"
#include "timerfd.h"

Timerfd::Timerfd(Epoll * epoll) : m_epoll(epoll) {
    m_timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (m_timerfd == -1 && errno == EINVAL) {
        m_timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
        if (m_timerfd > 0) {
            FdSet(m_timerfd, TFD_NONBLOCK);
            FdSet(m_timerfd, TFD_CLOEXEC);
        }
    }
}

Timerfd::~Timerfd() {
    OnIoClose();
}

void Timerfd::SetTimerFunc(function<void()> timercb, int interval) {
    if (m_timerfd == -1 || m_epoll == nullptr) {
        return;
    }
    if (interval <= 0 || !timercb) {
        return;
    }
    
    struct itimerspec   value;
    value.it_value.tv_sec = 0;
    value.it_value.tv_nsec = 1000000;
    value.it_interval.tv_sec = interval / 1000;
    value.it_interval.tv_nsec = interval % 1000;
    
    if (timerfd_settime(m_timerfd, 0, &value, nullptr) == -1) {
        ErrorLog(m_timerfd)(interval).Flush();
        return;
    }

    m_timercb = timercb;
    m_epoll->Operate(m_timerfd, EPOLLIN, this);
}

void Timerfd::SetCloseFunc(function<void()> closecb) {
    if (m_timerfd == -1) {
        return;
    }
    m_closecb = closecb;
}

void Timerfd::OnIoClose() {
    if (m_timerfd > 0 && m_epoll != nullptr) {
        m_epoll->Operate(m_timerfd, 0, this);
        Close(m_timerfd);
        m_timerfd = -1;
        if (m_closecb) {
            m_closecb();
        }
    }
}

int Timerfd::OnIoRead() {
    if (m_timerfd == -1) {
        return -1;
    }
    int         ret         = 0;
    uint64_t    value       = 0;
    while (true) {
        ret = read(m_timerfd, &value, sizeof(value));
        if (ret == -1 && errno == EAGAIN) {
            break;
        } else if (ret == -1 && errno == EINTR) {
            continue;
        } else if (ret == -1) {
            return 0;
        }
    }
    if (m_timercb) {
        m_timercb();
    }
    return 0;
}


