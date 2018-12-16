#include "TcpServer.h"

#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

TcpServer::TcpServer(const std::string &ip,
                     unsigned short port)
    : m_listenFd(::socket(AF_INET,
                          SOCK_STREAM | SOCK_NONBLOCK,
                          IPPROTO_TCP)),
      m_numSlaveExpected(0),
      m_numSlaveRunning(0)
{
    int yes = 1;
    setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR,
               &yes, sizeof yes);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_port = htons(port);
    if (::bind(m_listenFd,
               reinterpret_cast<struct sockaddr *>(&addr),
               sizeof addr) < 0)
    {
        printf("bind failed, ip: %s, port: %d, error: %s\n",
               ip.c_str(), port, strerror(errno));
        exit(1);
    }
}

TcpServer::~TcpServer()
{
    ::close(m_listenFd);
}

bool TcpServer::ListenAndFork(int numSlaves,
                              std::list<int> childIds)
{
    if (listen(m_listenFd, SOMAXCONN) < 0)
    {
        printf("listen failed, error: %s\n",
               strerror(errno));
        return false;
    }

    if (numSlaves < 1)
        numSlaves = 1;

    m_numSlaveExpected = numSlaves;

    return ForkSlaves(numSlaves, childIds);
}

void TcpServer::MonitorSlavesLoop()
{
    while(1)
    {
        int numSlaveNeedStart = m_numSlaveExpected - m_numSlaveRunning;
        if (numSlaveNeedStart > 0)
        {
            std::list<int> childIds;
            ForkSlaves(numSlaveNeedStart, childIds);
        }
        if(waitpid(-1, 0, WNOHANG))
            --m_numSlaveRunning;
        else sleep(1);
    }
}

void TcpServer::SetRequestHandler(const HandleRequest &handler)
{
    m_handleRequest = handler;
}

bool TcpServer::ForkSlaves(int numSlaves, std::list<int> childIds)
{
    for (int i = 0; i < numSlaves; ++i)
    {
        int ret = fork();
        if (ret == 0)
            SlaveRun();
        else if (ret > 0)
            childIds.push_back(ret);
        else
            printf("fork error: %s\n", strerror(errno));
    }

    m_numSlaveRunning += childIds.size();

    return !childIds.empty();
}

void TcpServer::SlaveRun()
{
    m_cs.reset(new CoSocket);
    m_cs->NewCoroutine(std::bind(&TcpServer::DoAccept, this));
    m_cs->Run();
}

void TcpServer::DoAccept()
{
    while(1)
    {
        int sockFd = m_cs->Accept(m_listenFd, 0, 0);
        if (sockFd)
        {
            int flags = fcntl(sockFd, F_GETFL, 0);
            fcntl(sockFd, F_SETFL, flags | O_NONBLOCK);

            ConnectorPtr connector(new TcpConnector(*m_cs, sockFd));
            if (m_handleRequest)
                SpawnOnNewRequest(connector);
            else
                ::close(sockFd);
        }
    }
}

void TcpServer::SpawnOnNewRequest(ConnectorPtr &connector)
{
    m_cs->NewCoroutine(std::bind(m_handleRequest,
                                 connector));
}
