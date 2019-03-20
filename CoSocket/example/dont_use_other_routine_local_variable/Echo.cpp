#include "TcpServer.h"
#include "CoSocket.h"
#include "Timer.h"
#include "SimpleLog.h"
#include <unistd.h>
#include <vector>

// Should use CoQueue to dipatch data

// TcpServer::ConnectorPtr g_connector;
// void HandleRequest(TcpServer::ConnectorPtr connector, unsigned short port)
// {
//     // When port=34567 connect, g_connector is 12345's ?
//     // Yes, because copy here
//     if (port == 12345)
//         g_connector = connector;

//     SIMPLE_LOG("My PID is: %d", getpid());

//     std::vector<char> buffer(1024);
//     while(1)
//     {
//         int ret = connector->Read(buffer.data(), buffer.size(), 300000);
//         if (ret <= 0)
//         {
//             SIMPLE_LOG("read failed, error: %d", ret);
//             return;
//         }

//         ret = g_connector->WriteAll(buffer.data(), ret, 300000);
//         if (ret < 0)
//         {
//             SIMPLE_LOG("write failed, error: %d", ret);
//             return;
//         }
//     }
// }

TcpServer::ConnectorPtr *g_connector;
void HandleRequest(TcpServer::ConnectorPtr connector, unsigned short port)
{
    // When port=34567 connect, g_connector is 12345's ?
    // No, g_connector is a address which pointing to the stack variable connector
    if (port == 12345)
        g_connector = &connector;

    SIMPLE_LOG("My PID is: %d", getpid());

    std::vector<char> buffer(1024);
    while(1)
    {
        int ret = connector->Read(buffer.data(), buffer.size(), 300000);
        if (ret <= 0)
        {
            SIMPLE_LOG("read failed, error: %d", ret);
            return;
        }

        ret = (*g_connector)->WriteAll(buffer.data(), ret, 300000);
        if (ret < 0)
        {
            SIMPLE_LOG("write failed, error: %d", ret);
            return;
        }
    }
}

int main()
{
    TcpServer echo;
    echo.Bind("0.0.0.0", 12345);
    echo.Bind("0.0.0.0", 34567);
    echo.SetRequestHandler(std::bind(
        &HandleRequest, std::placeholders::_1, std::placeholders::_2));
    std::list<int> childIds;
    echo.ListenAndFork(1, childIds);
    echo.MonitorSlavesLoop();
    return 0;
}

/*

telnet 127.0.0.1 12345
Trying 127.0.0.1...
Connected to 127.0.0.1.
Escape character is '^]'.


telnet 127.0.0.1 34567
Trying 127.0.0.1...
Connected to 127.0.0.1.
Escape character is '^]'.
123
123

*/
