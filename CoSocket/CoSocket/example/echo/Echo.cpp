#include "TcpServer.h"
#include "CoSocket.h"
#include "Timer.h"
#include "unistd.h"
#include <vector>

void HandleRequest(TcpServer::ConnectorPtr &connector)
{
    printf("My PID is: %d\n", getpid());

    std::vector<char> buffer(1024);
    while(1)
    {
        int ret = connector->Read(buffer.data(), buffer.size());
        if (ret <= 0)
            return;

        ret = connector->Write(buffer.data(), ret);
        if (ret < 0)
            return;
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
