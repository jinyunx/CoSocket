#include "CoSocket.h"
#include "Epoller.h"

#include <sys/timerfd.h>
#include <string.h>
#include <unistd.h>

extern "C"
{
#include "../3rd/coroutine.h"
}

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
        m_epoller->Poll(-1);
}

int CoSocket::NewCoroutine(const CoFunction &func)
{
    int coId = coroutine_new(m_schedule, &InterCoFunc,
                             (void *)&func);

    if (coId < 0)
        printf("coroutine_new failed\n");
    else
        coroutine_resume(m_schedule, coId);

    return coId;
}

ssize_t CoSocket::Read(int fd, char *buffer, size_t size)
{
    ssize_t ret = ::read(fd, buffer, size);
    if (ret >= 0)
    {
        return ret;
    }
    else
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            return errno;
    }

    if (!SaveToCoIdMap(CoKey(fd, EPOLLIN)))
        return -1;

    // TODO: Update function internal should use |
    if (!m_epoller->Update(fd, EPOLLIN))
    {
        m_coIdMap.erase(CoKey(fd, EPOLLIN));
        return -1;
    }

    coroutine_yield(m_schedule);

    ret = ::read(fd, buffer, size);

    m_epoller->Update(fd, 0);
    m_coIdMap.erase(CoKey(fd, EPOLLIN));
    return ret;
}

void CoSocket::InterCoFunc(struct schedule *s, void *ud)
{
    (void)s;

    CoFunction *func = reinterpret_cast<CoFunction *>(ud);
    if (func) (*func)();
}

bool CoSocket::SaveToCoIdMap(const CoKey &coKey)
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

void CoSocket::EventHandler(int fd, uint32_t events)
{
    CoIdMap::const_iterator it = m_coIdMap.find(CoKey(fd, events));
    if (it == m_coIdMap.end())
    {
        printf("Cannot find the event handler,"
               " fd: %d, events: %d\n", fd, events);
        return;
    }

    coroutine_resume(m_schedule, it->second);
}

