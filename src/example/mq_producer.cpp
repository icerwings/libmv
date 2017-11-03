
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

int doRecvAck(Buff * buff, uint32_t & bodySize, TcpClient * client) {
    if (buff == nullptr || client == nullptr) {
        return -1;
    }

    if (bodySize == 0) {
        MqHead * head = (MqHead *)buff->GetPopBuffByLen(sizeof(MqHead));
        if (head == nullptr) {
            PublishMsg(client);
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
    InfoLog(body.topic)(static_cast<int>(body.type)).Flush();
    bodySize = 0;
    
    if (body.type == Type::PUBLISH_ACK) {
        InfoLog(body.content).Flush();        
        return 1;
    }
    
    return -1;
}

int PublishMsg(TcpClient * client) {
    Stream      stream;
    MqBody      body;
    MqHead      head;

    body.topic = "my topic";
    body.content = "Hello world!";
    body.type = Type::PUBLISH;
    
    stream.MemPush((void *)&head, sizeof(head));
    stream << body;
    ((MqHead *)stream.str().c_str())->bodySize = stream.str().size() - sizeof(MqHead);
    
    return client->SendMsg(stream.str());
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

