#include "CoSocket.h"
#include "Timer.h"
#include "TcpClient.h"
#include "SimpleLog.h"
#include <iostream>

CoSocket coSocket;

void func(int index)
{
    TcpClient tcpClient(coSocket);
    SIMPLE_LOG("Connect start");
    tcpClient.Connect("220.181.57.216", 80);
    SIMPLE_LOG("Connect end");

    const std::string content = "GET / HTTP/1.1\r\n"
                                "User-Agent: curl/7.29.0\r\n"
                                "Host: baidu.com\r\n"
                                "Accept: */*\r\n\r\n";

    ssize_t size = tcpClient.Write(content.c_str(), content.size());
    SIMPLE_LOG("%d has been sent", static_cast<int>(size));

    char buffer[1024];
    size = tcpClient.Read(buffer, sizeof buffer - 1);
    buffer[size] = '\0';
    SIMPLE_LOG("%d has been read:%s", static_cast<int>(size),
               buffer);

    tcpClient.Close();

    for (int i = 0; i < 10000; ++i)
    {
        SIMPLE_LOG("Sleep func %d", index);
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

