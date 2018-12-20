#include "TcpClient.h"
#include "SimpleLog.h"

#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

TcpClient::TcpClient(CoSocket &cs)
    : m_cs(cs),
      m_fd(::socket(AF_INET, SOCK_STREAM |
                    SOCK_NONBLOCK | SOCK_CLOEXEC,
                    IPPROTO_TCP))
{
    if (m_fd < 0)
    {
        SIMPLE_LOG("socket failed, error: %s",
                   strerror(errno));
        exit(1);
    }
}

TcpClient::~TcpClient()
{
    Close();
}

int TcpClient::Connect(const std::string &ip,
                       unsigned short port,
                       int64_t timeoutMs)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_port = htons(port);
    return m_cs.Connect(
        m_fd, reinterpret_cast<struct sockaddr *>(&addr),
        sizeof addr, timeoutMs);
}

ssize_t TcpClient::Read(char *buffer, size_t size,
                        int64_t timeoutMs)
{
    return m_cs.Read(m_fd, buffer, size, timeoutMs);
}

ssize_t TcpClient::Write(const char *buffer, size_t size,
                         int64_t timeoutMs)
{
    return m_cs.Write(m_fd, buffer, size, timeoutMs);
}

void TcpClient::Close()
{
    if (m_fd >= 0)
        ::close(m_fd);
    m_fd = -1;
}

