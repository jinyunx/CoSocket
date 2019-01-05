#include "TcpServer.h"
#include "CoSocket.h"
#include "Timer.h"
#include "SimpleLog.h"
#include <unistd.h>
#include <vector>

void HandleRequest(TcpServer::ConnectorPtr connector)
{
    SIMPLE_LOG("My PID is: %d", getpid());

    std::vector<char> buffer(1024);
    while(1)
    {
        int ret = connector->Read(buffer.data(), buffer.size(), 3000);
        if (ret <= 0)
        {
            SIMPLE_LOG("read failed, error: %d", ret);
            return;
        }

        ret = connector->WriteAll(buffer.data(), ret, 3000);
        if (ret < 0)
        {
            SIMPLE_LOG("write failed, error: %d", ret);
            return;
        }
    }
}

int main()
{
    TcpServer echo("0.0.0.0", 12345);
    echo.SetRequestHandler(std::bind(&HandleRequest,
                                     std::placeholders::_1));
    std::list<int> childIds;
    echo.ListenAndFork(3, childIds);
    echo.MonitorSlavesLoop();
    return 0;
}
