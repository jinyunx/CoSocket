#include "TcpServer.h"
#include "TcpClient.h"
#include "CoSocket.h"
#include "SimpleLog.h"
#include <unistd.h>
#include <vector>

const static int kTimeoutMs = 3000;
const static int kBufferSize = 1024;
static unsigned short gListenPort = 0;
static unsigned short gLocalServerPort = 0;

void ReadServerCoroutine(TcpServer::ConnectorPtr connector,
                         std::shared_ptr<TcpClient> tcpClient)
{
    std::vector<char> buffer(kBufferSize);
    while (1)
    {
        ssize_t readBytes = tcpClient->Read(buffer.data(), buffer.size(), kTimeoutMs);

        if (readBytes <= 0)
        {
            SIMPLE_LOG("read failed, error: %d", readBytes);
            break;
        }
        ssize_t writeBytes = connector->WriteAll(buffer.data(), readBytes, kTimeoutMs);
        if (writeBytes != readBytes)
        {
            SIMPLE_LOG("write all failed, readBytes: %d, writeBytes: %d", readBytes, writeBytes);
            break;
        }
    }
}

void HandleRequest(TcpServer::ConnectorPtr connector)
{
    std::shared_ptr<TcpClient> tcpClient (new TcpClient(connector->GetCoSocket()));
    int ret = tcpClient->Connect("127.0.0.1", gLocalServerPort, kTimeoutMs);
    if (ret < 0)
    {
        SIMPLE_LOG("connect error: %d", ret);
        return;
    }

    connector->GetCoSocket().NewCoroutine(
        std::bind(&ReadServerCoroutine, connector, tcpClient));

    std::vector<char> buffer(kBufferSize);
    while (1)
    {
        ssize_t readBytes = connector->Read(buffer.data(), buffer.size(), kTimeoutMs);
        if (readBytes <= 0)
        {
            SIMPLE_LOG("read failed, error: %d", readBytes);
            break;
        }
        ssize_t writeBytes = tcpClient->WriteAll(buffer.data(), readBytes, kTimeoutMs);
        if (writeBytes != readBytes)
        {
            SIMPLE_LOG("write all failed, readBytes: %d, writeBytes: %d", readBytes, writeBytes);
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        SIMPLE_LOG("Usage: %s listen_port local_server_port");
        return 0;
    }

    gListenPort = atoi(argv[1]);
    gLocalServerPort = atoi(argv[2]);

    TcpServer echo("0.0.0.0", gListenPort);
    echo.SetRequestHandler(std::bind(&HandleRequest,
                                     std::placeholders::_1));
    std::list<int> childIds;
    echo.ListenAndFork(3, childIds);
    echo.MonitorSlavesLoop();
    return 0;
}
