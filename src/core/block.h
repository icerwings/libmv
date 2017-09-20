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

#ifndef __block_h__
#define __block_h__

#include <stdint.h>
#include <string.h>

#define min(x, y) ((x) < (y)) ? (x) : (y)

struct Block {
    char            *buff;
    uint32_t        capacity;
    uint32_t        in;
    uint32_t        out;
    Block           *next;

    explicit Block(uint32_t size) : capacity(size), in(0), out(0),
                            buff(new char[size]), next(nullptr) {}

    ~Block() {
        if (buff != nullptr) {
            delete buff;
            buff = nullptr;
        }
    }
    
    inline int Push(const char * pkg, uint32_t len) {
        if (pkg == nullptr || len == 0 || buff == nullptr) {
            return -1;
        }

        if ((int)(in - out) > (int)(capacity - len)) {
            len = capacity + out - in;
        }
        uint32_t l = min(len, capacity - (in & (capacity - 1)));
        memcpy(buff + (in & (capacity - 1)), pkg, l);
        memcpy(buff, pkg + l, len - l);
        in += len;

        return (int)len;
    }

    inline int Pop(char * pkg, uint32_t len) {
        if (pkg == nullptr || len == 0 || buff == nullptr) {
            return -1;
        }
        if (len > in - out) {
            len = in - out;
        }
        uint32_t l = min(len, capacity - (out & (capacity - 1)));
        memcpy(pkg, buff + (out & (capacity - 1)), l);
        memcpy(pkg + l, buff, len - l);
        out += len;

        return (int)len;
    }

    inline uint32_t ConsCap() {
        uint32_t    t = (in & (capacity - 1));
        return (in - out) < t ? (capacity - t) : (capacity + out - in);
    }

    inline uint32_t ConsBlock() {
        uint32_t    t = (out & (capacity - 1));
        return (in - out) < capacity - t ? (in - out) : (capacity - t);
    }

    inline char * Head() {
        return buff + (out & (capacity - 1));
    }

    inline char * Tail() {
        return buff + (in & (capacity - 1));
    }
};

#endif

