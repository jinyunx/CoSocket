#include "TcpClient.h"
#include "CoSocket.h"
#include "Timer.h"
#include <signal.h>

void Client(CoSocket &cs)
{
    TcpClient tcpClient(cs);
    Timer timer(cs);

    int ret = tcpClient.Connect("10.104.143.116", 12345);
    if (ret != 0)
        printf("connect failed, error: %d\n", ret);

    const char buff[] = "test over";
    while(1)
    {
        ret = tcpClient.Write(buff, sizeof buff);
        if (ret < 0)
            printf("write failed, error: %d\n", ret);
        timer.Wait(2000);
    }
}

int main()
{
    CoSocket coSocket;
    coSocket.NewCoroutine(std::bind(&Client,
                                    std::ref(coSocket)));
    coSocket.Run();
    return 0;
}
