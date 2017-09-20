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

#ifndef __signalfd_h__
#define __signalfd_h__

#include <signal.h>
#include <functional>
#include <map>
#include "io.h"
using namespace std;

class Signalfd: public IO {
public:
    explicit Signalfd(Epoll *epoll);
    virtual ~Signalfd();

    int         SetSigFunc(int sigval, function<void()> sigcb);
    void        SetCloseFunc(function<void()> closecb);
    int         SendSignal(int sigval);

protected:
    virtual     void OnIoClose() override;
    virtual     int OnIoRead() override;

private:
    int         GetSignalFd();

    Epoll               *m_epoll;
    int                 m_signalfd;    
    sigset_t            m_mask;
    function<void()>    m_sigcb;
    function<void()>    m_closecb;

    map<int, function<void()> >     m_sigSet;
};

#endif

