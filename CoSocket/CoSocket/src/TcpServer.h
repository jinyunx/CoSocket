#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <functional>
#include <memory>
#include <list>
#include "TcpConnector.h"
#include "CoSocket.h"

class TcpServer
{
public:
    typedef std::shared_ptr<TcpConnector> ConnectorPtr;

    typedef std::function<bool (void)> HandleInitFunc;
    typedef std::function<void (ConnectorPtr &)> HandleRequest;
    typedef std::function<void (void)> HandleFiniFunc;

    TcpServer(const std::string &ip,
              unsigned short port);
    ~TcpServer();

    // Master listen and fork, then return
    bool ListenAndFork(int numSlaves, std::list<int> childIds);
    // Master monitor the slaves
    void MonitorSlavesLoop();

    void SetRequestHandler(const HandleRequest &handler);

private:
    bool ForkSlaves(int numSlaves, std::list<int> childIds);
     // Slave run
    void SlaveRun();
    void DoAccept();
    void SpawnOnNewRequest(ConnectorPtr &connector);

    std::unique_ptr<CoSocket> m_cs;
    int m_listenFd;

    int m_numSlaveExpected;
    int m_numSlaveRunning;

    HandleInitFunc m_handleInit;
    HandleRequest m_handleRequest;
    HandleFiniFunc m_handleFini;
};

#endif // TCP_SERVER_H
