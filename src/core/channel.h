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

#ifndef __channel_h__
#define __channel_h__

#include <stdint.h>
#include <functional>
#include "kfifo.h"
#include "eventfd.h"
using namespace std;

class Epoll;
template<typename T>
class Channel {
public:
    Channel(Epoll *epoll, uint32_t size) {
        if (epoll == nullptr) {
            m_fifo = nullptr;
            m_eventfd = nullptr;
            return;
        }
        if (size == 0) {
            size = 1;
        }
        m_fifo = new Kfifo<T>(size);
        m_eventfd = new Eventfd(epoll);
    }
    ~Channel() {
        ChanClose();
    }

    int SetChanEvent(function<int(const T &)> evcb) {
        m_evcb = evcb;
        if (m_eventfd == nullptr) {
            return -1;
        }
        m_eventfd->SetCloseFunc([&]{
            ChanClose();
        });
        m_eventfd->SetEvFunc([&]{
            T           value;
            if (!m_evcb) {
                return;
            }
            while (m_fifo->Dequeue(value) == 0) {
                m_evcb(value);
            }
        });
        return 0;
    }
    
    int PushChan(const T & value) {
        if (m_fifo->Enqueue(value) != 0) {
            return -1;
        }
        return m_eventfd->SendEvent();
    }

    void ChanClose() {
        if (m_eventfd != nullptr) {
            delete m_eventfd;
            m_eventfd = nullptr;
        }
        if (m_fifo != nullptr) {
            delete m_fifo;
            m_fifo = nullptr;
        }
    }

private:
    Eventfd     *m_eventfd;
    Kfifo<T>    *m_fifo;
    function<int(const T &)>    m_evcb;
};

#endif

