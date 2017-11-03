
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

#ifndef __protocol_h__
#define __protocol_h__

#include <stdio.h>
#include <string>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <map>
using namespace std;


#define  CLASS_ENCODE(x) virtual void Serialize(Stream& s) const {\
    x; \
}
#define  CLASS_DECODE(x) virtual void UnSerialize(Stream& s) {\
    x; \
}

class Stream;
struct MsgBase {
    virtual void Serialize(Stream&) const = 0;
    virtual void UnSerialize(Stream&) = 0;
};

class Stream {
public:
    Stream() : begin(0) {}
    Stream(const string& s) : begin(0), stream(s) {}
    Stream(const char * str, size_t size) : begin(0) {
        stream.assign(str, size);
    }
    ~Stream() {}

    void Clear() {
        stream.clear();
        begin = 0;
    }

    string & str() {
        return stream;
    }

    Stream & operator <<(const char * val) {
        if (val != nullptr) {
            EncodeString(val, strlen(val));
        }
        return *this;
    }
    Stream & operator <<(const string & val) {
        EncodeString(val.c_str(), val.size());
        return *this;
    }
    template<typename T>
    Stream & operator <<(const vector<T> & val) {
        EncodeInt((uint64_t)val.size());
        for (const auto & v : val) {
            (*this) << v;
        }
        return *this;
    }
    template<typename K, typename V>
    Stream & operator <<(const map<K, V> & val) {
        EncodeInt((uint64_t)val.size());
        for (const auto & v : val) {
            (*this) << v.first << v.second;
        }
        return *this;
    }
    template<typename T>
    typename enable_if<is_arithmetic<T>::value, Stream &>::type operator <<(const T & val) {
        if (is_integral<T>::value) {
            EncodeInt((uint64_t)val);
        } else if (is_floating_point<T>::value) {
            char  *v = (char *)&val;
            stream.append(&v[0], &v[sizeof(val)]);
        }
        return *this;
    }
    template<typename T>
    typename enable_if<is_class<T>::value, Stream &>::type operator <<(const T & val) {
        static_assert(is_base_of<MsgBase, T>::value,  
            "Encode invalid type.");
        (dynamic_cast<const MsgBase *>(&val))->Serialize(*this);
        return *this;
    }
    template<typename T>
    typename enable_if<is_enum<T>::value, Stream &>::type operator <<(const T & val) {
        EncodeInt(static_cast<uint64_t>(val));
        return *this;
    }
    template<typename T>
    typename enable_if<(rank<T>::value) == 1, Stream &>::type operator <<(const T & val) {
        for (int i = 0; i < extent<T>::value; i++) {
            (*this) << val[i];
        }
        return *this;
    }
    template<typename T>
    typename enable_if<(rank<T>::value) == 2, Stream &>::type operator <<(const T & val) {
        for (int i = 0; i < extent<T>::value; i++) {
            for (int j = 0; j < extent<T, 1>::value; j++) {
                (*this) << val[i][j];
            }
        }
        return *this;
    }
    void MemPush(void * buff, size_t size) {
        if (buff != NULL && size != 0) {
            char   * str = (char *)buff;
            stream.append(&str[0], &str[size]);
        }
    }

    Stream & operator >>(char * val) {
        if (val != nullptr) {
            DecodeString(val);
        }        
        return *this;
    }
    Stream & operator >>(string & val) {
        DecodeString(val);
        return *this;
    }
    template<typename T>
    Stream & operator >>(vector<T> & val) {
        uint64_t     size    = 0;
        DecodeInt(size);
        for (auto i = 0; i < size; i++) {
            T       v;
            (*this) >> v;
            val.push_back(v);
        }
        return *this;
    }
    template<typename K, typename V>
    Stream & operator >>(map<K, V> & val) {
        uint64_t     size    = 0;
        DecodeInt(size);
        for (auto i = 0; i < size; i++) {
            K       k;
            V       v;
            (*this) >> k >> v;
            val.insert(make_pair(k, v));
        }
        return *this;
    }
    template<typename T>
    typename enable_if<is_arithmetic<T>::value, Stream &>::type operator >>(T & val) {
        if (is_integral<T>::value) {
            uint64_t   v   = 0;
            DecodeInt(v);
            val = (T)v;
        } else if (is_floating_point<T>::value) {
            val = *(T *)(stream.c_str() + begin);
            Step(sizeof(val));
        }
        return *this;
    }
    template<typename T>
    typename enable_if<is_class<T>::value, Stream &>::type operator >>(T & val) {
        static_assert(is_base_of<MsgBase, T>::value,  
            "Decode invalid type.");
        (dynamic_cast<MsgBase *>(&val))->UnSerialize(*this);
        return *this;
    }
    template<typename T>
    typename enable_if<is_enum<T>::value, Stream &>::type operator >>(T & val) {
        uint64_t   value  = 0;
        DecodeInt(value);
        val = static_cast<T>(value);
        return *this;
    }
    template<typename T>
    typename enable_if<(rank<T>::value) == 1, Stream &>::type operator >>(T & val) {
        for (int i = 0; i < extent<T>::value; i++) {
            (*this) >> val[i];
        }
        return *this;
    }
    template<typename T>
    typename enable_if<(rank<T>::value) == 2, Stream &>::type operator >>(T & val) {
        for (int i = 0; i < extent<T>::value; i++) {
            for (int j = 0; j < extent<T, 1>::value; j++) {
                (*this) >> val[i][j];
            }
        }
        return *this;
    }
    void MemPop(void * buff, size_t buffSize) {
        if (buff != NULL && buffSize != 0 && buffSize + begin <= stream.size()) {
            memcpy(buff, stream.c_str(), buffSize);
            Step(buffSize);
        }
    }
    
private:
    void EncodeInt(uint64_t val) {
        int     i       = 9;
        char    v[10]   = {0};
        int     bits    = 0x7f;
        while(true) {
            v[i] |= (val & bits);
            val = val >> 7;
            if (val == 0 || --i < 0) {
                break;
            }
            v[i] |= 0x80;
        }
        stream.append(&v[i], &v[10]);
    }
    void DecodeInt(uint64_t &val) {
        val = 0;        
        uint32_t    s  = 0;
        do {
            val = (val << 7) | (stream[begin + s] & 0x7f);
        }while ((stream[begin + s++] & 0x80) != 0);
        Step(s);
    }
    void EncodeString(const char * val, size_t size) {
        EncodeInt(size);
        stream.append(&val[0], &val[size]);
    }
    void DecodeString(string &val) {
        uint64_t  size = 0;
        DecodeInt(size);
        val.assign(stream.c_str() + begin, size);
        Step(size);
    }
    void DecodeString(char *val) {
        uint64_t  size = 0;
        DecodeInt(size);
        strncpy(val, stream.c_str() + begin, size);
        Step(size);
    }
    void Step(uint32_t step) {
        begin += step;
        if (begin == (uint32_t)stream.size()) {
            Clear();
        }
    }
    string      stream;
    uint32_t    begin;
};

#endif

