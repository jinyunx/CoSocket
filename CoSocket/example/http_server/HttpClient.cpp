#include "CoSocket.h"
#include "Timer.h"
#include "http/HttpClient.h"
#include "SimpleLog.h"
#include <string.h>

void func(CoSocket &coSocket)
{
    TcpClient tcpClient(coSocket);
    if (int ret = tcpClient.Connect("127.0.0.1", 12345, 3000) != 0)
    {
        SIMPLE_LOG("connect error: %d", ret);
        return;
    }

    Timer timer(coSocket);
    while (1)
    {
        HttpClient httpClient(tcpClient);

        httpClient.AddHeader("h1", "v1");
        httpClient.AddHeader("h2", "v2");

        httpClient.AddParameter("p1", "v1");
        httpClient.AddParameter("p2", "v2");

        httpClient.SetMethod("POST");
        httpClient.SetUrl("/");
        httpClient.SetBody("test body");

        if (ssize_t ret = httpClient.Request() != 0)
        {
            SIMPLE_LOG("request error: %d", ret);
            return;
        }
        SIMPLE_LOG("Status: %u, body: %s", httpClient.GetStatus(),
                   httpClient.GetBody().c_str());
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
