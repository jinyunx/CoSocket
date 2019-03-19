#include "TcpServer.h"
#include "SimpleLog.h"

#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

TcpServer::TcpServer()
    : m_numSlaveExpected(0),
      m_numSlaveRunning(0)
{

}

TcpServer::~TcpServer()
{
    for (size_t i = 0; i < m_listenFd.size(); ++i)
        ::close(m_listenFd[i]);
}

bool TcpServer::Bind(const std::string &ip, unsigned short port)
{
    int listenFd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);

    int yes = 1;
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_port = htons(port);
    if (::bind(listenFd, reinterpret_cast<struct sockaddr *>(&addr),
               sizeof addr) < 0)
    {
        SIMPLE_LOG("bind failed, ip: %s, port: %d, error: %s",
                   ip.c_str(), port, strerror(errno));
        return false;
    }
    m_ports.push_back(port);
    m_listenFd.push_back(listenFd);

    return true;
}

bool TcpServer::ListenAndFork(int numSlaves, std::list<int> childIds)
{
    if (!Listen())
        return false;

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

bool TcpServer::Listen()
{
    for (size_t i = 0; i < m_listenFd.size(); ++i)
    {
        if (listen(m_listenFd[i], SOMAXCONN) < 0)
        {
            SIMPLE_LOG("listen failed, error: %s", strerror(errno));
            return false;
        }
    }
    return true;
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
            SIMPLE_LOG("fork error: %s", strerror(errno));
    }

    m_numSlaveRunning += childIds.size();

    return !childIds.empty();
}

void TcpServer::SlaveRun()
{
    m_cs.reset(new CoSocket);
    for (size_t i = 0; i < m_listenFd.size(); ++i)
    {
        m_cs->NewCoroutine(std::bind(
            &TcpServer::DoAccept, this, m_listenFd[i], m_ports[i]));
    }
    m_cs->Run();
}

void TcpServer::DoAccept(int fd, unsigned short port)
{
    while(1)
    {
        int sockFd = m_cs->Accept(fd, 0, 0, -1);
        if (sockFd >= 0)
        {
            int flags = fcntl(sockFd, F_GETFL, 0);
            fcntl(sockFd, F_SETFL, flags | O_NONBLOCK);

            ConnectorPtr connector(new TcpConnector(*m_cs, sockFd));
            if (m_handleRequest)
                SpawnOnNewRequest(connector, port);
            else
                ::close(sockFd);
        }
    }
}

void TcpServer::SpawnOnNewRequest(ConnectorPtr &connector, unsigned short port)
{
    m_cs->NewCoroutine(std::bind(m_handleRequest, connector, port));
}
