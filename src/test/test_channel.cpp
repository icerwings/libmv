
#include <unistd.h>
#include <iostream>
#include <thread>
#include "epoll.h"
#include "channel.h"

using namespace std;

long long global  = 0;

int GoChannel() {
    Epoll * epoll = new Epoll();
    Channel<int> * chan = new Channel<int>(epoll, 100);
    if (epoll == nullptr || chan == nullptr) {
        return -1;
    }

    chan->SetChanEvent([](const int & val){
        global += val;
        return 0;
    });
    thread  consumer([epoll]{
        epoll->Loop();
    });
    consumer.detach();
    while (!epoll->IsRunning()) {}
    
    thread  producer([chan]{
        for (int iLoop = 1; iLoop < 1000000;) {
            if (chan->PushChan(iLoop) == 0) {
                iLoop++;
            }
        }
    });
    producer.detach();

    while (true) {
        cout << global << endl;
        sleep(1);
    }
    
    return 0;
}

int main() {
    GoChannel();
    return 0;
}

