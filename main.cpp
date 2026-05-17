#include "ThreadPool.h"

#include <iostream>



int main()
{
    ThreadPool p(2);

    p.start();

    sleep(1);

    p.addTask([](){ std::cout << gettid() << ":task1" << std::endl; });
    p.addTask([](){ std::cout << gettid() << ":task2" << std::endl; });

    sleep(1);

    p.stop();

    return 0;
}
