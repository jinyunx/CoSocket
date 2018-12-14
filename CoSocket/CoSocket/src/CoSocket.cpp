#include "CoSocket.h"
#include "Epoller.h"

#include <sys/timerfd.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

extern "C"
{
#include "coroutine/coroutine.h"
}

namespace 
{
class IgnoreSigPipe
{
public:
    IgnoreSigPipe()
    {
        ::signal(SIGPIPE, SIG_IGN);
    }
};

IgnoreSigPipe ignoreSigPipe;
} // namespace

CoSocket::CoSocket()
    : m_schedule(coroutine_open()),
      m_epoller(new Epoller(std::bind(&CoSocket::EventHandler,
                                      this, std::placeholders::_1,
                                      std::placeholders::_2)))
{
}

CoSocket::~CoSocket()
{
    if (m_schedule)
        coroutine_close(m_schedule);
}

void CoSocket::Run()
{
    while(1)
    {
        while (!m_coFuncList.empty())
        {
            CoFunction func = m_coFuncList.front();
            m_coFuncList.pop_front();
            NewCoroutine(func);
        }

        m_epoller->Poll(-1);
    }
}

void CoSocket::NewCoroutine(const CoFunction &func)
{
    if (coroutine_running(m_schedule) != -1)
    {
        m_coFuncList.push_back(func);
        return;
    }

    int coId = coroutine_new(m_schedule, &InterCoFunc,
                             (void *)&func);

    if (coId < 0)
        printf("coroutine_new failed\n");
    else
        coroutine_resume(m_schedule, coId);
}

int CoSocket::Connect(int fd, const struct sockaddr *addr,
                      socklen_t addrlen)
{
    int ret = ::connect(fd, addr, addrlen);
    if (ret == 0 || errno == EISCONN)
        return 0;

    if (errno != EAGAIN &&
        errno != EINPROGRESS &&
        errno != EINTR)
        return -errno;

    AddEventAndYield(fd, EPOLLOUT | EPOLLERR);
    socklen_t len = sizeof ret;
    if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &ret, &len) < 0)
    {
        printf("getsockopt fail, error: %s\n",
               strerror(errno));
        ret = errno;
    }
    DeleteEvent(fd);
    return -ret;
}

int CoSocket::Accept(int fd, struct sockaddr *addr,
                     socklen_t *addrlen)
{
    int ret = ::accept(fd, addr, addrlen);
    if (ret >= 0)
        return ret;

    if (errno != EAGAIN &&
        errno != EWOULDBLOCK &&
        errno != ECONNABORTED &&
        errno != EINTR &&
        errno != ETIMEDOUT)
        return -errno;

    AddEventAndYield(fd, EPOLLIN | EPOLLERR);
    ret = ::accept(fd, addr, addrlen);
    DeleteEvent(fd);
    if (ret == -1)
        return -errno;
    return ret;
}

ssize_t CoSocket::Read(int fd, char *buffer, size_t size)
{
    ssize_t ret = ::read(fd, buffer, size);
    if (ret >= 0)
        return ret;

    if (errno != EAGAIN &&
        errno != EWOULDBLOCK &&
        errno != EINTR)
        return -errno;

    AddEventAndYield(fd, EPOLLIN | EPOLLERR);
    ret = ::read(fd, buffer, size);
    DeleteEvent(fd);
    if (ret == -1)
        return -errno;
    return ret;
}

ssize_t CoSocket::Write(int fd, const char *buffer, size_t size)
{
    if (size == 0 || !buffer)
        return 0;

    ssize_t ret = ::write(fd, buffer, size);
    if (ret >= 0)
        return ret;

    if (errno != EAGAIN &&
        errno != EWOULDBLOCK &&
        errno != EINTR)
        return -errno;

    AddEventAndYield(fd, EPOLLOUT | EPOLLERR);
    ret = ::write(fd, buffer, size);
    DeleteEvent(fd);
    if (ret == -1)
        return -errno;
    return ret;
}

int CoSocket::AddEventAndYield(int fd, uint32_t events)
{
    if (!SaveToCoIdMap(fd))
        return -1;

    // TODO:
    // Not support read and write for the same fd
    // at the same time
    if (m_epoller->Update(fd, events) != 0)
    {
        EraseCoIdMap(fd);
        return -1;
    }

    coroutine_yield(m_schedule);
    return 0;
}

void CoSocket::DeleteEvent(int fd)
{
    m_epoller->Update(fd, 0);
    EraseCoIdMap(fd);
}

void CoSocket::InterCoFunc(struct schedule *s, void *ud)
{
    (void)s;

    CoFunction *func = reinterpret_cast<CoFunction *>(ud);
    if (func) (*func)();
}

bool CoSocket::SaveToCoIdMap(int coKey)
{
    CoIdMap::const_iterator it = m_coIdMap.find(coKey);
    if (it != m_coIdMap.end())
    {
        printf("Dumplicate co key is not allowed.\n");
        return false;
    }

    m_coIdMap[coKey] = coroutine_running(m_schedule);
    return true;
}

void CoSocket::EraseCoIdMap(int fd)
{
    m_coIdMap.erase(fd);
}

void CoSocket::EventHandler(int fd, uint32_t events)
{
    CoIdMap::const_iterator it = m_coIdMap.find(fd);
    if (it == m_coIdMap.end())
    {
        printf("Cannot find the event handler,"
               " fd: %d, events: %d\n", fd, events);
        return;
    }

    coroutine_resume(m_schedule, it->second);
}

