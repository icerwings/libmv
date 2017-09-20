#include "buff.h"
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <regex>
using namespace std;

int bufflen() {
    Buff   buff(512);
    char   pkg[1600];
    uint32_t * pi = (uint32_t *)pkg;
    *pi = 401;
    pi = (uint32_t *)&pkg[405];
    *pi = 103;
    pi = (uint32_t *)&pkg[512];
    *pi = 1037;
    pi = (uint32_t *)&pkg[1553];
    *pi = 43;
    int sum = 0;
    uint32_t offset = 0;
    while (sum < 1600) {
        buff.Write([&](char *buff, uint32_t capacity){
            if (offset == 1600) return 0;
            uint32_t size = (capacity + offset < 1600) ? capacity : 1600 - offset;
            memcpy(buff, pkg + offset, size);
            offset += size;
            return (int)size;
        }, true);

        uint32_t *p = (uint32_t *)buff.GetPopBuffByLen(4);
        if (p == nullptr) {
            continue;
        }
        
        sum += 4;
        uint32_t  len = *p;
        char *pt = buff.GetPopBuffByLen(len);
        if (pt == nullptr) {
            continue;
        }
        sum += len;        
    }

    printf("sum:%d\n", sum);
    return 0;
}

int buffstr() {
    Buff  buff(512);
    const char getpkg[] = "GET /form.html HTTP/1.1 \r\n"\
        "Accept:image/gif,image/x-xbitmap,image/jpeg,application/x-shockwave-flash,application/vnd.ms-excel,application/vnd.ms-powerpoint,application/msword,*/* \r\n"\
        "Accept-Language:zh-cn \r\n"\
        "Accept-Encoding:gzip,deflate \r\n"\
        "If-Modified-sINCE:wED,05 jAN 2007 11:21:25 GMT \r\n"\
        "If-None-MaTCH:w/\"80bc018f3c41:8317\" \r\n"\
        "User-Agent:Mozilla/4.0(compatible;MSIE6.0;Windows NT 5.0) \r\n"\
        "HosT:WWW.G.edu.cn \r\n"\
        "Connection:KeeP-alive \r\n\r\n";
    const char postpkg[] = "POST /reg.jsp HTTP/1.1 \r\n"\
        "Accept:image/gif,image/x-xbit,... \r\n"\
        "HOST:www.guet.edu.cn \r\n"\
        "Content-Length:22 \r\n"\
        "Connection:Keep-Alive \r\n"\
        "Cache-Control:O_CACHE \r\n\r\n"\
        "user=jeffrey&pwd=12345";

    uint32_t offset = 0;
    buff.Write([&](char *buff, uint32_t capacity){
        const char *pkg = postpkg;
        uint32_t size = (capacity < strlen(pkg) - offset) ? capacity : strlen(pkg) - offset;
        memcpy(buff, pkg + offset, size);
        offset += size;
        return (int)size;
    });
    uint32_t size = 0;
    char *p = buff.GetPopBuffUntilStr("\r\n\r\n", size);
    if (p == nullptr) {
        return -1;
    }
    if (strncasecmp(p, "GET", strlen("GET")) == 0) {
        printf("%d:%d %s\n", strlen(getpkg), size, p);
    } else if (strncasecmp(p, "POST", strlen("POST")) == 0) {
        istringstream ss(p);
        int len = 0;
        do {
            string sub;
            ss >> sub;
            if (strncasecmp(sub.c_str(), "Content-Length:", strlen("Content-Length:")) == 0) {
               len = atoi(sub.c_str() + strlen("Content-Length:"));
               cout << "len:" << len << endl;
               char *body = buff.GetPopBuffByLen(len);
               string stBody(body, len);
               cout << "body:" << stBody << endl;
               break;
            }
        }while (ss);
    }
    return 0;
}

int main() {
    //bufflen();
    buffstr();
    return 0;
}

