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

#include <stdint.h>
#include <stdlib.h>
#include <mutex>
#include <fstream>
#include <ctime>
#include <locale>
#include <thread>
#include "log.h"

static mutex                logLock;
static thread_local ofstream       logFs;

Log * Log::m_instance = nullptr;
thread_local ostringstream Log::logOs;

inline static string TimeStr() {
    char            tmstr[64] = {0};
    time_t t = time(nullptr);
    strftime(tmstr, sizeof(tmstr), "%F %T", localtime(&t));
    return string(tmstr);
}

Log * Log::Instance() {
    Log * temp = m_instance;
    __sync_synchronize();
    if (temp == nullptr) {
        lock_guard<mutex> lock(logLock);
        if (temp == nullptr) {
            temp = new Log();
        }
        __sync_synchronize();
        m_instance = temp;
    }
    return temp;
}

void Log::Flush(const string & module) {
    ostringstream  os;
    if (module.empty()) {
        os << m_path << "base.log";
    } else {
        os << m_path << module << ".log";
    }
    logFs.open(os.str(), ofstream::app);
    logFs << logOs.str() << endl;
    cout << endl;
    logFs.close();
    logOs.str("");
}

Log::Log() : Log_A(*this), Log_B(*this) {
    char   * path = getenv("LOGPATH");
    if (path != nullptr) {
        while (path[0] == ' ') {
            path++;
        }
    }
    if (path == nullptr || path[0] == 0) {
        m_path = "./";
    } else {
        m_path = path;
        if (m_path.back() != '/') {
            m_path.append("/");
        }
    }
}

Log & Log::Location(const string & srcfile, const int line, const string & func) {
    if (!logOs.str().empty()) {
        Flush();
    }
    logOs << "[" << TimeStr() << "][" << srcfile << "::" << line << ", func: " << func << "]";        
    cout << "[" << TimeStr() << "][" << srcfile << "::" << line << ", func: " << func << "]";        
    return *this;
}
Log & Log::Error(const string & level) {
    logOs << "[" << level << "]";
    cout << "[" << level << "]";
    return *this;
}
Log & Log::Assert(const string & cond) {
    logOs << "[Fail:" << cond << "]";
    cout << "[Fail:" << cond << "]";
    return *this;
}


