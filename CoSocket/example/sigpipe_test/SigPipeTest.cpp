#include "TcpClient.h"
#include "CoSocket.h"
#include "Timer.h"
#include "SimpleLog.h"
#include <signal.h>

void Client(CoSocket &cs)
{
    TcpClient tcpClient(cs);
    Timer timer(cs);

    int ret = tcpClient.Connect("10.104.143.116", 12345, 3000);
    if (ret != 0)
        SIMPLE_LOG("connect failed, error: %d", ret);

    const char buff[] = "test over";
    while(1)
    {
        ret = tcpClient.Write(buff, sizeof buff, 3000);
        if (ret < 0)
            SIMPLE_LOG("write failed, error: %d", ret);
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
