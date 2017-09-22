#include "comm.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <thread>
#include "log.h"
#include "epoll.h"
#include "buff.h"
#include "client.h"
#include "defer.h"
using namespace std;

int doRecvPublish(Buff * buff, uint32_t & bodySize) {
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
    InfoLog(body->topic)(static_cast<int>(body->type))(body->content).Flush();

    return size;
}

int SubscribeMsg(TcpClient * client) {
    char        *packet = (char *)calloc(sizeof(MqHead) + sizeof(MqBody), sizeof(char));
    if (packet == nullptr) {
        return -1;
    }
    Defer   msgGuard([packet]{
        delete packet;
    });

    MqHead      *msgHead = (MqHead *)packet;
    MqBody      *msgBody = (MqBody *)(packet + sizeof(MqHead));
    msgHead->bodySize = sizeof(MqBody);
    msgBody->type = Type::SUBSCRIBE;
    strncpy(msgBody->topic, "my topic", sizeof(msgBody->topic) - 1);
    
    return client->SendMsg(string(packet, sizeof(MqHead) + sizeof(MqBody)));
}

int threadRoute(Epoll * epoll) {
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
        return doRecvPublish(buff, bodySize);
    });

    if (SubscribeMsg(client) < 0) {
        return -1;
    }

    epoll->Loop();

    return 0;
}

int main(int argc, char ** argv) {
    int         nums         = 1;
    if (argc > 1) {
        nums = atoi(argv[1]);
    }

    for (int i = 0; i < nums; i++) {
        Epoll * epoll = new Epoll();
        if (epoll == nullptr) {
            return -1;
        }
        thread   work([&]{
            threadRoute(epoll);
        });
        work.detach();
        while (!epoll->IsRunning()) {
            usleep(10000);
        }
    }

    InfoLog("All thread worked!").Flush();

    while (1) {
        sleep(1);
    }

    return 0;
}

