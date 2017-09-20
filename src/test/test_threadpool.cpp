#include <unistd.h>
#include <iostream>
#include <chrono>
#include "threadpool.h"
using namespace chrono;

int global = 0;
void test() {
    __sync_fetch_and_add(&global, 1);
    //usleep(100000);
};

int main() {
    ThreadPool  *pool   = new ThreadPool(1000);
    int         count   = 0;
    cout << "start" << endl;
    auto start = steady_clock::now();
    for (count = 0; count < 10000;) {
        int ret = pool->DoTask([]{
            test();
        });
        if (ret == 0) {
            count++;
        }
        usleep(1);
    }
    auto dur = (duration_cast<milliseconds>(steady_clock::now() - start)).count();
    cout << "time:" << dur << endl;
    while (1) {
        cout << global << endl;
        sleep(1);
    }
    return 0;
}


