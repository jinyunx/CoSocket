#include "CoSocket.h"
#include "Epoller.h"
#include "SimpleLog.h"

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

class CoSocket::TimeoutSet
{
public:
    struct CoIdTimeout
    {
        int coId;
        Timestamp timestamp;

        CoIdTimeout()
            : coId(0)
        {
        }
    };

    TimeoutSet()
    {
    }

    ~TimeoutSet()
    {
    }

    void Insert(int coId, int64_t timeoutMs);
    void Delete(int coId);
    int GetExpired(Timestamp &now) const;
    int64_t GetNeareastTimeGap() const;

    friend bool operator < (const CoIdTimeout &lhs, const CoIdTimeout &rhs);
private:

    typedef std::set<CoIdTimeout> CoIdTimeoutSet;
    typedef CoIdTimeoutSet::iterator CoIdTimeoutSetPtr;
    typedef std::map<int, CoIdTimeoutSetPtr> CoIdTimeoutMap;

    CoIdTimeoutSet m_timeoutSet;
    CoIdTimeoutMap m_timeoutPtrMap;
};

void CoSocket::TimeoutSet::Insert(int coId, int64_t timeoutMs)
{
    CoIdTimeout timeout;
    timeout.coId = coId;
    timeout.timestamp.AddMillisecond(timeoutMs);
    std::pair<CoIdTimeoutSetPtr, bool> ret = m_timeoutSet.insert(timeout);
    if (!ret.second)
    {
        SIMPLE_LOG("timeout set insert fail");
        return;
    }
    m_timeoutPtrMap.insert(std::make_pair(coId, ret.first));
}

void CoSocket::TimeoutSet::Delete(int coId)
{
    CoIdTimeoutMap::iterator it = m_timeoutPtrMap.find(coId);

    if (it != m_timeoutPtrMap.end())
    {
        m_timeoutSet.erase(it->second);
        m_timeoutPtrMap.erase(it);
    }
    else
    {
        SIMPLE_LOG("Error: Not found the coId in timeout set");
    }
}

int CoSocket::TimeoutSet::GetExpired(Timestamp &now) const
{
    if (m_timeoutSet.empty() ||
        m_timeoutSet.begin()->timestamp > now)
        return -1;

    return m_timeoutSet.begin()->coId;
}

int64_t CoSocket::TimeoutSet::GetNeareastTimeGap() const
{
    if (m_timeoutSet.empty())
        return -1;

    int64_t timeGap = Timestamp() - m_timeoutSet.begin()->timestamp;

    if (timeGap < 0)
        timeGap = 0;

    return timeGap;
}

bool operator < (const CoSocket::TimeoutSet::CoIdTimeout &lhs,
                 const CoSocket::TimeoutSet::CoIdTimeout &rhs)
{
    if (lhs.timestamp < rhs.timestamp)
        return true;
    else if (lhs.timestamp == rhs.timestamp)
        return lhs.coId < rhs.coId;
    return false;
}

class SetTimeoutGuard
{
public:
    SetTimeoutGuard(CoSocket::TimeoutSet &timeoutSet, int coId, int64_t timeout)
        : m_coId(coId),
          m_timeout(timeout),
          m_timeSet(timeoutSet)
    {
        if (m_timeout > 0)
            m_timeSet.Insert(m_coId, m_timeout);
    }

    ~SetTimeoutGuard()
    {
        if (m_timeout > 0)
            m_timeSet.Delete(m_coId);
    }

private:
    int m_coId;
    int64_t m_timeout;
    CoSocket::TimeoutSet &m_timeSet;
};

CoSocket::CoSocket()
    : m_handlingTimeout(false), m_timeoutSet(new TimeoutSet),
      m_schedule(coroutine_open()),
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

        m_epoller->Poll(m_timeoutSet->GetNeareastTimeGap());

        m_handlingTimeout = true;
        Timestamp now;
        while (1)
        {
            int coId = m_timeoutSet->GetExpired(now);
            if (coId == -1)
                break;
            coroutine_resume(m_schedule, coId);
        }
        m_handlingTimeout = false;
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
        SIMPLE_LOG("coroutine_new failed");
    else
        coroutine_resume(m_schedule, coId);
}

int CoSocket::Connect(int fd, const sockaddr * addr,
                      socklen_t addrlen, int64_t timeoutMs)
{
    int ret = ::connect(fd, addr, addrlen);
    if (ret == 0 || errno == EISCONN)
        return 0;

    if (errno != EAGAIN &&
        errno != EINPROGRESS &&
        errno != EINTR)
        return -errno;

    SetTimeoutGuard setTimeout(
        *m_timeoutSet, coroutine_running(m_schedule), timeoutMs);
    AddEventAndYield(fd, EPOLLOUT | EPOLLERR);

    if (!m_handlingTimeout)
    {
        ret = 0;
        socklen_t len = sizeof ret;
        if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &ret, &len) < 0)
        {
            SIMPLE_LOG("getsockopt fail, error: %s",
                       strerror(errno));
            ret = errno;
        }
    }
    else
    {
        ret = ETIME;
    }

    DeleteEvent(fd);

    return -ret;
}

int CoSocket::Accept(int fd, struct sockaddr *addr,
                     socklen_t *addrlen, int64_t timeoutMs)
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

    SetTimeoutGuard setTimeout(
        *m_timeoutSet, coroutine_running(m_schedule), timeoutMs);
    AddEventAndYield(fd, EPOLLIN | EPOLLERR);

    ret = -1;
    if (!m_handlingTimeout)
        ret = ::accept(fd, addr, addrlen);
    else
        errno = ETIME;

    DeleteEvent(fd);

    if (ret == -1)
        return -errno;
    return ret;
}

ssize_t CoSocket::Read(int fd, char *buffer, size_t size,
                       int64_t timeoutMs)
{
    ssize_t ret = ::read(fd, buffer, size);
    if (ret >= 0)
        return ret;

    if (errno != EAGAIN &&
        errno != EWOULDBLOCK &&
        errno != EINTR)
        return -errno;

    SetTimeoutGuard setTimeout(
        *m_timeoutSet, coroutine_running(m_schedule), timeoutMs);

    AddEventAndYield(fd, EPOLLIN | EPOLLERR);

    ret = -1;
    if (!m_handlingTimeout)
        ret = ::read(fd, buffer, size);
    else
        errno = ETIME;

    DeleteEvent(fd);

    if (ret == -1)
        return -errno;

    return ret;
}

ssize_t CoSocket::Write(int fd, const char *buffer, size_t size,
                        int64_t timeoutMs)
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

    SetTimeoutGuard setTimeout(
        *m_timeoutSet, coroutine_running(m_schedule), timeoutMs);

    AddEventAndYield(fd, EPOLLOUT | EPOLLERR);

    ret = -1;
    if (!m_handlingTimeout)
        ret = ::write(fd, buffer, size);
    else
        errno = ETIME;

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
        SIMPLE_LOG("Dumplicate co key is not allowed.");
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
        SIMPLE_LOG("Cannot find the event handler,"
                   " fd: %d, events: %d", fd, events);
        return;
    }

    coroutine_resume(m_schedule, it->second);
}
