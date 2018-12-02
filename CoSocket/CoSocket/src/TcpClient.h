#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "CoSocket.h"

class TcpClient
{
public:
    TcpClient(CoSocket &cs);
    ~TcpClient();

    int Connect(const std::string &ip, unsigned short port);

    ssize_t Read(char *buffer, size_t size);
    ssize_t Write(const char *buffer, size_t size);

    void Close();

private:
    CoSocket &m_cs;
    int m_fd;
};

#endif // TCP_CLIENT_H

