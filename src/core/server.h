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

#ifndef __server_h__
#define __server_h__

#include <string>
#include <vector>
#include <functional>
#include "io.h"
using namespace std;

class Tcp;
class Buff;
class Signalfd;
class Server: public IO {
public:
    Server();
    virtual ~Server();

    int             Open(uint16_t port, uint32_t poolsize = 0);
    void            SetServFunc(function<int(Buff *, Tcp *)> svrcb);
    void            SetCloseFunc(function<void()> closecb);
    void            SetExitSig(int exitsig);
    void            SetExitFunc(function<void()> exitcb);
    int             Run();

protected:        
    virtual void    OnIoClose() override;
    virtual int     OnIoRead() override;

private:
    int             OpenSpareFile(const string & path);
    void            OnEmfile();
    int             Accept();
    
    Epoll                           *m_epoll;
    Signalfd                        *m_signalfd;
    int                             m_sockfd;
    function<int(Buff *, Tcp *)>    m_svrcb;
    function<void()>                m_closecb;
    int                             m_emfile;
    vector<Epoll *>                 m_pool;
    int                             m_exitsig;
};

#endif

