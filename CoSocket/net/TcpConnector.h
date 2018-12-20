#ifndef TCP_CONNECTOR_H
#define TCP_CONNECTOR_H

#include "CoSocket.h"

class TcpConnector
{
public:
    TcpConnector(CoSocket &cs, int fd);
    ~TcpConnector();

    ssize_t Read(char *buffer, size_t size,
                 int64_t timeoutMs);
    ssize_t Write(const char *buffer, size_t size,
                  int64_t timeoutMs);

    void Close();

    CoSocket &GetCoSocket() const;
    int GetFd() const;

private:
    CoSocket &m_cs;
    int m_fd;
};

#endif // TCP_CONNECTOR_H

