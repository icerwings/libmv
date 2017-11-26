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

#include "log.h"
#include "block.h"
#include "buff.h"

Buff::Buff(uint32_t size) : m_occupated(0), m_buff(nullptr), m_len(0) {
    m_head = new Block(size);
    m_tail = m_head;
}

Buff::~Buff() {
    while (m_head != nullptr) {
        m_tail      = m_head;
        m_head      = m_head->next;
        delete m_tail;
    }
}

int Buff::Push(const char * pkg, uint32_t len) {
    uint32_t        offset      = 0;
    while (true) {
        uint32_t size = m_tail->capacity + m_tail->out - m_tail->in;
        if (size >= len - offset) {
            m_tail->Push(pkg + offset, len - offset);
            break;
        }
        m_tail->Push(pkg + offset, size);        
        m_tail->next = new Block(m_tail->capacity);
        if (m_tail->next == nullptr) {
            return -1;
        }
        m_tail       = m_tail->next;
        offset       += size;
    }
    m_occupated += len;
    return (int)len;
}

int Buff::Pop(char * pkg, uint32_t len) {
    uint32_t        offset      = 0;
    while (true) {
        int ret = m_head->Pop(pkg + offset, len - offset);
        if (ret == -1) {
            return -1;
        } else if (ret == (int)(len - offset)) {
            break;
        }
        if (m_head->next == nullptr) {
            len = offset;
            break;
        }
        offset += ret;
        Block * cur = m_head;
        m_head = m_head->next;
        delete cur;
    }
    m_occupated -= len;
    return (int)len;
}

int Buff::Read(function<int(const char *, uint32_t)> readcb, bool once) {
    int             sum         = 0;
    if (!readcb) {
        return -1;
    }
    while (true) {
        uint32_t cap = m_head->ConsBlock();
        if (cap == 0) {
            if (m_head->next != nullptr) {
                Block   * temp = m_head;
                m_head = m_head->next;
                delete temp;
                continue;
            }
            break;
        }        
        int ret = readcb((const char *)m_head->Head(), cap);
        if (ret < 0) {
            return -1;
        }
        m_head->out += ret;
        sum += ret;
        if (ret < (int)cap || once) {
            break;
        }
    }
    m_occupated -= sum;
    return sum;
}

int Buff::Write(function<int(char *, uint32_t)> wtcb, bool once) {
    int             sum         = 0;
    if (!wtcb) {
        return -1;
    }
    while(true) {
        uint32_t cap = m_tail->ConsCap();
        if (cap == 0) {
            m_tail->next = new Block(m_head->capacity);
            if (m_tail->next == nullptr) {
                return -1;
            }
            m_tail       = m_tail->next;
            continue;
        }
        int ret = wtcb(m_tail->Tail(), cap);
        if (ret < 0) {
            return -1;
        }
        m_tail->in += ret;
        sum += ret;
        if (ret < (int)cap || once) {
            break;
        }
    }
    m_occupated += sum;
    return sum;
}

char * Buff::GetPopBuffByLen(uint32_t len) {
    if (m_occupated < len || len == 0) {
        return nullptr;
    }

    if (m_head->ConsBlock() > len) {
        char * buff = m_head->Head();
        m_head->out += len;
        m_occupated -= len;
        return buff;
    }
    if (m_len < len) {
        if (m_buff != nullptr) {
            delete m_buff;
        }
        m_buff = new char[len];
        if (m_buff == nullptr) {
            return nullptr;
        }
        m_len = len;
    }

    uint32_t    offset      = 0;
    while (true) {
        int ret = m_head->Pop(m_buff + offset, len - offset);
        if (ret < 0) {
            return nullptr;
        }
        m_occupated -= ret;
        if (ret == (int)(len - offset)) {
            return m_buff;
        } else if (m_head->next == nullptr) {
            return nullptr;
        } else {
            Block   * cur = m_head;
            m_head = m_head->next;
            delete cur;
            offset += ret;
        }
    }
}

int Buff::GetOffset(const string & end) {
    uint32_t        offset      = 0;
    uint32_t        sum         = 0;
    uint32_t        matched     = 0;
    Block   *       block       = m_head;
    while (true) {
        while (block->out + offset <= block->in) {
            uint32_t    loc     = (block->out + offset) & (block->capacity - 1);
            if (block->buff[loc] == end[matched]) {
                if (++matched == end.size()) {
                    sum += ++offset;
                    return sum;
                }
            } else if (matched > 0){
                offset -= matched;
                matched = 0;
            }
            offset++;
        }
        if (block->next == nullptr) {
            break;
        }
        block = block->next;
        sum += offset;
        offset = 0;
    }
    return -1;
}

char * Buff::GetPopBuffUntilStr(const string & end, uint32_t & size) {
    if (m_occupated == 0 || end.empty()) {
        return nullptr;
    }
    int ret     = GetOffset(end);
    if (ret <= 0) {
        return nullptr;
    }
    size = (uint32_t)ret;
    return GetPopBuffByLen(size);
}

