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

#ifndef __kfifo_h__
#define __kfifo_h__

#include <stdint.h>
#include <mutex>
#include "log.h"
using namespace std;

static inline uint32_t Pow2Up(uint32_t value) {
    if (value > 0) {
        value--;
        for (int iLoop = 1; iLoop < (int)(sizeof(value) * 8); iLoop *= 2) {
            value |= (value >> iLoop);
        }
    }
    
    return value + 1;
};

template<typename T>
class Kfifo {
public:
    explicit Kfifo(uint32_t count) {
        m_mask  = Pow2Up(count);
        m_buff  = new T[m_mask];
        m_in    = 0;
        m_out   = 0;
        m_mask--;
    }
    ~Kfifo() {
        if (m_buff != NULL) {
            delete [] m_buff;
        }
    }

    bool Valid() {
        return (m_buff != nullptr);
    }
    
    int Enqueue(const T & val, bool locked = false) {
        int         ret     = 0;
        if (m_buff == nullptr) {
            return -1;
        }
        if (!locked) {
            ret = Put(val);
        } else {
            lock_guard<mutex> lock(m_tLock);
            ret = Put(val);
        }
        return ret;
    }
    
    int Dequeue(T & val, bool locked = false) {
        int             ret     = 0;
        if (m_buff == nullptr) {
            return -1;
        }
        if (!locked) {
            ret = Get(val);
        } else {
            lock_guard<mutex> lock(m_hLock);
            ret = Get(val);
        }
        return ret;
    }

private:
    int Put(const T & val) {
        if (m_in - m_out > m_mask) {
            return -1;
        }
        m_buff[m_in & m_mask] = val;
        asm volatile("sfence" ::: "memory");
        m_in++;
        
        return 0;        
    }
    
    int Get(T & val) {
        if (m_in == m_out) {
            return -1;
        }
        val = std::move(m_buff[m_out & m_mask]);
        asm volatile("sfence" ::: "memory");
        m_out++;

        return 0;
    }

    T                   *m_buff;
    uint32_t            m_mask;
    uint32_t            m_in;
    uint32_t            m_out;
    mutex               m_hLock;
    mutex               m_tLock;
};

#endif

