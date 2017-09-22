#include "comm.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "log.h"
#include "epoll.h"
#include "buff.h"
#include "client.h"
#include "defer.h"
using namespace std;

int doRecvAck(Buff * buff, uint32_t & bodySize) {
    if (buff == nullptr) {
        return -1;
    }

    uint32_t    size        = 0;
    if (bodySize == 0) {
        MqHead * head = (MqHead *)buff->GetPopBuffByLen(sizeof(MqHead));
        if (head == nullptr) {
            return 0;
        }
        bodySize = head->bodySize;
        if (bodySize < sizeof(MqBody)) {
            return -1;
        }
        size = bodySize;
    }
    
    MqBody * body = (MqBody *)buff->GetPopBuffByLen(bodySize);
    if (body == nullptr) {
        return 0;
    }
    size += bodySize;
    bodySize = 0;

    if (body->type == Type::PUBLISH_ACK) {
        InfoLog(static_cast<int>(body->retcode))(body->topic).Flush();
    }
    return -1;
}

int PublishMsg(TcpClient * client) {
    string      msg     = "Hello world!";
    uint32_t    size    = sizeof(MqHead) + sizeof(MqBody) + msg.size() + 1;
    char        *packet = (char *)calloc(size, sizeof(char));
    if (packet == nullptr) {
        return -1;
    }
    Defer   msgGuard([&]{
        delete packet;
    });

    MqHead      *msgHead = (MqHead *)packet;
    MqBody      *msgBody = (MqBody *)(packet + sizeof(MqHead));
    msgHead->bodySize = size - sizeof(MqHead);
    msgBody->type = Type::PUBLISH;
    strncpy(msgBody->topic, "my topic", sizeof(msgBody->topic) - 1);
    strncpy(msgBody->content, msg.c_str(), msg.size());
    
    return client->SendMsg(string(packet, size));
}

int main() {
    Epoll       * epoll     = new Epoll();
    TcpClient   * client    = new TcpClient(epoll);
    if (epoll == nullptr || client == nullptr) {
        return -1;
    }

    sockaddr_in     serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(4567);
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

    if (PublishMsg(client) < 0) {
        return -1;
    }

    epoll->Loop();
    return 0;
}

