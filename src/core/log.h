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

#ifndef __log_h__
#define __log_h__

#include <iostream>
#include <errno.h>
#include <string.h>
#include <sstream>
#include <string>
using namespace std;

class Log {
public:
    Log & Log_A;
    Log & Log_B;

    static Log * Instance();
    void Flush(const string & module = string(""));
    
    template <typename T>
    Log & Value(const T & value, const string & name) {
        ostringstream os;
        ostringstream osStr;
        os << value;
        osStr << "\"" << value << "\"";
        if (name == os.str() || name == osStr.str()) {
            logOs << "[" << value << "]";            
            cout << "[" << value << "]";            
        } else if (name == "(*__errno_location ())") {
            logOs << "[errno: " << errno << ", errstr: " << strerror(errno) << "]";
            cout << "[errno: " << errno << ", errstr: " << strerror(errno) << "]";
        } else {
            logOs << "[" << name << ": " << value << "]";
            cout << "[" << name << ": " << value << "]";
        }
        return *this;
    }
    
    Log & Location(const string & srcfile, const int line, const string & func);
    Log & Error(const string & level);
    Log & Assert(const string & cond);
    
private:
    Log();
    static thread_local ostringstream  logOs;
    static Log              *m_instance;    
    string                  m_path;
};

#define Log_A(x) Log_OP(x, B)
#define Log_B(x) Log_OP(x, A)
#define Log_OP(x, next) Log_A.Value((x), #x).Log_##next

#define ErrorLog(x) Log::Instance()->Location(__FILE__, __LINE__, __FUNCTION__).Error("Error").Log_A(x)
#define InfoLog(x) Log::Instance()->Location(__FILE__, __LINE__, __FUNCTION__).Error("Info").Log_A(x)
#define DebugLog(x) Log::Instance()->Location(__FILE__, __LINE__, __FUNCTION__).Error("Debug").Log_A(x)

#endif

