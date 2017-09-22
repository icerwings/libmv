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

#ifndef __buff_h__
#define __buff_h__

#include <stdint.h>
#include <string.h>
#include <functional>
using namespace std;

struct Block;
class Buff {
public:
    explicit Buff(uint32_t size);
    ~Buff();
    int         Write(function<int(char *, uint32_t)> wtcb, bool once = false);
    int         Read(function<int(const char *, uint32_t)> readcb, bool once = false);
    char *      GetPopBuffByLen(uint32_t len);
    char *      GetPopBuffUntilStr(const string & end, uint32_t & size);
    int         Push(const char * pkg, uint32_t len);
    bool        Empty() {return (m_occupated == 0);}

private:    
    int         Pop(char * pkg, uint32_t len);
    int         GetOffset(const string & end);
    Block               *m_head;
    Block               *m_tail;
    char                *m_buff;    
    uint32_t            m_occupated;
    uint32_t            m_len;
};

#endif
