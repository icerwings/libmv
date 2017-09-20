#include <stdlib.h>
#include <iostream>
#include "signalfd.h"
#include "epoll.h"
using namespace std;

int GotSignal() {
    Epoll *epoll = new Epoll();
    Signalfd * sigfd = new Signalfd(epoll);
    if (epoll == nullptr || sigfd == nullptr) {
        return -1;
    }

    sigfd->SetCloseFunc([sigfd]{
        delete sigfd;
    });
    sigfd->SetSigFunc(SIGINT, []{
        cout << "Got SIGINT" << endl;
    });
    sigfd->SetSigFunc(SIGQUIT, []{
        cout << "Got sigquit" << endl;
        exit(0);
    });

    epoll->Loop();

    return 0;
}

int main() {
    GotSignal();
    
    return 0;
}

