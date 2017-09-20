
#include <unistd.h>
#include <iostream>
#include <thread>
#include "timerfd.h"
#include "epoll.h"
using namespace std;

int global = 0;

int GotSignal() {
    Epoll *epoll = new Epoll();
    Timerfd * timerfd = new Timerfd(epoll);
    if (epoll == nullptr || timerfd == nullptr) {
        return -1;
    }

    timerfd->SetCloseFunc([timerfd]{
        delete timerfd;
    });
    timerfd->SetTimerFunc([]{
        global++;
    }, 1000);

    epoll->Loop();

    return 0;
}

int main() {
    thread   work([]{
        GotSignal();
    });
    work.detach();

    while (true) {
        cout << global << endl;
        sleep(1);
    }
    
    return 0;
}

