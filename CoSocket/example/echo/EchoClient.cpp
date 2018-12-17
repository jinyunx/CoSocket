#include "CoSocket.h"
#include "Timer.h"
#include "TcpClient.h"
#include "SimpleLog.h"
#include <string.h>

void func(CoSocket &coSocket)
{
    TcpClient tcpClient(coSocket);
    tcpClient.Connect("127.0.0.1", 12345);
    Timer timer(coSocket);

    int i = 0;
    char buffer[1024];

    while(1)
    {
        sprintf(buffer, "echo test: %d", ++i);
        ssize_t size = tcpClient.Write(buffer, strlen(buffer));
        SIMPLE_LOG("%d has been sent:%s", static_cast<int>(size),
                   buffer);

        size = tcpClient.Read(buffer, sizeof buffer - 1);
        buffer[size] = '\0';
        SIMPLE_LOG("%d has been read:%s", static_cast<int>(size),
                   buffer);
        timer.Wait(2000);
    }
}

int main()
{
    CoSocket coSocket;
    coSocket.NewCoroutine(std::bind(&func, std::ref(coSocket)));
    coSocket.Run();
    return 0;
}
