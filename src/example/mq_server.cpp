#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <map>
#include <set>
#include "server.h"
#include "log.h"
#include "tcp.h"
#include "buff.h"
#include "defer.h"
#include "comm.h"
using namespace std;

static map<Tcp *, set<string> >  user2Topic;
static map<string, set<Tcp *> >  topic2User;

int OnSubscribe(const string & topic, Tcp * tcp) {
    if (topic.empty() || tcp == nullptr) {
        return -1;
    }

    topic2User[topic].insert(tcp);
    user2Topic[tcp].insert(topic);
    return 0;
}

void OnUserExit(Tcp * tcp) {
    auto topics = user2Topic.find(tcp);
    if (topics == user2Topic.end()) {
        return;
    }
    for (auto topic : topics->second) {
        auto users = topic2User.find(topic);
        if (users == topic2User.end()) {
            continue;
        }
        auto iter = users->second.find(tcp);
        if (iter != users->second.end()) {
            users->second.erase(iter);
            if (users->second.empty()) {
                topic2User.erase(users);
            }
        }
    }
    user2Topic.erase(topics);
}

int OnPublish(const string & topic, const string & content, Tcp * tcp) {
    if (topic.empty() || content.empty() || tcp == nullptr) {
        return -1;
    }

    char        *ack = (char *)calloc(sizeof(MqHead) + sizeof(MqBody), sizeof(char));
    if (ack == nullptr) {
        return -1;
    }
    Defer   ackGuard([&]{
        delete ack;
    });

    MqHead      *ackHead = (MqHead *)ack;
    MqBody      *ackBody = (MqBody *)(ack + sizeof(MqHead));
    ackHead->bodySize = sizeof(MqBody);
    ackBody->type = Type::PUBLISH_ACK;
    strncpy(ackBody->topic, topic.c_str(), sizeof(ackBody->topic) - 1);
    
    auto tcpSet = topic2User.find(topic);
    if (tcpSet == topic2User.end()) {
        ackBody->retcode = Code::NO_CONSUMER;
    } else {
        ackBody->retcode = Code::SUCCESS;

        uint32_t    msgSize = sizeof(MqHead) + sizeof(MqBody) + content.size() + 1;
        char        * msg = (char *)calloc(msgSize, sizeof(char));
        if (msg == nullptr) {
            return -1;
        }
        Defer   msgGuard([&]{
            delete msg;
        });
        MqHead     *msgHead = (MqHead *)msg;
        MqBody     *msgBody = (MqBody *)(msg + sizeof(MqHead));
        msgHead->bodySize = msgSize - sizeof(MqHead);
        msgBody->type = Type::MESSAGE;
        strncpy(msgBody->topic, topic.c_str(), sizeof(msgBody->topic) - 1);
        strncpy(msgBody->content, content.c_str(), content.size());

        for (auto & iter : tcpSet->second) {
            iter->SendMsg(string(msg, msgSize));
       }
    }

    return tcp->SendMsg(string(ack, sizeof(MqHead) + sizeof(MqBody)));
}

int HandleMsg(Buff * buff, Tcp * tcp, uint32_t & bodySize) {
    if (buff == nullptr || tcp == nullptr) {
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
    uint32_t contentLen = bodySize - sizeof(MqBody);
    bodySize = 0;
    int     ret     = -1;
    if (body->type == Type::SUBSCRIBE) {
        ret = OnSubscribe(body->topic, tcp);
    } else if (body->type == Type::PUBLISH) {
        ret = OnPublish(string(body->topic), string(body->content, contentLen), tcp);
    }
    
    if (ret < 0) {
        return -1;
    }
    return size;    
}

int main() {
    Server   svr;
    if (svr.Open(4567, 10) != 0) {
        ErrorLog(0).Flush();
    }
    uint32_t   bodySize     = 0;
    svr.SetIoCloseFunc([](Tcp * tcp) {
        OnUserExit(tcp);
    });
    svr.SetIoReadFunc([&bodySize](Buff * buff, Tcp * tcp) {
        return HandleMsg(buff, tcp, bodySize);
    });
    svr.SetExitFunc([]{
        cout << "server exit!" << endl;
    });
    svr.Run();
    return 0;
}


