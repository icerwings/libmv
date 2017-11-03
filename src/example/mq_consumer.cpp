
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

    if (bodySize == 0) {
        MqHead * head = (MqHead *)buff->GetPopBuffByLen(sizeof(MqHead));
        if (head == nullptr) {
            return 0;
        }
        bodySize = head->bodySize;
        if (bodySize == 0) {
            return -1;
        }
    }

    const char* msgptr = (const char*)buff->GetPopBuffByLen(bodySize);
    if (msgptr == nullptr) {
        return 0;
    }
    
    Stream   stream(msgptr, bodySize);
    MqBody    body;
    stream >> body;
    InfoLog(body.topic)(static_cast<int>(body.type))(body.content).Flush();
    bodySize = 0;

    return 1;
}

int SubscribeMsg(TcpClient * client) {
    Stream      stream;
    MqBody      body;
    MqHead      head;

    body.topic = "my topic";
    body.type = Type::SUBSCRIBE;
    
    stream.MemPush((void *)&head, sizeof(head));
    stream << body;
    ((MqHead *)stream.str().c_str())->bodySize = stream.str().size() - sizeof(MqHead);
    
    return client->SendMsg(stream.str());
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

    Epoll * epoll = new Epoll[nums];
    if (epoll == nullptr) {
        return -1;
    }
    for (int i = 0; i < nums; i++) {
        Epoll * epoll = new Epoll();
        if (epoll == nullptr) {
            return -1;
        }
        thread   work([epoll]{
            threadRoute(epoll);
        });
        work.detach();
        while (!epoll->IsRunning()) {
            usleep(10000);
        }
    }

    InfoLog("All thread work").Flush();

    while (1) {
        sleep(1);
    }

    return 0;
}
