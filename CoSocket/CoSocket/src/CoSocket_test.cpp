#include "CoSocket.h"
#include "Timer.h"
#include <iostream>

CoSocket coSocket;

void func(int index)
{
    for (int i = 0; i < 10000; ++i)
    {
        printf("Sleep func %d\n", index);
        Timer timer(coSocket);
        timer.Wait(2000);
    }
}

int main()
{
    coSocket.NewCoroutine(std::bind(&func, 1));
    coSocket.NewCoroutine(std::bind(&func, 2));
    coSocket.NewCoroutine(std::bind(&func, 3));
    coSocket.NewCoroutine(std::bind(&func, 4));
    coSocket.NewCoroutine(std::bind(&func, 5));

    coSocket.Run();
    return 0;
}

