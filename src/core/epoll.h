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

#ifndef __epoll_h__
#define __epoll_h__

#include <functional>
#include <sys/epoll.h>
using namespace std;

class Epoll {
public:
    Epoll();
    ~Epoll();
    
    int         Loop(int timeOut = -1);
    int         Operate(int sockfd, int events, void *io);
    bool        IsRunning();
    int         GetFdNum() {return m_fdnum;}
    void        SetStopFunc(function<void()> stopcb);
    void        StopLoop() {m_running = false;}
    void        CloseAll();
    
private:
    int         m_epfd;
    bool        m_running;
    int         m_fdnum;
    function<void()>    m_stopcb;
};

#endif

