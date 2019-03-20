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
    : m_conditionCounter(0), m_handlingTimeout(false),
      m_timeoutSet(new TimeoutSet), m_schedule(coroutine_open()),
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
        RunNewCoFunc();
        RunWaitResumeCo();
        m_epoller->Poll(m_timeoutSet->GetNeareastTimeGap());
        RunTimeoutCo();
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
    AddEventAndYield(fd, EPOLLOUT);

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

    DeleteEvent(fd, EPOLLOUT);

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
    AddEventAndYield(fd, EPOLLIN);

    if (!m_handlingTimeout)
    {
        if ((ret = ::accept(fd, addr, addrlen)) == -1)
            ret = -errno;
    }
    else
    {
        ret = -ETIME;
    }

    DeleteEvent(fd, EPOLLIN);

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

    AddEventAndYield(fd, EPOLLIN);

    if (!m_handlingTimeout)
    {
        if ((ret = ::read(fd, buffer, size)) == -1)
            ret = -errno;
    }
    else
    {
        ret = -ETIME;
    }

    DeleteEvent(fd, EPOLLIN);

    return ret;
}

ssize_t CoSocket::ReadFull(int fd, char * buffer, size_t size,
                           int64_t timeoutMs)
{
    size_t readBytes = 0;
    while (readBytes < size)
    {
        ssize_t ret = Read(fd, buffer + readBytes,
                           size - readBytes, timeoutMs);

        if (ret < 0)
            return ret;

        if (ret == 0)
            return readBytes;

        readBytes += ret;
    }
    return readBytes;
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

    AddEventAndYield(fd, EPOLLOUT);

    if (!m_handlingTimeout)
    {
        if ((ret = ::write(fd, buffer, size)) == -1)
            ret = -errno;
    }
    else
    {
        ret = -ETIME;
    }

    DeleteEvent(fd, EPOLLOUT);

    return ret;
}

ssize_t CoSocket::WriteAll(int fd, const char * buffer, size_t size,
                           int64_t timeoutMs)
{
    size_t writeBytes = 0;
    while (writeBytes < size)
    {
        ssize_t ret = Write(fd, buffer + writeBytes,
                            size - writeBytes, timeoutMs);
        if (ret < 0)
            return ret;
        writeBytes += ret;
    }
    return writeBytes;
}

uint64_t CoSocket::GetCondition()
{
    if (m_conditionCounter == 0xFFFFFFFFFFFFFFFF)
    {
        SIMPLE_LOG("Condition counter overflow");
        abort();
    }
    return m_conditionCounter++;
}

void CoSocket::ConditionWait(uint64_t condition)
{
    m_conditionMap[condition].insert(coroutine_running(m_schedule));
    coroutine_yield(m_schedule);
}

void CoSocket::ConditionNotify(uint64_t condition)
{
    ConditionWaitList &list = m_conditionMap[condition];
    m_waitResumeList.insert(m_waitResumeList.end(),
                            list.begin(),
                            list.end());
    list.clear();
}

int CoSocket::AddEventAndYield(int fd, uint32_t events)
{
    if (!SaveToCoIdMap(fd, events))
        return -1;

    events = GetEvents(fd);
    if (m_epoller->Update(fd, events) != 0)
    {
        SIMPLE_LOG("Epoll update failed, fatal error, exit now");
        abort();
    }

    coroutine_yield(m_schedule);
    return 0;
}

void CoSocket::DeleteEvent(int fd, uint32_t events)
{
    uint32_t oldEvents = 0;
    if (m_coIdMap[fd].HasReadCoroutine())
        oldEvents |= EPOLLIN;
    if (m_coIdMap[fd].HasWriteCoroutine())
        oldEvents |= EPOLLOUT;
    uint32_t newEvents = oldEvents & (~events);
    int ret = 0;
    if ((ret = m_epoller->Update(fd, newEvents)) != 0)
    {
        // FIXME: when fd close handly, fd will delete from epoll auto
        SIMPLE_LOG("Epoll update failed, fatal error: %d, exit now", ret);
        abort();
    }
    ResetInCoIdMap(fd, events);
}

void CoSocket::InterCoFunc(struct schedule *s, void *ud)
{
    (void)s;

    CoFunction *func = reinterpret_cast<CoFunction *>(ud);
    if (func) (*func)();
}

bool CoSocket::SaveToCoIdMap(int fd, uint32_t events)
{
    int coId = coroutine_running(m_schedule);

    FdWatcher &fdWatcher = m_coIdMap[fd];
    if (events & EPOLLIN)
    {
        if (fdWatcher.HasReadCoroutine())
        {
            SIMPLE_LOG("fd read dumplicate in "
                       "two coroutine is not allowed.");
            return false;
        }
        fdWatcher.SetReadCoId(coId);
    }

    if (events & EPOLLOUT)
    {
        if (fdWatcher.HasWriteCoroutine())
        {
            SIMPLE_LOG("fd write dumplicate in "
                       "two coroutine is not allowed.");
            return false;
        }
        fdWatcher.SetWriteCoId(coId);
    }

    return true;
}

void CoSocket::ResetInCoIdMap(int fd, uint32_t events)
{
    if (events & EPOLLIN)
        m_coIdMap[fd].ResetReadCoId();

    if (events & EPOLLOUT)
        m_coIdMap[fd].ResetWriteCoId();
}

uint32_t CoSocket::GetEvents(int fd)
{
    uint32_t events = 0;
    if (m_coIdMap[fd].HasReadCoroutine())
        events |= EPOLLIN;
    if (m_coIdMap[fd].HasWriteCoroutine())
        events |= EPOLLOUT;
    return events;
}

void CoSocket::EventHandler(int fd, uint32_t events)
{
    FdWatcher &fdWatcher = m_coIdMap[fd];

    if (!(fdWatcher.HasReadCoroutine()) &&
        !(fdWatcher.HasWriteCoroutine()))
    {
        SIMPLE_LOG("Cannot find the event handler,"
                   " fd: %d, events: %d",
                   fd,
                   events);
        return;
    }

    bool error = events & EPOLLERR ||
                 events & EPOLLHUP;

    if (error || events & EPOLLIN)
    {
        if (fdWatcher.HasReadCoroutine())
            coroutine_resume(m_schedule, fdWatcher.ReadCoId());
    }

    if (error || events & EPOLLOUT)
    {
        if (fdWatcher.HasWriteCoroutine())
            coroutine_resume(m_schedule, fdWatcher.WriteCoId());
    }

}

void CoSocket::RunNewCoFunc()
{
    CoFuncList::const_iterator it = m_coFuncList.begin();
    while (it != m_coFuncList.end())
    {
        NewCoroutine(*it);
        ++it;
    }
    m_coFuncList.clear();
}

void CoSocket::RunWaitResumeCo()
{
    WaitResumeList::const_iterator it = m_waitResumeList.begin();
    while (it != m_waitResumeList.end())
    {
        coroutine_resume(m_schedule, *it);
        ++it;
    }
    m_waitResumeList.clear();
}

void CoSocket::RunTimeoutCo()
{
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

bool CoSocket::FdWatcher::HasReadCoroutine() const
{
    return m_readCoId != -1;
}

bool CoSocket::FdWatcher::HasWriteCoroutine() const
{
    return m_writeCoId != -1;
}

int CoSocket::FdWatcher::ReadCoId() const
{
    return m_readCoId;
}

int CoSocket::FdWatcher::WriteCoId() const
{
    return m_writeCoId;
}

bool CoSocket::FdWatcher::SetReadCoId(int coId)
{
    if (m_readCoId != -1)
        return false;
    m_readCoId = coId;
    return true;
}

bool CoSocket::FdWatcher::SetWriteCoId(int coId)
{
    if (m_writeCoId != -1)
        return false;
    m_writeCoId = coId;
    return true;
}

void CoSocket::FdWatcher::ResetReadCoId()
{
    m_readCoId = -1;
}

void CoSocket::FdWatcher::ResetWriteCoId()
{
    m_writeCoId = -1;
}
