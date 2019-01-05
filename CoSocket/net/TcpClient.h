#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "CoSocket.h"

class TcpClient
{
public:
    TcpClient(CoSocket &cs);
    ~TcpClient();

    int Connect(const std::string &ip, unsigned short port,
                int64_t timeoutMs);

    ssize_t Read(char *buffer, size_t size, int64_t timeoutMs);
    ssize_t ReadFull(char *buffer, size_t size, int64_t timeoutMs);
    ssize_t Write(const char *buffer, size_t size, int64_t timeoutMs);
    ssize_t WriteAll(const char *buffer, size_t size, int64_t timeoutMs);

    void Close();

private:
    CoSocket &m_cs;
    int m_fd;
};

#endif // TCP_CLIENT_H

