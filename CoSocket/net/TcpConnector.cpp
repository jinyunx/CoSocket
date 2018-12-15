#include "TcpConnector.h"
#include <unistd.h>

TcpConnector::TcpConnector(CoSocket &cs, int fd)
    : m_cs(cs),
      m_fd(fd)
{
}

TcpConnector::~TcpConnector()
{
    Close();
}

ssize_t TcpConnector::Read(char *buffer, size_t size)
{
    return m_cs.Read(m_fd, buffer, size);
}

ssize_t TcpConnector::Write(const char *buffer, size_t size)
{
    return m_cs.Write(m_fd, buffer, size);
}

void TcpConnector::Close()
{
    if (m_fd >= 0)
        ::close(m_fd);
    m_fd = -1;
}

CoSocket &TcpConnector::GetCoSocket() const
{
    return m_cs;
}

int TcpConnector::GetFd() const
{
    return m_fd;
}

