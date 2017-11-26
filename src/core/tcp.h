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

#ifndef __tcp_h__
#define __tcp_h__

#include <sys/types.h>
#include <stdint.h>
#include <functional>
#include "io.h"
using namespace std;

#define     MIN_BUFF_SIZE       512

class Buff;
class Tcp : public IO {
public:
    Tcp(Epoll * epoll, int sockfd) : m_epoll(epoll), m_sockfd(sockfd), m_rbuff(nullptr), m_wbuff(nullptr)
                                   , m_wtbufsize(MIN_BUFF_SIZE), m_fin(false), m_epollOpr(0) {}
    virtual ~Tcp();

    void        SetReadFunc(function<int(Buff *)> readcb, uint32_t buffsize = 0);
    void        SetCloseFunc(function<void()> closecb);
    void        SetWtBuffSize(uint32_t size);
    int         SendMsg(const string & msg);
    void        SendFin();
    void        TcpClose();
    Epoll       *GetEpoll() {return m_epoll;}
    void        SetRemote(struct sockaddr *remote);
    void        GetRemote(struct sockaddr *remote);

protected:    
    virtual void OnIoClose() override;    
    virtual int OnIoWrite() override;    
    virtual int OnIoRead() override;

protected:    
    int         OnTcpWrite();
    
    int                     m_sockfd;
    Epoll                   *m_epoll;
    int                     m_epollOpr;
    Buff                    *m_rbuff;
    Buff                    *m_wbuff;
    bool                    m_fin;
    uint32_t                m_wtbufsize;
    function<void()>        m_closecb;
    function<int(Buff *)>   m_readcb;
    struct sockaddr         m_remote;
};

#endif

