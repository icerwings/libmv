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

#include <sys/signalfd.h>
#include <unistd.h>
#include <errno.h>
#include "net.h"
#include "signalfd.h"
#include "epoll.h"

Signalfd::Signalfd(Epoll *epoll) : m_epoll(epoll), m_signalfd(-1) {
    sigemptyset(&m_mask);
}

Signalfd::~Signalfd() {
    OnIoClose();
}

int Signalfd::GetSignalFd() {
    int sigfd = signalfd(m_signalfd, &m_mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (sigfd == -1 && errno == EINVAL) {
        sigfd = signalfd(m_signalfd, &m_mask, 0);
        if (sigfd > 0) {
            FdSet(sigfd, SFD_NONBLOCK);
            FdSet(sigfd, SFD_CLOEXEC);
        }
    }
    return sigfd;
}

int Signalfd::SetSigFunc(int sigval, function<void()> sigcb) {
    if (m_epoll == nullptr || sigval <= 0 || !sigcb) {
        return -1;
    }

    if (m_sigSet.find(sigval) == m_sigSet.end()) {
        sigaddset(&m_mask, sigval);
        if (sigprocmask(SIG_BLOCK, &m_mask, nullptr) == -1) {
            return -1;
        }
        m_signalfd = GetSignalFd();
        if (m_signalfd == -1) {
            return -1;
        }
    }

    m_sigSet[sigval] = sigcb;
    m_epoll->Operate(m_signalfd, EPOLLIN, this);

    return 0;
}

void Signalfd::SetCloseFunc(function<void()> closecb) {
    if (m_signalfd == -1) {
        return;
    }
    m_closecb = closecb;
}

int Signalfd::SendSignal(int sigval) {
    if (sigval <= 0) {
        return -1;
    }
    int         ret         = 0;
    struct signalfd_siginfo     sigs;
    sigs.ssi_signo  = sigval;

    do {
        ret = write(m_signalfd, &sigs, sizeof(sigs));
    }while (ret == -1 && errno == EINTR);

    return (ret != -1) ? 0 : -1;
}

void Signalfd::OnIoClose() {
    if (m_signalfd > 0 && m_epoll != nullptr) {
        m_epoll->Operate(m_signalfd, 0, this);
        Close(m_signalfd);
        m_signalfd = -1;
        if (m_closecb) {
            m_closecb();
        }
    }    
}

int Signalfd::OnIoRead() {
    if (m_signalfd == -1) {
        return -1;
    } 
    
    while (true) {
        struct signalfd_siginfo     sigs;
        int ret = read(m_signalfd, &sigs, sizeof(sigs));
        if (ret == -1 && errno == EAGAIN) {
            break;
        } else if (ret == -1 && errno == EINTR) {
            continue;
        } else if (ret == -1) {
            return 0;
        }

        if (m_sigSet.find(sigs.ssi_signo) == m_sigSet.end()) {
            continue;
        }
        if (m_sigSet[sigs.ssi_signo]) {
            m_sigSet[sigs.ssi_signo]();
        }
    }
    
    return 0;
}

