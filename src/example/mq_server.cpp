
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
    if (topic.empty() || content.empty()) {
        return -1;
    }

    Stream      stream;
    MqBody      body;
    MqHead      head;

    body.topic = topic;

    stream.MemPush((void *)&head, sizeof(head));
    
    auto tcpSet = topic2User.find(topic);
    if (tcpSet == topic2User.end()) {
        body.retcode = Code::NO_CONSUMER;
    } else {        
        body.content = content;
        body.retcode = Code::SUCCESS;
        body.type = Type::MESSAGE;

        stream << body;
        ((MqHead *)stream.str().c_str())->bodySize = stream.str().size() - sizeof(MqHead);

        for (auto & iter : tcpSet->second) {
            iter->SendMsg(stream.str());
        }
    }

    stream.Clear();
    body.type = Type::PUBLISH_ACK;
    body.content.clear();

    stream.MemPush((void *)&head, sizeof(head));
    stream << body;
    ((MqHead *)stream.str().c_str())->bodySize = stream.str().size() - sizeof(MqHead);
    return tcp->SendMsg(stream.str());
}

int HandleMsg(Buff * buff, Tcp * tcp, uint32_t & bodySize) {
    if (buff == nullptr || tcp == nullptr) {
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
    InfoLog(body.topic)(static_cast<int>(body.type))(body.content);
    bodySize = 0;
    
    int     ret     = -1;
    if (body.type == Type::SUBSCRIBE) {
        ret = OnSubscribe(body.topic, tcp);
    } else if (body.type == Type::PUBLISH) {
        ret = OnPublish(body.topic, body.content, tcp);
    }
    
    if (ret < 0) {
        return -1;
    }
    return 1;    
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
    svr.SetIoReadFunc([&bodySize](Buff * buff, Tcp * tcp){
        return HandleMsg(buff, tcp, bodySize);
    });
    svr.SetExitFunc([]{
        cout << "server exit!" << endl;
    });
    svr.Run();
    return 0;
}
    

