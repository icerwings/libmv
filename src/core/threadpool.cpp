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

#include <unistd.h>
#include <string.h>
#include <thread>
#include "kfifo.h"
#include "defer.h"
#include "eventfd.h"
#include "epoll.h"
#include "log.h"
#include "threadpool.h"

ThreadPool::ThreadPool(uint32_t size) {
    m_fifo = new Kfifo<Eventfd * >(size);
    if (m_fifo == nullptr) {
        return;
    }
    Defer   guard([&]{
        delete m_fifo;
    });
    if (!m_fifo->Valid()) {
        return;
    }
    if (GenThread(size) < 0) {
        return;
    }
    guard.Dismiss();
}

ThreadPool::~ThreadPool() {
    if (m_fifo != nullptr) {
        delete m_fifo;
    }
}

int ThreadPool::GenThread(uint32_t size) {
    int             iLoop       = 0;
    Epoll           *epoll[size];
    Eventfd         *eventfd[size];
    
    memset(epoll, 0, sizeof(epoll));
    memset(eventfd, 0, sizeof(eventfd));
    
    Defer       guard([&]{
        for (iLoop = 0; iLoop < (int)size; iLoop++) {
            if  (epoll[iLoop] != nullptr) {
                delete epoll[iLoop];
            }
            if (eventfd[iLoop] != nullptr) {
                delete eventfd[iLoop];
            }
        }
    });
    
    for (iLoop = 0; iLoop < (int)size; iLoop++) {
        epoll[iLoop]        = new Epoll();
        eventfd[iLoop]      = new Eventfd(epoll[iLoop]);
        if (epoll[iLoop] == nullptr || eventfd[iLoop] == nullptr) {
            return -1;
        }
        Eventfd     * evfd      = eventfd[iLoop];
        evfd->SetCloseFunc([=]{
            delete evfd;
        });
        m_fifo->Enqueue(eventfd[iLoop]);
        Epoll       * epl       = epoll[iLoop];
        thread    work([=]{
            epl->Loop();
        });
        work.detach();
    }    
    while (true) {
        for (iLoop = 0; iLoop < (int)size; iLoop++) {
            if (!epoll[iLoop]->IsRunning()) {
                break;
            }
        }
        if (iLoop == (int)size) {
            break;
        }
        usleep(100000);
    }
    guard.Dismiss();
    
    return 0;
}

void ThreadPool::OnEvRead(Task task, Eventfd * eventfd) {
    if (task) {
        task();
    }
   
    m_fifo->Enqueue(eventfd, true);
}

int ThreadPool::DoTask(function<void()> task, bool locked) {
    Eventfd         *eventfd    = nullptr;
    do {
        if (m_fifo->Dequeue(eventfd, locked) != 0) {
            return -1;
        }
    }while (eventfd == nullptr);
    eventfd->SetEvFunc([=]{
        OnEvRead(task, eventfd);
    });

    if (eventfd->SendEvent() < 0) {
        return -1;
    }
    
    return 0;
}


