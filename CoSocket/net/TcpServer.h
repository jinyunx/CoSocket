#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "TcpConnector.h"
#include "CoSocket.h"

#include <functional>
#include <memory>
#include <list>
#include <vector>

class TcpServer
{
public:
    typedef std::shared_ptr<TcpConnector> ConnectorPtr;

    typedef std::function<bool (void)> HandleInitFunc;
    typedef std::function<void (ConnectorPtr, unsigned short)> HandleRequest;
    typedef std::function<void (void)> HandleFiniFunc;

    TcpServer();
    ~TcpServer();

    bool Bind(const std::string &ip, unsigned short port);

    // Master listen and fork, then return
    bool ListenAndFork(int numSlaves, std::list<int> childIds);
    // Master monitor the slaves
    void MonitorSlavesLoop();

    void SetRequestHandler(const HandleRequest &handler);

private:
    bool Listen();
    bool ForkSlaves(int numSlaves, std::list<int> childIds);
     // Slave run
    void SlaveRun();
    void DoAccept(int fd, unsigned short port);
    void SpawnOnNewRequest(ConnectorPtr &connector, unsigned short port);

    std::unique_ptr<CoSocket> m_cs;
    std::vector<int> m_listenFd;
    std::vector<unsigned short> m_ports;

    int m_numSlaveExpected;
    int m_numSlaveRunning;

    HandleInitFunc m_handleInit;
    HandleRequest m_handleRequest;
    HandleFiniFunc m_handleFini;
};

#endif // TCP_SERVER_H
