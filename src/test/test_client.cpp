#include <iostream>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sstream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "log.h"
#include "epoll.h"
#include "buff.h"
#include "client.h"
using namespace std;

int doRecvAck(Buff * buff, uint32_t & bodySize) {
    if (buff == nullptr) {
        return -1;
    }
    InfoLog(bodySize).Flush();
    uint32_t size   = 0;
    if (bodySize == 0) {
        char * req = buff->GetPopBuffUntilStr("\r\n\r\n", size);
        if (req == nullptr) {
            return 0;
        }
        
        istringstream ss(req);
        for (string line; getline(ss, line); ) {
            if (strncasecmp(line.c_str(), "Content-Length:", strlen("Content-Length:")) == 0) {
               bodySize = atoi(line.c_str() + strlen("Content-Length:"));
               break;
            }
        }
    } 
    if (bodySize == 0) {
        return -1;
    }
    char *body = buff->GetPopBuffByLen(bodySize);
    if (body == nullptr) {
        return 0;
    }
    string stBody(body, bodySize);
    InfoLog(stBody).Flush();
    size += bodySize;
    bodySize = 0;
       
    return size;
}

int main() {
    Epoll       * epoll     = new Epoll();
    TcpClient   * client    = new TcpClient(epoll);
    if (epoll == nullptr || client == nullptr) {
        return -1;
    }

    sockaddr_in     serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(1234);
    serv.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (client->Connect((const struct sockaddr *)&serv) != 0) {
        ErrorLog(errno);
        return -1;
    }

    uint32_t    bodySize    = 0;
    client->SetCloseFunc([client]{
        delete client;
        exit(0);
    });
    client->SetReadFunc([&bodySize](Buff * buff){
        return doRecvAck(buff, bodySize);
    });

    string msg = "GET /form.html HTTP/1.1 \r\n"\
        "Accept-Language:zh-cn \r\n"\
        "Accept-Encoding:gzip,deflate \r\n"\
        "Host:172.0.0.1 \r\n"\
        "Connection:KeeP-alive \r\n\r\n";
    if (client->SendMsg(msg) < 0) {
        return -1;
    }

    epoll->Loop();

    return 0;
}

